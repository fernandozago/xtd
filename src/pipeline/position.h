#ifndef PIPELINE_POSITION_H
#define PIPELINE_POSITION_H

#include <cstddef>
#include <cstdint>

namespace xtd
{

struct position
{
friend struct segmented_byte_view;
friend class pipeline;

private:
    std::size_t m_segment_begin_offset = 0;
    std::size_t m_segment_offset = 0;
    std::uint64_t m_sequence_id = 0;
    
    std::size_t offset_in_sequence() const noexcept {
        return m_segment_begin_offset + m_segment_offset;
    }
    
    position(const std::size_t segmentBeginOffset, const std::size_t segmentOffset, const std::uint64_t sequenceId)
        : m_segment_begin_offset(segmentBeginOffset)
        , m_segment_offset(segmentOffset)
        , m_sequence_id(sequenceId)
    {}   

    bool operator>(const position& rhs) const noexcept {
        return offset_in_sequence() > rhs.offset_in_sequence();
    }
  
public:
    position() 
        : m_segment_begin_offset(0)
        , m_segment_offset(0)
        , m_sequence_id(0)
    {}
    
    // Returns a new position advanced by the given offset.
    position operator+(const std::size_t offset) const noexcept {
        return {m_segment_begin_offset, m_segment_offset + offset, m_sequence_id};
    }

    // Advances this position by the given offset.
    position& operator+=(const std::size_t offset) noexcept {
        m_segment_offset += offset;
        return *this;
    }

    // Advances this position by one byte.
    position& operator++() noexcept {
        ++m_segment_offset;
        return *this;
    }

    // Advances this position by one byte, returning the previous value.
    position operator++(int) noexcept {
        position old = *this;
        ++m_segment_offset;
        return old;
    }

    // Compares two positions for equality.
    bool operator==(const position& rhs) const noexcept {
        return m_sequence_id == rhs.m_sequence_id &&
            offset_in_sequence() == rhs.offset_in_sequence();
    }
};

} // namespace xtd

#endif // PIPELINE_POSITION_H
