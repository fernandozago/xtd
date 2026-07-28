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

    const segmented_byte_view m_buffer;
    const bool m_completed;
    const bool m_cancelled;

    static segmented_byte_view make_readable_view(const std::deque<data_segment>& segments, std::uint64_t sequence_id)
    {
        std::vector<std::span<const std::byte>> readable_segments;
        readable_segments.reserve(segments.size());

        std::size_t total_size = 0;

        for (const data_segment& segment : segments) {
            const std::size_t readable_size = segment.readable_size();

            if (readable_size != 0) {
                readable_segments.emplace_back(segment.readable_bytes());
                total_size += readable_size;
            }
        }

        return segmented_byte_view{
            std::move(readable_segments),
            sequence_id,
            total_size
        };
    }

    explicit read_result(const std::deque<data_segment>& segments, std::uint64_t sequence_id, bool completed)
        : m_buffer(make_readable_view(segments, sequence_id))
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
