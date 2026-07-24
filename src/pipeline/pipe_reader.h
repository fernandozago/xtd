#ifndef PIPELINE_PIPE_READER_H
#define PIPELINE_PIPE_READER_H

#include <stop_token>

#include "position.h"
#include "segmented_byte_view.h"
#include "read_result.h"

namespace xtd
{

class pipeline;

template<class pipeline_t = pipeline>
requires std::same_as<pipeline_t, pipeline>
class pipe_reader_impl
{
private:
    friend pipeline_t;
    pipeline_t& m_state;
                
    explicit pipe_reader_impl(pipeline_t& state) noexcept
        : m_state(state) 
    {}
public:
    pipe_reader_impl(const pipe_reader_impl&) = delete;
    pipe_reader_impl& operator=(const pipe_reader_impl&) = delete;
    pipe_reader_impl(pipe_reader_impl&&) = delete;
    pipe_reader_impl& operator=(pipe_reader_impl&&) = delete;

    // Reads currently available data from the pipeline.
    // Returns a read result containing the buffer and completion status.
    read_result read(std::stop_token stop_token = {}) const {
        return m_state.read_at_least(0, stop_token);
    }

    read_result read_at_least(const std::size_t min_size, std::stop_token stop_token = {}) const {
        return m_state.read_at_least(min_size, stop_token);
    }

    // Advances the reader by consumed and examined positions from the most recent read buffer.
    // consumed: The position up to which data has been consumed.
    // examined: The position up to which data has been examined.
    void advance(const position& consumed, const position& examined) {
        m_state.advance(consumed, examined);
    }

    // Advances the reader by a single position for both consumed and examined.
    // consumed: The position treated as both consumed and examined.
    void advance(const position& consumed) {
        advance(consumed, consumed);
    }

    // Advances the reader using the sequence boundaries as consumed and examined positions.
    // sequence: The sequence whose begin/end define consumed/examined.
    void advance(const segmented_byte_view& sequence) {
        advance(sequence.begin(), sequence.end());
    }

    // Completes the reader, clears buffered state, and wakes waiting writers/readers.
    void complete() {
        m_state.complete_reader();
    }
};

using pipe_reader = pipe_reader_impl<pipeline>;

} // namespace xtd

#endif // PIPELINE_PIPE_READER_H
