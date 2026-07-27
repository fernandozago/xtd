#ifndef XTD_TESTS_POSITION_TESTS_H
#define XTD_TESTS_POSITION_TESTS_H

#include "../third_party/doctest.h"
#include "../../src/pipeline/pipeline.h"

namespace xtd_position_tests {

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


TEST_CASE("position: default constructed value is invalid")
{
    const xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE("position: arithmetic on invalid position remains invalid")
{
    xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos + 1));
    pos += 3;
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE("position: equality for invalid positions")
{
    const xtd::position lhs{};
    const xtd::position rhs{};
    CHECK(lhs == rhs);
    CHECK_FALSE(lhs != rhs);
}
} // namespace xtd_position_tests

#endif
