#ifndef XTD_TESTS_POSITION_TESTS_H
#define XTD_TESTS_POSITION_TESTS_H
#include "../third_party/catch2/catch_amalgamated.hpp"

#include "pipeline/position.h"

struct PositionTests {};

TEST_CASE_METHOD(xtd::position, "default constructed value is invalid")
{
    const xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE_METHOD(PositionTests, "public constructor creates valid value")
{
    const xtd::position pos{42};
    CHECK(static_cast<bool>(pos));
    CHECK(pos.sequence_offset() == 42);
}

TEST_CASE_METHOD(PositionTests, "arithmetic on valid value preserves sequence and updates offset")
{
    const xtd::position base{10};

    const xtd::position plus = base + 5;
    CHECK(static_cast<bool>(plus));
    CHECK(plus.sequence_offset() == 15);

    const xtd::position minus = plus - 3;
    CHECK(static_cast<bool>(minus));
    CHECK(minus.sequence_offset() == 12);
}

TEST_CASE_METHOD(PositionTests, "arithmetic on invalid position remains invalid")
{
    xtd::position pos{};
    CHECK_FALSE(static_cast<bool>(pos + 1));
    pos += 3;
    CHECK_FALSE(static_cast<bool>(pos));
}

TEST_CASE_METHOD(PositionTests, "increment and decrement operators advance offset")
{
    xtd::position pos{5};

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

TEST_CASE_METHOD(PositionTests, "equality for invalid positions")
{
    const xtd::position lhs{};
    const xtd::position rhs{};
    CHECK(lhs == rhs);
    CHECK_FALSE(lhs != rhs);
}

TEST_CASE_METHOD(PositionTests, "greater-than compares only compatible valid positions")
{
    const xtd::position low{3};
    const xtd::position high{7};
    const xtd::position otherSequence{8};
    const xtd::position invalid{};

    CHECK(high > low);
    CHECK_FALSE(low > high);
    CHECK_FALSE(high > otherSequence);
    CHECK_FALSE(high > invalid);
    CHECK_FALSE(invalid > low);
}

TEST_CASE_METHOD(PositionTests, "copy construction and assignment preserve value")
{
    const xtd::position original{42};

    const xtd::position copied{original};
    CHECK(copied == original);
    CHECK(copied.sequence_offset() == 42);

    xtd::position assigned{};
    assigned = original;

    CHECK(assigned == original);
    CHECK(assigned.sequence_offset() == 42);
}

TEST_CASE_METHOD(PositionTests, "compound arithmetic updates valid position")
{
    xtd::position pos{10};
    const xtd::position sameSequence{12};

    pos += 5;
    CHECK(pos.sequence_offset() == 15);

    pos -= 3;
    CHECK(pos.sequence_offset() == 12);

    // Also verifies that the sequence ID is preserved.
    CHECK(pos == sameSequence);
}

TEST_CASE_METHOD(PositionTests, "subtraction on invalid position remains invalid")
{
    const xtd::position invalid{};

    const xtd::position result = invalid - 5;

    CHECK_FALSE(static_cast<bool>(result));
    CHECK(result.sequence_offset() == 0);
}

TEST_CASE_METHOD(PositionTests, "compound subtraction on invalid position has no effect")
{
    xtd::position invalid{};

    invalid -= 5;

    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(invalid.sequence_offset() == 0);
}

TEST_CASE_METHOD(PositionTests, "prefix increment and decrement ignore invalid position")
{
    xtd::position invalid{};

    const xtd::position& incrementResult = ++invalid;

    CHECK(&incrementResult == &invalid);
    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(invalid.sequence_offset() == 0);

    const xtd::position& decrementResult = --invalid;

    CHECK(&decrementResult == &invalid);
    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(invalid.sequence_offset() == 0);
}

TEST_CASE_METHOD(PositionTests, "postfix increment and decrement preserve invalidity")
{
    xtd::position invalid{};

    const xtd::position beforeIncrement = invalid++;

    CHECK_FALSE(static_cast<bool>(beforeIncrement));
    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(invalid.sequence_offset() == 0);

    const xtd::position beforeDecrement = invalid--;

    CHECK_FALSE(static_cast<bool>(beforeDecrement));
    CHECK_FALSE(static_cast<bool>(invalid));
    CHECK(invalid.sequence_offset() == 0);
}

TEST_CASE_METHOD(PositionTests, "equality handles one valid and one invalid position")
{
    const xtd::position valid{10};
    const xtd::position invalid{};

    CHECK_FALSE(valid == invalid);
    CHECK_FALSE(invalid == valid);

    CHECK(valid != invalid);
    CHECK(invalid != valid);
}

#endif
