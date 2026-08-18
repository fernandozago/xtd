#include "../third_party/catch2/catch_amalgamated.hpp"

#include <future>
#include <latch>
#include <thread>

#include "channel/channel.h"

using namespace std::chrono_literals;

namespace reader_tests {

    struct ReaderTests2 {};

    enum class ChannelMode : std::size_t {
        Bounded = 8,
        Unbounded = 0
    };

    struct MoveOnly
    {
        explicit MoveOnly(int value)
            : m_value(value)
        {
        }

        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;

        MoveOnly(MoveOnly&&) noexcept = default;
        MoveOnly& operator=(MoveOnly&&) noexcept = default;

        int m_value;
    };

    TEST_CASE_METHOD(ReaderTests2, "try_read returns empty when channel is open and empty", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));

        xtd::channel_reader reader(channel);

        CHECK(reader.size() == 0);

        const auto value = reader.try_read();

        CHECK(value.error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "try_read returns values in FIFO order", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));

        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(10));
        REQUIRE(writer.push(20));
        REQUIRE(writer.push(30));

        CHECK(reader.size() == 3);

        {
            const auto value = reader.try_read();

            REQUIRE(value);
            CHECK(*value == 10);
            CHECK(reader.size() == 2);
        }

        {
            const auto value = reader.try_read();

            REQUIRE(value);
            CHECK(*value == 20);
            CHECK(reader.size() == 1);
        }

        {
            const auto value = reader.try_read();

            REQUIRE(value);
            CHECK(*value == 30);
            CHECK(reader.size() == 0);
        }

        CHECK(reader.try_read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
    }

    TEST_CASE_METHOD(ReaderTests2, "read returns a queued value", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));

        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(42));

        auto value = reader.read();

        REQUIRE(value);
        CHECK(*value == 42);
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "read and try_read support move-only values", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<MoveOnly> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.emplace(42));
        REQUIRE(writer.emplace(43));

        {
            auto value = reader.try_read();

            REQUIRE(value);
            CHECK(value->m_value == 42);
        }

        {
            auto value = reader.read();

            REQUIRE(value);
            CHECK(value->m_value == 43);
        }

        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "size reflects the number of queued values", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        CHECK(reader.size() == 0);

        REQUIRE(writer.push(1));
        CHECK(reader.size() == 1);

        REQUIRE(writer.push(2));
        CHECK(reader.size() == 2);

        REQUIRE(writer.push(3));
        CHECK(reader.size() == 3);

        REQUIRE(reader.try_read().has_value());
        CHECK(reader.size() == 2);

        REQUIRE(reader.read().has_value());
        CHECK(reader.size() == 1);

        REQUIRE(reader.read().has_value());
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "completion preserves queued values", "[channel][reader]",
        )
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(10));
        REQUIRE(writer.push(20));

        writer.complete();

        CHECK(reader.size() == 2);

        {
            auto value = reader.try_read();

            REQUIRE(value);
            CHECK(*value == 10);
        }

        {
            auto value = reader.read();

            REQUIRE(value);
            CHECK(*value == 20);
        }

        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "reads return empty after completion and draining", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(42));
        writer.complete();

        auto queued = reader.read();

        REQUIRE(queued.has_value());
        CHECK(*queued == 42);

        CHECK(reader.read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.try_read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "reads return empty when channel is completed and empty", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        writer.complete();

        CHECK(reader.read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.try_read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "a blocked read receives a subsequently pushed value", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        std::latch started{1};
        std::promise<std::expected<int, xtd::channel_read_errors>> result_promise;
        auto result = result_promise.get_future();

        std::jthread read_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(reader.read(stop_token));
            });

        started.wait();

        REQUIRE(writer.push(42));

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            read_thread.request_stop();
        }

        REQUIRE(status == std::future_status::ready);

        auto value = result.get();

        REQUIRE(value.has_value());
        CHECK(*value == 42);

        read_thread.join();

        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "completion wakes a blocked reader", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        std::latch started{1};
        std::promise<std::expected<int, xtd::channel_read_errors>> result_promise;
        auto result = result_promise.get_future();

        std::jthread read_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(reader.read(stop_token));
            });

        started.wait();

        writer.complete();

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            read_thread.request_stop();
        }

        REQUIRE(status == std::future_status::ready);
        CHECK_FALSE(result.get().has_value());

        read_thread.join();
    }

    TEST_CASE_METHOD(ReaderTests2, "should wait to read when channel is empty and open", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        std::latch started{1};
        std::promise<bool> result_promise;
        auto result = result_promise.get_future();

        std::jthread read_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(reader.wait_to_read(stop_token));
            });

        started.wait();

        REQUIRE(writer.push(42));

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            read_thread.request_stop();
        }

        REQUIRE(status == std::future_status::ready);
        CHECK(result.get());
        read_thread.join();
    }

    TEST_CASE_METHOD(ReaderTests2, "an already requested stop token cancels read", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        std::stop_source stop_source;
        REQUIRE(stop_source.request_stop());

        auto value = reader.read(stop_source.get_token());

        CHECK_FALSE(value.has_value());
        CHECK(reader.size() == 0);

        // Cancelling one read must not complete or damage the channel.
        REQUIRE(writer.push(42));

        auto subsequent_value = reader.read();

        REQUIRE(subsequent_value.has_value());
        CHECK(*subsequent_value == 42);
    }

    TEST_CASE_METHOD(ReaderTests2, "requesting stop wakes a blocked reader", "[channel][reader]")
    {
        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);
        xtd::channel<int> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        std::latch started{1};
        std::promise<std::expected<int, xtd::channel_read_errors>> result_promise;
        auto result = result_promise.get_future();

        std::jthread read_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(reader.read(stop_token));
            });

        started.wait();
        read_thread.request_stop();

        const auto status = result.wait_for(1s);

        // Prevent the test from hanging during cleanup if cancellation is broken.
        if (status != std::future_status::ready)
        {
            writer.complete();
        }

        REQUIRE(status == std::future_status::ready);
        CHECK_FALSE(result.get().has_value());

        read_thread.join();

        // Cancelling one read must not complete the channel.
        REQUIRE(writer.push(42));

        auto subsequent_value = reader.read();

        REQUIRE(subsequent_value.has_value());
        CHECK(*subsequent_value == 42);
    }

    TEST_CASE_METHOD(ReaderTests2, "reading from a full bounded channel releases capacity", "[channel][reader]")
    {
        xtd::channel<int> channel(1);
        xtd::channel_writer writer(channel);
        xtd::channel_reader reader(channel);

        REQUIRE(writer.push(10));
        REQUIRE(reader.size() == 1);

        std::latch started{1};
        std::promise<bool> result_promise;
        auto result = result_promise.get_future();

        std::jthread write_thread(
            [&](std::stop_token stop_token)
            {
                started.count_down();
                result_promise.set_value(writer.push(stop_token, 20));
            });

        started.wait();

        auto first = reader.read();

        REQUIRE(first.has_value());
        CHECK(*first == 10);

        const auto status = result.wait_for(1s);

        if (status != std::future_status::ready)
        {
            write_thread.request_stop();
        }

        REQUIRE(status == std::future_status::ready);
        REQUIRE(result.get());

        write_thread.join();

        auto second = reader.read();

        REQUIRE(second.has_value());
        CHECK(*second == 20);
        CHECK(reader.size() == 0);
    }

    TEST_CASE_METHOD(ReaderTests2, "read_all generator tests", "[channel][reader]")
    {
        struct my_data {
            enum class def { copy, move, consume };
            def m_definition;
            int m_value;

            my_data(def definition, int value)
                : m_definition{definition}
                , m_value{value}
            {
            }

            my_data(const my_data&) = default;

            my_data(my_data&& other) noexcept
                : m_definition{other.m_definition}
                , m_value{other.m_value}
            {
                other.m_value = 0;
            }

            my_data& operator=(const my_data&) = default;
            my_data& operator=(my_data&&) = default;
        };

        const ChannelMode mode = GENERATE(ChannelMode::Bounded, ChannelMode::Unbounded);
        CAPTURE(mode);

        xtd::channel<my_data> channel(static_cast<size_t>(mode));
        xtd::channel_writer writer(channel);

        REQUIRE(writer.emplace(my_data::def::copy, 10));
        REQUIRE(writer.emplace(my_data::def::move, 20));
        REQUIRE(writer.emplace(my_data::def::consume, 30));
        writer.complete();

        xtd::channel_reader reader(channel);

        int items_read = 0;

        for (my_data&& val : reader.read_all()) {
            ++items_read;

            switch (val.m_definition) {
                case my_data::def::copy: {
                    CHECK(val.m_value == 10);

                    my_data copy = val;

                    CHECK(copy.m_value == 10);
                    CHECK(val.m_value == 10);
                    break;
                }

                case my_data::def::move: {
                    CHECK(val.m_value == 20);

                    my_data moved = std::move(val);

                    CHECK(moved.m_value == 20);
                    CHECK(val.m_value == 0);
                    break;
                }

                case my_data::def::consume: {
                    CHECK(val.m_value == 30);
                    break;
                }
            }
        }

        CHECK(items_read == 3);
    }
}
