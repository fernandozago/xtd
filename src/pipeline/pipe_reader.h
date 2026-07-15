#ifndef PIPELINE_PIPE_READER_H
#define PIPELINE_PIPE_READER_H

#include "position.h"
#include "segmented_byte_view.h"
#include "read_result.h"

namespace xtd
{

class pipeline;

class pipe_reader
{
friend class pipeline;
private:
    pipeline& m_state;
             
    // Initializes a reader endpoint bound to a shared pipeline state.
    // state: Shared pipeline state.
    pipe_reader(pipeline& state) noexcept
        : m_state(state) 
    {}
public:
    pipe_reader(const pipe_reader&) = delete;
    pipe_reader& operator=(const pipe_reader&) = delete;
    pipe_reader(pipe_reader&&) = delete;
    pipe_reader& operator=(pipe_reader&&) = delete;

    // Reads currently available data from the pipeline.
    // Returns a read result containing the buffer and completion status.
    [[nodiscard]] 
    read_result read() const;

    [[nodiscard]]
    read_result read_at_least(const std::size_t min_bytes) const;

    // Advances the reader by consumed and examined positions from the most recent read buffer.
    // consumed: The position up to which data has been consumed.
    // examined: The position up to which data has been examined.
    void advance(const position& consumed, const position& examined);

    // Advances the reader by a single position for both consumed and examined.
    // consumed: The position treated as both consumed and examined.
    void advance(const position& consumed);

    // Advances the reader using the sequence boundaries as consumed and examined positions.
    // sequence: The sequence whose begin/end define consumed/examined.
    void advance(const segmented_byte_view& sequence);

    // Completes the reader, clears buffered state, and wakes waiting writers/readers.
    void complete() noexcept;

};

} // namespace xtd

#endif // PIPELINE_PIPE_READER_H
