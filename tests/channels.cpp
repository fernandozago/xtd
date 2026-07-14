#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest.h"
#include <chrono>
#include <future>
#include <memory>
#include "channel/unbounded_channel.h"
#include "channel/bounded_channel.h"

TEST_CASE("BoundedChannel semantics - copy, move, emplace")
{
    struct ChannelSemanticsProbe
    {
        std::shared_ptr<int> payload;
    
        explicit ChannelSemanticsProbe(int value)
            : payload(std::make_shared<int>(value))
        {
        }
    };

    {
        xtd::bounded_channel<ChannelSemanticsProbe, 5> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(42);

        CHECK(writer.push(data));
        CHECK(data.payload != nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();
        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 42);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::bounded_channel<ChannelSemanticsProbe, 5> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(43);

        CHECK(writer.push(std::move(data)));
        CHECK(data.payload == nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();
        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 43);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::bounded_channel<ChannelSemanticsProbe, 5> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(44);

        CHECK(writer.emplace(data));
        CHECK(data.payload != nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();
        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 44);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::bounded_channel<ChannelSemanticsProbe, 5> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(45);

        CHECK(writer.emplace(std::move(data)));
        CHECK(data.payload == nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();
        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 45);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::bounded_channel<ChannelSemanticsProbe, 5> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();

        CHECK(writer.emplace(46));

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();
        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 46);
        CHECK(!reader.try_read().has_value());
    }
}

TEST_CASE("UnboundedChannel semantics - copy, move, emplace")
{
    struct ChannelSemanticsProbe
    {
        std::shared_ptr<int> payload;

        explicit ChannelSemanticsProbe(int value)
            : payload(std::make_shared<int>(value))
        {
        }
    };

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(42);

        CHECK(writer.push(data));
        CHECK(data.payload != nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();

        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 42);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(43);

        CHECK(writer.push(std::move(data)));
        CHECK(data.payload == nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();

        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 43);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        writer.complete(); // complete the channel before pushing data
        
        ChannelSemanticsProbe data(43);
        CHECK(!writer.push(std::move(data)));
        CHECK(data.payload != nullptr); // did not move because push failed
        CHECK(!writer.try_push(std::move(data)));
        CHECK(data.payload != nullptr); // did not move because push failed
    }

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(44);

        CHECK(writer.emplace(data));
        CHECK(data.payload != nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();

        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 44);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();
        ChannelSemanticsProbe data(45);

        CHECK(writer.emplace(std::move(data)));
        CHECK(data.payload == nullptr);

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();

        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 45);
        CHECK(!reader.try_read().has_value());
    }

    {
        xtd::unbounded_channel<ChannelSemanticsProbe> channel;
        xtd::channel_writer<ChannelSemanticsProbe>& writer = channel.writer();

        CHECK(writer.emplace(46));

        writer.complete();

        xtd::channel_reader<ChannelSemanticsProbe>& reader = channel.reader();
        auto value = reader.read();

        CHECK(value.has_value());
        CHECK(value->payload != nullptr);
        CHECK(*value->payload == 46);
        CHECK(!reader.try_read().has_value());
    }
}

TEST_CASE("BoundedChannel try_push returns false when full and does not block")
{
    xtd::bounded_channel<int, 1> channel;
    xtd::channel_writer<int>& writer = channel.writer();

    int first = 1;
    CHECK(writer.try_push(first));

    const auto start = std::chrono::steady_clock::now();
    CHECK_FALSE(writer.try_push(2));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < 10);
}

TEST_CASE("BoundedChannel try_push returns false after completion")
{
    xtd::bounded_channel<int, 2> channel;
    xtd::channel_writer<int>& writer = channel.writer();

    writer.complete();

    int value = 7;
    CHECK_FALSE(writer.try_push(value));
    CHECK_FALSE(writer.try_push(8));
}

TEST_CASE("BoundedChannel try_emplace returns false when full and does not block")
{
    xtd::bounded_channel<int, 1> channel;
    xtd::channel_writer<int>& writer = channel.writer();

    CHECK(writer.try_emplace(1));

    const auto start = std::chrono::steady_clock::now();
    CHECK_FALSE(writer.try_emplace(2));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() < 10);
}

TEST_CASE("BoundedChannel try_emplace returns false after completion")
{
    xtd::bounded_channel<int, 2> channel;
    xtd::channel_writer<int>& writer = channel.writer();

    writer.complete();

    CHECK_FALSE(writer.try_emplace(7));
}

TEST_CASE("UnboundedChannel try_emplace succeeds before completion and fails after")
{
    xtd::unbounded_channel<int> channel;
    xtd::channel_writer<int>& writer = channel.writer();
    xtd::channel_reader<int>& reader = channel.reader();

    CHECK(writer.try_emplace(42));

    auto value = reader.read();
    CHECK(value.has_value());
    CHECK(*value == 42);

    writer.complete();
    CHECK_FALSE(writer.try_emplace(43));
}

TEST_CASE("UnboundedChannel try_push succeeds before completion and fails after")
{
    xtd::unbounded_channel<int> channel;
    xtd::channel_writer<int>& writer = channel.writer();
    xtd::channel_reader<int>& reader = channel.reader();

    int lvalue = 10;
    CHECK(writer.try_push(lvalue));
    CHECK(writer.try_push(11));

    auto first = reader.read();
    auto second = reader.read();
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(*first == 10);
    CHECK(*second == 11);

    writer.complete();
    CHECK_FALSE(writer.try_push(lvalue));
    CHECK_FALSE(writer.try_push(12));
}

TEST_CASE("BoundedChannel reader get_size reflects enqueue and dequeue")
{
    xtd::bounded_channel<int, 3> channel;
    xtd::channel_writer<int>& writer = channel.writer();
    xtd::channel_reader<int>& reader = channel.reader();

    CHECK(reader.size() == 0);
    CHECK(writer.try_push(1));
    CHECK(writer.try_push(2));
    CHECK(reader.size() == 2);

    auto first = reader.read();
    CHECK(first.has_value());
    CHECK(*first == 1);
    CHECK(reader.size() == 1);
}

TEST_CASE("UnboundedChannel reader get_size reflects enqueue and dequeue")
{
    xtd::unbounded_channel<int> channel;
    xtd::channel_writer<int>& writer = channel.writer();
    xtd::channel_reader<int>& reader = channel.reader();

    CHECK(reader.size() == 0);
    CHECK(writer.try_emplace(21));
    CHECK(writer.try_emplace(22));
    CHECK(reader.size() == 2);

    auto first = reader.read();
    CHECK(first.has_value());
    CHECK(*first == 21);
    CHECK(reader.size() == 1);
}

TEST_CASE("BoundedChannel push blocks when full and resumes after read")
{
    using namespace std::chrono_literals;

    xtd::bounded_channel<int, 1> channel;
    auto& writer = channel.writer();
    auto& reader = channel.reader();

    CHECK(writer.push(1));

    auto blockedPush = std::async(std::launch::async, [&]() {
        return writer.push(2);
    });

    CHECK(blockedPush.wait_for(50ms) == std::future_status::timeout);

    auto first = reader.read();
    REQUIRE(first.has_value());
    CHECK(*first == 1);

    REQUIRE(blockedPush.wait_for(1s) == std::future_status::ready);
    CHECK(blockedPush.get());

    auto second = reader.read();
    REQUIRE(second.has_value());
    CHECK(*second == 2);

    writer.complete();
    CHECK_FALSE(reader.read().has_value());
}

TEST_CASE("BoundedChannel blocked push returns false when channel is completed")
{
    using namespace std::chrono_literals;

    xtd::bounded_channel<int, 1> channel;
    auto& writer = channel.writer();

    CHECK(writer.push(1));

    auto blockedPush = std::async(std::launch::async, [&]() {
        return writer.push(2);
    });

    CHECK(blockedPush.wait_for(50ms) == std::future_status::timeout);

    writer.complete();

    REQUIRE(blockedPush.wait_for(1s) == std::future_status::ready);
    CHECK_FALSE(blockedPush.get());
}

TEST_CASE("BoundedChannel read blocks while empty and unblocks on complete")
{
    using namespace std::chrono_literals;

    xtd::bounded_channel<int, 2> channel;
    auto& writer = channel.writer();
    auto& reader = channel.reader();

    auto blockedRead = std::async(std::launch::async, [&]() {
        return reader.read();
    });

    CHECK(blockedRead.wait_for(50ms) == std::future_status::timeout);

    writer.complete();

    REQUIRE(blockedRead.wait_for(1s) == std::future_status::ready);
    CHECK_FALSE(blockedRead.get().has_value());
}

TEST_CASE("UnboundedChannel read blocks while empty and unblocks on pushed value")
{
    using namespace std::chrono_literals;

    xtd::unbounded_channel<int> channel;
    auto& writer = channel.writer();
    auto& reader = channel.reader();

    auto blockedRead = std::async(std::launch::async, [&]() {
        return reader.read();
    });

    CHECK(blockedRead.wait_for(50ms) == std::future_status::timeout);

    CHECK(writer.push(99));

    REQUIRE(blockedRead.wait_for(1s) == std::future_status::ready);
    auto value = blockedRead.get();
    REQUIRE(value.has_value());
    CHECK(*value == 99);

    writer.complete();
}

TEST_CASE("UnboundedChannel read returns nullopt after completion when empty")
{
    xtd::unbounded_channel<int> channel;
    auto& writer = channel.writer();
    auto& reader = channel.reader();

    writer.complete();
    writer.complete();

    CHECK_FALSE(reader.read().has_value());
    CHECK_FALSE(reader.try_read().has_value());
    CHECK_FALSE(writer.push(1));
}

TEST_CASE("BoundedChannel supports ring-buffer wrap-around correctly")
{
    xtd::bounded_channel<int, 3> channel;
    auto& writer = channel.writer();
    auto& reader = channel.reader();

    CHECK(writer.push(1));
    CHECK(writer.push(2));
    CHECK(writer.push(3));

    auto one = reader.read();
    auto two = reader.read();
    REQUIRE(one.has_value());
    REQUIRE(two.has_value());
    CHECK(*one == 1);
    CHECK(*two == 2);

    CHECK(writer.push(4));
    CHECK(writer.push(5));

    auto three = reader.read();
    auto four = reader.read();
    auto five = reader.read();
    REQUIRE(three.has_value());
    REQUIRE(four.has_value());
    REQUIRE(five.has_value());
    CHECK(*three == 3);
    CHECK(*four == 4);
    CHECK(*five == 5);

    writer.complete();
    CHECK_FALSE(reader.read().has_value());
}
