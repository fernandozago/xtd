#ifndef PIPELINE_DATASEGMENTPOOL_H
#define PIPELINE_DATASEGMENTPOOL_H

#include <algorithm>
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

    data_segment_pool(std::size_t buffer_size, std::size_t pause_writer_threshold)
        : m_buffer_size(buffer_size)
        , m_max_pooled_segments(std::max<std::size_t>(1, pause_writer_threshold / buffer_size + 2))
    {
        m_segments.reserve(m_max_pooled_segments);
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