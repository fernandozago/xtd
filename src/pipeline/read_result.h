#ifndef PIPELINE_READ_RESULT_H
#define PIPELINE_READ_RESULT_H

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

#include "segmented_byte_view.h"
#include "pipeline/data_segment.h"

namespace xtd
{

struct read_result
{
private:
    friend class pipeline;

    segmented_byte_view m_buffer;
    bool m_completed;
    bool m_cancelled;

    static std::vector<std::span<const std::byte>> make_readable_segments(const std::deque<data_segment>& segments)
    {
        std::vector<std::span<const std::byte>> result;
        result.reserve(segments.size());

        for (const data_segment& segment : segments) {
            if (segment.readable_size() != 0) {
                result.emplace_back(segment.readable_bytes());
            }
        }

        return result;
    }

    explicit read_result(
        const std::deque<data_segment>& segments,
        std::uint64_t sequence_id,
        bool completed)
        : m_buffer(make_readable_segments(segments), sequence_id)
        , m_completed(completed)
        , m_cancelled(false)
    {
    }

    explicit read_result(bool cancelled)
        : m_buffer{}
        , m_completed(false)
        , m_cancelled(cancelled)
    {
    }

public:
    read_result() = delete;

    read_result(const read_result&) = delete;
    read_result& operator=(const read_result&) = delete;

    read_result(read_result&&) noexcept = default;
    read_result& operator=(read_result&&) noexcept = default;

    explicit constexpr operator bool() const noexcept
    {
        return !m_cancelled;
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
};

} // namespace xtd

#endif // PIPELINE_READ_RESULT_H