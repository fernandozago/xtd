#ifndef XTD_CONCURRENT_CACHE_TESTS_H
#define XTD_CONCURRENT_CACHE_TESTS_H

#include <barrier>

#include "../third_party/catch2/catch_amalgamated.hpp"
#include "cache/concurrent_cache.h"

struct ConcurrentCacheTests {};

TEST_CASE_METHOD(ConcurrentCacheTests, "simple test: concurrent_cache can store and retrieve values")
{
    xtd::concurrent_cache<int, int> cache;
    CHECK(cache.size() == 0);
    CHECK(cache.empty());
    CHECK_FALSE(cache.contains(1));

    const xtd::cache_value value = cache.get(1);
    CHECK_FALSE(value);

    const xtd::cache_value value2 = cache.insert_or_assign(1, 1);
    CHECK(value2);
    CHECK(cache.size() == 1);
    CHECK_FALSE(cache.empty());
    CHECK(cache.contains(1));
    CHECK(*value2 == 1);
}

TEST_CASE_METHOD(ConcurrentCacheTests, "simple test: concurrent_cache get_or_create lazy loads values")
{
    xtd::concurrent_cache<int, int> cache;
    CHECK(cache.size() == 0);
    CHECK(cache.empty());
    CHECK_FALSE(cache.contains(1));
    
    int external_data = 10;
    const xtd::cache_value result = cache.get_or_create(1, [&external_data](const int key) {
        return external_data + key;
    });
    CHECK(result);
    CHECK(*result == 11);
    CHECK(cache.size() == 1);
    CHECK_FALSE(cache.empty());
    CHECK(cache.contains(1));
}

TEST_CASE_METHOD(ConcurrentCacheTests, "erased value remains valid while caller owns it")
{
    struct tracked_value {
        int m_id;
        std::string m_name;
    };

    xtd::concurrent_cache<std::size_t, tracked_value> cache;

    const xtd::cache_value value = cache.insert_or_assign( 1, 10, std::string{"this should be still alive"});

    REQUIRE(value);
    REQUIRE(cache.contains(1));

    CHECK(cache.erase(1));

    CHECK_FALSE(cache.contains(1));
    CHECK(cache.get(1) == nullptr);

    // Erasing removes only the cache's shared_ptr.
    REQUIRE(value);
    CHECK(value->m_id == 10);
    CHECK(value->m_name == "this should be still alive");
}

TEST_CASE_METHOD(ConcurrentCacheTests, "simple test: concurrent_cache can store and retrieve custom type")
{
    struct my_data{
        int id;
        std::string b;
    };
    
    xtd::concurrent_cache<int, my_data> cache;
    CHECK(cache.size() == 0);
    CHECK(cache.empty());
    CHECK_FALSE(cache.contains(1));
    CHECK_FALSE(cache.get(1));
    CHECK_FALSE(cache.contains(2));
    CHECK_FALSE(cache.get(2));

    {
        const xtd::cache_value value = cache.insert_or_assign(1, 1, "test");
        CHECK(value);
        CHECK(cache.size() == 1);
        CHECK_FALSE(cache.empty());
        CHECK(cache.contains(1));
        CHECK(value->id == 1);
    }

    {
        const xtd::cache_value value = cache.insert_or_assign(1, my_data{2, "test2"});
        CHECK(value);
        CHECK(cache.size() == 1);
        CHECK_FALSE(cache.empty());
        CHECK(cache.contains(1));
        CHECK(value->id == 2);
    }
}

TEST_CASE_METHOD(ConcurrentCacheTests, "simple test: concurrent_cache protects against cache stampede")
{
    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    std::atomic_size_t loader_calls{0};
    std::barrier start{10};

    const auto loader = [&loader_calls](const std::size_t key) {
            ++loader_calls;
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            return key * 2;
        };

    std::vector<std::thread> threads;
    std::vector<xtd::cache_value<std::size_t>> results(10);

    for (std::size_t i = 0; i < 10; ++i) {
        threads.emplace_back([&cache, &results, &loader, &start, i] {
            start.arrive_and_wait();
            results[i] = cache.get_or_create(i % 3, loader);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Three distinct keys: 0, 1 and 2.
    CHECK(loader_calls.load() == 3);

    for (std::size_t i = 0; i < results.size(); ++i) {
        REQUIRE(results[i]);
        CHECK(*results[i] == (i % 3) * 2);
    }

    // Calls for the same key receive the same shared_ptr.
    for (std::size_t i = 0; i < results.size(); ++i) {
        for (std::size_t j = i + 1; j < results.size(); ++j) {
            if (i % 3 == j % 3) {
                CHECK(results[i] == results[j]);
            }
        }
    }
}

TEST_CASE_METHOD(ConcurrentCacheTests, "concurrent_cache propagates loader failure and allows retry")
{
    static constexpr std::size_t thread_count = 10;
    static constexpr std::size_t key = 42;

    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    std::atomic_size_t loader_calls{0};
    std::barrier start{thread_count};

    const auto failing_loader = [&loader_calls](const std::size_t) -> std::size_t {
        ++loader_calls;

        // Keep the loader active long enough for the other callers
        // to join the same in-flight operation.
        std::this_thread::sleep_for(std::chrono::milliseconds{100});

        throw std::runtime_error{"load failed"};
    };

    std::vector<std::thread> threads;
    std::vector<std::exception_ptr> errors(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&cache, &start, &failing_loader, &errors, i] {
            start.arrive_and_wait();

            try {
                static_cast<void>(cache.get_or_create(key, failing_loader));
            }
            catch (...) {
                errors[i] = std::current_exception();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Cache stampede protection: only one loader executed.
    CHECK(loader_calls.load() == 1);

    // Every caller received the loader exception.
    for (const auto& error : errors) {
        REQUIRE(error);

        try {
            std::rethrow_exception(error);
            FAIL("expected get_or_create to throw");
        }
        catch (const std::runtime_error& exception) {
            CHECK(
                std::string_view{exception.what()} ==
                "load failed");
        }
    }

    // The failed in-flight state must have been removed,
    // allowing a subsequent call to retry successfully.
    const auto value = cache.get_or_create(
        key,
        [&loader_calls](const std::size_t current_key) {
            ++loader_calls;
            return current_key * 2;
        });

    REQUIRE(value);
    CHECK(*value == 84);
    CHECK(loader_calls.load() == 2);
}

TEST_CASE_METHOD(
    ConcurrentCacheTests,
    "concurrent_cache returns an existing value")
{
    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    const auto inserted = cache.insert_or_assign(10, 20);
    const auto current = cache.get(10);

    REQUIRE(current);
    CHECK(current == inserted);
    CHECK(*current == 20);

    CHECK(cache.contains(10));
    CHECK(cache.size() == 1);
    CHECK_FALSE(cache.empty());
}

TEST_CASE_METHOD(ConcurrentCacheTests, "concurrent_cache replaces a value without invalidating previous readers")
{
    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    const auto previous = cache.insert_or_assign(10, 20);
    const auto current = cache.insert_or_assign(10, 40);

    REQUIRE(previous);
    REQUIRE(current);

    CHECK(previous != current);
    CHECK(*previous == 20);
    CHECK(*current == 40);

    const auto cached = cache.get(10);

    REQUIRE(cached);
    CHECK(cached == current);
    CHECK(*cached == 40);

    CHECK(cache.size() == 1);
}

TEST_CASE_METHOD(ConcurrentCacheTests, "concurrent_cache erases values while preserving existing references")
{
    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    const auto held_value = cache.insert_or_assign(10, 20);

    REQUIRE(held_value);
    REQUIRE(cache.contains(10));

    CHECK(cache.erase(10));
    CHECK_FALSE(cache.erase(10));

    CHECK_FALSE(cache.contains(10));
    CHECK(cache.get(10) == nullptr);
    CHECK(cache.size() == 0);
    CHECK(cache.empty());

    // The cache released ownership, but this caller still owns the value.
    CHECK(*held_value == 20);
}

TEST_CASE_METHOD(ConcurrentCacheTests, "get_or_create does not invoke loader when value is cached")
{
    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    const auto inserted = cache.insert_or_assign(10, 20);

    std::atomic_size_t loader_calls{0};

    const auto result = cache.get_or_create(
        10,
        [&loader_calls](const std::size_t key) {
            ++loader_calls;
            return key * 10;
        });

    REQUIRE(result);

    CHECK(result == inserted);
    CHECK(*result == 20);
    CHECK(loader_calls.load() == 0);
}

TEST_CASE_METHOD(ConcurrentCacheTests, "explicit insertion wins over an active get_or_create loader")
{
    constexpr std::size_t key = 10;

    xtd::concurrent_cache<std::size_t, std::size_t> cache;

    std::promise<void> loader_started_promise;
    std::future<void> loader_started =
        loader_started_promise.get_future();

    std::promise<void> continue_loader_promise;
    std::shared_future<void> continue_loader =
        continue_loader_promise.get_future().share();

    xtd::cache_value<std::size_t> loaded_result;

    std::thread loader_thread{
        [&] {
            loaded_result = cache.get_or_create(
                key,
                [&](const std::size_t current_key) {
                    loader_started_promise.set_value();

                    continue_loader.wait();

                    return current_key * 2;
                });
        }
    };

    loader_started.wait();

    // This value is published while the loader is outside the cache lock.
    const auto explicitly_inserted =
        cache.insert_or_assign(key, 99);

    continue_loader_promise.set_value();
    loader_thread.join();

    REQUIRE(loaded_result);
    REQUIRE(explicitly_inserted);

    // get_or_create publishes the existing explicit value,
    // not the stale value produced by its loader.
    CHECK(loaded_result == explicitly_inserted);
    CHECK(*loaded_result == 99);

    const auto current = cache.get(key);

    REQUIRE(current);
    CHECK(current == explicitly_inserted);
}

struct tracked_value
{
    inline static std::size_t direct_constructions = 0;
    inline static std::size_t copy_constructions = 0;
    inline static std::size_t move_constructions = 0;
    inline static std::size_t destructions = 0;

    int m_id{};
    std::string m_name;
    bool moved_from{false};

    tracked_value()
    {
        ++direct_constructions;
    }

    tracked_value(int id, std::string name)
        : m_id{id}
        , m_name{std::move(name)}
    {
        ++direct_constructions;
    }

    tracked_value(const tracked_value& other)
        : m_id{other.m_id}
        , m_name{other.m_name}
    {
        ++copy_constructions;
    }

    tracked_value(tracked_value&& other) noexcept
        : m_id{other.m_id}
        , m_name{std::move(other.m_name)}
    {
        ++move_constructions;
        other.moved_from = true;
    }

    tracked_value& operator=(const tracked_value&) = delete;
    tracked_value& operator=(tracked_value&&) = delete;

    ~tracked_value()
    {
        ++destructions;
    }

    static void reset()
    {
        direct_constructions = 0;
        copy_constructions = 0;
        move_constructions = 0;
        destructions = 0;
    }
};

TEST_CASE_METHOD(ConcurrentCacheTests, "insert_or_assign perfectly forwards all supported argument forms")
{
    enum class invocation_type
    {
        lvalue,
        const_lvalue,
        moved_lvalue,
        temporary,
        constructor_arguments,
        default_constructor
    };

    struct test_case
    {
        std::string_view name;
        invocation_type invocation;

        int expected_id;
        std::string_view expected_name;

        std::size_t expected_direct_constructions;
        std::size_t expected_copy_constructions;
        std::size_t expected_move_constructions;
        std::size_t expected_destructions;

        bool has_source;
        bool expected_source_moved;
    };

    constexpr std::array test_cases{
        test_case{
            .name = "copies an lvalue",
            .invocation = invocation_type::lvalue,
            .expected_id = 10,
            .expected_name = "lvalue",
            .expected_direct_constructions = 0,
            .expected_copy_constructions = 1,
            .expected_move_constructions = 0,
            .expected_destructions = 0,
            .has_source = true,
            .expected_source_moved = false
        },
        test_case{
            .name = "copies a const lvalue",
            .invocation = invocation_type::const_lvalue,
            .expected_id = 20,
            .expected_name = "const lvalue",
            .expected_direct_constructions = 0,
            .expected_copy_constructions = 1,
            .expected_move_constructions = 0,
            .expected_destructions = 0,
            .has_source = true,
            .expected_source_moved = false
        },
        test_case{
            .name = "moves an explicitly moved lvalue",
            .invocation = invocation_type::moved_lvalue,
            .expected_id = 30,
            .expected_name = "moved lvalue",
            .expected_direct_constructions = 0,
            .expected_copy_constructions = 0,
            .expected_move_constructions = 1,
            .expected_destructions = 0,
            .has_source = true,
            .expected_source_moved = true
        },
        test_case{
            .name = "moves a temporary value",
            .invocation = invocation_type::temporary,
            .expected_id = 40,
            .expected_name = "temporary",
            .expected_direct_constructions = 1,
            .expected_copy_constructions = 0,
            .expected_move_constructions = 1,
            .expected_destructions = 1,
            .has_source = false,
            .expected_source_moved = false
        },
        test_case{
            .name = "constructs directly from arguments",
            .invocation = invocation_type::constructor_arguments,
            .expected_id = 50,
            .expected_name = "direct construction",
            .expected_direct_constructions = 1,
            .expected_copy_constructions = 0,
            .expected_move_constructions = 0,
            .expected_destructions = 0,
            .has_source = false,
            .expected_source_moved = false
        },
        test_case{
            .name = "default constructs the value",
            .invocation = invocation_type::default_constructor,
            .expected_id = 0,
            .expected_name = "",
            .expected_direct_constructions = 1,
            .expected_copy_constructions = 0,
            .expected_move_constructions = 0,
            .expected_destructions = 0,
            .has_source = false,
            .expected_source_moved = false
        }
    };

    for (const auto& test : test_cases) {
        DYNAMIC_SECTION("insert_or_assign " << test.name) {
            xtd::concurrent_cache<std::size_t, tracked_value> cache;

            /*
             * Keep source values alive until after all assertions.
             * Their initial construction occurs before reset(), so it is
             * intentionally excluded from the measured operations.
             */
            std::unique_ptr<tracked_value> source;

            switch (test.invocation) {
                case invocation_type::lvalue:
                case invocation_type::const_lvalue:
                case invocation_type::moved_lvalue:
                    source = std::make_unique<tracked_value>(
                        test.expected_id,
                        std::string{test.expected_name});
                    break;

                case invocation_type::temporary:
                case invocation_type::constructor_arguments:
                case invocation_type::default_constructor:
                    break;
            }

            tracked_value::reset();

            xtd::cache_value<tracked_value> current;

            switch (test.invocation) {
                case invocation_type::lvalue:
                    current = cache.insert_or_assign(1, *source);
                    break;

                case invocation_type::const_lvalue:
                    current = cache.insert_or_assign(
                        1,
                        std::as_const(*source));
                    break;

                case invocation_type::moved_lvalue:
                    current = cache.insert_or_assign(
                        1,
                        std::move(*source));
                    break;

                case invocation_type::temporary:
                    current = cache.insert_or_assign(
                        1,
                        tracked_value{
                            test.expected_id,
                            std::string{test.expected_name}
                        });
                    break;

                case invocation_type::constructor_arguments:
                    current = cache.insert_or_assign(
                        1,
                        test.expected_id,
                        std::string{test.expected_name});
                    break;

                case invocation_type::default_constructor:
                    current = cache.insert_or_assign(1);
                    break;
            }

            REQUIRE(current);

            CHECK(current->m_id == test.expected_id);
            CHECK(current->m_name == test.expected_name);
            CHECK(tracked_value::direct_constructions == test.expected_direct_constructions);
            CHECK(tracked_value::copy_constructions == test.expected_copy_constructions);
            CHECK(tracked_value::move_constructions == test.expected_move_constructions);
            CHECK(tracked_value::destructions == test.expected_destructions);

            if (test.has_source) {
                REQUIRE(source);

                CHECK(current.get() != source.get());
                CHECK(
                    source->moved_from ==
                    test.expected_source_moved);
            }
            else {
                CHECK_FALSE(source);
            }

            const auto cached = cache.get(1);

            REQUIRE(cached);
            CHECK(cached == current);
        }
    }
}

#endif
