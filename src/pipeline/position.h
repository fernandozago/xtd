#ifndef PIPELINE_POSITION_H
#define PIPELINE_POSITION_H

#include <cstddef>
#include <cstdint>

namespace xtd
{

struct segmented_byte_view;
class pipeline;

struct position
{
    friend struct segmented_byte_view;
    friend class pipeline;

private:
    std::size_t m_offset = 0;
    std::uint64_t m_sequence_id = 0;

    constexpr position(std::size_t offset, std::uint64_t sequence_id) noexcept
        : m_offset(offset)
        , m_sequence_id(sequence_id)
    {
    }

    [[nodiscard]]
    constexpr std::size_t offset_in_sequence() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]]
    constexpr bool belongs_to(std::uint64_t sequence_id) const noexcept
    {
        return m_sequence_id == sequence_id;
    }

    [[nodiscard]]
    constexpr bool operator>(const position& rhs) const noexcept
    {
        return m_sequence_id == rhs.m_sequence_id &&
               m_offset > rhs.m_offset;
    }

public:
    constexpr position() noexcept = default;

    // Returns a new position advanced by the given offset.
    [[nodiscard]]
    constexpr position operator+(std::size_t offset) const noexcept
    {
        return position{
            m_offset + offset,
            m_sequence_id
        };
    }

    // Advances this position by the given offset.
    constexpr position& operator+=(std::size_t offset) noexcept
    {
        m_offset += offset;
        return *this;
    }

    // Advances this position by one byte.
    constexpr position& operator++() noexcept
    {
        ++m_offset;
        return *this;
    }

    // Advances this position by one byte and returns the previous value.
    constexpr position operator++(int) noexcept
    {
        position previous = *this;
        ++m_offset;
        return previous;
    }

    // Positions are equal only when they belong to the same sequence
    // and represent the same absolute offset.
    [[nodiscard]]
    constexpr bool operator==(const position& rhs) const noexcept
    {
        return m_sequence_id == rhs.m_sequence_id &&
               m_offset == rhs.m_offset;
    }

    [[nodiscard]]
    constexpr bool operator!=(const position& rhs) const noexcept
    {
        return !(*this == rhs);
    }
};

} // namespace xtd

#endif // PIPELINE_POSITION_H