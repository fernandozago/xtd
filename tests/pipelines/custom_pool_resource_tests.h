#ifndef XTD_TESTS_CUSTOM_POOL_RESOURCE_TESTS_H
#define XTD_TESTS_CUSTOM_POOL_RESOURCE_TESTS_H
#define XTD_ALLOW_EXPERIMENTAL
#include "../third_party/catch2/catch_amalgamated.hpp"

#include "pipeline/custom_allocators/arena_pool_resource.h"
#include "pipeline/custom_allocators/fixed_pool_resource.h"
#include "pipeline/data_segment.h"
#include "pipeline/pipeline_impl.h"

namespace xtd_custom_pool_resource_tests {

struct CustomPoolResourceTests {};

inline std::unique_ptr<std::pmr::memory_resource> make_pool(
    const xtd::allocator_kind kind,
    const std::size_t buffer_size,
    const std::size_t max_pool_size)
{
    switch (kind) {
        case xtd::allocator_kind::fixed_pool_resource:
            return std::make_unique<xtd::fixed_pool_resource>(buffer_size, max_pool_size);
        case xtd::allocator_kind::arena_pool_resource:
            return std::make_unique<xtd::arena_pool_resource>(buffer_size, max_pool_size);
        case xtd::allocator_kind::unsynchronized_pool_resource:
            return std::make_unique<std::pmr::unsynchronized_pool_resource>(std::pmr::pool_options{
                .max_blocks_per_chunk = max_pool_size,
                .largest_required_pool_block = buffer_size,
            });
    }

    throw std::logic_error{"unsupported allocator kind"};
}

TEST_CASE_METHOD(CustomPoolResourceTests, "custom pool starts empty with full writable capacity", "[pipeline][custom-pool]")
{
    const auto kind = GENERATE(
        xtd::allocator_kind::fixed_pool_resource,
        xtd::allocator_kind::arena_pool_resource,
        xtd::allocator_kind::unsynchronized_pool_resource
    );

    CAPTURE(kind);

    auto pool = make_pool(kind, 8, 1);
    xtd::data_segment segment(8, *pool);

    CHECK(segment.capacity() == 8);
    CHECK(segment.readable_size() == 0);
    CHECK(segment.writable_size() == 8);
    CHECK(segment.readable_bytes().empty());
    CHECK(segment.writable_bytes().size() == 8);
    CHECK_FALSE(segment.full());
}


TEST_CASE_METHOD(CustomPoolResourceTests, "custom pool copy_from appends readable bytes until capacity", "[pipeline][custom-pool]")
{
    const auto kind = GENERATE(
        xtd::allocator_kind::fixed_pool_resource,
        xtd::allocator_kind::arena_pool_resource,
        xtd::allocator_kind::unsynchronized_pool_resource
    );

    CAPTURE(kind);

    auto pool = make_pool(kind, 5, 1);
    xtd::data_segment segment(5, *pool);
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
    const std::span<const std::byte> first_readable = segment.readable_bytes();
    REQUIRE(first_readable.size() == 3);
    CHECK(first_readable[0] == std::byte{'A'});
    CHECK(first_readable[1] == std::byte{'B'});
    CHECK(first_readable[2] == std::byte{'C'});
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
TEST_CASE_METHOD(CustomPoolResourceTests, "custom pool advance consumes readable bytes and rejects over-consume", "[pipeline][custom-pool]")
{
    const auto kind = GENERATE(
        xtd::allocator_kind::fixed_pool_resource,
        xtd::allocator_kind::arena_pool_resource,
        xtd::allocator_kind::unsynchronized_pool_resource
    );

    CAPTURE(kind);

    auto pool = make_pool(kind, 6, 1);
    xtd::data_segment segment(6, *pool);
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
TEST_CASE_METHOD(CustomPoolResourceTests, "read_result skips empty data segments when building readable view", "[pipeline][custom-pool]")
{
    const auto kind = GENERATE(
        xtd::allocator_kind::fixed_pool_resource,
        xtd::allocator_kind::arena_pool_resource,
        xtd::allocator_kind::unsynchronized_pool_resource
    );

    CAPTURE(kind);

    auto pool = make_pool(kind, 8, 3);
    std::deque<xtd::data_segment> segments;
    segments.emplace_back(8, *pool); // remains empty
    segments.emplace_back(8, *pool); // receives payload
    segments.emplace_back(8, *pool); // remains empty
    const std::array<std::byte, 4> payload = {
        std::byte{'t'},
        std::byte{'e'},
        std::byte{'s'},
        std::byte{'t'},
    };
    REQUIRE(segments[1].copy_from(payload.data(), payload.size()) == payload.size());
    REQUIRE(segments[0].readable_size() == 0);
    REQUIRE(segments[2].readable_size() == 0);
    std::size_t pending_read_size = 0;
    const xtd::read_result result(segments, false, pending_read_size);
    const xtd::segmented_byte_view buffer = result.buffer();
    CHECK_FALSE(result.completed());
    CHECK(buffer.size() == payload.size());
    CHECK(buffer.to_string() == "test");
    CHECK(buffer.segment_count() == 1);
    CHECK(pending_read_size == payload.size());
}

} // namespace xtd_custom_pool_resource_tests

#endif