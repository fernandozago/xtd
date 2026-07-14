#ifndef PIPELINE_DATA_SEGMENT_H
#define PIPELINE_DATA_SEGMENT_H

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace xtd
{

struct data_segment
{
friend struct data_segment_pool;
friend class pipeline;

private:
    std::vector<std::byte> m_buffer;
    std::size_t m_start = 0;
    std::size_t m_end = 0;
    
    std::size_t readable_size() const noexcept {
        return m_end - m_start;
    }
    
    std::size_t writable_size() const noexcept {
        return m_buffer.size() - m_end;
    }

    bool full() const noexcept {
        return writable_size() == 0;
    }

    std::size_t copy_from(const std::byte* source, std::size_t size)
    {
        const std::size_t remaining = std::min(writable_size(), size);
        std::copy_n(source, remaining, m_buffer.begin() + m_end);
        m_end += remaining;
        return remaining;
    }
    
    void reset_for_write(std::size_t capacity)
    {
        if (m_buffer.size() != capacity) {
            throw std::logic_error("data_segment capacity mismatch");
        }
        
        m_start = 0;
        m_end = 0;
    }

public:
    data_segment(std::size_t capacity)
        : m_buffer(capacity)
    {}

    data_segment(const data_segment&) = delete;
    data_segment& operator=(const data_segment&) = delete;

    data_segment(data_segment&&) noexcept = default;
    data_segment& operator=(data_segment&&) noexcept = default;
};

} // namespace xtd

#endif // PIPELINE_DATA_SEGMENT_H
