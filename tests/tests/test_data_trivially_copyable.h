#pragma once
#include "../third_party/doctest.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>
#include <print>
#include <vector>
#include <future>
#include <istream>
#include "pipeline/pipeline.h"

struct test_data_trivially_copyable
{
    enum class status : std::uint8_t
    {
        sure,
        that,
        it,
        can, 
        have, 
        multiple,
        and_any,
        values
    };

    struct nested_data
    {
        std::int32_t value = 0;
        double ratio = 0.0;

        void member_function() {}
    };

    union trivial_union
    {
        std::uint64_t integer;
        double floating;

        constexpr trivial_union() : integer(0) {}
    };

    // Boolean and character types
    bool booleanValue = false;
    char charValue = '\0';
    signed char signedCharValue = 0;
    unsigned char unsignedCharValue = 0;
    wchar_t wideCharValue = L'\0';
    char8_t utf8CharValue = u8'\0';
    char16_t utf16CharValue = u'\0';
    char32_t utf32CharValue = U'\0';
    std::byte byteValue{};

    // Integer types
    short shortValue = 0;
    unsigned short unsignedShortValue = 0;

    int intValue = 0;
    unsigned int unsignedIntValue = 0;

    long longValue = 0;
    unsigned long unsignedLongValue = 0;

    long long longLongValue = 0;
    unsigned long long unsignedLongLongValue = 0;

    // Fixed-width integers
    std::int8_t int8Value = 0;
    std::uint8_t uint8Value = 0;
    std::int16_t int16Value = 0;
    std::uint16_t uint16Value = 0;
    std::int32_t int32Value = 0;
    std::uint32_t uint32Value = 0;
    std::int64_t int64Value = 0;
    std::uint64_t uint64Value = 0;

    // Floating-point types
    float floatValue = 0.0F;
    double doubleValue = 0.0;
    long double longDoubleValue = 0.0L;

    // Enumeration
    status statusValue = status::and_any;

    // Nested trivially copyable object
    nested_data nested{};

    // Arrays
    int rawArray[8]{};
    std::array<std::uint64_t, 8> standardArray{};

    // Union
    trivial_union unionValue{};

    // Arbitrary raw payload
    std::array<std::byte, 512> rawData{};

    [[nodiscard]]
    bool operator==(const test_data_trivially_copyable& other) const noexcept
    {
        return std::memcmp(this, &other, sizeof(*this)) == 0;
    }

    [[nodiscard]]
    bool operator!=(const test_data_trivially_copyable& other) const noexcept
    {
        return !(*this == other);
    }

    void print() const;
    static void test(std::istream& source);
};

static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable>);
static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::nested_data>);
static_assert(std::is_trivially_copyable_v<test_data_trivially_copyable::trivial_union>);

inline void test_data_trivially_copyable::test(std::istream& source)
{
    constexpr std::size_t expected_count = 1'000;
    std::vector<test_data_trivially_copyable> expected_values(expected_count);

    xtd::pipeline pipeline;
    std::future<std::size_t> producer = std::async(
        std::launch::async,
        [&pipeline, &source, &expected_values] {           
            std::size_t written_count = 0;
            test_data_trivially_copyable mydata;
            xtd::pipe_writer& writer = pipeline.writer();
            while (written_count < expected_count)
            {
                // Get bytes from source into the struct
                source.read(reinterpret_cast<char*>(&mydata), static_cast<std::streamsize>(sizeof(mydata)));

                // If uncomplete read, clear the error state and try again
                if (sizeof(mydata) > static_cast<std::size_t>(source.gcount())) {
                    source.clear();
                    continue;
                }

                // Ensure that the booleanValue is consistent with the unsignedIntValue for testing purposes
                // This is important because the random data from /dev/urandom may not have a valid booleanValue
                // So we set it based on the unsignedIntValue.
                mydata.booleanValue = mydata.unsignedIntValue % 2 == 0;
                
                // Ensure that the statusValue is within the valid range of the enum
                mydata.statusValue = static_cast<test_data_trivially_copyable::status>(mydata.unsignedIntValue % 3);

                // Writes a copy of the struct to the expected_values vector for later verification
                expected_values[written_count] = mydata;
                
                // Serialize the struct to the pipeline writer
                REQUIRE(writer.write(mydata) == sizeof(mydata));
                ++written_count;
            }

            writer.complete();
            return written_count;
        });

        
    std::size_t received_count = 0;
    test_data_trivially_copyable mydata;
    xtd::pipe_reader& reader = pipeline.reader();
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view buffer = result.buffer();

        // While there is enough data in the buffer to deserialize a complete struct
        // Copy it to mydata and verify it against the expected values
        while (buffer.size() >= sizeof(mydata))
        {
            REQUIRE(received_count < expected_values.size());
            REQUIRE(buffer.copy_to(mydata));
            CHECK(mydata == expected_values[received_count]);
            ++received_count;
            //mydata.print();

            buffer = buffer.slice(sizeof(mydata), buffer.end());
        }

        reader.advance(buffer);

        if (result.completed()) {
            break;
        }
    }

    const std::size_t written_count = producer.get();
    reader.complete();

    CHECK(written_count == expected_count);
    CHECK(received_count == expected_count);
}

inline void test_data_trivially_copyable::print() const
{
    std::println(
        "test_data_trivially_copyable "
        "[sizeof = {} bytes] "
        "[booleanValue = {}] "
        "[charValue = {}] "
        "[signedCharValue = {}] "
        "[unsignedCharValue = {}] "
        "[wideCharValue = {}] "
        "[utf8CharValue = {}] "
        "[utf16CharValue = {}] "
        "[utf32CharValue = {}] "
        "[byteValue = {:#04x}] "
        "[shortValue = {}] "
        "[unsignedShortValue = {}] "
        "[intValue = {}] "
        "[unsignedIntValue = {}] "
        "[longValue = {}] "
        "[unsignedLongValue = {}] "
        "[longLongValue = {}] "
        "[unsignedLongLongValue = {}] "
        "[int8Value = {}] "
        "[uint8Value = {}] "
        "[int16Value = {}] "
        "[uint16Value = {}] "
        "[int32Value = {}] "
        "[uint32Value = {}] "
        "[int64Value = {}] "
        "[uint64Value = {}] "
        "[floatValue = {}] "
        "[doubleValue = {}] "
        "[longDoubleValue = {}] "
        "[statusValue = {}] "
        "[nested.value = {}] "
        "[nested.ratio = {}] "
        "[unionValue.integer = {}] "
        "[rawArray = {} elements, {} bytes] "
        "[standardArray = {} elements, {} bytes] "
        "[rawData = {} bytes]",
        sizeof(*this),

        booleanValue,
        static_cast<int>(charValue),
        static_cast<int>(signedCharValue),
        static_cast<unsigned int>(unsignedCharValue),
        static_cast<std::uint32_t>(wideCharValue),
        static_cast<std::uint32_t>(utf8CharValue),
        static_cast<std::uint32_t>(utf16CharValue),
        static_cast<std::uint32_t>(utf32CharValue),
        std::to_integer<unsigned int>(byteValue),

        shortValue,
        unsignedShortValue,
        intValue,
        unsignedIntValue,
        longValue,
        unsignedLongValue,
        longLongValue,
        unsignedLongLongValue,

        static_cast<int>(int8Value),
        static_cast<unsigned int>(uint8Value),
        int16Value,
        uint16Value,
        int32Value,
        uint32Value,
        int64Value,
        uint64Value,

        floatValue,
        doubleValue,
        longDoubleValue,

        static_cast<std::uint32_t>(statusValue),
        nested.value,
        nested.ratio,
        unionValue.integer,

        std::size(rawArray),
        sizeof(rawArray),
        standardArray.size(),
        sizeof(standardArray),
        rawData.size()
    );
}