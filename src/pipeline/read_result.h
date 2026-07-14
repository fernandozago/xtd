#ifndef PIPELINE_READ_RESULT_H
#define PIPELINE_READ_RESULT_H

#include <utility>

#include "segmented_byte_view.h"

namespace xtd
{

struct read_result
{
friend class pipeline;

private:
    segmented_byte_view m_buffer;
    bool m_completed = false;
    
    /// <summary>
    /// Initializes a read result with a buffer and completion state.
    /// </summary>
    /// <param name="buffer">The buffer returned by a read operation.</param>
    /// <param name="isCompleted">Whether the writer has completed.</param>
    read_result(segmented_byte_view&& buffer, const bool completed)
        : m_buffer(std::move(buffer))
        , m_completed(completed)
    {}
    
public:
    read_result() = delete;
    read_result(const read_result&) = delete;
    read_result& operator=(const read_result&) = delete;
    read_result(read_result&&) noexcept = default;
    read_result& operator=(read_result&&) noexcept = default;

    /// <summary>
    /// Gets the read-only buffer associated with this result.
    /// </summary>
    /// <returns>The current read-only sequence.</returns>
    [[nodiscard]] 
    segmented_byte_view buffer() const noexcept {
        return m_buffer; 
    }

    /// <summary>
    /// Indicates whether no more data will be written.
    /// </summary>
    /// <returns>True when the writer is completed; otherwise, false.</returns>
    [[nodiscard]] 
    bool completed() const noexcept {
        return m_completed; 
    }
};

} // namespace xtd

#endif // PIPELINE_READ_RESULT_H
