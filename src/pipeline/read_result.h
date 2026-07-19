#ifndef PIPELINE_READ_RESULT_H
#define PIPELINE_READ_RESULT_H

#include <cstdint>
#include <deque>
#include <ranges>
#include <vector>

#include "segmented_byte_view.h"
#include "pipeline/data_segment.h"

namespace xtd
{

struct read_result
{
friend class pipeline;

public:
    read_result() = delete;

    explicit constexpr operator bool() const noexcept
    {
        return true; // Always true, as a read result is always valid.
    }

    [[nodiscard]]
    const segmented_byte_view& buffer() const noexcept
    {
        return m_buffer;
    }

    [[nodiscard]]
    bool completed() const noexcept
    {
        return m_completed;
    }

private:
    segmented_byte_view m_buffer;
    bool m_completed = false;

    static std::vector<std::span<const std::byte>>make_readable_segments(const std::deque<data_segment>& segments)
    {
        std::vector<std::span<const std::byte>> result;
        result.reserve(segments.size());

        for (const data_segment& segment : segments) {
            if (segment.readable_size() != 0) {
                result.push_back(segment.readable_bytes());
            }
        }

        return result;
    }

    // read_result is constructed by the pipeline to represent the root result of a read operation.
    explicit read_result(const std::deque<data_segment>& segments, std::uint64_t sequence_id, bool completed)
        : m_buffer(make_readable_segments(segments), sequence_id)
        , m_completed(completed)
    {
    }
};

} // namespace xtd

#endif // PIPELINE_READ_RESULT_H