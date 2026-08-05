#ifndef XTD_CONCURRENT_CACHE_EXPIRATIONS_TESTS_H
#define XTD_CONCURRENT_CACHE_EXPIRATIONS_TESTS_H

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>

#include "../third_party/catch2/catch_amalgamated.hpp"
#include "cache/concurrent_cache.h"

using namespace std::chrono_literals;

struct manual_clock final
{
    using duration = std::chrono::milliseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<manual_clock, duration>;

    static constexpr bool is_steady = true;

    [[nodiscard]]
    static time_point now() noexcept
    {
        return current_time;
    }

    static void reset(const duration value = duration::zero()) noexcept
    {
        current_time = time_point{value};
    }

    static void advance(const duration value) noexcept
    {
        current_time += value;
    }

    inline static time_point current_time{duration::zero()};
};

struct ConcurrentCacheExpirationTests {};

template<typename key_t, typename value_t>
using concurrent_cache_expirations_test = xtd::concurrent_cache<key_t, value_t, std::hash<key_t>, std::equal_to<key_t>, manual_clock>;

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts insert_or_assign keeps value alive when ttl is zero")
{
    manual_clock::reset();

    concurrent_cache_expirations_test<int, int> cache;
    const auto inserted = cache.insert_or_assign(10, {}, 20);

    REQUIRE(inserted);

    manual_clock::advance(24h);

    const auto current = cache.get(10);

    REQUIRE(current);
    CHECK(current == inserted);
    CHECK(*current == 20);
    CHECK(cache.contains(10));
}

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts insert_or_assign expires values at ttl boundary")
{
    manual_clock::reset();

    concurrent_cache_expirations_test<std::size_t, std::size_t> cache;
    xtd::cache_entry_opts options{};
    options.expire_after_write = 5ms;

    const auto inserted = cache.insert_or_assign(10, options, 20);

    REQUIRE(inserted);

    manual_clock::advance(4ms);
    REQUIRE(cache.get(10));

    manual_clock::advance(1ms);

    CHECK_FALSE(cache.get(10));
    CHECK_FALSE(cache.contains(10));
    CHECK(cache.size() == 0);
    CHECK(cache.empty());
}

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts insert_or_assign treats zero and negative ttl as no expiration")
{
    manual_clock::reset();

    concurrent_cache_expirations_test<std::size_t, std::size_t> cache;

    SECTION("zero ttl") {
        xtd::cache_entry_opts options{};
        options.expire_after_write = 0ms;

        const auto inserted = cache.insert_or_assign(1, options, 10);

        REQUIRE(inserted);
        manual_clock::advance(1h);
        const auto current = cache.get(1);
        REQUIRE(current);
        CHECK(current == inserted);
        CHECK(*current == 10);
        CHECK(cache.contains(1));
    }

    SECTION("negative ttl") {
        const auto inserted = cache.insert_or_assign(2, {-1ms}, 20);

        REQUIRE(inserted);
        manual_clock::advance(1h);
        const auto current = cache.get(2);
        REQUIRE(current);
        CHECK(current == inserted);
        CHECK(*current == 20);
        CHECK(cache.contains(2));
    }
}

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts get_or_create reuses cached value before expiration and reloads after expiration")
{
    manual_clock::reset();

    concurrent_cache_expirations_test<std::size_t, std::size_t> cache;
    xtd::cache_entry_opts options{};
    options.expire_after_write = 3ms;

    std::atomic_size_t loader_calls{0};

    const auto loader = [&loader_calls](const std::size_t key) {
        return key * 10 + (++loader_calls);
    };

    const auto first = cache.get_or_create(7, options, loader);

    REQUIRE(first);
    CHECK(*first == 71);
    CHECK(loader_calls.load() == 1);

    manual_clock::advance(2ms);
    const auto second = cache.get_or_create(7, options, loader);

    REQUIRE(second);
    CHECK(second == first);
    CHECK(*second == 71);
    CHECK(loader_calls.load() == 1);

    manual_clock::advance(1ms);
    const auto third = cache.get_or_create(7, options, loader);

    REQUIRE(third);
    CHECK(third != first);
    CHECK(*third == 72);
    CHECK(loader_calls.load() == 2);
}

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts purge_expired removes only expired values")
{
    concurrent_cache_expirations_test<std::size_t, std::size_t> cache;
    manual_clock::reset();

    xtd::cache_entry_opts short_ttl{};
    short_ttl.expire_after_write = 2ms;

    xtd::cache_entry_opts long_ttl{};
    long_ttl.expire_after_write = 10ms;

    REQUIRE(cache.insert_or_assign(1, short_ttl, 100));
    REQUIRE(cache.insert_or_assign(2, long_ttl, 200));
    REQUIRE(cache.insert_or_assign(3, xtd::cache_entry_opts{}, 300));

    manual_clock::advance(3ms);

    CHECK(cache.purge_expired() == 1);
    CHECK_FALSE(cache.contains(1));
    CHECK(cache.contains(2));
    CHECK(cache.contains(3));
    CHECK(cache.size() == 2);
}

TEST_CASE_METHOD(ConcurrentCacheExpirationTests, "cache_entry_opts get_or_create protects against stampede after expiration")
{
    using cache_type = xtd::concurrent_cache<std::size_t, std::size_t, std::hash<std::size_t>, std::equal_to<std::size_t>, manual_clock>;

    static constexpr std::size_t thread_count = 8;
    static constexpr std::size_t key = 5;

    manual_clock::reset();

    cache_type cache;

    xtd::cache_entry_opts initial_options{};
    initial_options.expire_after_write = 1ms;
    REQUIRE(cache.insert_or_assign(key, initial_options, 100));

    manual_clock::advance(2ms);

    xtd::cache_entry_opts reload_options{};
    reload_options.expire_after_write = 10ms;

    std::atomic_size_t loader_calls{0};
    std::barrier start{thread_count};

    const auto loader = [&loader_calls](const std::size_t current_key) {
        ++loader_calls;
        std::this_thread::sleep_for(50ms);
        return current_key * 100;
    };

    std::vector<std::thread> threads;
    std::vector<xtd::cache_value<std::size_t>> results(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&cache, &start, &results, &loader, &reload_options, i] {
            start.arrive_and_wait();
            results[i] = cache.get_or_create(key, reload_options, loader);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    CHECK(loader_calls.load() == 1);

    for (const auto& result : results) {
        REQUIRE(result);
        CHECK(*result == 500);
    }

    for (std::size_t i = 0; i < results.size(); ++i) {
        for (std::size_t j = i + 1; j < results.size(); ++j) {
            CHECK(results[i] == results[j]);
        }
    }
}

#endif