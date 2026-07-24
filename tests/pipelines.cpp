#include "pipeline/pipeline_impl.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest.h"

#include <cstddef>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <thread>
#include <array>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "pipeline/segmented_byte_view.h"
#include "pipeline/data_segment.h"
#include "pipeline/fixed_buffer_pool.h"
#include "pipeline/position.h"
#include "pipeline/pipeline.h"
#include "pipeline/pipe_utils.h"
#include "tests/test_data_trivially_copyable.h"


namespace xtd {
class test_helper_segmented_byte_view {
public:
    static segmented_byte_view create_from_segments(std::vector<std::span<const std::byte>>&& segments, std::uint64_t sequenceId) {
        return segmented_byte_view(std::move(segments), sequenceId);
    }

    static std::size_t get_first_segment_begin(const segmented_byte_view& seq) {
        return seq.m_begin_offset;
    }

    static std::uint64_t get_sequence_id(const segmented_byte_view& seq) {
        return seq.m_sequence_id;
    }
};
}

std::size_t readCurrentRssKb()
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

TEST_CASE("pipeline: multiple c_str writes are parsed into complete lines")
{
    const std::byte delimiter = std::byte{'\0'};
    const std::array<std::string, 3> expected = {
        "one", 
        "two", 
        "three"
    };

    xtd::pipeline pipeline;
    
    std::thread t([&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        for (const auto& msg : expected) {
            writer.write(reinterpret_cast<const std::byte*>(msg.data()), msg.size() + 1);
        }
        writer.complete();
    });
    
    int index = 0;
    xtd::pipe_reader& reader = pipeline.reader();
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view seq = result.buffer();
        
        while (xtd::position pos = seq.position_of(delimiter))
        {
            CHECK(expected[index++] == seq.slice(seq.begin(), pos).to_string());
            seq = seq.slice(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());

        if (result.completed()) break;
    }

    t.join();
    CHECK(index == expected.size());
}

TEST_CASE("pipeline: delayed character writes are parsed into complete lines")
{
    xtd::pipeline pipeline;
    
    std::thread producer([&]()
    {
        using namespace std::chrono_literals;
        xtd::pipe_writer& writer = pipeline.writer();
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
    std::vector<std::string> received;
    xtd::pipe_reader& reader = pipeline.reader();
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view ros = result.buffer();
        readCount++;
        
        while (xtd::position pos = ros.position_of('\n'))
        {
            received.push_back(ros.slice(pos).to_string());
            ros = ros.slice(pos + 1, ros.end());
        }

        reader.advance(ros.begin(), ros.end());

        if (result.completed()) break;
    }

    producer.join();
    CHECK(received.size() == 1);
    for (const std::string& line : received)
    {
        CHECK(line == "hello");
    }
    CHECK(readCount == 7);
}

TEST_CASE("pipeline: isCompleted set only after all data is consumed")
{
    xtd::pipeline pipeline;
    std::thread t([&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        CHECK(writer.write("done\n") == 5);
        writer.complete();
    });
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: message spans multiple buffers when exceeding buffer_size")
{
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,  // small buffer to force segmentation
    });
    
    const std::string message("this_is_a_long_message_that_spans_many_buffers");
    
    // write all data then complete
    xtd::pipe_writer& writer = pipeline.writer();
    CHECK(writer.write(message) == message.size());
    writer.complete();
    
    // Single read returns all segments even though message is split into small buffers
    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.segment_count() == 12);
    CHECK(buffer.to_string() == message);
    CHECK(buffer.size() == message.length());
    CHECK(result.completed());
    reader.advance(buffer.end(), buffer.end());
}

TEST_CASE("pipe_options: rejects invalid thresholds")
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

TEST_CASE("pipe_options: rejects zero buffer size")
{
    CHECK_THROWS_AS(
        xtd::pipeline(xtd::pipe_options{
            .buffer_size = 0,
        }),
        std::invalid_argument
    );
}

TEST_CASE("Writer: rejects null data when length is non-zero")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();

    CHECK(writer.write(nullptr, 10) == 0);
    {
        const auto t1 = std::make_unique<std::byte[]>(0);
        CHECK(writer.write(t1.get(), 0) == 0);
    }
}

TEST_CASE("Writer: write after complete throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();

    writer.complete();

    CHECK_THROWS_AS(
        writer.write("x"),
        std::runtime_error
    );
}

TEST_CASE("Reader: cancel read via std::stop_token")
{
    xtd::pipeline pipeline;
    xtd::pipe_reader& reader = pipeline.reader();

    std::stop_source stopSource;
    stopSource.request_stop();
    CHECK_FALSE(reader.read(stopSource.get_token()));
}

TEST_CASE("Writer: cancel write via std::stop_token")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();

    std::stop_source stopSource;
    stopSource.request_stop();
    CHECK(writer.write("x", stopSource.get_token()) == 0);
}

TEST_CASE("Writer: write after reader complete throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    reader.complete();

    CHECK_THROWS_AS(
        writer.write("x"),
        std::runtime_error
    );
}

TEST_CASE("Writer: templated write with std::array<T, N>")
{
    const std::array<uint32_t, 3> values = { 0xDEADBEEF, 0xCAFEBABE, 0x12345678 };

    xtd::pipeline pipeline;
    {
        xtd::pipe_writer& writer = pipeline.writer();
        CHECK(writer.write(values) == sizeof(values));
        writer.complete();
    }

    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.size() == sizeof(values));

    std::array<uint32_t, 3> readBack{};
    CHECK(buffer.copy_to(readBack));
    CHECK(readBack == values);

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}

TEST_CASE("Reader: advance without pending read throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_reader& reader = pipeline.reader();
    xtd::position position{};

    CHECK_THROWS_AS(
        reader.advance(position, position),
        std::runtime_error
    );
}

TEST_CASE("Reader: read twice without advance throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    writer.write("abc");

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(buffer.to_string() == "abc");

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::runtime_error
    );

    reader.advance(buffer.end(), buffer.end());
}

TEST_CASE("Reader: read_at_least returns buffered data")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    CHECK(writer.write("abcd") == 4);
    writer.complete();

    const xtd::read_result result = reader.read_at_least(3);
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(buffer.to_string() == "abcd");
    CHECK(buffer.size() == 4);
    CHECK(result.completed());

    reader.advance(buffer.end(), buffer.end());
}

TEST_CASE("Reader: advance rejects consumed greater than examined")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    CHECK(writer.write("abc") == 3);
    writer.complete();

    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();
    const xtd::segmented_byte_view consumedSlice = buffer.slice(2, 0);
    const xtd::segmented_byte_view examinedSlice = buffer.slice(1, 0);

    CHECK_THROWS_AS(
        reader.advance(consumedSlice.begin(), examinedSlice.begin()),
        std::invalid_argument
    );

    reader.advance(buffer.end(), buffer.end());
}

TEST_CASE("Reader: advance rejects positions from another read buffer")
{
    xtd::pipeline pipe1;
    {
        xtd::pipe_writer& writer1 = pipe1.writer();
        CHECK(writer1.write("abc") == 3);
        writer1.complete();
    }

    xtd::pipeline pipe2;
    {
        xtd::pipe_writer& writer2 = pipe2.writer();
        CHECK(writer2.write("xyz") == 3);
        writer2.complete();
    }
    
    xtd::pipe_reader& reader1 = pipe1.reader();
    xtd::pipe_reader& reader2 = pipe2.reader();
    const xtd::read_result result = reader1.read();
    const xtd::read_result otherResult = reader2.read();

    CHECK_THROWS_AS(
        reader1.advance(result.buffer().begin(), otherResult.buffer().begin()),
        std::invalid_argument
    );

    reader1.advance(result.buffer().begin(), result.buffer().end());
    reader2.advance(otherResult.buffer().begin(), otherResult.buffer().end());
}

TEST_CASE("Reader: advance rejects examined offset beyond most recent read size")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    CHECK(writer.write("abc") == 3);
    writer.complete();

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK_THROWS_AS(
        reader.advance(buffer.begin(), buffer.end() + 1),
        std::invalid_argument
    );

    reader.advance(buffer.begin(), buffer.end());
    reader.complete();
}

TEST_CASE("segmented_byte_view: position arithmetic beyond end is rejected by slice")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    CHECK(writer.write("abc") == 3);
    writer.complete();

    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK_THROWS_AS(
        static_cast<void>(buffer.slice(buffer.end() + 1, buffer.end() + 1)),
        std::out_of_range
    );

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}

TEST_CASE("segmented_byte_view: slice can span multiple segments")
{
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 3,
    });

    {
        xtd::pipe_writer& writer = pipeline.writer();
        CHECK(writer.write("abcdefghi") == 9);
        writer.complete();
    }
    
    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(buffer.segment_count() == 3);
    const xtd::segmented_byte_view middle = buffer.slice(2, 5);
    CHECK(middle.begin() == buffer.slice(0, 2).end());
    CHECK(middle.to_string() == "cdefg");

    reader.advance(buffer.end());
    reader.complete();
}

TEST_CASE("segmented_byte_view: position_of finds delimiter across segmented buffer")
{
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 2,
    });

    {
        xtd::pipe_writer& writer = pipeline.writer();
        CHECK(writer.write("ab\ncd") == 5);
        writer.complete();
    }
    
    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();

    const xtd::segmented_byte_view buffer = result.buffer();

    xtd::position pos{};
    CHECK((pos = buffer.position_of('\n')));

    CHECK(pos == buffer.slice(0, 2).end());
    CHECK(buffer.slice(pos).to_string() == "ab");
    CHECK(buffer.slice(0, pos).to_string() == "ab");
    CHECK(buffer.slice(buffer.begin(), pos).to_string() == "ab");

    reader.advance(buffer.end());
}

TEST_CASE("pipeline: Unconsumed data / examined behavior")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    writer.write("hello\nwo");

    {
        const xtd::read_result result = reader.read();
        xtd::segmented_byte_view seq = result.buffer();

        xtd::position pos{};
        CHECK((pos = seq.position_of('\n')));

        CHECK(seq.slice(seq.begin(), pos).to_string() == "hello");
        CHECK(seq.slice(0, pos).to_string() == "hello");
        CHECK(seq.slice(pos).to_string() == "hello");

        const xtd::position consumed = seq.slice(pos + 1, seq.end()).begin();
        reader.advance(consumed, seq.end());
    }

    writer.write("rld\n");
    writer.complete();

    {
        const xtd::read_result result = reader.read();
        xtd::segmented_byte_view seq = result.buffer();

        xtd::position pos{};
        CHECK((pos = seq.position_of('\n')));
        CHECK(seq.slice(seq.begin(), pos).to_string() == "world");

        reader.advance(seq.begin(), seq.end());
    }
}

TEST_CASE("pipeline: not examining everything allows immediate reread of same data")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write("abc");

    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: examined-all without consuming waits for data change before next read")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("pipeline: read does not block when data arrives between read and advance")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write("abc");
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: supports binary data containing null bytes")
{
    const std::vector<std::byte> expected{
        std::byte{0x41}, // A
        std::byte{0x00},
        std::byte{0x42}, // B
        std::byte{0xFF},
        std::byte{0x43}, // C
    };
    
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 2,
    });
    
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write(expected.data(), expected.size());
    writer.complete();
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: serializes and deserializes non trivially copyable struct instances")
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
            buffer = buffer.slice(totalSize, buffer.end());
            return true;
        }

        bool operator==(const Message& other) const = default;
    };

    static_assert(!std::is_trivially_copyable_v<Message>);

    xtd::pipeline pipeline;
    std::vector<Message> expected;
    expected.reserve(10);
    
    for (std::uint32_t i = 0; i < 10; ++i) {
        expected.emplace_back(1000u + i, i == 0 ? "" : "some_text_with" + std::to_string(i));
    }
    
    std::thread producer([&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        for (const Message& message : expected) {
            CHECK(message.serialize(writer) == sizeof(std::uint32_t) + sizeof(std::uint32_t) + message.some_text.size());
        }
        writer.complete();
    });
    
    std::vector<Message> actual;
    actual.reserve(expected.size());
    std::size_t trailingBytes = 0;
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: completed read can still contain buffered data")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write("abc");
    writer.complete();

    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    CHECK(result.completed());
    CHECK(buffer.to_string() == "abc");

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}

TEST_CASE("pipeline: writer complete wakes blocked reader")
{
    xtd::pipeline pipeline;

    auto future = std::async(std::launch::async, [&]()
    {
        return pipeline.reader().read();
    });
    
    CHECK(future.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout);
    
    pipeline.writer().complete();

    REQUIRE(future.wait_for(std::chrono::seconds(1)) == std::future_status::ready);

    const xtd::read_result result = future.get();
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK(result.completed());
    CHECK(buffer.empty());

    xtd::pipe_reader& reader = pipeline.reader();
    reader.advance(buffer.begin(), buffer.end());
    reader.complete();
}

TEST_CASE("Reader: read after complete throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_reader& reader = pipeline.reader();

    reader.complete();

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::runtime_error
    );
}

TEST_CASE("Reader: complete is idempotent")
{
    xtd::pipeline pipeline;
    xtd::pipe_reader& reader = pipeline.reader();

    reader.complete();
    reader.complete();

    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::runtime_error
    );
}

TEST_CASE("Writer: complete after reader complete is valid")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    reader.complete();
    writer.complete();
    writer.complete();

    CHECK_THROWS_AS(writer.write("x"), std::runtime_error);
    CHECK_THROWS_AS(
        static_cast<void>(reader.read()),
        std::runtime_error
    );
}

TEST_CASE("Reader: advance after complete throws")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write("abc");

    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    const xtd::segmented_byte_view buffer = result.buffer();

    reader.complete();

    CHECK_THROWS_AS(
        reader.advance(buffer.begin(), buffer.end()),
        std::runtime_error
    );
}

TEST_CASE("Use /dev/urandom as stream source to test trivially copyable struct serialization and deserialization") {
    std::ifstream random("/dev/urandom", std::ios::binary);
    REQUIRE(random.is_open());
    test_data_trivially_copyable::test(random);   
}

TEST_CASE("Use /dev/zero as stream source to test trivially copyable struct serialization and deserialization") {
    std::ifstream zero("/dev/zero", std::ios::binary);
    REQUIRE(zero.is_open());
    test_data_trivially_copyable::test(zero);
}

TEST_CASE("Utility: threaded_copy_file_from_path streams file contents and completes")
{
    const std::string path = "./tests/bin/pipe_file_to_writer_test.bin";
    
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

    xtd::pipeline pipeline;
    std::thread producer = xtd::pipe_utils::threaded_copy_file_from_path(path, pipeline.writer()); // start background file copying...

    const auto startedAt = std::chrono::steady_clock::now();
    std::size_t actualByteCount = 0;
    xtd::pipe_reader& reader = pipeline.reader();
    
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

TEST_CASE("Utility: threaded_copy_file_from_path rejects invalid path")
{
    xtd::pipeline pipeline;
    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_file_from_path("./tests/bin/this_file_does_not_exist.txt", pipeline.writer()),
        std::runtime_error
    );
}

TEST_CASE("pipeline: examined-all without consuming should never unblock the writer to avoid unbounded growth of the buffer")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        writer.write("12345678");
        writer.write("abcd");
        writer.complete();
    });
    
    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result first = reader.read();
    CHECK(first.buffer().to_string() == "12345678");

    // No bytes consumed and all bytes examined must keep the writer blocked.
    const xtd::segmented_byte_view firstBuffer = first.buffer();
    reader.advance(firstBuffer.begin(), firstBuffer.end());

    CHECK(producer.wait_for(50ms) == std::future_status::timeout);

    // Completing the reader should release the blocked writer with an error.
    reader.complete();

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    CHECK_THROWS_AS(producer.get(), std::runtime_error);
}

TEST_CASE("pipeline: reader advance() wakes blocked writer")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });
    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        writer.write("12345678");
        writer.write("abcd");
        writer.complete();
    });
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: reader complete wakes blocked writer")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });
    
    auto producer = std::async(std::launch::async, [&]()
    {
        xtd::pipe_writer& writer = pipeline.writer();
        writer.write("12345678");
        
        CHECK_THROWS_AS(
            writer.write("abcd"),
            std::runtime_error
        );
    });
    
    xtd::pipe_reader& reader = pipeline.reader();
    const xtd::read_result result = reader.read();
    CHECK(result.buffer().to_string() == "12345678");

    reader.complete();

    REQUIRE(producer.wait_for(1s) == std::future_status::ready);
    producer.get();
}

TEST_CASE("Writer: null data with zero length is a no-op")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("Writer: write before advance appends into the same segment when space is available")
{
    // buffer_size > combined write length → both writes land in segment A
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4096,
    });
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write("test");

    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("Writer: write before advance allocates a new segment when current segment is full")
{
    // buffer_size == first write length → segment A is full; second write needs segment B
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
    });
    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("Reader: with buffer_size 4, consuming first byte keeps remaining segments readable with fresh slices")
{
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
    });

    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("Reader: stale positions are rejected after a segment is returned to the pool and reused")
{
    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
    });

    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

    writer.write("abcd");

    xtd::position staleMidpoint{};
    {
        const xtd::read_result first = reader.read();
        CHECK(first.buffer().to_string() == "abcd");

        staleMidpoint = first.buffer().slice(0, 2).end();

        // Consuming the full read returns its only segment to the pool.
        reader.advance(first.buffer().end(), first.buffer().end());
    }

    writer.write("wxyz");
    writer.complete();

    {
        const xtd::read_result second = reader.read();
        const xtd::segmented_byte_view buffer = second.buffer();
        CHECK(buffer.to_string() == "wxyz");

        // The second read reuses the same pool path, but stale positions from the
        // first read must still be rejected because they carry the old read token.
        CHECK_THROWS_AS(
            reader.advance(staleMidpoint, buffer.end()),
            std::invalid_argument
        );

        CHECK_THROWS_AS(
            static_cast<void>(buffer.slice(staleMidpoint, buffer.end())),
            std::invalid_argument
        );

        reader.advance(buffer.end(), buffer.end());
    }
}

TEST_CASE("Utility: threaded_copy_file_from_path rejects zero chunk size")
{
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();

    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_file_from_path("./bin/pipe_file_to_writer_test.bin", writer, 0),
        std::invalid_argument
    );
}

TEST_CASE("Utility: threaded_copy_from_socket rejects invalid socket descriptor")
{
    xtd::pipeline pipeline;

    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_from_socket(-1, pipeline.writer()),
        std::invalid_argument
    );
}

TEST_CASE("Utility: threaded_copy_from_socket rejects zero chunk size")
{
    xtd::pipeline pipeline;

    CHECK_THROWS_AS(
        xtd::pipe_utils::threaded_copy_from_socket(STDIN_FILENO, pipeline.writer(), 0),
        std::invalid_argument
    );
}

TEST_CASE("Utility: threaded_copy_from_socket completes when recv fails with non-EINTR error")
{
    int pipeFds[2] = {-1, -1};
    REQUIRE(::pipe(pipeFds) == 0);

    xtd::pipeline pipeline;
    xtd::pipe_reader& reader = pipeline.reader();
    std::thread copier = xtd::pipe_utils::threaded_copy_from_socket(pipeFds[0], pipeline.writer(), 16);

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

TEST_CASE("Utility: threaded_copy_from_socket copies split null-delimited records")
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
            const auto n = ::send(fd, data + sent, length - sent, 0);
            if (n <= 0)
                return false;

            sent += n;
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
        xtd::pipeline pipeline;  
        std::thread copier = xtd::pipe_utils::threaded_copy_from_socket(connectionFd, pipeline.writer(), 3);
        
        xtd::pipe_reader& reader = pipeline.reader();
        while (const xtd::read_result result = reader.read())
        {
            xtd::segmented_byte_view seq = result.buffer();

            while (xtd::position pos = seq.position_of(delimiter))
            {
                received.push_back(seq.slice(pos).to_string());
                seq = seq.slice(pos + 1, seq.end());
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

TEST_CASE("Memory: process RSS remains bounded after repeated write/read cycles")
{
    auto runBatch = [](const std::size_t pipesCount)
    {
        constexpr std::size_t writesPerPipe = 24;
        constexpr std::size_t payloadSize = 4096;
        constexpr std::size_t bytesPerPipe = writesPerPipe * payloadSize;

        const std::string payload(payloadSize, 'x');

        for (std::size_t pipeIndex = 0; pipeIndex < pipesCount; ++pipeIndex)
        {
            xtd::pipeline pipeline(xtd::pipe_options{
                .buffer_size = 4096,
                .resume_writer_threshold = bytesPerPipe,
                .pause_writer_threshold = bytesPerPipe + payloadSize,
            });

            xtd::pipe_writer& writer = pipeline.writer();
            xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("pipeline: Feeds it self for 8GB of data")
{
    const auto startedAt = std::chrono::steady_clock::now();

    const std::size_t totalBytes = 8ULL * 1024 * 1024 * 1024;
    xtd::pipeline pipeline;
    xtd::pipe_writer& writer = pipeline.writer();
    writer.write(std::string(1024 * 16, 'A'));
    
    xtd::pipe_reader& reader = pipeline.reader();
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

TEST_CASE("pipeline: same-thread write-before-advance can block until advance with small thresholds")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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

TEST_CASE("pipeline: writer pauses exactly at pause threshold and resumes after advance")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipeline(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer& writer = pipeline.writer();
    xtd::pipe_reader& reader = pipeline.reader();

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
    CHECK(secondBuffer.to_string() == "5678Z");
    CHECK(second.completed());

    reader.advance(secondBuffer.end(), secondBuffer.end());
    reader.complete();
}


TEST_CASE("data_segment: starts empty with full writable capacity")
{
    xtd::fixed_buffer_pool pool(8, 0);
    xtd::data_segment segment(pool);

    CHECK(segment.capacity() == 8);
    CHECK(segment.readable_size() == 0);
    CHECK(segment.writable_size() == 8);
    CHECK(segment.readable_bytes().empty());
    CHECK(segment.writable_bytes().size() == 8);
    CHECK_FALSE(segment.full());
}

TEST_CASE("data_segment: copy_from appends readable bytes until capacity")
{
    xtd::fixed_buffer_pool pool(5, 0);
    xtd::data_segment segment(pool);
    const std::array<std::byte, 7> source = {
        std::byte{'A'},
        std::byte{'B'},
        std::byte{'C'},
        std::byte{'D'},
        std::byte{'E'},
        std::byte{'F'},
        std::byte{'G'},
    };

    CHECK(segment.copy_from(source.data(), 3) == 3);
    CHECK(segment.readable_size() == 3);
    CHECK(segment.writable_size() == 2);

    const std::span<const std::byte> firstReadable = segment.readable_bytes();
    REQUIRE(firstReadable.size() == 3);
    CHECK(firstReadable[0] == std::byte{'A'});
    CHECK(firstReadable[1] == std::byte{'B'});
    CHECK(firstReadable[2] == std::byte{'C'});

    CHECK(segment.copy_from(source.data() + 3, 4) == 2);
    CHECK(segment.readable_size() == 5);
    CHECK(segment.writable_size() == 0);
    CHECK(segment.full());

    const std::span<const std::byte> readable = segment.readable_bytes();
    REQUIRE(readable.size() == 5);
    CHECK(readable[0] == std::byte{'A'});
    CHECK(readable[1] == std::byte{'B'});
    CHECK(readable[2] == std::byte{'C'});
    CHECK(readable[3] == std::byte{'D'});
    CHECK(readable[4] == std::byte{'E'});
}

TEST_CASE("data_segment: advance consumes readable bytes and rejects over-consume")
{
    xtd::fixed_buffer_pool pool(6, 0);
    xtd::data_segment segment(pool);
    const std::array<std::byte, 4> source = {
        std::byte{'x'},
        std::byte{'y'},
        std::byte{'z'},
        std::byte{'!'},
    };

    REQUIRE(segment.copy_from(source.data(), source.size()) == source.size());
    REQUIRE(segment.readable_size() == 4);

    segment.advance_read(2);

    CHECK(segment.readable_size() == 2);
    const std::span<const std::byte> readable = segment.readable_bytes();
    REQUIRE(readable.size() == 2);
    CHECK(readable[0] == std::byte{'z'});
    CHECK(readable[1] == std::byte{'!'});
    CHECK_THROWS_AS(segment.advance_read(3), std::out_of_range);
    segment.advance_read(2);
    CHECK(segment.readable_bytes().empty());
}

TEST_CASE("fixed_buffer_pool: empty") {
    xtd::fixed_buffer_pool pool(1, 0);
    CHECK(pool.pool_size() == 0);
}

TEST_CASE("data_segment returns buffers to the pool on destruction")
{
    xtd::fixed_buffer_pool pool(1, 3);
    CHECK(pool.pool_size() == 0);

    {
        xtd::data_segment segment1(pool);
        CHECK(pool.pool_size() == 0);
    }

    CHECK(pool.pool_size() == 1);

    {
        xtd::data_segment segment2(pool);
        CHECK(pool.pool_size() == 0); // Reuses the pooled buffer.
    }

    CHECK(pool.pool_size() == 1);

    {
        xtd::data_segment segment1(pool);
        xtd::data_segment segment2(pool);
        xtd::data_segment segment3(pool);

        CHECK(pool.pool_size() == 0);
    }

    CHECK(pool.pool_size() == 3);
}

TEST_CASE("segmented_byte_view: Construction with segments from test helper")
{
    std::vector<std::byte> seg1(10, std::byte{0x11});
    std::vector<std::byte> seg2(15, std::byte{0x22});
    std::vector<std::byte> seg3(20, std::byte{0x33});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        45
    );

    CHECK(seq.size() == 45);
    CHECK(seq.segment_count() == 3);
    CHECK_FALSE(seq.empty());
}

TEST_CASE("segmented_byte_view: Empty sequence construction")
{
    std::vector<std::byte> seg(0);
    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(seg.data(), seg.size())},
        0
    );

    CHECK(seq.size() == 0);
    CHECK(seq.empty());
}

TEST_CASE("segmented_byte_view: Single segment sequence")
{
    std::vector<std::byte> data(50, std::byte{0xAA});
    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        50
    );

    CHECK(seq.size() == 50);
    CHECK(seq.segment_count() == 1);
}

TEST_CASE("segmented_byte_view: Slicing with offset returns correct size")
{
    std::vector<std::byte> seg1(10, std::byte{0x66});
    std::vector<std::byte> seg2(10, std::byte{0x66});
    std::vector<std::byte> seg3(10, std::byte{0x66});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        30
    );

    CHECK(seq.size() == 30);
    xtd::segmented_byte_view sliced = seq.slice(5, 20);
    CHECK(sliced.size() == 20);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(sliced) == 5);
}

TEST_CASE("segmented_byte_view: Slicing preserves segment count when crossing boundaries")
{
    std::vector<std::byte> seg1(10, std::byte{0x11});
    std::vector<std::byte> seg2(10, std::byte{0x22});
    std::vector<std::byte> seg3(10, std::byte{0x33});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        30
    );

    // Slice that uses (offset, size) - begin at offset 8, size 10
    xtd::segmented_byte_view sliced = seq.slice(8, 10);
    CHECK(sliced.size() == 10);
    CHECK(sliced.segment_count() >= 1);
}

TEST_CASE("segmented_byte_view: Nested slicing maintains consistency")
{
    std::vector<std::byte> data(100, std::byte{0xFF});
    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        100
    );

    xtd::segmented_byte_view slice1 = seq.slice(10, 80);
    CHECK(slice1.size() == 80);

    xtd::segmented_byte_view slice2 = slice1.slice(10, 60);
    CHECK(slice2.size() == 60);
}

TEST_CASE("segmented_byte_view: position_of finds byte in single segment")
{
    std::vector<std::byte> data = {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
        std::byte{0x04}, std::byte{0x05}
    };

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        5
    );

    CHECK(seq.position_of(std::byte{0x03}));
}

TEST_CASE("segmented_byte_view: position_of finds char in sequence")
{
    std::vector<std::byte> data{
        std::byte{'h'}, std::byte{'e'}, std::byte{'l'},
        std::byte{'l'}, std::byte{'o'}
    };

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        5
    );

    CHECK(seq.position_of('l'));
}

TEST_CASE("segmented_byte_view: position_of returns false when not found")
{
    std::vector<std::byte> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        3
    );

    CHECK_FALSE(seq.position_of(std::byte{0xFF}));
}

TEST_CASE("segmented_byte_view: position_of in empty sequence returns false")
{
    std::vector<std::byte> empty_data;

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(empty_data.data(), empty_data.size())},
        0
    );

    CHECK_FALSE(seq.position_of(std::byte{'x'}));
}

TEST_CASE("segmented_byte_view: copy_to with raw byte buffer")
{
    std::vector<std::byte> source{
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD}
    };

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(source.data(), source.size())},
        4
    );

    std::vector<std::byte> dest(4);
    std::size_t copied = seq.copy_to(dest.data(), dest.size());

    CHECK(copied == 4);
    CHECK(dest == source);
}

TEST_CASE("segmented_byte_view: copy_to truncates when destination smaller than sequence")
{
    std::vector<std::byte> source(10, std::byte{0x77});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(source.data(), source.size())},
        10
    );

    std::vector<std::byte> dest(5);
    std::size_t copied = seq.copy_to(dest.data(), dest.size());

    CHECK(copied == 5);
    for (const auto& b : dest) {
        CHECK(b == std::byte{0x77});
    }
}

TEST_CASE("segmented_byte_view: copy_to with vector destination")
{
    std::vector<std::byte> source(8, std::byte{0x99});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(source.data(), source.size())},
        8
    );

    std::vector<std::byte> dest(8);
    std::size_t copied = seq.copy_to(dest);

    CHECK(copied == 8);
    CHECK(dest == source);
}

TEST_CASE("segmented_byte_view: copy_to with trivially copyable type")
{
    std::uint32_t value = 0x12345678;
    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(reinterpret_cast<const std::byte*>(&value), sizeof(value))},
        0
    );

    std::uint32_t dest = 0;
    CHECK(seq.copy_to(dest));
    CHECK(dest == value);
}

TEST_CASE("segmented_byte_view: to_string converts single segment correctly")
{
    std::string original = "Hello";
    std::vector<std::byte> data;
    for (char c : original) {
        data.push_back(std::byte(static_cast<unsigned char>(c)));
    }

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        5
    );

    CHECK(seq.to_string() == "Hello");
}

TEST_CASE("segmented_byte_view: to_string for empty sequence")
{
    std::vector<std::byte> empty_data;

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(empty_data.data(), empty_data.size())},
        0
    );

    CHECK(seq.to_string() == "");
}

TEST_CASE("segmented_byte_view: segments() provides span of segment views")
{
    std::vector<std::byte> seg1(5, std::byte{0x11});
    std::vector<std::byte> seg2(5, std::byte{0x22});
    std::vector<std::byte> seg3(5, std::byte{0x33});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        15
    );

    auto segments = seq.segments();
    CHECK(segments.size() == 3);
    CHECK(segments[0].size() == 5);
    CHECK(segments[1].size() == 5);
    CHECK(segments[2].size() == 5);
}

TEST_CASE("segmented_byte_view: segments_size returns correct count")
{
    std::vector<std::byte> seg1(4, std::byte{0xAA});
    std::vector<std::byte> seg2(4, std::byte{0xBB});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size())
        },
        8
    );

    CHECK(seq.segment_count() == 2);
}

TEST_CASE("segmented_byte_view: begin() and end() positions are obtainable")
{
    std::vector<std::byte> data(20, std::byte{0x44});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        20
    );

    // Verify positions can be used (e.g., in slicing)
    xtd::segmented_byte_view sliced = seq.slice(seq.begin(), seq.end());
    CHECK(sliced.size() == 20);
}

TEST_CASE("segmented_byte_view: Slicing across multiple segments maintains boundaries")
{
    std::vector<std::byte> seg1(6, std::byte{0x11});
    std::vector<std::byte> seg2(6, std::byte{0x22});
    std::vector<std::byte> seg3(6, std::byte{0x33});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        18
    );

    // Slice using (beginOffset, size) - from offset 3 spanning 12 bytes
    xtd::segmented_byte_view sliced = seq.slice(3, 12);
    CHECK(sliced.size() == 12);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(sliced) == 3);
}

TEST_CASE("segmented_byte_view: First segment begin offset after slicing")
{
    std::vector<std::byte> seg1(10, std::byte{0xAA});
    std::vector<std::byte> seg2(10, std::byte{0xBB});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size())
        },
        14
    );

    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(seq) == 0);

    xtd::segmented_byte_view sliced = seq.slice(7, 10);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(sliced) == 7);
}

TEST_CASE("segmented_byte_view: copy_to with null pointer and zero size is safe")
{
    std::vector<std::byte> data(10, std::byte{0xFF});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        10
    );

    std::size_t copied = seq.copy_to(nullptr, 0);
    CHECK(copied == 0);
}

TEST_CASE("segmented_byte_view: Relative slicing works correctly")
{
    std::vector<std::byte> data(20, std::byte{0xCC});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        20
    );

    // Slice(offset, size) - from byte 5 for 10 bytes
    xtd::segmented_byte_view sliced = seq.slice(5, 10);
    CHECK(sliced.size() == 10);
}

TEST_CASE("segmented_byte_view: Slicing maintains sequence ID")
{
    std::vector<std::byte> data(50, std::byte{0xEE});
    const std::uint64_t original_sequence_id = 12345;

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        original_sequence_id
    );

    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(seq) == original_sequence_id);

    // Slice the sequence
    xtd::segmented_byte_view sliced = seq.slice(10, 30);

    // Verify sequence ID is maintained after slicing
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(sliced) == original_sequence_id);
}

TEST_CASE("segmented_byte_view: Multiple slices maintain same sequence ID")
{
    std::vector<std::byte> seg1(8, std::byte{0x11});
    std::vector<std::byte> seg2(8, std::byte{0x22});
    std::vector<std::byte> seg3(8, std::byte{0x33});
    const std::uint64_t sequence_id = 99999;

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            std::span<const std::byte>(seg1.data(), seg1.size()),
            std::span<const std::byte>(seg2.data(), seg2.size()),
            std::span<const std::byte>(seg3.data(), seg3.size())
        },
        sequence_id
    );

    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(seq) == sequence_id);

    xtd::segmented_byte_view slice1 = seq.slice(5, 10);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(slice1) == sequence_id);

    xtd::segmented_byte_view slice2 = seq.slice(12, 10);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(slice2) == sequence_id);

    // Nested slice
    xtd::segmented_byte_view nested_slice = slice1.slice(2, 6);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(nested_slice) == sequence_id);
}

TEST_CASE("segmented_byte_view: Multiple overlapping slices remain independent")
{
    std::vector<std::byte> data(30, std::byte{0xDD});

    xtd::segmented_byte_view seq = xtd::test_helper_segmented_byte_view::create_from_segments(
        {std::span<const std::byte>(data.data(), data.size())},
        18
    );

    // slice(offset, size) - offset 5, size 15
    xtd::segmented_byte_view slice1 = seq.slice(5, 15);
    // slice(offset, size) - offset 10, size 15
    xtd::segmented_byte_view slice2 = seq.slice(10, 15);

    CHECK(slice1.size() == 15);
    CHECK(slice2.size() == 15);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(slice1) == 5);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(slice2) == 10);
}

TEST_CASE("pipeline docs example A: minimal text pipeline")
{
    xtd::pipeline pipe(xtd::pipe_options{.buffer_size = 3});
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

    CHECK(writer.write("hello") == 5);
    writer.complete();

    const xtd::read_result rr = reader.read();
    const xtd::segmented_byte_view buffer = rr.buffer();
    CHECK(buffer.segment_count() == 2);

    const xtd::position pos = buffer.position_of('o');
    CHECK(buffer[pos] == std::byte{'o'});
    
    CHECK(buffer[0] == std::byte{'h'});
    CHECK(buffer[1] == std::byte{'e'});
    CHECK(buffer[2] == std::byte{'l'});
    CHECK(buffer[3] == std::byte{'l'});
    CHECK(buffer[4] == std::byte{'o'});
    CHECK(buffer.to_string() == "hello");
    CHECK(rr.completed());

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}

TEST_CASE("deserialize windows strings with \r\n") {
    xtd::pipeline pipe;
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

    CHECK(writer.write("hello\r\nworld\r\n") == 14);
    writer.complete();

    std::vector<std::string> lines;

    while (const xtd::read_result rr = reader.read())
    {
        xtd::segmented_byte_view seq = rr.buffer();

        while (xtd::position pos = seq.position_of('\n'))
        {
            auto line_bytes = seq.slice(pos);
            if (auto carriege_return_pos = line_bytes.position_of('\r')) {
                line_bytes = line_bytes.slice(0, carriege_return_pos);
            }
            lines.push_back(line_bytes.to_string());
            seq = seq.slice(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());
        if (rr.completed()) break;
    }

    CHECK(lines.size() == 2);
    CHECK(lines[0] == "hello");
    CHECK(lines[1] == "world");
}

TEST_CASE("pipeline docs example B: delimiter parser across segmented buffers")
{
    xtd::pipeline pipe(xtd::pipe_options{.buffer_size = 3});
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

    CHECK(writer.write("ab\ncd\nef\n") == 9);
    writer.complete();

    std::vector<std::string> lines;

    while (const xtd::read_result rr = reader.read())
    {
        xtd::segmented_byte_view seq = rr.buffer();

        while (xtd::position pos = seq.position_of('\n'))
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

TEST_CASE("pipeline docs example C: backpressure with producer thread")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

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
    CHECK(secondBuffer.to_string() == "5678abcd");
    CHECK(second.completed());

    reader.advance(secondBuffer.end(), secondBuffer.end());
    reader.complete();
}

TEST_CASE("pipeline docs convenience overload: advance(sequence) maps to consumed=begin examined=end")
{
    xtd::pipeline pipe;
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

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

TEST_CASE("pipeline docs reader complete invalidates pending read")
{
    xtd::pipeline pipe;
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

    CHECK(writer.write("abc") == 3);

    const xtd::read_result rr = reader.read();
    const xtd::segmented_byte_view seq = rr.buffer();
    CHECK(seq.to_string() == "abc");

    // Completing the reader clears pending-read state and invalidates this read context.
    reader.complete();

    CHECK_THROWS_AS(reader.advance(seq.begin(), seq.end()), std::runtime_error);
    CHECK_THROWS_AS(static_cast<void>(reader.read()), std::runtime_error);
    CHECK_THROWS_AS(writer.write("z"), std::runtime_error);
}

TEST_CASE("pipeline: large write blocks mid-call when pause threshold is reached")
{
    using namespace std::chrono_literals;

    xtd::pipeline pipe(xtd::pipe_options{
        .buffer_size = 64,
        .resume_writer_threshold = 64,
        .pause_writer_threshold = 128,
    });

    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();
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