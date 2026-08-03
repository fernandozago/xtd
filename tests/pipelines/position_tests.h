#ifndef XTD_TESTS_POSITION_TESTS_H
#define XTD_TESTS_POSITION_TESTS_H

#include "../third_party/catch2/catch_amalgamated.hpp"

#include "pipeline/position.h"

struct PositionTests {};

TEST_CASE_METHOD(PositionTests, "default constructor creates position at zero")
{
    const xtd::position pos{};

    CHECK(pos.sequence_offset() == 0);
}

TEST_CASE_METHOD(PositionTests, "public constructor creates position with given offset")
{
    const xtd::position pos{42};

    CHECK(pos.sequence_offset() == 42);
}

TEST_CASE_METHOD(PositionTests, "arithmetic returns updated position without modifying original")
{
    const xtd::position base{10};

    const xtd::position plus = base + 5;
    const xtd::position minus = base - 3;

    CHECK(base.sequence_offset() == 10);
    CHECK(plus.sequence_offset() == 15);
    CHECK(minus.sequence_offset() == 7);
}

TEST_CASE_METHOD(PositionTests, "compound arithmetic updates position and returns same instance")
{
    xtd::position pos{10};

    xtd::position& additionResult = (pos += 5);

    CHECK(&additionResult == &pos);
    CHECK(pos.sequence_offset() == 15);

    xtd::position& subtractionResult = (pos -= 3);

    CHECK(&subtractionResult == &pos);
    CHECK(pos.sequence_offset() == 12);
}

TEST_CASE_METHOD(PositionTests, "prefix increment and decrement update position and return same instance")
{
    xtd::position pos{5};

    xtd::position& incrementResult = ++pos;

    CHECK(&incrementResult == &pos);
    CHECK(pos.sequence_offset() == 6);

    xtd::position& decrementResult = --pos;

    CHECK(&decrementResult == &pos);
    CHECK(pos.sequence_offset() == 5);
}

TEST_CASE_METHOD(PositionTests, "postfix increment and decrement return previous position")
{
    xtd::position pos{5};

    const xtd::position beforeIncrement = pos++;

    CHECK(beforeIncrement.sequence_offset() == 5);
    CHECK(pos.sequence_offset() == 6);

    const xtd::position beforeDecrement = pos--;

    CHECK(beforeDecrement.sequence_offset() == 6);
    CHECK(pos.sequence_offset() == 5);
}

TEST_CASE_METHOD(PositionTests, "equality compares position offsets")
{
    const xtd::position lhs{10};
    const xtd::position equal{10};
    const xtd::position different{11};

    CHECK(lhs == equal);
    CHECK_FALSE(lhs != equal);

    CHECK_FALSE(lhs == different);
    CHECK(lhs != different);
}

TEST_CASE_METHOD(PositionTests, "ordering compares position offsets")
{
    const xtd::position low{3};
    const xtd::position equal{3};
    const xtd::position high{7};

    CHECK(low < high);
    CHECK(low <= high);
    CHECK(high > low);
    CHECK(high >= low);

    CHECK(low <= equal);
    CHECK(low >= equal);

    CHECK_FALSE(low > high);
    CHECK_FALSE(high < low);
    CHECK_FALSE(low < equal);
    CHECK_FALSE(low > equal);
}

TEST_CASE_METHOD(PositionTests, "copy construction and assignment preserve position offset")
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

TEST_CASE_METHOD(PositionTests, "test found/notfound") {
    
    const xtd::position found{42};
    const xtd::position notfound{};

    CHECK(found);
    CHECK(static_cast<bool>(found));
    CHECK_FALSE(notfound);
    CHECK_FALSE(static_cast<bool>(notfound));
}

#endif // XTD_TESTS_POSITION_TESTS_H