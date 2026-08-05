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

template<std::copy_constructible Key>
using cache_key = std::shared_ptr<const Key>;

struct cache_entry_opts final
{
    std::chrono::nanoseconds expire_after_write{};
};

template<
    std::copy_constructible key_t, 
    typename value_t, 
    typename hash_t = std::hash<key_t>, 
    typename key_equal_t = std::equal_to<key_t>, 
    typename clock_t = std::chrono::steady_clock>
class concurrent_cache
{
private:
    using time_point = typename clock_t::time_point;
    using duration = typename clock_t::duration;
    static constexpr time_point no_expiration_time = clock_t::time_point::min();

    struct entry final
    {
        cache_value<value_t> value;
        time_point expires_at{no_expiration_time};

        [[nodiscard]]
        bool expired(const time_point now = clock_t::now()) const noexcept
        {
            return expires_at != no_expiration_time && now >= expires_at;
        }
    };

    struct load_state final
    {
        std::promise<cache_value<value_t>> promise;
        std::shared_future<cache_value<value_t>> future;

        load_state()
            : future{promise.get_future().share()}
        {
        }
    };

    using values_map = std::unordered_map<key_t, entry, hash_t, key_equal_t>;
    using in_flight_map = std::unordered_map<key_t, std::shared_ptr<load_state>, hash_t, key_equal_t>;
    using values_node = typename values_map::node_type;

    struct shard final
    {
        shard(const hash_t& hash, const key_equal_t& key_equal)
            : values{0, hash, key_equal}
            , in_flight{0, hash, key_equal}
        {
        }

        mutable std::shared_mutex mutex;

        values_map values;
        in_flight_map in_flight;
    };

    [[no_unique_address]]
    hash_t m_hash;

    [[no_unique_address]]
    key_equal_t m_key_equal;

    std::vector<std::unique_ptr<shard>> m_shards;

    [[nodiscard]]
    shard& shard_for(const key_t& key) const
    {
        return *m_shards[
            static_cast<std::size_t>(m_hash(key)) % m_shards.size()
        ];
    }

    [[nodiscard]]
    static time_point expiration_time(const cache_entry_opts& options)
    {
        const time_point now = clock_t::now();
        const duration ttl = std::chrono::duration_cast<duration>(options.expire_after_write);

        if (ttl <= clock_t::duration::zero()) {
            return no_expiration_time;
        }

        const duration remaining = clock_t::time_point::max() - now;

        if (ttl >= remaining) {
            return clock_t::time_point::max();
        }

        return now + ttl;
    }

    static void remove_in_flight(shard& selected, const key_t& key, const std::shared_ptr<load_state>& expected_state)
    {
        std::unique_lock lock{selected.mutex};
        const auto it = selected.in_flight.find(key);

        if (it != selected.in_flight.end() && it->second == expected_state) {
            selected.in_flight.erase(it);
        }
    }

public:
    explicit concurrent_cache(std::size_t shard_count = 16, hash_t hash = {}, key_equal_t key_equal = {})
        : m_hash{std::move(hash)}
        , m_key_equal{std::move(key_equal)}
    {
        assert(shard_count > 0);
        m_shards.reserve(shard_count);

        for (std::size_t i = 0; i < shard_count; ++i) {
            m_shards.push_back(std::make_unique<shard>(m_hash, m_key_equal));
        }
    }

    concurrent_cache(const concurrent_cache&) = delete;
    concurrent_cache& operator=(const concurrent_cache&) = delete;

    concurrent_cache(concurrent_cache&&) = delete;
    concurrent_cache& operator=(concurrent_cache&&) = delete;

    [[nodiscard]]
    cache_value<value_t> get(const key_t& key) const
    {
        shard& selected = shard_for(key);

        {
            std::shared_lock lock{selected.mutex};
            const auto it = selected.values.find(key);

            if (it == selected.values.end()) {
                return {};
            }

            if (!it->second.expired()) {
                return it->second.value;
            }
        }

        // Destroy the expired key and value after releasing the shard lock.
        values_node expired_entry;

        {
            std::unique_lock lock{selected.mutex};
            const auto it = selected.values.find(key);

            if (it == selected.values.end()) {
                return {};
            }

            // The entry may have been replaced while changing locks.
            if (!it->second.expired()) {
                return it->second.value;
            }

            expired_entry = selected.values.extract(it);
        }

        return {};
    }

    [[nodiscard]]
    bool contains(const key_t& key) const
    {
        return static_cast<bool>(get(key));
    }

    template<typename... Args>
    requires std::constructible_from<value_t, Args...>
    [[nodiscard]]
    cache_value<value_t> insert_or_assign(key_t key, cache_entry_opts options, Args&&... args)
    {
        cache_value<value_t> new_value = std::make_shared<const value_t>(std::forward<Args>(args)...);
        entry new_entry{new_value, expiration_time(options)};

        shard& selected = shard_for(key);

        // Keep the replaced entry alive until after releasing the shard lock.
        std::optional<entry> old_entry;

        {
            std::unique_lock lock{selected.mutex};
            auto [it, inserted] = selected.values.try_emplace(std::move(key), std::move(new_entry));

            if (!inserted) {
                old_entry.emplace(std::exchange(it->second, std::move(new_entry)));
            }
        }

        return new_value;
    }

    template<typename... Args>
    requires std::constructible_from<value_t, Args...>
    [[nodiscard]]
    cache_value<value_t> insert_or_assign(key_t key, Args&&... args)
    {
        return insert_or_assign(std::move(key), cache_entry_opts{}, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    bool erase(const key_t& key)
    {
        shard& selected = shard_for(key);
        values_node removed;

        {
            std::unique_lock lock{selected.mutex};
            const auto it = selected.values.find(key);

            if (it == selected.values.end()) {
                return false;
            }

            removed = selected.values.extract(it);
        }

        return true;
    }

    template<typename loader_t>
    requires requires(loader_t&& loader, const key_t& key) {
        {
            std::invoke(std::forward<loader_t>(loader), key)
        } -> std::same_as<value_t>;
    }
    [[nodiscard]]
    cache_value<value_t> get_or_create(const key_t& key, cache_entry_opts options, loader_t&& loader)
    {
        shard& selected = shard_for(key);
        std::shared_ptr<load_state> state;
        bool owns_load = false;

        {
            // Keep an expired entry alive until after releasing the shard lock.
            values_node expired_entry;

            std::unique_lock lock{selected.mutex};
            const auto value_it = selected.values.find(key);

            if (value_it != selected.values.end()) {
                if (!value_it->second.expired()) {
                    return value_it->second.value;
                }

                expired_entry = selected.values.extract(value_it);
            }

            if (const auto load_it = selected.in_flight.find(key); load_it != selected.in_flight.end()) {
                state = load_it->second;
            }
            else {
                state = std::make_shared<load_state>();
                selected.in_flight.emplace(key, state);
                owns_load = true;
            }
        }

        if (!owns_load) {
            return state->future.get();
        }

        cache_value<value_t> published;

        try {
            value_t loaded_value = std::invoke(std::forward<loader_t>(loader), key);
            cache_value<value_t> loaded = std::make_shared<const value_t>(std::move(loaded_value));

            {
                // A concurrent insertion may have happened while the loader was running.
                values_node expired_entry;

                std::unique_lock lock{selected.mutex};
                const auto value_it = selected.values.find(key);

                if (value_it != selected.values.end() && !value_it->second.expired()) {
                    published = value_it->second.value;
                }
                else {
                    if (value_it != selected.values.end()) {
                        expired_entry = selected.values.extract(value_it);
                    }

                    auto [it, inserted] = selected.values.try_emplace(key, entry{loaded, expiration_time(options)});
                    assert(inserted);
                    published = it->second.value;
                }

                const auto load_it = selected.in_flight.find(key);

                if (load_it != selected.in_flight.end() && load_it->second == state) {
                    selected.in_flight.erase(load_it);
                }
            }
        }
        catch (...) {
            const std::exception_ptr error = std::current_exception();
            state->promise.set_exception(error);
            remove_in_flight(selected, key, state);
            std::rethrow_exception(error);
        }

        state->promise.set_value(published);
        return published;
    }

    template<typename loader_t>
    requires requires(loader_t&& loader, const key_t& key) {
        {
            std::invoke(std::forward<loader_t>(loader), key)
        } -> std::same_as<value_t>;
    }
    [[nodiscard]]
    cache_value<value_t> get_or_create(const key_t& key, loader_t&& loader)
    {
        return get_or_create(key, cache_entry_opts{}, std::forward<loader_t>(loader));
    }

    [[nodiscard]]
    std::size_t purge_expired() const
    {
        const time_point now = clock_t::now();
        std::size_t result = 0;

        for (const auto& selected : m_shards) {
            std::vector<values_node> removed;

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