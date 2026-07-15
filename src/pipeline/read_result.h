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

private:
    segmented_byte_view m_buffer;
    bool m_completed = false;

    // read_result is constructed by the pipeline to represent the root result of a read operation.
    read_result(const std::deque<data_segment>& segments, std::uint64_t sequence_id, bool completed)
        : m_buffer(segments
            | std::views::filter(
                [](const data_segment& segment) noexcept {
                    return segment.readable_size() > 0;
                })
            | std::views::transform(
                [](const data_segment& segment) noexcept {
                    return segment.readable_bytes();
                })
            | std::ranges::to<std::vector>(), sequence_id)
        , m_completed(completed)
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
};

} // namespace xtd

#endif // PIPELINE_READ_RESULT_H