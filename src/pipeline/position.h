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

public:
    explicit position() noexcept = default;
    explicit operator bool() const noexcept
    {
        return m_valid;
    }

    [[nodiscard]]
    position operator+(const std::size_t offset) const noexcept
    {
        if (!m_valid) {
            return position{};
        }

        return position{m_offset + offset, m_sequence_id};
    }

    // Advances this position by the given offset.
    position& operator+=(const std::size_t offset) noexcept
    {
        if (m_valid) {
            m_offset += offset;
        }

        return *this;
    }

    // Advances this position by one byte.
    position& operator++() noexcept
    {
        if (m_valid) { 
            ++m_offset; 
        }
        return *this;
    }

    // Advances this position by one byte and returns the previous value.
    position operator++(int) noexcept
    {
        position previous = *this;
        ++(*this);
        return previous;
    }

    [[nodiscard]]
    bool operator==(const position& rhs) const noexcept
    {
        if (!m_valid || !rhs.m_valid) {
            return m_valid == rhs.m_valid;
        }

        return m_sequence_id == rhs.m_sequence_id 
            && m_offset == rhs.m_offset;
    }

    [[nodiscard]]
    bool operator!=(
        const position& rhs) const noexcept
    {
        return !(*this == rhs);
    }

private:
    std::size_t m_offset = 0;
    std::uint64_t m_sequence_id = 0;
    bool m_valid = false;

    explicit position(const std::size_t offset, const std::uint64_t sequence_id) noexcept
        : m_offset(offset)
        , m_sequence_id(sequence_id)
        , m_valid(true)
    {
    }

    [[nodiscard]]
    std::size_t sequence_offset() const noexcept
    {
        return m_offset;
    }

    [[nodiscard]]
    bool operator>(const position& rhs) const noexcept
    {
        return m_valid &&
               rhs.m_valid &&
               m_sequence_id == rhs.m_sequence_id &&
               m_offset > rhs.m_offset;
    }
};

} // namespace xtd

#endif // PIPELINE_POSITION_H