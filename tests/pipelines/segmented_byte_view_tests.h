#ifndef XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H
#define XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H

#include "../third_party/catch2/catch_amalgamated.hpp"

#include <vector>

#include "pipeline/segmented_byte_view.h"

namespace xtd {
    class test_helper_segmented_byte_view {
    public:
        static segmented_byte_view create_from_segments(std::vector<std::span<const std::byte>>&& segments) {
            std::vector<std::span<const std::byte>> readable_segments;
            readable_segments.reserve(segments.size());

            std::size_t total_size = 0;
            for (const auto segment : segments) {
                if (segment.empty()) {
                    continue;
                }

                readable_segments.emplace_back(segment);
                total_size += segment.size();
            }

            return segmented_byte_view{
                std::move(readable_segments),
                total_size
            };
        }

        static segmented_byte_view create_from_segments(const std::vector<std::string_view>& segments) {
            std::vector<std::span<const std::byte>> byte_segments;
            byte_segments.reserve(segments.size());

            for (const auto segment : segments) {
                byte_segments.emplace_back(std::as_bytes(
                    std::span<const char>{segment.data(), segment.size()}
                ));
            }

            return create_from_segments(std::move(byte_segments));
        }

        static segmented_byte_view create_from_segments(const std::initializer_list<std::string_view> segments) {
            return create_from_segments(
                std::vector<std::string_view>{segments}
            );
        }

        static std::size_t get_first_segment_begin(const segmented_byte_view& sequence) {
            return sequence.m_begin_offset;
        }
    };
}

namespace {
    struct slice_case {
        std::size_t offset;
        std::size_t size;
        std::string_view expected;
    };

    constexpr std::array slice_cases = {
        slice_case{0, 0, ""},
        slice_case{0, 3, "abc"},
        slice_case{0, 9, "abcdefghi"},
        slice_case{2, 1, "c"},
        slice_case{2, 5, "cdefg"},
        slice_case{3, 3, "def"},
        slice_case{5, 4, "fghi"},
        slice_case{9, 0, ""}
    };

    void check_slice_result(const xtd::segmented_byte_view& sequence,const slice_case& test) {
        CHECK(sequence.to_string() == test.expected);
        CHECK(sequence.size() == test.size);
        if (!sequence.empty()) {
            CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(sequence) == test.offset);
        }
    }

    struct test_data_trivially_copyable {
        struct nested_data {
            std::uint16_t code;
            std::uint32_t amount;
        };

        union trivial_union {
            std::uint64_t integer;
            double floating_point;
        };

        std::uint32_t id;
        nested_data nested;
        trivial_union value;
    };

    static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable>);
    static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::nested_data>);
    static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::trivial_union>);
}

struct SegmentedByteViewTests{};

TEST_CASE_METHOD(SegmentedByteViewTests, "construction normalizes segments") {
    struct test_case {
        std::vector<std::string_view> segments;
        std::size_t expected_size;
        std::size_t expected_segment_count;
        std::string_view expected_text;
    };

    const std::array cases = {
        test_case{{}, 0, 0, ""},
        test_case{{""}, 0, 0, ""},
        test_case{{"abc"}, 3, 1, "abc"},
        test_case{{"ab", "", "cde"}, 5, 2, "abcde"},
        test_case{{"abc", "def", "ghi"}, 9, 3, "abcdefghi"}
    };

    for (const auto& test : cases) {
        const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            test.segments
        );

        CHECK(sequence.size() == test.expected_size);
        CHECK(sequence.segment_count() == test.expected_segment_count);
        CHECK(sequence.empty() == test.expected_text.empty());
        CHECK(sequence.to_string() == test.expected_text);

        const auto segments = sequence.segments();
        CHECK(segments.size() == test.expected_segment_count);

        std::size_t readable_index = 0;
        for (const auto source_segment : test.segments) {
            if (source_segment.empty()) {
                continue;
            }

            CHECK(segments[readable_index].size() == source_segment.size());
            ++readable_index;
        }
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, size_t)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        check_slice_result(sequence.slice(test.offset, test.size), test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, size_t) supports nested slices") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"0123456789", "abcdefghij", "KLMNOPQRST"}
    );

    const xtd::segmented_byte_view outer = sequence.slice(5, 20);
    const xtd::segmented_byte_view nested = outer.slice(4, 10);

    CHECK(outer.to_string() == "56789abcdefghijKLMNO");
    CHECK(nested.to_string() == "9abcdefghi");
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(outer) == 5);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(nested) == 9);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, size_t) supports overlapping slices") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"0123456789", "abcdefghij", "KLMNOPQRST"}
    );

    const xtd::segmented_byte_view first = sequence.slice(5, 20);
    const xtd::segmented_byte_view second = sequence.slice(10, 15);

    CHECK(first.to_string() == "56789abcdefghijKLMNO");
    CHECK(second.to_string() == "abcdefghijKLMNO");
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(first) == 5);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(second) == 10);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, size_t) that reaches end keeps trailing segments") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cd", "efg"}
    );

    const auto sliced = sequence.slice(1, 6);

    CHECK(sliced.to_string() == "bcdefg");
    CHECK(sliced.size() == 6);
    CHECK(sliced.segment_count() == 3);
    CHECK(sliced.begin() == sequence.begin() + 1);
    CHECK(sliced.end() == sequence.end());
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, size_t) rejects invalid ranges") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(4, 0)), std::out_of_range);
    CHECK_THROWS_AS(static_cast<void>(sequence.slice(2, 2)), std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, position)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        const xtd::position end = sequence.begin() + test.offset + test.size;
        check_slice_result(sequence.slice(test.offset, end), test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(size_t, position) rejects invalid positions") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(0, beyond_end)), std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(position, position)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        const xtd::position begin = sequence.begin() + test.offset;
        const xtd::position end = begin + test.size;
        check_slice_result(sequence.slice(begin, end), test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(position, position) rejects invalid positions") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(
        static_cast<void>(sequence.slice(beyond_end, beyond_end)),
        std::out_of_range
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(position)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const std::array cases = {
        slice_case{0, 0, ""},
        slice_case{0, 3, "abc"},
        slice_case{0, 9, "abcdefghi"}
    };

    for (const auto& test : cases) {
        CAPTURE(test.size);
        CAPTURE(test.expected);

        const xtd::position end = sequence.begin() + test.size;
        check_slice_result(sequence.slice(end), test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice(position) rejects invalid positions") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(beyond_end)), std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "position_of(char)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "\n", "cd"}
    );

    struct test_case {
        char value;
        std::size_t offset;
    };

    const std::array cases = {
        test_case{'a', 0},
        test_case{'b', 1},
        test_case{'\n', 2},
        test_case{'d', 4}
    };

    for (const auto& test : cases) {
        CAPTURE(test.value);
        CAPTURE(test.offset);

        const xtd::position result = sequence.position_of(test.value);
        CHECK(result);
        CHECK(result == sequence.begin() + test.offset);
    }

    CHECK_FALSE(sequence.position_of('x'));

    const auto empty = xtd::test_helper_segmented_byte_view::create_from_segments({""});
    CHECK_FALSE(empty.position_of('x'));
}

TEST_CASE_METHOD(SegmentedByteViewTests, "position_of(byte)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "\n", "cd"}
    );

    struct test_case {
        std::byte value;
        std::size_t offset;
    };

    const std::array cases = {
        test_case{std::byte{'a'}, 0},
        test_case{std::byte{'b'}, 1},
        test_case{std::byte{'\n'}, 2},
        test_case{std::byte{'d'}, 4}
    };

    for (const auto& test : cases) {
        CAPTURE(test.offset);

        const auto result = sequence.position_of(test.value);
        CHECK(result);
        CHECK(result == sequence.begin() + test.offset);
    }

    CHECK_FALSE(sequence.position_of(std::byte{0xFF}));

    const xtd::segmented_byte_view empty = xtd::test_helper_segmented_byte_view::create_from_segments({""});
    CHECK_FALSE(empty.position_of(std::byte{'x'}));
}

template<typename T>
struct position_of_any_test_data {
    T value;
    std::size_t offset;
};

TEST_CASE_METHOD(SegmentedByteViewTests, "position_of_any tests") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cd"}
    );

    SECTION("string_view") {
        const std::array cases = {
            position_of_any_test_data<std::string>{"-a-", 0},
            position_of_any_test_data<std::string>{"-b-", 1},
            position_of_any_test_data<std::string>{"-c-", 2},
            position_of_any_test_data<std::string>{"-d-", 3},
        };

        for (const auto& test : cases) {
            CAPTURE(test.offset);
            const std::string_view value = test.value;
            const xtd::position result = sequence.position_of_any(value);
            CHECK(result);
            CHECK(result == sequence.begin() + test.offset);
        }
    }

    SECTION("span<char>") {
        const std::array cases = {
            position_of_any_test_data<std::vector<char>>{{'-', 'a', '-'}, 0},
            position_of_any_test_data<std::vector<char>>{{'-', 'b', '-'}, 1},
            position_of_any_test_data<std::vector<char>>{{'-', 'c', '-'}, 2},
            position_of_any_test_data<std::vector<char>>{{'-', 'd', '-'}, 3},
        };

        for (const auto& test : cases) {
            CAPTURE(test.offset);
            const xtd::position result = sequence.position_of_any(test.value);
            CHECK(result);
            CHECK(result == sequence.begin() + test.offset);
        }
    }

    SECTION("span<byte>") {
        const std::array cases = {
            position_of_any_test_data<std::vector<std::byte>>{{std::byte{'-'}, std::byte{'a'}, std::byte{'-'}}, 0},
            position_of_any_test_data<std::vector<std::byte>>{{std::byte{'-'}, std::byte{'b'}, std::byte{'-'}}, 1},
            position_of_any_test_data<std::vector<std::byte>>{{std::byte{'-'}, std::byte{'c'}, std::byte{'-'}}, 2},
            position_of_any_test_data<std::vector<std::byte>>{{std::byte{'-'}, std::byte{'d'}, std::byte{'-'}}, 3},
        };

        for (const auto& test : cases) {
            CAPTURE(test.offset);
            const xtd::position result = sequence.position_of_any(test.value);
            CHECK(result);
            CHECK(result == sequence.begin() + test.offset);
        }
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(byte*, size_t)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"}
    );
    constexpr std::string_view expected = "abcdef";

    for (const std::size_t capacity : {
        std::size_t{0},
        std::size_t{3},
        std::size_t{6},
        std::size_t{9}
    }) {
        CAPTURE(capacity);

        std::vector<std::byte> destination(capacity, std::byte{0xFF});
        const std::size_t copied = sequence.copy_to(destination.data(), destination.size());
        const std::size_t expected_copied = std::min(capacity, expected.size());

        CHECK(copied == expected_copied);
        CHECK(std::equal(
            destination.begin(),
            destination.begin() + static_cast<std::ptrdiff_t>(copied),
            expected.begin(),
            [](const std::byte left, const char right) {
                return left == static_cast<std::byte>(right);
            }
        ));
        CHECK(std::all_of(
            destination.begin() + static_cast<std::ptrdiff_t>(copied),
            destination.end(),
            [](const std::byte value) { return value == std::byte{0xFF}; }
        ));
    }

    CHECK(sequence.copy_to(nullptr, 0) == 0);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(vector<byte>&)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"}
    );
    constexpr std::string_view expected = "abcdef";

    for (const std::size_t capacity : {
        std::size_t{0},
        std::size_t{3},
        std::size_t{6},
        std::size_t{9}
    }) {
        CAPTURE(capacity);

        std::vector<std::byte> destination(capacity, std::byte{0xFF});
        const std::size_t copied = sequence.copy_to(destination);
        const std::size_t expected_copied = std::min(capacity, expected.size());

        CHECK(copied == expected_copied);
        CHECK(std::equal(
            destination.begin(),
            destination.begin() + static_cast<std::ptrdiff_t>(copied),
            expected.begin(),
            [](const std::byte left, const char right) {
                return left == static_cast<std::byte>(right);
            }
        ));
        CHECK(std::all_of(
            destination.begin() + static_cast<std::ptrdiff_t>(copied),
            destination.end(),
            [](const std::byte value) { return value == std::byte{0xFF}; }
        ));
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(T&) copies a scalar") {
    const std::uint32_t source = 0x12345678;
    const auto raw = std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(&source),
        sizeof(source)
    };

    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {raw.first<2>(), raw.last<2>()}
    );

    std::uint32_t destination = 0;
    CHECK(sequence.copy_to(destination));
    CHECK(destination == source);
}
TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(T&) copies a trivially copyable object") {
    test_data_trivially_copyable source{};
    source.id = 0x12345678;
    source.nested.code = 0x4321;
    source.nested.amount = 0xABCDEF01;
    source.value.integer = 0x0123456789ABCDEF;

    const auto bytes = std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(&source),
        sizeof(source)
    };

    const std::size_t first_size =
        offsetof(test_data_trivially_copyable, nested) + 1;

    const std::size_t second_size =
        offsetof(test_data_trivially_copyable, value) + 1 - first_size;

    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            bytes.first(first_size),
            bytes.subspan(first_size, second_size),
            bytes.subspan(first_size + second_size)
        }
    );

    test_data_trivially_copyable destination{};

    REQUIRE(sequence.copy_to(destination));
    CHECK(destination.id == source.id);
    CHECK(destination.nested.code == source.nested.code);
    CHECK(destination.nested.amount == source.nested.amount);
    CHECK(destination.value.integer == source.value.integer);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "is_single_segment reflects current slice layout") {
    const xtd::segmented_byte_view single = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abcdef"}
    );

    const xtd::segmented_byte_view multiple = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def"}
    );

    CHECK(single.is_single_segment());
    CHECK_FALSE(multiple.is_single_segment());

    const xtd::segmented_byte_view sliced_single = multiple.slice(1, 2);
    CHECK(sliced_single.is_single_segment());

    const xtd::segmented_byte_view sliced_multiple = multiple.slice(1, 4);
    CHECK_FALSE(sliced_multiple.is_single_segment());
}

TEST_CASE_METHOD(SegmentedByteViewTests, "as_span returns contiguous bytes for a single segment") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abcdef"}
    );

    REQUIRE(sequence.is_single_segment());

    const std::span<const std::byte>& span = sequence.as_span();

    REQUIRE(span.size() == sequence.size());

    constexpr std::string_view expected = "abcdef";
    CHECK(std::equal(
        span.begin(),
        span.end(),
        expected.begin(),
        [](const std::byte left, const char right) {
            return left == static_cast<std::byte>(right);
        }
    ));
}

TEST_CASE_METHOD(SegmentedByteViewTests, "as_string_view returns text for a single segment") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"hello world"}
    );

    REQUIRE(sequence.is_single_segment());
    CHECK(sequence.as_string_view() == "hello world");
}

TEST_CASE_METHOD(SegmentedByteViewTests, "as_string_view rejects multi-segment views") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"hello", " world"}
    );

    REQUIRE_FALSE(sequence.is_single_segment());

    CHECK_THROWS_AS(
        static_cast<void>(sequence.as_string_view()),
        std::invalid_argument
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](from_end)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"hel", "lo"}
    );

    CHECK(sequence[xtd::from_end{1}] == std::byte{'o'});
    CHECK(sequence[xtd::from_end{5}] == std::byte{'h'});
    CHECK_THROWS_AS(sequence[xtd::from_end{0}], std::out_of_range);
    CHECK_THROWS_AS(sequence[xtd::from_end{6}], std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice_in_place(size_t, size_t)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"}
        );

        sequence.slice_in_place(test.offset, test.size);
        check_slice_result(sequence, test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice_in_place(size_t, position)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"}
        );
        const xtd::position end = sequence.begin() + test.offset + test.size;

        sequence.slice_in_place(test.offset, end);
        check_slice_result(sequence, test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice_in_place(position, position)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"}
        );
        const xtd::position begin = sequence.begin() + test.offset;
        const xtd::position end = begin + test.size;

        sequence.slice_in_place(begin, end);
        check_slice_result(sequence, test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice_in_place(position)") {
    const std::array cases = {
        slice_case{0, 0, ""},
        slice_case{0, 3, "abc"},
        slice_case{0, 9, "abcdefghi"}
    };

    for (const auto& test : cases) {
        CAPTURE(test.size);
        CAPTURE(test.expected);

        xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"}
        );
        const xtd::position end = sequence.begin() + test.size;

        sequence.slice_in_place(end);
        check_slice_result(sequence, test);
    }
}

TEST_CASE_METHOD(SegmentedByteViewTests, "begin and end reflect slice boundaries") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);

    CHECK(sliced.begin() == sequence.begin() + 2);
    CHECK(sliced.end() == sequence.begin() + 7);
    CHECK(sliced.size() == 5);
    CHECK(sliced.to_string() == "cdefg");

    const auto empty = sequence.slice(4, 0);

    CHECK(empty.begin() == sequence.begin() + 4);
    CHECK(empty.end() == sequence.begin() + 4);
    CHECK(empty.size() == 0);
    CHECK(empty.segment_count() == 0);
    CHECK(empty.empty());
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slices reject reversed ranges") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def"}
    );

    CHECK_THROWS_AS(
        static_cast<void>(sequence.slice(
            sequence.begin() + 4,
            sequence.begin() + 2
        )),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        static_cast<void>(sequence.slice(
            4,
            sequence.begin() + 2
        )),
        std::out_of_range
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "nested slices reject positions outside their boundaries") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);

    CHECK_THROWS_AS(
        static_cast<void>(sliced.slice(sequence.begin() + 1)),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        static_cast<void>(sliced.slice(
            sequence.begin() + 1,
            sliced.end()
        )),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        static_cast<void>(sliced.slice(
            sliced.begin(),
            sliced.end() + 1
        )),
        std::out_of_range
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](size_t)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"}
    );

    constexpr std::string_view expected = "abcdef";

    for (std::size_t index = 0; index < expected.size(); ++index) {
        CAPTURE(index);
        CHECK(sequence[index] == static_cast<std::byte>(expected[index]));
    }

    CHECK_THROWS_AS(sequence[expected.size()], std::out_of_range);

    const xtd::segmented_byte_view empty =
        xtd::test_helper_segmented_byte_view::create_from_segments({});

    CHECK_THROWS_AS(empty[0], std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](size_t) works on sliced views") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);
    constexpr std::string_view expected = "cdefg";

    for (std::size_t index = 0; index < expected.size(); ++index) {
        CAPTURE(index);
        CHECK(sliced[index] == static_cast<std::byte>(expected[index]));
    }

    CHECK_THROWS_AS(sliced[expected.size()], std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](position)") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"}
    );

    CHECK(sequence[sequence.begin()] == std::byte{'a'});
    CHECK(sequence[sequence.begin() + 2] == std::byte{'c'});
    CHECK(sequence[sequence.end() - 1] == std::byte{'f'});

    CHECK_THROWS_AS(sequence[sequence.end()], std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](position) respects sliced boundaries") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);

    CHECK(sliced[sliced.begin()] == std::byte{'c'});
    CHECK(sliced[sliced.end() - 1] == std::byte{'g'});

    CHECK_THROWS_AS(
        sliced[sequence.begin() + 1],
        std::out_of_range
    );

    CHECK_THROWS_AS(
        sliced[sliced.end()],
        std::out_of_range
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "operator[](from_end) works on sliced views") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);

    CHECK(sliced[xtd::from_end{1}] == std::byte{'g'});
    CHECK(sliced[xtd::from_end{3}] == std::byte{'e'});
    CHECK(sliced[xtd::from_end{5}] == std::byte{'c'});

    CHECK_THROWS_AS(sliced[xtd::from_end{6}], std::out_of_range);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "position_of only searches inside a sliced view") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"x", "abc", "xdefx"}
    );

    const auto sliced = sequence.slice(1, 7);
    CHECK(sliced.to_string() == "abcxdef");

    const xtd::position found = sliced.position_of('x');

    REQUIRE(found);
    CHECK(found == sequence.begin() + 4);
    CHECK_FALSE(sliced.position_of('z'));

    const xtd::segmented_byte_view without_x = sequence.slice(1, 3);
    CHECK(without_x.to_string() == "abc");
    CHECK_FALSE(without_x.position_of('x'));
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(byte*, size_t) rejects a null destination") {
    const xtd::segmented_byte_view sequence =
        xtd::test_helper_segmented_byte_view::create_from_segments({"abc"});

    CHECK_THROWS_AS(
        static_cast<void>(sequence.copy_to(nullptr, 1)),
        std::invalid_argument
    );
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to(T&) rejects an undersized view") {
    const xtd::segmented_byte_view sequence =
        xtd::test_helper_segmented_byte_view::create_from_segments({"abc"});

    std::uint32_t destination = 0xAABBCCDD;

    CHECK_THROWS_AS(
        static_cast<void>(sequence.copy_to(destination)),
        std::invalid_argument
    );

    CHECK(destination == 0xAABBCCDD);
}

TEST_CASE_METHOD(SegmentedByteViewTests, "copy_to copies a sliced view") {
    const xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"01", "234", "56789"}
    );

    const auto sliced = sequence.slice(1, 7);
    std::vector<std::byte> destination(sliced.size());

    REQUIRE(sliced.copy_to(destination) == sliced.size());

    constexpr std::string_view expected = "1234567";

    CHECK(std::equal(
        destination.begin(),
        destination.end(),
        expected.begin(),
        [](const std::byte left, const char right) {
            return left == static_cast<std::byte>(right);
        }
    ));
}

TEST_CASE_METHOD(SegmentedByteViewTests, "repeated slice_in_place uses relative offsets") {
    xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    sequence.slice_in_place(2, 5);

    CHECK(sequence.to_string() == "cdefg");
    CHECK(sequence.begin() == xtd::position{2});

    sequence.slice_in_place(1, 3);

    CHECK(sequence.to_string() == "def");
    CHECK(sequence.size() == 3);
    CHECK(sequence.begin() == xtd::position{3});
    CHECK(sequence.end() == xtd::position{6});
}

TEST_CASE_METHOD(SegmentedByteViewTests, "slice_in_place failures leave the view unchanged") {
    xtd::segmented_byte_view sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def"}
    );

    const auto original_begin = sequence.begin();
    const auto original_end = sequence.end();
    const auto original_size = sequence.size();
    const auto original_segment_count = sequence.segment_count();
    const auto original_text = sequence.to_string();

    CHECK_THROWS_AS(
        sequence.slice_in_place(7, 0),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        sequence.slice_in_place(4, sequence.begin() + 2),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        sequence.slice_in_place(
            sequence.begin() + 4,
            sequence.begin() + 2
        ),
        std::out_of_range
    );

    CHECK_THROWS_AS(
        sequence.slice_in_place(sequence.end() + 1),
        std::out_of_range
    );

    CHECK(sequence.begin() == original_begin);
    CHECK(sequence.end() == original_end);
    CHECK(sequence.size() == original_size);
    CHECK(sequence.segment_count() == original_segment_count);
    CHECK(sequence.to_string() == original_text);
}

#endif
