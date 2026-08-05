#ifndef XTD_TESTS_FIXED_BUFFER_POOL_TESTS_H
#define XTD_TESTS_FIXED_BUFFER_POOL_TESTS_H
#include "../third_party/catch2/catch_amalgamated.hpp"

#include "pipeline/fixed_pool_resource.h"
#include "pipeline/data_segment.h"

namespace xtd_fixed_buffer_pool_tests {

struct FixedBufferPoolTests {};

TEST_CASE_METHOD(FixedBufferPoolTests, "starts empty with full writable capacity")
{
    xtd::fixed_pool_resource pool(8, 1);
    xtd::data_segment segment(8, pool);

    CHECK(segment.capacity() == 8);
    CHECK(segment.readable_size() == 0);
    CHECK(segment.writable_size() == 8);
    CHECK(segment.readable_bytes().empty());
    CHECK(segment.writable_bytes().size() == 8);
    CHECK_FALSE(segment.full());
}


TEST_CASE_METHOD(FixedBufferPoolTests, "copy_from appends readable bytes until capacity")
{
    xtd::fixed_pool_resource pool(5, 1);
    xtd::data_segment segment(5, pool);
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


TEST_CASE_METHOD(FixedBufferPoolTests, "advance consumes readable bytes and rejects over-consume")
{
    xtd::fixed_pool_resource pool(6, 1);
    xtd::data_segment segment(6, pool);
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


TEST_CASE_METHOD(FixedBufferPoolTests, "fixed_buffer_pool: empty") {
    xtd::fixed_pool_resource pool(8, 1);
    CHECK(pool.pool_size() == 0);
}


TEST_CASE_METHOD(FixedBufferPoolTests, "returns buffers to the pool on destruction")
{
    xtd::fixed_pool_resource pool(8, 3);
    CHECK(pool.pool_size() == 0);

    {
        xtd::data_segment segment1(8, pool);
        CHECK(pool.pool_size() == 0);
    }

    CHECK(pool.pool_size() == 1);

    {
        xtd::data_segment segment2(8, pool);
        CHECK(pool.pool_size() == 0); // Reuses the pooled buffer.
    }

    CHECK(pool.pool_size() == 1);

    {
        xtd::data_segment segment1(8, pool);
        xtd::data_segment segment2(8, pool);
        xtd::data_segment segment3(8, pool);

        CHECK(pool.pool_size() == 0);
    }

    CHECK(pool.pool_size() == 3);
}


} // namespace xtd_fixed_buffer_pool_tests

#endif
