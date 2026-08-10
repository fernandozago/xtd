#ifndef XTD_TESTS_PIPELINE_TESTS_H
#define XTD_TESTS_PIPELINE_TESTS_H
#define XTD_ALLOW_EXPERIMENTAL

#include "../third_party/catch2/catch_amalgamated.hpp"

#include <array>
#include <cstddef>
#include <future>
#include <print>
#include <type_traits>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pipeline/pipeline.h"
#include "pipeline/pipe_utils.h"
#include "pipeline/segmented_byte_view.h"


namespace xtd_pipeline_tests {
template <typename T>
struct PipelineTests {};

struct FixedAllocatorMode {};
struct ArenaAllocatorMode {};
struct UnsynchronizedAllocatorMode {};

template <typename Mode>
xtd::allocator_kind allocator_for_mode()
{
    if constexpr (std::is_same_v<Mode, FixedAllocatorMode>) {
        return xtd::allocator_kind::fixed_pool_resource;
    }
    else if constexpr (std::is_same_v<Mode, ArenaAllocatorMode>) {
        return xtd::allocator_kind::arena_pool_resource;
    }
    else {
        static_assert(std::is_same_v<Mode, UnsynchronizedAllocatorMode>);
        return xtd::allocator_kind::unsynchronized_pool_resource;
    }
}

template <typename Mode>
xtd::pipeline make_pipeline(xtd::pipe_options options = {})
{
    options.allocator = allocator_for_mode<Mode>();
    return xtd::pipeline(options);
}

inline std::size_t readCurrentRssKb()
{
    std::ifstream status("/proc/self/status");
    if (!status)
        throw std::runtime_error("failed to read /proc/self/status");

    std::string key;
    while (status >> key)
    {
        if (key == "VmRSS:")
        {
            std::size_t valueKb = 0;
            status >> valueKb;
            return valueKb;
        }

        std::string restOfLine;
        std::getline(status, restOfLine);
    }

    throw std::runtime_error("VmRSS not found in /proc/self/status");
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: multiple c_str writes are parsed into complete lines", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const std::byte delimiter = std::byte{'\0'};
    const std::array<std::string, 3> expected = {
        "one", 
        "two", 
        "three"
    };

    xtd::pipeline pipeline = make_pipeline<TestType>();
    
    std::thread t([&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        for (const auto& msg : expected) {
            writer.write(reinterpret_cast<const std::byte*>(msg.data()), msg.size() + 1);
        }
        writer.complete();
    });
    
    std::size_t index = 0;
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view seq = result.buffer();
        
        while (const xtd::position pos = seq.position_of(delimiter))
        {
            ++index;
            seq.slice_in_place(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());

        if (result.completed()) break;
    }

    t.join();
    CHECK(index == expected.size());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: delayed character writes are parsed into complete lines", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    
    std::thread producer([&]()
    {
        using namespace std::chrono_literals;
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("h") == 1);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("e") == 1);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("l") == 1);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("l") == 1);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("o") == 1);
        std::this_thread::sleep_for(.1s);
        CHECK(writer.write("\n") == 1);
        std::this_thread::sleep_for(.1s);
        writer.complete();
    });
    
    
    int readCount = 0;
    std::size_t received_lines = 0;
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view ros = result.buffer();
        readCount++;
        
        while (const xtd::position pos = ros.position_of('\n'))
        {
            ++received_lines;
            ros.slice_in_place(pos + 1, ros.end());
        }

        reader.advance(ros.begin(), ros.end());

        if (result.completed()) break;
    }

    producer.join();
    CHECK(received_lines == 1);
    CHECK(readCount == 7);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: isCompleted set only after all data is consumed", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    std::thread t([&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        CHECK(writer.write("done\n") == 5);
        writer.complete();
    });
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    bool sawCompleted = false;
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view ros = result.buffer();
        
        reader.advance(ros.begin(), ros.end());

        if (result.completed()) {
            sawCompleted = true;
            break;
        }
    }

    t.join();
    CHECK(sawCompleted);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: message spans multiple buffers when exceeding buffer_size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,  // small buffer to force segmentation
    });
    
    const std::string message("this_is_a_long_message_that_spans_many_buffers");
    
    // write all data then complete
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    CHECK(writer.write(message) == message.size());
    writer.complete();
    
    // Single read returns all segments even though message is split into small buffers
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.segment_count() == 12);
    CHECK(buffer.to_string() == message);
    CHECK(buffer.size() == message.length());
    CHECK(result.completed());
    reader.advance(buffer.end(), buffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipe_options: rejects invalid thresholds", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    CHECK_THROWS_AS(
        xtd::pipeline(xtd::pipe_options{
            .buffer_size = 4,
            .resume_writer_threshold = 8,
            .pause_writer_threshold = 4,
        }),
        std::invalid_argument
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipe_options: rejects zero buffer size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    CHECK_THROWS_AS(
        xtd::pipeline(xtd::pipe_options{
            .buffer_size = 0,
        }),
        std::invalid_argument
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipe_options: supports pause_writer_threshold not multiple of buffer_size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();

    const std::string message = "0123456789";

    std::thread writer_thread([&pipeline, &message]() {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        CHECK(writer.write(message) == message.size());
        writer.complete();
    });

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    std::string received;

    while (const xtd::read_result result = reader.read()) {
        const xtd::segmented_byte_view buffer = result.buffer();
        received += buffer.to_string();
        reader.advance(buffer.end());

        if (result.completed()) {
            break;
        }
    }

    writer_thread.join();
    CHECK(received == message);
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: rejects null data when length is non-zero", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);

    CHECK(writer.write(nullptr, 10) == 0);
    {
        const auto t1 = std::make_unique<std::byte[]>(0);
        CHECK(writer.write(t1.get(), 0) == 0);
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: write after complete throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);

    writer.complete();

    CHECK_THROWS_AS(
        writer.write("x"),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: cancel read via std::stop_token", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    std::stop_source stopSource;
    stopSource.request_stop();
    CHECK_FALSE(reader.read(stopSource.get_token()));
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: cancel write via std::stop_token", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);

    std::stop_source stopSource;
    stopSource.request_stop();
    CHECK(writer.write("x", stopSource.get_token()) == 0);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: write after reader complete throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    reader.complete();

    CHECK_THROWS_AS(
        writer.write("x"),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: templated write with std::array<T, N>", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const std::array<uint32_t, 3> values = { 0xDEADBEEF, 0xCAFEBABE, 0x12345678 };

    xtd::pipeline pipeline = make_pipeline<TestType>();
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        CHECK(writer.write(values) == sizeof(values));
        writer.complete();
    }

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.size() == sizeof(values));

    std::array<uint32_t, 3> readBack{};
    CHECK(buffer.copy_to(readBack));
    CHECK(readBack == values);

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: advance without pending read throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    xtd::position position{};

    CHECK_THROWS_AS(
        reader.advance(position, position),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: read twice without advance throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("abc");

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.to_string() == "abc");

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::logic_error
    );

    reader.advance(buffer.end(), buffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: read_at_least returns buffered data", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    CHECK(writer.write("abcd") == 4);
    writer.complete();

    const xtd::read_result result = reader.read_at_least(3);
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(buffer.to_string() == "abcd");
    CHECK(buffer.size() == 4);
    CHECK(result.completed());

    reader.advance(buffer.end(), buffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: advance rejects consumed greater than examined", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    CHECK(writer.write("abc") == 3);
    writer.complete();

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    const xtd::position consumed = buffer.begin() + 2;
    const xtd::position examined = buffer.begin() + 1;

    CHECK_THROWS_AS(
        reader.advance(consumed, examined),
        std::invalid_argument
    );

    reader.advance(buffer.end(), buffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: advance rejects examined offset beyond most recent read size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    CHECK(writer.write("abc") == 3);
    writer.complete();

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK_THROWS_AS(
        reader.advance(buffer.begin(), buffer.end() + 1),
        std::out_of_range
    );

    reader.advance(buffer.begin(), buffer.end());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: stale positions are rejected after a segment is returned to the pool and reused", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("abcd");

    {
        const xtd::read_result first = reader.read();
        CHECK(first.buffer().to_string() == "abcd");

        // Consuming the full read returns its only segment to the pool.
        reader.advance(first.buffer().end(), first.buffer().end());
    }

    writer.write("wxyz");
    writer.complete();

    {
        const xtd::read_result second = reader.read();
        const xtd::segmented_byte_view buffer = second.buffer();
        CHECK(buffer.to_string() == "wxyz");

        reader.advance(buffer.end(), buffer.end());
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: Unconsumed data / examined behavior", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("hello\nwo");

    {
        const xtd::read_result result = reader.read();
        xtd::segmented_byte_view seq = result.buffer();

        xtd::position pos = seq.position_of('\n');
        CHECK(pos);

        const xtd::position consumed = pos + 1;
        reader.advance(consumed, seq.end());
    }

    writer.write("rld\n");
    writer.complete();

    {
        const xtd::read_result result = reader.read();
        xtd::segmented_byte_view seq = result.buffer();

        xtd::position pos = seq.position_of('\n');
        CHECK(pos);

        reader.advance(pos + 1, seq.end());
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: not examining everything allows immediate reread of same data", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write("abc");

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        CHECK(buffer.to_string() == "abc");
        reader.advance(buffer.begin(), buffer.begin());
    }

    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        CHECK(buffer.to_string() == "abc");
        reader.advance(buffer.end(), buffer.end());
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: examined-all without consuming waits for data change before next read", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("abcd");

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view firstBuffer = first.buffer();
    CHECK(firstBuffer.to_string() == "abcd");

    // Consume nothing and examine all so the next read waits for a size change.
    reader.advance(firstBuffer.begin(), firstBuffer.end());

    auto nextRead = std::async(std::launch::async, [&]()
    {
        return reader.read();
    });

    CHECK(nextRead.wait_for(50ms) == std::future_status::timeout);

    writer.write("e");

    REQUIRE(nextRead.wait_for(1s) == std::future_status::ready);
    const xtd::read_result second = nextRead.get();
    const xtd::segmented_byte_view secondBuffer = second.buffer();
    CHECK(secondBuffer.to_string() == "abcde");

    writer.complete();
    reader.advance(secondBuffer.end(), secondBuffer.end());

    const xtd::read_result done = reader.read();
    const xtd::segmented_byte_view doneBuffer = done.buffer();
    CHECK(done.completed());
    CHECK(doneBuffer.empty());

    reader.advance(doneBuffer.begin(), doneBuffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: read does not block when data arrives between read and advance", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write("abc");
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view buffer = first.buffer();
    CHECK(buffer.to_string() == "abc");

    writer.write("def");

    reader.advance(buffer.begin(), buffer.end());

    auto nextRead = std::async(std::launch::async, [&]()
    {
        return reader.read();
    });

    REQUIRE(nextRead.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);

    const xtd::read_result second = nextRead.get();
    const xtd::segmented_byte_view buffer2 = second.buffer();
    CHECK(buffer2.to_string() == "abcdef");

    writer.complete();
    reader.advance(buffer2.end(), buffer2.end());

    const xtd::read_result done = reader.read();
    const xtd::segmented_byte_view doneBuffer = done.buffer();
    CHECK(doneBuffer.empty());
    CHECK(done.completed());
    reader.advance(doneBuffer.begin(), doneBuffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: supports binary data containing null bytes", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const std::vector<std::byte> expected{
        std::byte{0x41}, // A
        std::byte{0x00},
        std::byte{0x42}, // B
        std::byte{0xFF},
        std::byte{0x43}, // C
    };
    
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 2,
    });
    
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write(expected.data(), expected.size());
    writer.complete();
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    
    
    std::vector<std::byte> actual(expected.size());
    const std::size_t copied = buffer.copy_to(actual);

    CHECK(copied == expected.size());
    CHECK(actual == expected);
    CHECK(buffer.size() == expected.size());
    CHECK(buffer.segment_count() == 3);
    CHECK(result.completed());

    reader.advance(buffer.end(), buffer.end());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: serializes and deserializes non trivially copyable struct instances", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    struct Message {
        std::uint32_t id;
        std::string some_text;

        std::size_t serialize(xtd::pipe_writer& writer) const {
            std::size_t written = 0;

            // Trivially copyable types can be copied directly to the buffer
            written += writer.write(id);

            // Non-trivially copyable types like std::string need to be serialized in a custom way
            written += writer.write(static_cast<std::uint32_t>(some_text.size())); // write the size of the string first
            written += writer.write(some_text); // write the actual string data
            return written;
        }

        /// [4 bytes for id][4 bytes for some_text size][some_text data]
        /// This function shows how to deserialize a non-trivially copyable struct from a segmented_byte_view.
        /// It returns true if a complete message was successfully deserialized, and false if there wasn't enough data in the buffer.
        /// It also advances the buffer to remove the consumed message if deserialization was successful.
        /// This method explicity do a step-by-step deserialization. But you can improve it by using a more efficient approach
        static bool tryDeserialize(xtd::segmented_byte_view& buffer, Message& deserialized) {
            // Fixed header size: 4 bytes for id + 4 bytes for some_text size
            constexpr std::size_t headerSize = sizeof(std::uint32_t) + sizeof(std::uint32_t);
            
            // Check if the buffer has enough data for the header
            if (buffer.size() < headerSize) {
                return false;
            }

            std::uint32_t id = 0;
            std::uint32_t someTextSize = 0;

            // Trivially copyable types can be copied directly from the buffer
            if (!buffer.slice(0, sizeof(id)).copy_to(id) || !buffer.slice(sizeof(id), sizeof(someTextSize)).copy_to(someTextSize)) {
                throw std::runtime_error("Failed to copy data from buffer"); // this should never happen, because we already checked the size, but just in case
            }
            
            // Calculate the total size needed for the entire message (header + symbol)
            const std::size_t totalSize = headerSize + static_cast<std::size_t>(someTextSize);

            // Check if the buffer has enough data for the entire message
            if (buffer.size() < totalSize) {
                return false;
            }

            // Deserialize the symbol string from the buffer
            deserialized = Message{
                .id = id,
                //even if someTextSize is 0, the slice will be valid and will return an empty string
                .some_text = buffer.slice(headerSize, static_cast<std::size_t>(someTextSize)).to_string(),
            };

            // Advance the buffer to remove the consumed message
            buffer.slice_in_place(totalSize, buffer.end());
            return true;
        }

        bool operator==(const Message& other) const = default;
    };

    static_assert(!std::is_trivially_copyable_v<Message>);

    xtd::pipeline pipeline = make_pipeline<TestType>();
    std::vector<Message> expected;
    expected.reserve(10);
    
    for (std::uint32_t i = 0; i < 10; ++i) {
        expected.emplace_back(1000u + i, i == 0 ? "" : "some_text_with" + std::to_string(i));
    }
    
    std::thread producer([&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        for (const Message& message : expected) {
            CHECK(message.serialize(writer) == sizeof(std::uint32_t) + sizeof(std::uint32_t) + message.some_text.size());
        }
        writer.complete();
    });
    
    std::vector<Message> actual;
    actual.reserve(expected.size());
    std::size_t trailingBytes = 0;
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view buffer = result.buffer();
        Message message{};
        while (Message::tryDeserialize(buffer, message)) {
            actual.push_back(message);
        }

        if (result.completed()) {
            trailingBytes = buffer.size(); // Get the size after advance() is safe.
            //BUT: Reading the buffer after advance() is not safe because the buffer may be invalidated after advance() is called.
            //     So, if you need to get any data from the buffer, do it before calling advance().
        }
        
        reader.advance(buffer.begin(), buffer.end());
        if (result.completed()) break;
    }

    producer.join();

    CHECK(trailingBytes == 0);
    REQUIRE(actual.size() == expected.size());
    CHECK(actual == expected);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: completed read can still contain buffered data", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write("abc");
    writer.complete();

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(result.completed());
    CHECK(buffer.to_string() == "abc");

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: writer complete wakes blocked reader", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();

    auto future = std::async(std::launch::async, [&]()
    {
        return xtd::pipe_reader(pipeline).read();
    });
    
    CHECK(future.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
    
    xtd::pipe_writer(pipeline).complete();

    REQUIRE(future.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    const xtd::read_result result = future.get();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(result.completed());
    CHECK(buffer.empty());

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    reader.advance(buffer.begin(), buffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: read after complete throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    reader.complete();

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: complete is idempotent", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    reader.complete();
    reader.complete();

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: complete after reader complete is valid", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    reader.complete();
    writer.complete();
    writer.complete();

    CHECK_THROWS_AS(writer.write("x"), std::logic_error);
    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::logic_error
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: advance after complete throws", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write("abc");

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    reader.complete();

    CHECK_THROWS_AS(
        reader.advance(buffer.begin(), buffer.end()),
        std::logic_error
    );
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_file_from_path streams file contents and completes", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const std::string path = "/tmp/xtd_test_file.bin";
 
    constexpr std::size_t fileSize = 10 * 1024 * 1024;
    { /* Create a test file */
        constexpr std::size_t chunkSize = 64 * 1024;
        std::ofstream out(path, std::ios::binary);
        REQUIRE(static_cast<bool>(out));
        std::array<char, chunkSize> chunk;
        std::uint32_t seed = 0xC0FFEEu;
        auto next = [&seed]()
        {
            seed = seed * 1664525u + 1013904223u;
            return seed;
        };
        std::size_t written = 0;
        while (written < fileSize)
        {
            const std::size_t bytesToWrite = std::min(chunk.size(), fileSize - written);
            for (std::size_t i = 0; i < bytesToWrite; ++i)
            {
                chunk[i] = static_cast<char>(next() & 0xFF);
            }
            out.write(chunk.data(), static_cast<std::streamsize>(bytesToWrite));
            written += bytesToWrite;
        }
        REQUIRE(static_cast<bool>(out));
        CHECK(written == fileSize);
    }
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    std::thread producer = xtd::pipe_utils::threaded_copy_file_from_path(path, writer); // start background file copying...
    const auto startedAt = std::chrono::steady_clock::now();
    std::size_t actualByteCount = 0;
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
 
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view buffer = result.buffer();
        actualByteCount += buffer.size();
        reader.advance(buffer.end());
        if (result.completed()) break;
    }
    reader.complete();
    producer.join();
    const auto finishedAt = std::chrono::steady_clock::now();
    INFO("xtd::pipeline reader processed "
        << actualByteCount
        << " bytes in "
        << std::chrono::duration_cast<std::chrono::milliseconds>(finishedAt - startedAt).count()
        << " ms");
    CHECK(actualByteCount == fileSize);
    std::remove(path.c_str());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: pipeline can write random text to a slow disk sink without blocking the writer", "[pipeline]",
    FixedAllocatorMode,
    ArenaAllocatorMode,
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    const std::string path = "/tmp/xtd_pipeline_slow_disk_sink.txt";
    std::remove(path.c_str());
    constexpr std::size_t payloadSize = 128 * 1024;
    constexpr std::size_t bufferSize = 4096;
    constexpr auto simulatedDiskDelay = 10ms;

    const auto makeRandomText = []()
    {
        constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789     ";
        std::string text;
        text.resize(payloadSize);

        std::uint32_t seed = 0x51A7C0DEu;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            seed = seed * 1664525u + 1013904223u;

            if ((i + 1) % 79 == 0) {
                text[i] = '\n';
            }
            else {
                text[i] = alphabet[seed % (sizeof(alphabet) - 1)];
            }
        }

        return text;
    };

    const std::string expected = makeRandomText();

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = bufferSize,
        .resume_writer_threshold = payloadSize,
        .pause_writer_threshold = payloadSize + bufferSize,
    });

    auto diskWriter = std::async(std::launch::async, [&]()
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(static_cast<bool>(out));

        std::size_t copied = 0;
        xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
        while (const xtd::read_result result = reader.read())
        {
            const xtd::segmented_byte_view buffer = result.buffer();
            for (const std::span<const std::byte> segment : buffer.segments())
            {
                REQUIRE(segment.size() <= bufferSize);
                out.write(
                    reinterpret_cast<const char*>(segment.data()),
                    static_cast<std::streamsize>(segment.size())
                );
                REQUIRE(static_cast<bool>(out));
                copied += segment.size();
            }

            std::this_thread::sleep_for(simulatedDiskDelay);

            reader.advance(buffer.end());
            if (result.completed()) break;
        }

        reader.complete();
        return copied;
    });

    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        const std::size_t written = writer.write(expected);
        writer.complete();
        return written;
    });

    REQUIRE(producer.wait_for(100ms) == std::future_status::ready);
    CHECK(diskWriter.wait_for(0ms) == std::future_status::timeout);
    CHECK(producer.get() == expected.size());
    CHECK(diskWriter.get() == expected.size());

    std::ifstream in(path, std::ios::binary);
    REQUIRE(static_cast<bool>(in));

    const std::string actual{
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>()};

    CHECK(actual == expected);
    std::remove(path.c_str());
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_file_from_path rejects invalid path", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_file_from_path("./tests/bin/this_file_does_not_exist.txt", writer),
        std::ios_base::failure
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: examined-all without consuming should never unblock the writer to avoid unbounded growth of the buffer", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        writer.write("12345678");
        writer.write("abcd");
        writer.complete();
    });
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result first = reader.read();
    CHECK(first.buffer().to_string() == "12345678");

    // No bytes consumed and all bytes examined must keep the writer blocked.
    const xtd::segmented_byte_view firstBuffer = first.buffer();
    reader.advance(firstBuffer.begin(), firstBuffer.end());

    CHECK(producer.wait_for(50ms) == std::future_status::timeout);

    // Completing the reader should release the blocked writer with an error.
    reader.complete();

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    CHECK_THROWS_AS(producer.get(), std::logic_error);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: reader advance() wakes blocked writer", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });
    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        writer.write("12345678");
        writer.write("abcd");
        writer.complete();
    });
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result first = reader.read();
    CHECK(first.buffer().to_string() == "12345678");

    CHECK(producer.wait_for(50ms) == std::future_status::timeout);

    reader.advance(first.buffer().slice(0, 4).end());

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    producer.get();

    const xtd::read_result second = reader.read();
    CHECK(second.buffer().to_string() == "5678abcd");
    CHECK(second.completed());

    reader.advance(second.buffer().end(), second.buffer().end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: reader complete wakes blocked writer", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });
    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        writer.write("12345678");
        
        CHECK_THROWS_AS(
            writer.write("abcd"),
            std::logic_error
        );
    });
    
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    const xtd::read_result result = reader.read();
    CHECK(result.buffer().to_string() == "12345678");

    reader.complete();

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    producer.get();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: null data with zero length is a no-op", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write(nullptr, 0);
    writer.write("x");
    writer.complete();

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.to_string() == "x");
    CHECK(result.completed());

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: write before advance appends into the same segment when space is available", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    // buffer_size > combined write length → both writes land in segment A
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4096,
    });
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write("test");

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        CHECK(buffer.to_string() == "test");
        CHECK(buffer.segment_count() == 1);

        // second write happens while the consumer still holds the read buffer
        writer.write("more");

        // consume nothing, examine nothing → next read returns all buffered data
        reader.advance(buffer.begin(), buffer.begin());
    }

    writer.complete();

    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        // both writes are visible as a single contiguous segment
        CHECK(buffer.to_string() == "testmore");
        CHECK(buffer.segment_count() == 1);
        reader.advance(buffer.begin(), buffer.end());
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Writer: write before advance allocates a new segment when current segment is full", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    // buffer_size == first write length → segment A is full; second write needs segment B
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
    });
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("test"); // fills segment A completely (4/4 bytes)

    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        CHECK(buffer.to_string() == "test");
        CHECK(buffer.segment_count() == 1);

        writer.write("more"); // segment A is full → new segment B is allocated

        reader.advance(buffer.begin(), buffer.begin());
    }

    writer.complete();

    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        CHECK(buffer.to_string() == "testmore");
        CHECK(buffer.segment_count() == 2); // two distinct segments
        reader.advance(buffer.end(), buffer.end());
    }
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Reader: with buffer_size 4, consuming first byte keeps remaining segments readable with fresh slices", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("abcdefgh");
    writer.complete();

    {
        const xtd::read_result result = reader.read();
        xtd::segmented_byte_view buffer = result.buffer();

        REQUIRE(buffer.to_string() == "abcdefgh");
        CHECK(buffer.segment_count() == 2);
        CHECK(buffer.size() == 8);
        reader.advance(buffer.slice(1, buffer.end()));
    }

    {
        const xtd::read_result result = reader.read();
        const xtd::segmented_byte_view buffer = result.buffer();
        REQUIRE(buffer.to_string() == "bcdefgh");
        CHECK(buffer.segment_count() == 2);
        CHECK(buffer.size() == 7);
        reader.advance(buffer.end());
    }

    {
        const xtd::read_result result = reader.read();
        CHECK(result.buffer().empty());
        CHECK(result.completed());
    }
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_file_from_path rejects zero chunk size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);

    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_file_from_path("./bin/pipe_file_to_writer_test.bin", writer, 0),
        std::invalid_argument
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_from_socket rejects invalid socket descriptor", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_from_socket(-1, writer),
        std::invalid_argument
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_from_socket rejects zero chunk size", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);

    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_from_socket(STDIN_FILENO, writer, 0),
        std::invalid_argument
    );
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_from_socket completes when recv fails with non-EINTR error", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    int pipeFds[2] = {-1, -1};
    REQUIRE(::pipe(pipeFds) == 0);

    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    std::thread copier = xtd::pipe_utils::threaded_copy_from_socket(pipeFds[0], writer, 16);

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(result.completed());
    CHECK(buffer.empty());

    reader.advance(buffer.begin(), buffer.end());
    reader.complete();

    copier.join();
    ::close(pipeFds[0]);
    ::close(pipeFds[1]);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Utility: threaded_copy_from_socket copies split null-delimited records", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const std::byte delimiter = std::byte{0x00};
    const std::vector<std::string> lines{
            "data",
            "to",
            "receive",
            "over",
            "the",
            "socket",
            "splitted",
            "by",
            "nullbyte",
            "!"
        };

    const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(listenFd >= 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_port = 0;

    REQUIRE(::bind(listenFd, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) == 0);
    REQUIRE(::listen(listenFd, 1) == 0);

    sockaddr_in boundAddr{};
    socklen_t boundLen = sizeof(boundAddr);
    REQUIRE(::getsockname(listenFd, reinterpret_cast<sockaddr*>(&boundAddr), &boundLen) == 0);

    const std::uint16_t port = ntohs(boundAddr.sin_port);
    REQUIRE(port != 0);

    auto sendAll = [](int fd, const std::byte* data, std::size_t length)
    {
        std::size_t sent = 0;
        while (sent < length)
        {
            const ssize_t n = ::send(fd, data + sent, length - sent, 0);
            if (n <= 0)
                return false;

            sent += static_cast<std::size_t>(n);
        }

        return true;
    };

    std::thread client([port, sendAll, delimiter, lines]()
    {
        const int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (clientFd < 0)
            return;

        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(port);
        remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::connect(clientFd, reinterpret_cast<const sockaddr*>(&remote), sizeof(remote)) != 0)
        {
            ::close(clientFd);
            return;
        }

        for (const auto& line : lines)
        {
            const auto* lineBytes = reinterpret_cast<const std::byte*>(line.data());
            if (!sendAll(clientFd, lineBytes, line.size()))
                break;

            if (!sendAll(clientFd, &delimiter, 1))
                break;
        }

        ::shutdown(clientFd, SHUT_WR);
        ::close(clientFd);
    });

    const int connectionFd = ::accept(listenFd, nullptr, nullptr);
    REQUIRE(connectionFd >= 0);
    
    std::vector<std::string> received;

    {
        xtd::pipeline pipeline = make_pipeline<TestType>();  
        xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
        std::thread copier = xtd::pipe_utils::threaded_copy_from_socket(connectionFd, writer, 3);
        
        xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
        while (const xtd::read_result result = reader.read())
        {
            xtd::segmented_byte_view seq = result.buffer();

            while (const xtd::position pos = seq.position_of(delimiter))
            {
                received.push_back(seq.slice(pos).to_string());
                seq.slice_in_place(pos + 1, seq.end());
            }

            reader.advance(seq.begin(), seq.end());

            if (result.completed()) break;
        }

        reader.complete();
        copier.join();
        client.join();

        ::close(connectionFd);
        ::close(listenFd);
    }

    REQUIRE(received.size() == lines.size());
    CHECK(received == lines);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Memory: process RSS remains bounded after repeated write/read cycles", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    auto runBatch = [](const std::size_t pipesCount)
    {
        constexpr std::size_t writesPerPipe = 24;
        constexpr std::size_t payloadSize = 4096;
        constexpr std::size_t bytesPerPipe = writesPerPipe * payloadSize;

        const std::string payload(payloadSize, 'x');

        for (std::size_t pipeIndex = 0; pipeIndex < pipesCount; ++pipeIndex)
        {
            xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
                .buffer_size = 4096,
                .resume_writer_threshold = bytesPerPipe,
                .pause_writer_threshold = bytesPerPipe + payloadSize,
            });

            xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
            xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

            for (std::size_t writeIndex = 0; writeIndex < writesPerPipe; ++writeIndex)
            {
                writer.write(payload);
            }
            writer.complete();

            std::size_t bytesRead = 0;
            while (const xtd::read_result result = reader.read())
            {
                const xtd::segmented_byte_view buffer = result.buffer();
                bytesRead += buffer.size();
                reader.advance(buffer.begin(), buffer.end());

                if (result.completed())
                    break;
            }

            if (bytesRead != writesPerPipe * payloadSize)
            {
                FAIL_CHECK("bytesRead mismatch in memory batch");
            }
            reader.complete();
        }
    };

    // Warm up allocators and pools so we compare steady-state memory.
    runBatch(40);
    const std::size_t rssBeforeKb = readCurrentRssKb();

    runBatch(240);
    const std::size_t rssAfterKb = readCurrentRssKb();

    const std::size_t allowedGrowthKb = std::max<std::size_t>(8192, rssBeforeKb / 5);

    INFO("rssBeforeKb=" << rssBeforeKb << ", rssAfterKb=" << rssAfterKb << ", allowedGrowthKb=" << allowedGrowthKb);
    CHECK(rssAfterKb <= rssBeforeKb + allowedGrowthKb);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: Feeds it self for 8GB of data", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    const auto startedAt = std::chrono::steady_clock::now();

    const std::size_t totalBytes = 8ULL * 1024 * 1024 * 1024;
    xtd::pipeline pipeline = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    writer.write(std::string(1024 * 16, 'A'));

    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    std::size_t bytesRead = 0;
    while (const xtd::read_result result = reader.read()) {
        const xtd::segmented_byte_view buffer = result.buffer();
        bytesRead += buffer.size();
        if (bytesRead < totalBytes) {
            writer.write(buffer.to_string());
        } 
        else {
            writer.complete();
        }
        reader.advance(buffer.end());
        if (result.completed()) break;
    }
    reader.complete();

    const auto endedAt = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endedAt - startedAt).count();
    printf("Duration 8GB transfer: %lld milliseconds\n", static_cast<long long>(durationMs));

    CHECK(bytesRead == totalBytes);
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: same-thread write-before-advance can block until advance with small thresholds", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    writer.write("1234");

    const xtd::read_result first = reader.read();
    xtd::segmented_byte_view buffer = first.buffer();
    CHECK(buffer.to_string() == "1234");

    auto blockedWrite = std::async(std::launch::async, [&]()
    {
        writer.write("ABCDEFGH");
    });

    // The pending write reaches pause threshold and remains blocked until the
    // reader advances enough data to satisfy resume policy.
    CHECK(blockedWrite.wait_for(50ms) == std::future_status::timeout);

    reader.advance(buffer.end(), buffer.end());

    REQUIRE(blockedWrite.wait_for(1s) == std::future_status::ready);
    blockedWrite.get();

    writer.complete();

    const xtd::read_result second = reader.read();
    buffer = second.buffer();
    CHECK(buffer.to_string() == "ABCDEFGH");
    CHECK(second.completed());

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: writer pauses exactly at pause threshold and resumes after advance", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);

    CHECK(writer.write("12345678") == 8);

    auto blockedWrite = std::async(std::launch::async, [&]()
    {
        return writer.write("Z");
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view firstBuffer = first.buffer();
    CHECK(firstBuffer.to_string() == "12345678");

    CHECK(blockedWrite.wait_for(50ms) == std::future_status::timeout);

    reader.advance(firstBuffer.slice(0, 4).end(), firstBuffer.end());

    REQUIRE(blockedWrite.wait_for(1s) == std::future_status::ready);
    CHECK(blockedWrite.get() == 1);

    writer.complete();

    const xtd::read_result second = reader.read();
    const xtd::segmented_byte_view secondBuffer = second.buffer();
    CHECK(secondBuffer.size() == 5);
    CHECK(second.completed());

    reader.advance(secondBuffer.end(), secondBuffer.end());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "deserialize windows strings with CRLF line endings", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipe = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    CHECK(writer.write("hello\r\nworld\r\n") == 14);
    writer.complete();

    std::vector<std::string> lines;

    while (const xtd::read_result rr = reader.read())
    {
        xtd::segmented_byte_view seq = rr.buffer();

        while (const xtd::position pos = seq.position_of('\n'))
        {
            xtd::segmented_byte_view line_bytes = seq.slice(pos);
            if (line_bytes[xtd::from_end(1)] == std::byte{'\r'}) {
                line_bytes = line_bytes.slice(0, line_bytes.size() - 1);
            }
            lines.push_back(line_bytes.to_string());
            seq.slice_in_place(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());
        if (rr.completed()) break;
    }

    CHECK(lines.size() == 2);
    CHECK(lines[0] == "hello");
    CHECK(lines[1] == "world");
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline docs example B: delimiter parser across segmented buffers", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{.buffer_size = 3});
    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    CHECK(writer.write("ab\ncd\nef\n") == 9);
    writer.complete();

    std::vector<std::string> lines;

    while (const xtd::read_result rr = reader.read())
    {
        xtd::segmented_byte_view seq = rr.buffer();

        while (const xtd::position pos = seq.position_of('\n'))
        {
            lines.push_back(seq.slice(pos).to_string());
            seq = seq.slice(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());
        if (rr.completed()) break;
    }

    CHECK(lines.size() == 3);
    CHECK(lines[0] == "ab");
    CHECK(lines[1] == "cd");
    CHECK(lines[2] == "ef");
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline docs example C: backpressure with producer thread", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    auto producer = std::async(std::launch::async, [&]() {
        CHECK(writer.write("12345678") == 8);
        CHECK(writer.write("abcd") == 4);
        writer.complete();
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view firstBuffer = first.buffer();
    CHECK(firstBuffer.to_string() == "12345678");

    // Consume first 4 bytes and mark the same range examined.
    reader.advance(firstBuffer.slice(0, 4).end());

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    producer.get();

    const xtd::read_result second = reader.read();
    const xtd::segmented_byte_view secondBuffer = second.buffer();
    CHECK(secondBuffer.size() == 8);
    CHECK(second.completed());

    reader.advance(secondBuffer.end(), secondBuffer.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline docs convenience overload: advance(sequence) maps to consumed=begin examined=end", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipe = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    CHECK(writer.write("abcdef") == 6);

    {
        const xtd::read_result rr = reader.read();
        const xtd::segmented_byte_view seq = rr.buffer();

        const xtd::segmented_byte_view tail = seq.slice(2, seq.end());
        reader.advance(tail); // equivalent to advance(tail.begin(), tail.end())
    }

    writer.complete();

    const xtd::read_result rr2 = reader.read();
    const xtd::segmented_byte_view seq2 = rr2.buffer();
    CHECK(seq2.to_string() == "cdef");
    CHECK(rr2.completed());

    reader.advance(seq2.end(), seq2.end());
    reader.complete();
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline docs reader complete invalidates pending read", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    xtd::pipeline pipe = make_pipeline<TestType>();
    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    CHECK(writer.write("abc") == 3);

    const xtd::read_result rr = reader.read();
    const xtd::segmented_byte_view seq = rr.buffer();
    CHECK(seq.to_string() == "abc");

    // Completing the reader clears pending-read state and invalidates this read context.
    reader.complete();

    CHECK_THROWS_AS(reader.advance(seq.begin(), seq.end()), std::logic_error);
    CHECK_THROWS_AS(static_cast<void>(reader.read()), std::logic_error);
    CHECK_THROWS_AS(writer.write("z"), std::logic_error);
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: equal pause and resume thresholds resume after a full segment is released", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 128,
        .pause_writer_threshold = 128,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    const std::string payload(129, 'x');

    auto write_future = std::async(std::launch::async, [&]() {
        const std::size_t written = writer.write(payload);
        writer.complete();
        return written;
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view first_buffer = first.buffer();

    REQUIRE(first_buffer.size() == 128);
    CHECK(write_future.wait_for(50ms) == std::future_status::timeout);

    // Release one full segment of actual capacity.
    reader.advance(first_buffer.begin() + 64, first_buffer.end());

    REQUIRE(write_future.wait_for(1s) == std::future_status::ready);
    CHECK(write_future.get() == payload.size());

    const xtd::read_result second = reader.read();
    const xtd::segmented_byte_view second_buffer = second.buffer();

    CHECK(second_buffer.size() == 65);
    CHECK(second.completed());

    reader.advance(second_buffer.end(), second_buffer.end());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: paused writer resumes only after full segment is returned to pool", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 128,
        .pause_writer_threshold = 128,
        .allocator = xtd::allocator_kind::fixed_pool_resource,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    const std::string payload(129, 'x');

    auto write_future = std::async(std::launch::async, [&]() {
        const std::size_t written = writer.write(payload);
        writer.complete();
        return written;
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view first_buffer = first.buffer();
    REQUIRE(first_buffer.size() == 128);

    // Consume less than one full segment; writer must remain paused.
    reader.advance(first_buffer.begin() + 63, first_buffer.begin() + 63);
    CHECK(write_future.wait_for(50ms) == std::future_status::timeout);

    const xtd::read_result second = reader.read();
    const xtd::segmented_byte_view second_buffer = second.buffer();
    REQUIRE(second_buffer.size() == 65);

    // Consume one more byte so the first segment can be released.
    reader.advance(second_buffer.begin() + 1, second_buffer.begin() + 1);

    REQUIRE(write_future.wait_for(1s) == std::future_status::ready);
    CHECK(write_future.get() == payload.size());

    const xtd::read_result third = reader.read();
    const xtd::segmented_byte_view third_buffer = third.buffer();
    CHECK(third_buffer.size() == 65);
    CHECK(third.completed());

    reader.advance(third_buffer.end(), third_buffer.end());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: non-multiple pause threshold resumes only after full segment is returned", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 10,
        .pause_writer_threshold = 10,
        .allocator = xtd::allocator_kind::fixed_pool_resource,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    const std::string payload = "123456789";

    auto write_future = std::async(std::launch::async, [&]() {
        const std::size_t written = writer.write(payload);
        writer.complete();
        return written;
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view first_buffer = first.buffer();
    REQUIRE(first_buffer.size() == 8);
    CHECK(write_future.wait_for(50ms) == std::future_status::timeout);

    // Consume less than one full segment; writer must remain blocked.
    reader.advance(first_buffer.begin() + 3, first_buffer.begin() + 3);
    CHECK(write_future.wait_for(50ms) == std::future_status::timeout);

    const xtd::read_result second = reader.read();
    const xtd::segmented_byte_view second_buffer = second.buffer();
    REQUIRE(second_buffer.size() == 5);

    // Consume one more byte to release the first segment.
    reader.advance(second_buffer.begin() + 1, second_buffer.begin() + 1);

    REQUIRE(write_future.wait_for(1s) == std::future_status::ready);
    CHECK(write_future.get() == payload.size());

    const xtd::read_result third = reader.read();
    const xtd::segmented_byte_view third_buffer = third.buffer();
    CHECK(third_buffer.size() == 5);
    CHECK(third.completed());

    reader.advance(third_buffer.end(), third_buffer.end());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: full writer does not busy-spin with equal thresholds", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 128,
        .pause_writer_threshold = 128,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    const std::string payload(129, 'x');

    std::jthread writer_thread(
        [&](const std::stop_token stop_token)
        {
            try {
                static_cast<void>(writer.write(payload, stop_token));
            }
            catch (const std::logic_error&) {
                // reader.complete() is used to release the writer below.
            }
        }
    );

    clockid_t writer_clock{};
    const int clock_id_result =
        pthread_getcpuclockid(writer_thread.native_handle(), &writer_clock);

    if (clock_id_result != 0) {
        writer_thread.request_stop();
        reader.complete();
        writer_thread.join();
    }

    REQUIRE(clock_id_result == 0);

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(buffer.size() == 128);
    CHECK_FALSE(result.completed());

    timespec cpu_before{};
    timespec cpu_after{};

    const int before_result = clock_gettime(writer_clock, &cpu_before);

    if (before_result == 0) {
        std::this_thread::sleep_for(500ms);
    }

    const int after_result =
        before_result == 0
            ? clock_gettime(writer_clock, &cpu_after)
            : -1;

    writer_thread.request_stop();
    reader.complete();
    writer_thread.join();

    REQUIRE(before_result == 0);
    REQUIRE(after_result == 0);

    const auto to_nanoseconds = [](const timespec& value)
    {
        return std::chrono::seconds(value.tv_sec)
            + std::chrono::nanoseconds(value.tv_nsec);
    };

    const auto writer_cpu_time =
        to_nanoseconds(cpu_after) - to_nanoseconds(cpu_before);

    INFO("Writer CPU time: "
         << std::chrono::duration_cast<std::chrono::milliseconds>(
                writer_cpu_time
            ).count()
         << " ms");

    CHECK(writer_cpu_time < 100ms);
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: large write blocks mid-call when pause threshold is reached", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 64,
        .pause_writer_threshold = 128,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);
    const std::string payload(1024, 'x');

    auto writeFuture = std::async(std::launch::async, [&]() {
        const std::size_t written = writer.write(payload);
        writer.complete();
        return written;
    });

    const xtd::read_result first = reader.read();
    const xtd::segmented_byte_view firstBuffer = first.buffer();

    CHECK(firstBuffer.size() == 128);
    CHECK_FALSE(first.completed());
    CHECK(writeFuture.wait_for(50ms) == std::future_status::timeout);

    reader.advance(firstBuffer.end(), firstBuffer.end());

    std::size_t totalRead = firstBuffer.size();
    while (const xtd::read_result rr = reader.read())
    {
        const xtd::segmented_byte_view seq = rr.buffer();
        totalRead += seq.size();
        reader.advance(seq.end(), seq.end());

        if (rr.completed()) break;
    }

    REQUIRE(writeFuture.wait_for(1s) == std::future_status::ready);
    CHECK(writeFuture.get() == payload.size());
    CHECK(totalRead == payload.size());
    reader.complete();
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "pipeline: equal pause and resume thresholds exercise writer repause path", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe = make_pipeline<TestType>(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 128,
        .pause_writer_threshold = 128,
    });

    xtd::pipe_writer writer = xtd::pipe_writer(pipe);
    xtd::pipe_reader reader = xtd::pipe_reader(pipe);

    // One byte more than the threshold leaves data remaining after the
    // pipeline becomes full.
    const std::string payload(129, 'x');

    auto write_future = std::async(std::launch::async, [&]() {
        return writer.write(payload);
    });

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    REQUIRE(buffer.size() == 128);
    CHECK_FALSE(result.completed());

    // The writer cannot finish because no capacity was released.
    CHECK(write_future.wait_for(50ms) == std::future_status::timeout);

    // Stop the spinning writer without consuming the pending read.
    reader.complete();

    REQUIRE(write_future.wait_for(1s) == std::future_status::ready);
    CHECK_THROWS_AS(write_future.get(), std::logic_error);
}

struct test_data_trivially_copyable
{
    enum class status : std::uint8_t
    {
        sure,
        that,
        it,
        can, 
        have, 
        multiple,
        and_any,
        values
    };

    struct nested_data
    {
        std::int32_t value = 0;
        double ratio = 0.0;

        void member_function() {}
    };

    union trivial_union
    {
        std::uint64_t integer;
        double floating;

        constexpr trivial_union() : integer(0) {}
    };

    // Boolean and character types
    bool booleanValue = false;
    char charValue = '\0';
    signed char signedCharValue = 0;
    unsigned char unsignedCharValue = 0;
    wchar_t wideCharValue = L'\0';
    char8_t utf8CharValue = u8'\0';
    char16_t utf16CharValue = u'\0';
    char32_t utf32CharValue = U'\0';
    std::byte byteValue{};

    // Integer types
    short shortValue = 0;
    unsigned short unsignedShortValue = 0;

    int intValue = 0;
    unsigned int unsignedIntValue = 0;

    long longValue = 0;
    unsigned long unsignedLongValue = 0;

    long long longLongValue = 0;
    unsigned long long unsignedLongLongValue = 0;

    // Fixed-width integers
    std::int8_t int8Value = 0;
    std::uint8_t uint8Value = 0;
    std::int16_t int16Value = 0;
    std::uint16_t uint16Value = 0;
    std::int32_t int32Value = 0;
    std::uint32_t uint32Value = 0;
    std::int64_t int64Value = 0;
    std::uint64_t uint64Value = 0;

    // Floating-point types
    float floatValue = 0.0F;
    double doubleValue = 0.0;
    long double longDoubleValue = 0.0L;

    // Enumeration
    status statusValue = status::and_any;

    // Nested trivially copyable object
    nested_data nested{};

    // Arrays
    int rawArray[8]{};
    std::array<std::uint64_t, 8> standardArray{};

    // Union
    trivial_union unionValue{};

    // Arbitrary raw payload
    std::array<std::byte, 512> rawData{};

    [[nodiscard]]
    bool operator==(const test_data_trivially_copyable& other) const noexcept
    {
        return std::memcmp(this, &other, sizeof(*this)) == 0;
    }

    [[nodiscard]]
    bool operator!=(const test_data_trivially_copyable& other) const noexcept
    {
        return !(*this == other);
    }

    void print() const;
    template <typename Mode>
    static void test(std::istream& source);
};

static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable>);
static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::nested_data>);
static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::trivial_union>);

template <typename Mode>
inline void test_data_trivially_copyable::test(std::istream& source)
{
    constexpr std::size_t expected_count = 1'000;
    std::vector<test_data_trivially_copyable> expected_values(expected_count);

    xtd::pipeline pipeline = make_pipeline<Mode>();
    std::future<std::size_t> producer = std::async(std::launch::async,
        [&pipeline, &source, &expected_values] {
            std::size_t written_count = 0;
            test_data_trivially_copyable mydata;
            xtd::pipe_writer writer = xtd::pipe_writer(pipeline);
            while (written_count < expected_count)
            {
                // Get bytes from source into the struct
                source.read(reinterpret_cast<char*>(&mydata), static_cast<std::streamsize>(sizeof(mydata)));

                // If uncomplete read, clear the error state and try again
                if (sizeof(mydata) > static_cast<std::size_t>(source.gcount())) {
                    source.clear();
                    continue;
                }

                // Ensure that the booleanValue is consistent with the unsignedIntValue for testing purposes
                // This is important because the random data from /dev/urandom may not have a valid booleanValue
                // So we set it based on the unsignedIntValue.
                mydata.booleanValue = mydata.unsignedIntValue % 2 == 0;
                
                // Ensure that the statusValue is within the valid range of the enum
                mydata.statusValue = static_cast<test_data_trivially_copyable::status>(mydata.unsignedIntValue % 3);

                // Writes a copy of the struct to the expected_values vector for later verification
                expected_values[written_count] = mydata;
                
                // Serialize the struct to the pipeline writer
                REQUIRE(writer.write(mydata) == sizeof(mydata));
                ++written_count;
            }

            writer.complete();
            return written_count;
        });

        
    std::size_t received_count = 0;
    test_data_trivially_copyable mydata;
    xtd::pipe_reader reader = xtd::pipe_reader(pipeline);
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view buffer = result.buffer();

        // While there is enough data in the buffer to deserialize a complete struct
        // Copy it to mydata and verify it against the expected values
        while (buffer.size() >= sizeof(mydata))
        {
            REQUIRE(received_count < expected_values.size());
            REQUIRE(buffer.copy_to(mydata));
            CHECK(mydata == expected_values[received_count]);
            ++received_count;

            buffer.slice_in_place(sizeof(mydata), buffer.end());
        }

        reader.advance(buffer);

        if (result.completed()) {
            break;
        }
    }

    const std::size_t written_count = producer.get();
    reader.complete();

    CHECK(written_count == expected_count);
    CHECK(received_count == expected_count);
}

inline void test_data_trivially_copyable::print() const
{
    std::println(
        "test_data_trivially_copyable "
        "[sizeof = {} bytes] "
        "[booleanValue = {}] "
        "[charValue = {}] "
        "[signedCharValue = {}] "
        "[unsignedCharValue = {}] "
        "[wideCharValue = {}] "
        "[utf8CharValue = {}] "
        "[utf16CharValue = {}] "
        "[utf32CharValue = {}] "
        "[byteValue = {:#04x}] "
        "[shortValue = {}] "
        "[unsignedShortValue = {}] "
        "[intValue = {}] "
        "[unsignedIntValue = {}] "
        "[longValue = {}] "
        "[unsignedLongValue = {}] "
        "[longLongValue = {}] "
        "[unsignedLongLongValue = {}] "
        "[int8Value = {}] "
        "[uint8Value = {}] "
        "[int16Value = {}] "
        "[uint16Value = {}] "
        "[int32Value = {}] "
        "[uint32Value = {}] "
        "[int64Value = {}] "
        "[uint64Value = {}] "
        "[floatValue = {}] "
        "[doubleValue = {}] "
        "[longDoubleValue = {}] "
        "[statusValue = {}] "
        "[nested.value = {}] "
        "[nested.ratio = {}] "
        "[unionValue.integer = {}] "
        "[rawArray = {} elements, {} bytes] "
        "[standardArray = {} elements, {} bytes] "
        "[rawData = {} bytes]",
        sizeof(*this),

        booleanValue,
        static_cast<int>(charValue),
        static_cast<int>(signedCharValue),
        static_cast<unsigned int>(unsignedCharValue),
        static_cast<std::uint32_t>(wideCharValue),
        static_cast<std::uint32_t>(utf8CharValue),
        static_cast<std::uint32_t>(utf16CharValue),
        static_cast<std::uint32_t>(utf32CharValue),
        std::to_integer<unsigned int>(byteValue),

        shortValue,
        unsignedShortValue,
        intValue,
        unsignedIntValue,
        longValue,
        unsignedLongValue,
        longLongValue,
        unsignedLongLongValue,

        static_cast<int>(int8Value),
        static_cast<unsigned int>(uint8Value),
        int16Value,
        uint16Value,
        int32Value,
        uint32Value,
        int64Value,
        uint64Value,

        floatValue,
        doubleValue,
        longDoubleValue,

        static_cast<std::uint32_t>(statusValue),
        nested.value,
        nested.ratio,
        unionValue.integer,

        std::size(rawArray),
        sizeof(rawArray),
        standardArray.size(),
        sizeof(standardArray),
        rawData.size()
    );
}

TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Use /dev/urandom as stream source to test trivially copyable struct serialization and deserialization", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    std::ifstream random("/dev/urandom", std::ios::binary);
    REQUIRE(random.is_open());
    test_data_trivially_copyable::test<TestType>(random);   
}


TEMPLATE_TEST_CASE_METHOD(PipelineTests, "Use /dev/zero as stream source to test trivially copyable struct serialization and deserialization", "[pipeline]", 
    FixedAllocatorMode, 
    ArenaAllocatorMode, 
    UnsynchronizedAllocatorMode)
{
    std::ifstream zero("/dev/zero", std::ios::binary);
    REQUIRE(zero.is_open());
    test_data_trivially_copyable::test<TestType>(zero);
}

} // namespace xtd_pipeline_tests

#endif
