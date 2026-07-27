#ifndef XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H
#define XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H

#include "../third_party/doctest.h"
#include "../../src/pipeline/pipeline.h"
#include "pipeline_test_common.h"

namespace xtd_segmented_byte_view_tests {

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


TEST_CASE("segmented_byte_view: slice_in_place(position) and from_end validation")
{
    xtd::pipeline pipe(xtd::pipe_options{.buffer_size = 3});
    xtd::pipe_writer& writer = pipe.writer();
    xtd::pipe_reader& reader = pipe.reader();

    CHECK(writer.write("hello") == 5);
    writer.complete();

    const xtd::read_result rr = reader.read();
    xtd::segmented_byte_view buffer = rr.buffer();

    const xtd::position endPos = buffer.position_of('o') + 1;
    buffer.slice_in_place(endPos);
    CHECK(buffer.to_string() == "hello");

    CHECK_THROWS_AS(buffer[xtd::from_end{0}], std::out_of_range);
    CHECK_THROWS_AS(buffer[xtd::from_end{6}], std::out_of_range);

    reader.advance(buffer.end(), buffer.end());
    reader.complete();
}


} // namespace xtd_segmented_byte_view_tests

#endif
