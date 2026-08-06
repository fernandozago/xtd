#ifndef XTD_CONCURRENT_CACHE_H
#define XTD_CONCURRENT_CACHE_H

#include <cassert>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xtd
{

template<typename value_t>
using cache_value = std::shared_ptr<const value_t>;

struct cache_entry_opts final
{
    static constexpr std::chrono::nanoseconds max_supported_ttl = 
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::hours{24LL * 365LL * 292LL});

    cache_entry_opts()
        : m_ttl{std::chrono::nanoseconds::zero()}
    {
    }

    cache_entry_opts(const std::chrono::nanoseconds ttl)
        : m_ttl{ttl}
    {
        assert(m_ttl >= std::chrono::nanoseconds::zero());
        assert(m_ttl <= max_supported_ttl);
    }

    std::chrono::nanoseconds m_ttl;
};

template<
    std::copy_constructible key_t, typename value_t, 
    typename hash_t = std::hash<key_t>, 
    typename key_equal_t = std::equal_to<key_t>, 
    typename clock_t = std::chrono::steady_clock>
class concurrent_cache final
{
private:
    using time_point = typename clock_t::time_point;
    using duration = typename clock_t::duration;
    using cache_value_t = xtd::cache_value<value_t>;
    static constexpr time_point no_expiration_time = clock_t::time_point::min();

    struct entry final
    {
        cache_value_t value;
        time_point expires_at{no_expiration_time};

        [[nodiscard]]
        bool expired(const time_point now = clock_t::now()) const noexcept
        {
            return expires_at != no_expiration_time && now >= expires_at;
        }
    };

    struct factory_state final
    {
        std::promise<cache_value_t> promise;
        std::shared_future<cache_value_t> future;

        factory_state()
            : future{promise.get_future().share()}
        {
        }
    };

    
    using value_node_t = typename std::unordered_map<key_t, entry, hash_t, key_equal_t>::node_type;
    struct shard_t final
    {
        shard_t(const hash_t& hash, const key_equal_t& key_equal)
        : values{0, hash, key_equal}
        , in_flight{0, hash, key_equal}
        {
        }
        
        mutable std::shared_mutex mutex;
        
        std::unordered_map<key_t, entry, hash_t, key_equal_t> values;
        std::unordered_map<key_t, std::shared_ptr<factory_state>, hash_t, key_equal_t> in_flight;
    };

    [[no_unique_address]]
    hash_t m_hash;

    [[no_unique_address]]
    key_equal_t m_key_equal;

    std::vector<std::unique_ptr<shard_t>> m_shards;

    [[nodiscard]]
    shard_t& shard_for(const key_t& key) const
    {
        return *m_shards[
            static_cast<std::size_t>(m_hash(key)) % m_shards.size()
        ];
    }

    [[nodiscard]]
    static time_point expiration_time(const cache_entry_opts& options)
    {
        return options.m_ttl == std::chrono::nanoseconds::zero()
            ? no_expiration_time
            : clock_t::now() + std::chrono::duration_cast<duration>(options.m_ttl);
    }

    static void remove_in_flight(shard_t& selected, const key_t& key, const std::shared_ptr<factory_state>& expected_state)
    {
        std::unique_lock lock{selected.mutex};
        const auto it = selected.in_flight.find(key);

        if (it != selected.in_flight.end() && it->second == expected_state) {
            selected.in_flight.erase(it);
        }
    }

    enum class lookup_status
    {
        missing,
        expired,
        found
    };

    struct lookup_result
    {
        lookup_status status;
        cache_value_t value;
    };


    [[nodiscard]]
    lookup_result internal_fast_get(const key_t& key, shard_t& selected) const
    {
        std::shared_lock lock{selected.mutex};

        const auto it = selected.values.find(key);
        if (it == selected.values.end()) {
            return {lookup_status::missing, nullptr};
        }
        if (it->second.expired()) {
            return {lookup_status::expired, nullptr};
        }

        return {lookup_status::found, it->second.value};
    }
public:
    explicit concurrent_cache(std::size_t shard_count = 16, hash_t hash = {}, key_equal_t key_equal = {})
        : m_hash{std::move(hash)}
        , m_key_equal{std::move(key_equal)}
    {
        assert(shard_count > 0);
        m_shards.reserve(shard_count);

        for (std::size_t i = 0; i < shard_count; ++i) {
            m_shards.push_back(std::make_unique<shard_t>(m_hash, m_key_equal));
        }
    }

    concurrent_cache(const concurrent_cache&) = delete;
    concurrent_cache& operator=(const concurrent_cache&) = delete;

    concurrent_cache(concurrent_cache&&) = delete;
    concurrent_cache& operator=(concurrent_cache&&) = delete;

    [[nodiscard]]
    cache_value_t get(const key_t& key) const
    {
        shard_t& shard = shard_for(key);
        
        // Fast path: valid cached values only require shared access.
        if (const auto lookup = internal_fast_get(key, shard); lookup.status == lookup_status::found) {
            return lookup.value;
        } else if (lookup.status == lookup_status::expired) {
            value_node_t expired_entry; // Optimization: Avoid calling <value_node_t> deconstruction while holding the shard lock.
            std::unique_lock lock{shard.mutex};
            const auto it = shard.values.find(key);

            if (it != shard.values.end() && !it->second.expired()) {
                expired_entry = shard.values.extract(it);   
            }
        }

        return nullptr;
    }

    [[nodiscard]]
    bool contains(const key_t& key) const
    {
        shard_t& shard = shard_for(key);
        std::shared_lock lock{shard.mutex};
        const auto it = shard.values.find(key);
        return (it != shard.values.end() && !it->second.expired());
    }

    template<typename... Args>
    requires std::constructible_from<value_t, Args...>
    [[nodiscard]]
    cache_value_t insert_or_assign(key_t key, cache_entry_opts options, Args&&... args)
    {
        shard_t& shard = shard_for(key);
        // Optimization: Avoid calling <entry> deconstruction while holding the shard lock.
        std::optional<entry> old_entry; 
        
        entry new_entry {
            std::make_shared<const value_t>(std::forward<Args>(args)...), 
            expiration_time(options)
        };
        std::unique_lock lock{shard.mutex};
        auto [it, inserted] = shard.values.try_emplace(std::move(key), std::move(new_entry));

        if (!inserted) {
            old_entry.emplace(std::exchange(it->second, std::move(new_entry)));
        }

        return it->second.value;
    }

    template<typename... Args>
    requires std::constructible_from<value_t, Args...>
    [[nodiscard]]
    cache_value_t insert_or_assign(key_t key, Args&&... args)
    {
        return insert_or_assign(std::move(key), cache_entry_opts{}, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    bool erase(const key_t& key)
    {
        shard_t& shard = shard_for(key);

        // Optimization: Avoid calling <value_node_t> deconstruction while holding the shard lock
        value_node_t to_be_removed;

        std::unique_lock lock{shard.mutex};
        const auto it = shard.values.find(key);
        if (it == shard.values.end()) {
            return false;
        }
        to_be_removed = shard.values.extract(it);
        return true;
    }

    template<typename factory_t>
    requires requires(factory_t&& factory, const key_t& key) {
        {
            std::invoke(std::forward<factory_t>(factory), key)
        } -> std::same_as<value_t>;
    }

    [[nodiscard]]
    cache_value_t get_or_create(const key_t& key, cache_entry_opts options, factory_t&& factory)
    {
        shard_t& shard = shard_for(key);

        // Fast path.
        if (const auto lookup = internal_fast_get(key, shard);
            lookup.status == lookup_status::found) {
            return lookup.value;
        }
        
        // Element does not exists or has expired. We need to execute the factory to create a new value.

        std::shared_ptr<factory_state> state;
        {
            // Destroy the extracted node after releasing the shard lock.
            value_node_t expired_entry;

            std::unique_lock lock{shard.mutex};

            // Mandatory recheck after transitioning from shared to exclusive access.
            const auto value_it = shard.values.find(key);

            if (value_it != shard.values.end()) {
                if (!value_it->second.expired()) {
                    return value_it->second.value;
                }

                expired_entry = shard.values.extract(value_it);
            }

            if (const auto factory_it = shard.in_flight.find(key);
                factory_it != shard.in_flight.end()) {
                auto existing_state = factory_it->second;

                // The factory owner needs this mutex to publish its result.
                lock.unlock();
                return existing_state->future.get();
            }

            state = std::make_shared<factory_state>();
            shard.in_flight.emplace(key, state);
        }

        // Reaching here means this thread owns the factory execution.
        cache_value_t published;

        try {
            cache_value_t new_value = std::make_shared<const value_t>(
                std::invoke(std::forward<factory_t>(factory), key)
            );

            value_node_t expired_entry;

            std::unique_lock lock{shard.mutex};

            const auto value_it = shard.values.find(key);

            // Another operation may have published a value while the
            // factory was executing.
            if (value_it != shard.values.end() && !value_it->second.expired()) {
                published = value_it->second.value;
            }
            else {
                if (value_it != shard.values.end()) {
                    expired_entry = shard.values.extract(value_it);
                }

                auto [inserted_it, inserted] =
                    shard.values.try_emplace(
                        key,
                        entry{
                            std::move(new_value),
                            expiration_time(options)
                        });

                assert(inserted);
                published = inserted_it->second.value;
            }

            const auto factory_it = shard.in_flight.find(key);

            if (factory_it != shard.in_flight.end() &&
                factory_it->second == state) {
                shard.in_flight.erase(factory_it);
            }
        }
        catch (...) {
            const std::exception_ptr error = std::current_exception();

            state->promise.set_exception(error);
            remove_in_flight(shard, key, state);

            std::rethrow_exception(error);
        }

        state->promise.set_value(published);
        return published;
    }

    template<typename factory_t>
    requires requires(factory_t&& factory, const key_t& key) {
        {
            std::invoke(std::forward<factory_t>(factory), key)
        } -> std::same_as<value_t>;
    }
    [[nodiscard]]
    cache_value_t get_or_create(const key_t& key, factory_t&& factory)
    {
        return get_or_create(key, cache_entry_opts{}, std::forward<factory_t>(factory));
    }

    [[nodiscard]]
    std::size_t purge_expired() const
    {
        const time_point now = clock_t::now();
        std::size_t result = 0;

        for (const auto& selected : m_shards) {
            std::vector<value_node_t> removed;

            {
                std::unique_lock lock{selected->mutex};
                removed.reserve(selected->values.size());

                for (auto it = selected->values.begin(); it != selected->values.end();) {
                    if (!it->second.expired(now)) {
                        ++it;
                        continue;
                    }

                    const auto current = it++;
                    removed.push_back(selected->values.extract(current));
                }
            }

            result += removed.size();
        }

        return result;
    }

    [[nodiscard]]
    std::size_t size() const
    {
        const auto now = clock_t::now();
        std::size_t result = 0;

        for (const auto& selected : m_shards) {
            std::shared_lock lock{selected->mutex};

            for (const auto& item : selected->values) {
                if (!item.second.expired(now)) {
                    ++result;
                }
            }
        }

        return result;
    }

    [[nodiscard]]
    bool empty() const
    {
        const auto now = clock_t::now();

        for (const auto& selected : m_shards) {
            std::shared_lock lock{selected->mutex};

            for (const auto& item : selected->values) {
                if (!item.second.expired(now)) {
                    return false;
                }
            }
        }

        return true;
    }
};

}

#endif