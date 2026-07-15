#ifndef PIPELINE_DATASEGMENTPOOL_H
#define PIPELINE_DATASEGMENTPOOL_H

#include <cstddef>
#include <utility>
#include <vector>

#include "data_segment.h"

namespace xtd
{

struct data_segment_pool
{
friend class pipeline;

private:
    const std::size_t m_buffer_size;
    const std::size_t m_max_pooled_segments;
    std::vector<data_segment> m_segments;

    [[nodiscard]]
    static std::size_t calculate_max_pooled_segments(const std::size_t buffer_size, const std::size_t pause_writer_threshold) noexcept
    {
        assert(buffer_size > 0);

        const std::size_t segments_for_threshold =
            pause_writer_threshold / buffer_size
            + (pause_writer_threshold % buffer_size != 0);

        return segments_for_threshold + 1; // one spare segment
    }

    data_segment_pool(std::size_t buffer_size, std::size_t pause_writer_threshold) noexcept
        : m_buffer_size(buffer_size)
        , m_max_pooled_segments(
            calculate_max_pooled_segments(
                buffer_size,
                pause_writer_threshold))
    {
    }

    [[nodiscard]]
    data_segment rent_segment()
    {
        if (m_segments.empty()) {
            return data_segment(m_buffer_size);
        }

        data_segment segment(std::move(m_segments.back()));
        m_segments.pop_back();

        segment.reset();
        return segment;
    }

    void return_segment(data_segment&& segment)
    {
        if (m_segments.size() >= m_max_pooled_segments) {
            return;
        }

        m_segments.push_back(std::move(segment));
    }
};

} // namespace xtd

#endif // PIPELINE_DATASEGMENTPOOL_H