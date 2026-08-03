#ifndef PIPELINE_PIPE_READER_H
#define PIPELINE_PIPE_READER_H

#include "pipeline_impl.h"

namespace xtd
{

    class pipe_reader
    {
    private:
        pipeline& m_state;
                
    public:
        explicit pipe_reader(pipeline& state) noexcept 
            : m_state(state)
        {
        }
        
        // Reads currently available data from the pipeline.
        // Returns a read result containing the buffer and completion status.
        read_result read(std::stop_token stop_token = {}) const
        {
            return m_state.read_at_least(0, stop_token);
        }

        // Reads at least min_size bytes from the pipeline, blocking until available or completed.
        // min_size: Minimum number of bytes to read.
        // Returns a read result containing the buffer and completion status.
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
            m_state.advance(consumed, consumed);
        }

        // Advances the reader using the sequence boundaries as consumed and examined positions.
        // sequence: The sequence whose begin/end define consumed/examined.
        void advance(const segmented_byte_view& sequence) {
            m_state.advance(sequence.begin(), sequence.end());
        }

        // Completes the reader, clears buffered state, and wakes waiting writers/readers.
        void complete() {
            m_state.complete_reader();
        }
    };

} // namespace xtd

#endif // PIPELINE_PIPE_READER_H
