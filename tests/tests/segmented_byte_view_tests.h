#ifndef XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H
#define XTD_TESTS_SEGMENTED_BYTE_VIEW_TESTS_H

#include "../third_party/doctest.h"
#include "../../src/pipeline/segmented_byte_view.h"

namespace xtd {
    class test_helper_segmented_byte_view {
    public:
        static segmented_byte_view create_from_segments(std::vector<std::span<const std::byte>>&& segments, const std::uint64_t sequence_id) {
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
                sequence_id,
                total_size
            };
        }

        static segmented_byte_view create_from_segments(const std::vector<std::string_view>& segments, const std::uint64_t sequence_id = 1) {
            std::vector<std::span<const std::byte>> byte_segments;
            byte_segments.reserve(segments.size());

            for (const auto segment : segments) {
                byte_segments.emplace_back(std::as_bytes(
                    std::span<const char>{segment.data(), segment.size()}
                ));
            }

            return create_from_segments(std::move(byte_segments), sequence_id);
        }

        static segmented_byte_view create_from_segments(const std::initializer_list<std::string_view> segments, const std::uint64_t sequence_id = 1) {
            return create_from_segments(
                std::vector<std::string_view>{segments},
                sequence_id
            );
        }

        static std::size_t get_first_segment_begin(const segmented_byte_view& sequence) {
            return sequence.m_begin_offset;
        }

        static std::uint64_t get_sequence_id(const segmented_byte_view& sequence) {
            return sequence.m_sequence_id;
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

    void check_slice_result(const xtd::segmented_byte_view& sequence,const slice_case& test, const std::uint64_t sequence_id) {
        CHECK(sequence.to_string() == test.expected);
        CHECK(sequence.size() == test.size);
        CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(sequence) == sequence_id);
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

TEST_CASE("segmented_byte_view: construction normalizes segments") {
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
            test.segments,
            45
        );

        CHECK(sequence.size() == test.expected_size);
        CHECK(sequence.segment_count() == test.expected_segment_count);
        CHECK(sequence.empty() == test.expected_text.empty());
        CHECK(sequence.to_string() == test.expected_text);
        CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(sequence) == 45);

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

TEST_CASE("segmented_byte_view: slice(size_t, size_t)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        check_slice_result(sequence.slice(test.offset, test.size), test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice(size_t, size_t) supports nested slices") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"0123456789", "abcdefghij", "KLMNOPQRST"},
        99
    );

    const auto outer = sequence.slice(5, 20);
    const auto nested = outer.slice(4, 10);

    CHECK(outer.to_string() == "56789abcdefghijKLMNO");
    CHECK(nested.to_string() == "9abcdefghi");
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(outer) == 5);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(nested) == 9);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(outer) == 99);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(nested) == 99);
}

TEST_CASE("segmented_byte_view: slice(size_t, size_t) supports overlapping slices") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"0123456789", "abcdefghij", "KLMNOPQRST"},
        99
    );

    const auto first = sequence.slice(5, 20);
    const auto second = sequence.slice(10, 15);

    CHECK(first.to_string() == "56789abcdefghijKLMNO");
    CHECK(second.to_string() == "abcdefghijKLMNO");
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(first) == 5);
    CHECK(xtd::test_helper_segmented_byte_view::get_first_segment_begin(second) == 10);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(first) == 99);
    CHECK(xtd::test_helper_segmented_byte_view::get_sequence_id(second) == 99);
}

TEST_CASE("segmented_byte_view: slice(size_t, size_t) that reaches end keeps trailing segments") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cd", "efg"},
        18
    );

    const auto sliced = sequence.slice(1, 6);

    CHECK(sliced.to_string() == "bcdefg");
    CHECK(sliced.size() == 6);
    CHECK(sliced.segment_count() == 3);
    CHECK(sliced.begin() == sequence.begin() + 1);
    CHECK(sliced.end() == sequence.end());
}

TEST_CASE("segmented_byte_view: slice(size_t, size_t) rejects invalid ranges") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(4, 0)), std::out_of_range);
    CHECK_THROWS_AS(static_cast<void>(sequence.slice(2, 2)), std::out_of_range);
}

TEST_CASE("segmented_byte_view: slice(size_t, position)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        const xtd::position end = sequence.begin() + test.offset + test.size;
        check_slice_result(sequence.slice(test.offset, end), test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice(size_t, position) rejects invalid positions") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(0, beyond_end)), std::out_of_range);
}

TEST_CASE("segmented_byte_view: slice(size_t, position) rejects foreign positions") {
    const auto first = xtd::test_helper_segmented_byte_view::create_from_segments({"abc"}, 11);
    const auto second = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"}, 22);

    CHECK_THROWS_AS(static_cast<void>(second.slice(0, first.end())), std::invalid_argument);
}

TEST_CASE("segmented_byte_view: slice(position, position)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
    );

    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        const xtd::position begin = sequence.begin() + test.offset;
        const xtd::position end = begin + test.size;
        check_slice_result(sequence.slice(begin, end), test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice(position, position) rejects invalid positions") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(
        static_cast<void>(sequence.slice(beyond_end, beyond_end)),
        std::out_of_range
    );
}

TEST_CASE("segmented_byte_view: slice(position, position) rejects foreign positions") {
    const auto first = xtd::test_helper_segmented_byte_view::create_from_segments({"abc"}, 11);
    const auto second = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"}, 22);

    CHECK_THROWS_AS(
        static_cast<void>(second.slice(first.begin(), second.end())),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        static_cast<void>(second.slice(second.begin(), first.end())),
        std::invalid_argument
    );
}

TEST_CASE("segmented_byte_view: slice(position)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
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
        check_slice_result(sequence.slice(end), test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice(position) rejects invalid positions") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"});
    const xtd::position beyond_end = sequence.end() + 1;

    CHECK_THROWS_AS(static_cast<void>(sequence.slice(beyond_end)), std::out_of_range);
}

TEST_CASE("segmented_byte_view: slice(position) rejects foreign positions") {
    const auto first = xtd::test_helper_segmented_byte_view::create_from_segments({"abc"}, 11);
    const auto second = xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"}, 22);

    CHECK_THROWS_AS(static_cast<void>(second.slice(first.end())), std::invalid_argument);
}

TEST_CASE("segmented_byte_view: position_of(char)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: position_of(byte)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

        const xtd::position result = sequence.position_of(test.value);
        CHECK(result);
        CHECK(result == sequence.begin() + test.offset);
    }

    CHECK_FALSE(sequence.position_of(std::byte{0xFF}));

    const auto empty = xtd::test_helper_segmented_byte_view::create_from_segments({""});
    CHECK_FALSE(empty.position_of(std::byte{'x'}));
}

TEST_CASE("segmented_byte_view: copy_to(byte*, size_t)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: copy_to(vector<byte>&)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: copy_to(T&) copies a scalar") {
    const std::uint32_t source = 0x12345678;
    const auto raw = std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(&source),
        sizeof(source)
    };

    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {raw.first<2>(), raw.last<2>()},
        7
    );

    std::uint32_t destination = 0;
    CHECK(sequence.copy_to(destination));
    CHECK(destination == source);
}

TEST_CASE("segmented_byte_view: copy_to(T&) copies a trivially copyable object") {
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

    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {
            bytes.first(first_size),
            bytes.subspan(first_size, second_size),
            bytes.subspan(first_size + second_size)
        },
        8
    );

    test_data_trivially_copyable destination{};

    REQUIRE(sequence.copy_to(destination));
    CHECK(destination.id == source.id);
    CHECK(destination.nested.code == source.nested.code);
    CHECK(destination.nested.amount == source.nested.amount);
    CHECK(destination.value.integer == source.value.integer);
}

TEST_CASE("segmented_byte_view: operator[](from_end)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"hel", "lo"}
    );

    CHECK(sequence[xtd::from_end{1}] == std::byte{'o'});
    CHECK(sequence[xtd::from_end{5}] == std::byte{'h'});
    CHECK_THROWS_AS(sequence[xtd::from_end{0}], std::out_of_range);
    CHECK_THROWS_AS(sequence[xtd::from_end{6}], std::out_of_range);
}

TEST_CASE("segmented_byte_view: slice_in_place(size_t, size_t)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"},
            73
        );

        sequence.slice_in_place(test.offset, test.size);
        check_slice_result(sequence, test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice_in_place(size_t, position)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"},
            73
        );
        const xtd::position end = sequence.begin() + test.offset + test.size;

        sequence.slice_in_place(test.offset, end);
        check_slice_result(sequence, test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice_in_place(position, position)") {
    for (const auto& test : slice_cases) {
        CAPTURE(test.offset);
        CAPTURE(test.size);
        CAPTURE(test.expected);

        auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"},
            73
        );
        const xtd::position begin = sequence.begin() + test.offset;
        const xtd::position end = begin + test.size;

        sequence.slice_in_place(begin, end);
        check_slice_result(sequence, test, 73);
    }
}

TEST_CASE("segmented_byte_view: slice_in_place(position)") {
    const std::array cases = {
        slice_case{0, 0, ""},
        slice_case{0, 3, "abc"},
        slice_case{0, 9, "abcdefghi"}
    };

    for (const auto& test : cases) {
        CAPTURE(test.size);
        CAPTURE(test.expected);

        auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
            {"abc", "def", "ghi"},
            73
        );
        const xtd::position end = sequence.begin() + test.size;

        sequence.slice_in_place(end);
        check_slice_result(sequence, test, 73);
    }
}

TEST_CASE("segmented_byte_view: begin and end reflect slice boundaries") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
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

TEST_CASE("segmented_byte_view: slices reject reversed ranges") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: nested slices reject positions outside their boundaries") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: operator[](size_t)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"}
    );

    constexpr std::string_view expected = "abcdef";

    for (std::size_t index = 0; index < expected.size(); ++index) {
        CAPTURE(index);
        CHECK(sequence[index] == static_cast<std::byte>(expected[index]));
    }

    CHECK_THROWS_AS(sequence[expected.size()], std::out_of_range);

    const auto empty =
        xtd::test_helper_segmented_byte_view::create_from_segments({});

    CHECK_THROWS_AS(empty[0], std::out_of_range);
}

TEST_CASE("segmented_byte_view: operator[](size_t) works on sliced views") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: operator[](position)") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"ab", "cde", "f"},
        41
    );

    CHECK(sequence[sequence.begin()] == std::byte{'a'});
    CHECK(sequence[sequence.begin() + 2] == std::byte{'c'});
    CHECK(sequence[sequence.end() - 1] == std::byte{'f'});

    CHECK_THROWS_AS(sequence[sequence.end()], std::out_of_range);

    const auto foreign =
        xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"}, 42);

    CHECK_THROWS_AS(sequence[foreign.begin()], std::invalid_argument);
}

TEST_CASE("segmented_byte_view: operator[](position) respects sliced boundaries") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: operator[](from_end) works on sliced views") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"}
    );

    const auto sliced = sequence.slice(2, 5);

    CHECK(sliced[xtd::from_end{1}] == std::byte{'g'});
    CHECK(sliced[xtd::from_end{3}] == std::byte{'e'});
    CHECK(sliced[xtd::from_end{5}] == std::byte{'c'});

    CHECK_THROWS_AS(sliced[xtd::from_end{6}], std::out_of_range);
}

TEST_CASE("segmented_byte_view: position_of only searches inside a sliced view") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"x", "abc", "xdefx"},
        51
    );

    const auto sliced = sequence.slice(1, 7);
    CHECK(sliced.to_string() == "abcxdef");

    const auto found = sliced.position_of('x');

    REQUIRE(found);
    CHECK(found == sequence.begin() + 4);
    CHECK_FALSE(sliced.position_of('z'));

    const auto without_x = sequence.slice(1, 3);
    CHECK(without_x.to_string() == "abc");
    CHECK_FALSE(without_x.position_of('x'));
}

TEST_CASE("segmented_byte_view: copy_to(byte*, size_t) rejects a null destination") {
    const auto sequence =
        xtd::test_helper_segmented_byte_view::create_from_segments({"abc"});

    CHECK_THROWS_AS(
        static_cast<void>(sequence.copy_to(nullptr, 1)),
        std::invalid_argument
    );
}

TEST_CASE("segmented_byte_view: copy_to(T&) rejects an undersized view") {
    const auto sequence =
        xtd::test_helper_segmented_byte_view::create_from_segments({"abc"});

    std::uint32_t destination = 0xAABBCCDD;

    CHECK_THROWS_AS(
        static_cast<void>(sequence.copy_to(destination)),
        std::invalid_argument
    );

    CHECK(destination == 0xAABBCCDD);
}

TEST_CASE("segmented_byte_view: copy_to copies a sliced view") {
    const auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
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

TEST_CASE("segmented_byte_view: repeated slice_in_place uses relative offsets") {
    auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def", "ghi"},
        73
    );

    sequence.slice_in_place(2, 5);

    CHECK(sequence.to_string() == "cdefg");
    CHECK(sequence.begin() == xtd::position{2, 73});

    sequence.slice_in_place(1, 3);

    CHECK(sequence.to_string() == "def");
    CHECK(sequence.size() == 3);
    CHECK(sequence.begin() == xtd::position{3, 73});
    CHECK(sequence.end() == xtd::position{6, 73});
}

TEST_CASE("segmented_byte_view: slice_in_place failures leave the view unchanged") {
    auto sequence = xtd::test_helper_segmented_byte_view::create_from_segments(
        {"abc", "def"},
        61
    );

    const auto foreign =
        xtd::test_helper_segmented_byte_view::create_from_segments({"xyz"}, 62);

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

    CHECK_THROWS_AS(
        sequence.slice_in_place(foreign.end()),
        std::invalid_argument
    );

    CHECK_THROWS_AS(
        sequence.slice_in_place(0, foreign.end()),
        std::invalid_argument
    );

    CHECK(sequence.begin() == original_begin);
    CHECK(sequence.end() == original_end);
    CHECK(sequence.size() == original_size);
    CHECK(sequence.segment_count() == original_segment_count);
    CHECK(sequence.to_string() == original_text);
}

#endif