#ifndef XTD_TESTS_POSITION_TESTS_H
#define XTD_TESTS_POSITION_TESTS_H

#include "../third_party/doctest.h"
#include "../../src/pipeline/position.h"

namespace xtd_position_tests {

TEST_CASE("position: default constructed value is invalid")
{
    const xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE("position: public constructor creates valid value")
{
    const xtd::position pos{42, 7};
    CHECK(static_cast<bool>(pos));
    CHECK(pos.sequence_offset() == 42);
}

TEST_CASE("position: arithmetic on valid value preserves sequence and updates offset")
{
    const xtd::position base{10, 99};

    const xtd::position plus = base + 5;
    CHECK(static_cast<bool>(plus));
    CHECK(plus.sequence_offset() == 15);

    const xtd::position minus = plus - 3;
    CHECK(static_cast<bool>(minus));
    CHECK(minus.sequence_offset() == 12);
}

TEST_CASE("position: arithmetic on invalid position remains invalid")
{
    xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos + 1));
    pos += 3;
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE("position: increment and decrement operators advance offset")
{
    xtd::position pos{5, 11};

    const xtd::position beforePostInc = pos++;
    CHECK(beforePostInc.sequence_offset() == 5);
    CHECK(pos.sequence_offset() == 6);

    const xtd::position beforePostDec = pos--;
    CHECK(beforePostDec.sequence_offset() == 6);
    CHECK(pos.sequence_offset() == 5);

    ++pos;
    CHECK(pos.sequence_offset() == 6);

    --pos;
    CHECK(pos.sequence_offset() == 5);
}

TEST_CASE("position: equality for invalid positions")
{
    const xtd::position lhs{};
    const xtd::position rhs{};
    CHECK(lhs == rhs);
    CHECK_FALSE(lhs != rhs);
}

TEST_CASE("position: equality requires same sequence and offset")
{
    const xtd::position a{8, 2};
    const xtd::position b{8, 2};
    const xtd::position differentOffset{9, 2};
    const xtd::position differentSequence{8, 3};

    CHECK(a == b);
    CHECK_FALSE(a != b);

    CHECK_FALSE(a == differentOffset);
    CHECK(a != differentOffset);

    CHECK_FALSE(a == differentSequence);
    CHECK(a != differentSequence);
}

TEST_CASE("position: greater-than compares only compatible valid positions")
{
    const xtd::position low{3, 4};
    const xtd::position high{7, 4};
    const xtd::position otherSequence{8, 5};
    const xtd::position invalid{};

    CHECK(high > low);
    CHECK_FALSE(low > high);
    CHECK_FALSE(high > otherSequence);
    CHECK_FALSE(high > invalid);
    CHECK_FALSE(invalid > low);
}
} // namespace xtd_position_tests

#endif
