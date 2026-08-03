#ifndef PIPELINE_POSITION_H
#define PIPELINE_POSITION_H

#include <cstddef>

namespace xtd
{
    struct position
    {
    public:

        explicit position(const std::size_t offset) noexcept
            : m_offset(offset)
            , m_valid(true)
        {}

        explicit position() 
            : m_offset(0)
            , m_valid(false)
        {}

        position(const position&) noexcept = default;
        position& operator=(const position&) noexcept = default;

        explicit operator bool() const noexcept
        {
            return m_valid;
        }

        [[nodiscard]]
        std::size_t sequence_offset() const noexcept
        {
            return m_offset;
        }

        [[nodiscard]]
        position operator+(const std::size_t offset) const noexcept
        {
            if (!m_valid) return position{};
            return position{m_offset + offset};
        }

        [[nodiscard]]
        position operator-(const std::size_t offset) const noexcept
        {
            if (!m_valid) return position{};
            return position{m_offset - offset};
        }

        // Advances this position by the given offset.
        position& operator+=(const std::size_t offset) noexcept
        {
            if (m_valid) {
                m_offset += offset;
            }

            return *this;
        }

        position& operator-=(const std::size_t offset) noexcept
        {
            if (m_valid) {
                m_offset -= offset;
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

        position& operator--() noexcept
        {
            if (m_valid) { 
                --m_offset; 
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

        position operator--(int) noexcept
        {
            position previous = *this;
            --(*this);
            return previous;
        }

        [[nodiscard]]
        bool operator==(const position& rhs) const noexcept
        {
            if (m_valid == rhs.m_valid) {
                return !m_valid || m_offset == rhs.m_offset;
            }

            return false;
        }

        [[nodiscard]]
        bool operator!=(const position& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]]
        bool operator>(const position& rhs) const noexcept
        {
            return m_valid && rhs.m_valid 
                && m_offset > rhs.m_offset;
        }

        [[nodiscard]]
        bool operator<(const position& rhs) const noexcept
        {
            return m_valid && rhs.m_valid 
                && m_offset < rhs.m_offset;
        }

        [[nodiscard]]
        bool operator>=(const position& rhs) const noexcept
        {
            return m_valid && rhs.m_valid 
                && m_offset >= rhs.m_offset;
        }

        [[nodiscard]]
        bool operator<=(const position& rhs) const noexcept
        {
            return m_valid && rhs.m_valid && m_offset <= rhs.m_offset;
        }
    private:
        std::size_t m_offset = 0;
        bool m_valid = false;
    };

} // namespace xtd

#endif // PIPELINE_POSITION_H
