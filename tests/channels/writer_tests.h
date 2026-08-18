#include "../third_party/catch2/catch_amalgamated.hpp"

#include <latch>
#include <thread>
#include <future>

#include "channel/channel.h"

using namespace std::chrono_literals;

namespace writer_tests {
    struct WriterTests {};

    enum class ChannelMode : std::size_t {
        Bounded = 8,
        Unbounded = 0
    };

    namespace channel_tests
    {
        struct Probe
        {
            struct Counters
            {
                std::size_t constructions = 0;
                std::size_t copies = 0;
                std::size_t moves = 0;
                std::size_t destructions = 0;
            };

            using counters_ptr = std::shared_ptr<Counters>;

            static counters_ptr make_counters()
            {
                return std::make_shared<Counters>();
            }

            explicit Probe(
                int value,
                counters_ptr counters = make_counters())
                : m_value(value)
                , m_counters(std::move(counters))
            {
                ++m_counters->constructions;
            }

            Probe(const Probe& other)
                : m_value(other.m_value)
                , m_counters(other.m_counters)
            {
                ++m_counters->constructions;
                ++m_counters->copies;
            }

            Probe(Probe&& other) noexcept
                : m_value(other.m_value)
                , m_counters(other.m_counters)
            {
                ++m_counters->constructions;
                ++m_counters->moves;

                other.m_moved_from = true;
            }

            ~Probe()
            {
                ++m_counters->destructions;
            }

            Probe& operator=(const Probe&) = delete;
            Probe& operator=(Probe&&) = delete;

            [[nodiscard]] bool moved_from() const noexcept
            {
                return m_moved_from;
            }

            [[nodiscard]] const Counters& counters() const noexcept
            {
                return *m_counters;
            }

            int m_value;

        private:
            counters_ptr m_counters;
            bool m_moved_from = false;
        };
    }

    TEST_CASE_METHOD(WriterTests, "channel copies an lvalue", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<channel_tests::Probe> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);

        SECTION("channel push copies an lvalue")
        {
            channel_tests::Probe data(42);

            CHECK(writer.push(data));

            CHECK_FALSE(data.moved_from());
            CHECK(data.counters().copies == 1);
            CHECK(data.counters().moves == 0);
        }

        SECTION("channel try_push copies an lvalue")
        {
            channel_tests::Probe data(43);

            CHECK(writer.try_push(data));

            CHECK_FALSE(data.moved_from());
            CHECK(data.counters().copies == 1);
            CHECK(data.counters().moves == 0);
        }
    }

    TEST_CASE_METHOD(WriterTests, "channel moves a rvalue", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<channel_tests::Probe> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);

        SECTION("push moves an rvalue")
        {
            channel_tests::Probe data{42};
            REQUIRE(writer.push(std::move(data)));

            CHECK(data.moved_from());
            CHECK(data.counters().constructions == 2);
            CHECK(data.counters().copies == 0);
            CHECK(data.counters().moves == 1);
        }

        SECTION("try_push moves an rvalue")
        {
            channel_tests::Probe data{43};
            REQUIRE(writer.try_push(std::move(data)));

            CHECK(data.moved_from());
            CHECK(data.counters().constructions == 2);
            CHECK(data.counters().copies == 0);
            CHECK(data.counters().moves == 1);
        }
    }

    TEST_CASE_METHOD(WriterTests, "channel constructs a value", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<channel_tests::Probe> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);

        SECTION("push constructs a value")
        {
            auto counters = channel_tests::Probe::make_counters();

            REQUIRE(writer.emplace(44, counters));

            CHECK(counters->constructions == 1);
            CHECK(counters->copies == 0);
            CHECK(counters->moves == 0);
        }

        SECTION("try_emplace constructs a value")
        {
            auto counters = channel_tests::Probe::make_counters();

            REQUIRE(writer.try_emplace(44, counters));

            CHECK(counters->constructions == 1);
            CHECK(counters->copies == 0);
            CHECK(counters->moves == 0);
        }
    }

    TEST_CASE_METHOD(WriterTests, "try_emplace succeeds while open and fails after completion", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        CHECK(writer.try_emplace(42));

        auto value = reader.read();
        REQUIRE(value.has_value());
        CHECK(*value == 42);

        writer.complete();
        CHECK_FALSE(writer.try_emplace(43));
    }

    TEST_CASE_METHOD(WriterTests, "try operations fail without consuming values when bounded channel is full", "[channel][writer]")
    {
        xtd::channel<channel_tests::Probe> channel(1);
        xtd::channel_writer writer(channel);

        REQUIRE(writer.emplace(1));

        SECTION("try_push lvalue does not copy")
        {
            channel_tests::Probe value{2};

            CHECK_FALSE(writer.try_push(value));
            CHECK_FALSE(value.moved_from());
            CHECK(value.counters().copies == 0);
            CHECK(value.counters().moves == 0);
        }

        SECTION("try_push rvalue does not move")
        {
            channel_tests::Probe value{2};

            CHECK_FALSE(writer.try_push(std::move(value)));
            CHECK_FALSE(value.moved_from());
            CHECK(value.counters().copies == 0);
            CHECK(value.counters().moves == 0);
        }

        SECTION("try_emplace does not construct")
        {
            auto counters = channel_tests::Probe::make_counters();

            CHECK_FALSE(writer.try_emplace(2, counters));
            CHECK(counters->constructions == 0);
        }
    }

    TEST_CASE_METHOD(WriterTests, "writer operations reject values after completion", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<channel_tests::Probe> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);

        writer.complete();
        writer.complete();

        channel_tests::Probe lvalue{1};
        CHECK_FALSE(writer.push(lvalue));
        CHECK_FALSE(writer.try_push(lvalue));
        CHECK(lvalue.counters().copies == 0);

        channel_tests::Probe rvalue{2};
        CHECK_FALSE(writer.push(std::move(rvalue)));
        CHECK_FALSE(writer.try_push(std::move(rvalue)));
        CHECK_FALSE(rvalue.moved_from());
        CHECK(rvalue.counters().moves == 0);

        auto counters = channel_tests::Probe::make_counters();
        CHECK_FALSE(writer.emplace(3, counters));
        CHECK_FALSE(writer.try_emplace(3, counters));
        CHECK(counters->constructions == 0);
    }

    TEST_CASE_METHOD(WriterTests, "stop token cancels a blocked push", "[channel][writer]")
    {
        xtd::channel<int> channel(1);
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(1));

        std::atomic<bool> result{true};
        std::latch started{1};

        std::jthread producer([&](std::stop_token token)
        {
            started.count_down();
            result.store(writer.push(token, 2));
        });

        started.wait();
        producer.request_stop();
        producer.join();

        CHECK_FALSE(result.load());

        auto value = reader.read();
        REQUIRE(value.has_value());
        CHECK(*value == 1);
    }

    TEST_CASE_METHOD(WriterTests, "completion preserves queued values", "[channel][writer]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(42));
        writer.complete();

        auto value = reader.read();
        REQUIRE(value.has_value());
        CHECK(*value == 42);

        CHECK_FALSE(reader.read().has_value());
    }

    TEST_CASE_METHOD(WriterTests, "cancel while waiting to push", "[channel][writer]")
    {
        xtd::channel<int> channel(1);
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(42));
        REQUIRE(reader.size() == 1);

        std::latch started{1};
        std::promise<bool> result_promise;
        auto result = result_promise.get_future();

        std::jthread write_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(writer.push(stop_token, 43));
            });

        started.wait();
        REQUIRE(result.wait_for(50ms) == std::future_status::timeout);

        write_thread.request_stop();

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            writer.complete();
        }

        REQUIRE(status == std::future_status::ready);
        CHECK_FALSE(result.get());

        write_thread.join();

        CHECK(reader.size() == 1);

        auto value = reader.read();

        REQUIRE(value.has_value());
        CHECK(*value == 42);
        CHECK(reader.size() == 0);

        REQUIRE(writer.push(44));

        auto subsequent_value = reader.read();

        REQUIRE(subsequent_value.has_value());
        CHECK(*subsequent_value == 44);
    }

    TEST_CASE_METHOD(WriterTests, "read while waiting to push", "[channel][writer]")
    {
        xtd::channel<int> channel(1);
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(42));
        REQUIRE(reader.size() == 1);

        std::latch started{1};
        std::promise<bool> result_promise;
        auto result = result_promise.get_future();

        std::thread write_thread(
            [&]
            {
                started.count_down();
                result_promise.set_value(writer.push(43));
            });

        started.wait();
        REQUIRE(result.wait_for(50ms) == std::future_status::timeout);

        auto first = reader.read();

        REQUIRE(first.has_value());
        CHECK(*first == 42);

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            writer.complete();
        }

        REQUIRE(status == std::future_status::ready);
        CHECK(result.get());

        write_thread.join();

        CHECK(reader.size() == 1);

        auto second = reader.read();

        REQUIRE(second.has_value());
        CHECK(*second == 43);
        CHECK(reader.size() == 0);
    }

}
