#ifndef XTD_CONCURRENT_CACHE_H
#define XTD_CONCURRENT_CACHE_H

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
#include <cassert>

namespace xtd
{

template<typename value_t>
using cache_value = std::shared_ptr<const value_t>;

template<std::copy_constructible Key>
using cache_key = std::shared_ptr<const Key>;

template<std::copy_constructible key_t, typename value_t, typename hash_t = std::hash<key_t>, typename key_equal_t = std::equal_to<key_t>>
class concurrent_cache final
{
public:
    private:
    struct load_state final
    {
        std::promise<cache_value<value_t>> promise;
        std::shared_future<cache_value<value_t>> future;

        load_state()
            : future{promise.get_future().share()}
        {
        }
    };

    using values_map = std::unordered_map<key_t, cache_value<value_t>, hash_t, key_equal_t>;
    using in_flight_map = std::unordered_map<key_t, std::shared_ptr<load_state>, hash_t, key_equal_t>;

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
    shard& shard_for(const key_t& key)
    {
        return *m_shards[
            static_cast<std::size_t>(m_hash(key)) % m_shards.size()
        ];
    }

    [[nodiscard]]
    const shard& shard_for(const key_t& key) const
    {
        return *m_shards[
            static_cast<std::size_t>(m_hash(key)) % m_shards.size()
        ];
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
        const shard& selected = shard_for(key);
        std::shared_lock lock{selected.mutex};
        const auto it = selected.values.find(key);

        if (it == selected.values.end()) {
            return {};
        }

        return it->second;
    }

    [[nodiscard]]
    bool contains(const key_t& key) const
    {
        const shard& selected = shard_for(key);
        std::shared_lock lock{selected.mutex};
        return selected.values.contains(key);
    }

    template<typename... Args>
    requires std::constructible_from<value_t, Args...>
    [[nodiscard]]
    cache_value<value_t> insert_or_assign(key_t key, Args&&... args)
    {
        cache_value<value_t> new_value = std::make_shared<const value_t>(std::forward<Args>(args)...);

        shard& selected = shard_for(key);

        // Keep the replaced value alive until after releasing the shard lock.
        cache_value<value_t> old_value;

        {
            std::unique_lock lock{selected.mutex};

            auto [it, inserted] = selected.values.try_emplace(std::move(key), new_value);

            if (!inserted) {
                old_value = std::exchange(it->second,new_value);
            }
        }

        return new_value;
    }

    [[nodiscard]]
    bool erase(const key_t& key)
    {
        shard& selected = shard_for(key);
        typename values_map::node_type removed;
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
    cache_value<value_t> get_or_create(const key_t& key, loader_t&& loader)
    {
        shard& selected = shard_for(key);
        std::shared_ptr<load_state> state;
        bool owns_load = false;

        {
            std::unique_lock lock{selected.mutex};

            if (const auto value_it = selected.values.find(key); value_it != selected.values.end()) {
                return value_it->second;
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

        try {
            value_t loaded_value = std::invoke(std::forward<loader_t>(loader), key);
            cache_value<value_t> loaded = std::make_shared<const value_t>(std::move(loaded_value));
            cache_value<value_t> published;

            {
                std::scoped_lock lock{selected.mutex};
                auto [it, inserted] = selected.values.try_emplace(key, loaded);
                published = it->second;

                const auto load_it = selected.in_flight.find(key);

                if (load_it != selected.in_flight.end() &&
                    load_it->second == state) {
                    selected.in_flight.erase(load_it);
                }
            }

            state->promise.set_value(published);
            return published;
        }
        catch (...) {
            const std::exception_ptr error = std::current_exception();
            state->promise.set_exception(error);
            remove_in_flight(selected, key, state);
            std::rethrow_exception(error);
        }
    }

    [[nodiscard]]
    std::size_t size() const
    {
        std::size_t result = 0;

        for (const auto& selected : m_shards) {
            std::shared_lock lock{selected->mutex};
            result += selected->values.size();
        }

        return result;
    }

    [[nodiscard]]
    bool empty() const
    {
        for (const auto& selected : m_shards) {
            std::shared_lock lock{selected->mutex};

            if (!selected->values.empty()) {
                return false;
            }
        }

        return true;
    }
};

}

#endif