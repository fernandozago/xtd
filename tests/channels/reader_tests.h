#include "../third_party/catch2/catch_amalgamated.hpp"

#include <future>
#include <latch>
#include <thread>

#include "channel/channel.h"
#include "channel/channel_impl.h"

using namespace std::chrono_literals;

namespace reader_tests {
    template <typename T>
    struct ReaderTests {};

    struct BoundedChannelMode {};
    struct UnboundedChannelMode {};

    template <typename Mode, typename T>
    xtd::channel<T> make_channel(std::size_t capacity = 8)
    {
        if constexpr (std::is_same_v<Mode, BoundedChannelMode>)
        {
            return xtd::channel<T>(capacity);
        }
        else
        {
            return xtd::channel<T>();
        }
    }

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "try_read returns empty when channel is open and empty", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& reader = channel.reader();

        CHECK(reader.size() == 0);

        const auto value = reader.try_read();

        CHECK(value.error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.size() == 0);
    }

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "try_read returns values in FIFO order", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "read returns a queued value", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

        REQUIRE(writer.push(42));

        auto value = reader.read();

        REQUIRE(value);
        CHECK(*value == 42);
        CHECK(reader.size() == 0);
    }

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "read and try_read support move-only values", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, MoveOnly>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "size reflects the number of queued values", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "completion preserves queued values", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "reads return empty after completion and draining", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "reads return empty when channel is completed and empty", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

        writer.complete();

        CHECK(reader.read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.try_read().error() == xtd::channel_read_errors::CHANNEL_EMPTY);
        CHECK(reader.size() == 0);
    }

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "a blocked read receives a subsequently pushed value", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "completion wakes a blocked reader", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "should wait to read when channel is empty and open", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "an already requested stop token cancels read", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "requesting stop wakes a blocked reader", "[channel][reader]",
        BoundedChannelMode,
        UnboundedChannelMode)
    {
        auto channel = make_channel<TestType, int>();
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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

    TEMPLATE_TEST_CASE_METHOD(ReaderTests, "reading from a full bounded channel releases capacity", "[channel][reader]",
        BoundedChannelMode)
    {
        auto channel = make_channel<BoundedChannelMode, int>(1);
        auto& writer = channel.writer();
        auto& reader = channel.reader();

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
}
