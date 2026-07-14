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
             
    /// <summary>
    /// Initializes a reader endpoint bound to a shared pipeline state.
    /// </summary>
    /// <param name="state">Shared pipeline state.</param>
    pipe_reader(pipeline& state) noexcept
        : m_state(state) 
    {}
public:
    pipe_reader(const pipe_reader&) = delete;
    pipe_reader& operator=(const pipe_reader&) = delete;
    pipe_reader(pipe_reader&&) = delete;
    pipe_reader& operator=(pipe_reader&&) = delete;

    /// <summary>
    /// Reads currently available data from the pipeline.
    /// </summary>
    /// <returns>A read result containing the buffer and completion status.</returns>
    [[nodiscard]] 
    read_result read();

    /// <summary>
    /// Advances the reader by consumed and examined positions from the most recent read buffer.
    /// </summary>
    /// <param name="consumed">The position up to which data has been consumed.</param>
    /// <param name="examined">The position up to which data has been examined.</param>
    void advance(const position& consumed, const position& examined);

    /// <summary>
    /// Advances the reader by a single position for both consumed and examined.
    /// </summary>
    /// <param name="consumed">The position treated as both consumed and examined.</param>
    void advance(const position& consumed);

    /// <summary>
    /// Advances the reader using the sequence boundaries as consumed and examined positions.
    /// </summary>
    /// <param name="sequence">The sequence whose begin/end define consumed/examined.</param>
    void advance(const segmented_byte_view& sequence);

    /// <summary>
    /// Completes the reader, clears buffered state, and wakes waiting writers/readers.
    /// </summary>
    void complete() noexcept;

};

} // namespace xtd

#endif // PIPELINE_PIPE_READER_H
