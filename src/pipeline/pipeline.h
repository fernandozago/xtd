#ifndef PIPELINE_PIPELINE_H
#define PIPELINE_PIPELINE_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <stdexcept>
#include <stop_token>

#include "data_segment.h"
#include "fixed_buffer_pool.h"
#include "pipe_reader.h"
#include "pipe_writer.h"
#include "position.h"
#include "read_result.h"

namespace xtd
{

struct pipe_options {
    std::size_t buffer_size = 1024 * 4;
    std::size_t resume_writer_threshold = 1024 * 32;
    std::size_t pause_writer_threshold = 1024 * 128;
};

class pipeline
{
public:
    friend class pipe_reader;
    friend class pipe_writer;

    pipeline(const pipeline&) = delete;
    pipeline& operator=(const pipeline&) = delete;
    pipeline(pipeline&&) = delete;
    pipeline& operator=(pipeline&&) = delete;

    ~pipeline() {
        complete_writer();
        complete_reader();
    }

    // Initializes a pipeline with the provided options.
    // options: pipeline buffering and backpressure options.
    pipeline(pipe_options options = {})
        : m_buffer_size(validate_buffer_size(options.buffer_size))
        , m_pause_writer_threshold(validate_pause_threshold(options.pause_writer_threshold))
        , m_resume_writer_threshold(validate_resume_threshold(options.resume_writer_threshold, m_pause_writer_threshold))
        , m_data_segment_pool(
            m_buffer_size,
            calculate_max_pooled_segments(m_buffer_size, m_pause_writer_threshold)
        )
        , m_writer(*this)
        , m_reader(*this)
    {
    }

    // Gets the pipeline reader endpoint.
    // Returns a reference to the reader.
    [[nodiscard]]
    pipe_reader& reader() noexcept {
        return m_reader;
    }

    // Gets the pipeline writer endpoint.
    // Returns a reference to the writer.
    [[nodiscard]]
    pipe_writer& writer() noexcept {
        return m_writer;
    }

private:
    const std::size_t m_buffer_size;
    const std::size_t m_pause_writer_threshold;
    const std::size_t m_resume_writer_threshold;

    xtd::fixed_buffer_pool m_data_segment_pool;
    std::deque<data_segment> m_segments;

    mutable std::mutex m_mutex;
    mutable std::condition_variable_any m_data_available;
    mutable std::condition_variable_any m_space_available;

    std::uint64_t m_pending_read_sequence_id = 0;

    std::size_t m_buffered_size = 0;
    std::size_t m_examined_size = 0;
    std::size_t m_pending_read_size = 0;
    
    bool m_reader_waiting = false;
    bool m_has_pending_read = false;
    bool m_writer_completed = false;
    bool m_reader_completed = false;
    bool m_writer_paused = false;

    pipe_writer m_writer;
    pipe_reader m_reader;

    [[nodiscard]]
    static std::size_t calculate_max_pooled_segments(const std::size_t buffer_size, const std::size_t pause_writer_threshold) noexcept
    {
        assert(buffer_size > 0);
        const std::size_t segments_for_threshold =
            pause_writer_threshold / buffer_size +
            (pause_writer_threshold % buffer_size != 0);

        return segments_for_threshold + 1;
    }

    inline static void runtime_assert(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    inline static void argument_assert(bool condition, const char* message) {
        if (!condition) {
            throw std::invalid_argument(message);
        }
    }
    
    inline std::size_t validate_buffer_size(const std::size_t size) const {
        argument_assert(size > 0, "buffer_size must be > 0");
        return size;
    }

    inline std::size_t validate_pause_threshold(const std::size_t pauseThreshold) const {
        argument_assert(pauseThreshold > 0, "pause_writer_threshold must be > 0");
        return pauseThreshold;
    }

    inline std::size_t validate_resume_threshold(const std::size_t resumeThreshold, const std::size_t pauseThreshold) const {
        argument_assert(resumeThreshold <= pauseThreshold, "resume_writer_threshold must be <= pause_writer_threshold");
        return resumeThreshold;
    }

    static std::uint64_t next_read_sequence_id() {
        static std::atomic<std::uint64_t> next{1};
        return next.fetch_add(1, std::memory_order_relaxed);
    }

    // Writer backpressure is based only on the total amount of unconsumed buffered data.
    //
    // `m_examined_size` must not affect writer resumption. Examined bytes are still
    // owned by the pipeline and cannot be recycled until the reader consumes them.
    //
    // Using examined bytes as available capacity could let the buffer grow without
    // bound when the reader repeatedly examines data without consuming it.
    //
    // The configured pause and resume thresholds must therefore be large enough to
    // accommodate the largest expected message.
    [[nodiscard]]
    bool should_resume_writer() const noexcept
    {
        return m_buffered_size <= m_resume_writer_threshold;
    }

    [[nodiscard]]
    bool has_available_data(const std::size_t min_size) const noexcept
    {
        return m_reader_completed
            || m_writer_completed
            || (
                m_buffered_size > m_examined_size &&
                (min_size == 0 || m_buffered_size >= min_size)
            );
    }
    
    read_result read_at_least(const std::size_t min_size, std::stop_token stop_token)
    {
        runtime_assert(!m_has_pending_read, "advance(consumed, examined) must be called before the next read");

        std::unique_lock lock{m_mutex};
        m_reader_waiting = true;
        const bool w_result = m_data_available.wait(lock, stop_token, [this, min_size] { return has_available_data(min_size); });
        m_reader_waiting = false;

        if (!w_result || stop_token.stop_requested()) {
            return read_result(true);
        }

        runtime_assert(!m_reader_completed, "pipeline reader is completed");
        runtime_assert(!m_has_pending_read, "advance(consumed, examined) must be called before the next read");

        xtd::read_result result(
            m_segments,
            m_pending_read_sequence_id = next_read_sequence_id(),
            m_writer_completed
        );

        m_pending_read_size = result.buffer().size();
        m_has_pending_read = true;
        return result;
    }

    bool advance_core(const std::size_t consumed_offset, const std::size_t examined_offset)
    {
        std::size_t remaining_consumed = consumed_offset;

        while (remaining_consumed > 0) {
            data_segment& head = m_segments.front();
            const std::size_t readable_size = head.readable_size();

            if (remaining_consumed < readable_size) {
                head.advance(remaining_consumed);
                m_buffered_size -= remaining_consumed;
                remaining_consumed = 0;
                break;
            }

            // The entire head segment has been consumed, so remove it from the pipeline.
            m_segments.pop_front();

            remaining_consumed -= readable_size;
            m_buffered_size -= readable_size;
        }

        m_examined_size = examined_offset - consumed_offset;
        if (m_writer_paused && should_resume_writer()) {
            m_writer_paused = false;
        }

        m_pending_read_size = 0;
        m_has_pending_read = false;
        return m_writer_completed;
    }

    void advance(const position& consumed, const position& examined)
    {
        const std::size_t consumed_offset = consumed.sequence_offset();
        const std::size_t examined_offset = examined.sequence_offset();
        argument_assert(consumed_offset <= examined_offset, "consumed must be <= examined");

        bool notify_writer = false;
        {
            std::scoped_lock lock{m_mutex};
            runtime_assert(!m_reader_completed, "pipeline reader is completed");
            runtime_assert(m_has_pending_read, "advance called without a pending read");
            argument_assert(consumed.m_sequence_id == m_pending_read_sequence_id, "consumed position must belong to the most recent read buffer");
            argument_assert(examined.m_sequence_id == m_pending_read_sequence_id, "examined position must belong to the most recent read buffer");
            argument_assert(examined_offset <= m_pending_read_size, "examined exceeds the most recent read buffer length");

            const bool was_paused = m_writer_paused;
            advance_core(consumed_offset, examined_offset);
            notify_writer = was_paused && !m_writer_paused;
        }

        if (notify_writer) {
            m_space_available.notify_one();
        }
        m_data_available.notify_all();
    }

    inline data_segment& get_segment() {
        if (m_segments.empty() || m_segments.back().full()) {
            m_segments.emplace_back(m_data_segment_pool);
        }
        return m_segments.back();
    }

    inline bool has_available_space() const noexcept {
        return m_reader_completed
            || m_writer_completed
            || !m_writer_paused
            || should_resume_writer();
    }

    std::size_t write(const std::byte* data, const std::size_t length, std::stop_token stop_token)
    {
        if (length == 0 || data == nullptr) {
            return 0;
        }

        runtime_assert(!m_writer_completed, "pipeline writer is completed");
        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        std::span<const std::byte> remaining{data, length};
        std::unique_lock lock(m_mutex);

        runtime_assert(!m_writer_completed, "pipeline writer is completed");
        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        while (!remaining.empty()) {
            const bool w_result = m_space_available.wait(lock, stop_token, [this] {
                return m_writer_completed ||
                    m_reader_completed ||
                    has_available_space();
            });

            if (!w_result || stop_token.stop_requested())  {
                return length - remaining.size();
            }

            runtime_assert(!m_writer_completed, "pipeline writer is completed");
            runtime_assert(!m_reader_completed, "pipeline reader is completed");

            if (m_writer_paused && should_resume_writer()) {
                m_writer_paused = false;
            }

            bool notify_data_available = false;

            while (!remaining.empty() && !m_writer_paused) {
                if (m_buffered_size >= m_pause_writer_threshold) {
                    m_writer_paused = true;
                    break;
                }

                const std::size_t available_before_pause = m_pause_writer_threshold - m_buffered_size;

                const std::size_t requested_size = std::min(remaining.size(), available_before_pause);

                const std::size_t copied_size = get_segment()
                    .copy_from(remaining.data(), requested_size);

                remaining = remaining.subspan(copied_size);
                m_buffered_size += copied_size;
                notify_data_available = m_reader_waiting;

                if (m_buffered_size == m_pause_writer_threshold) {
                    m_writer_paused = true;
                }
            }

            if (notify_data_available) {
                lock.unlock();
                m_data_available.notify_one();

                if (remaining.empty()) {
                    return length;
                }

                lock.lock();
            }
        }

        return length;
    }

    void complete_writer() {
        {
            std::scoped_lock lock(m_mutex);
            m_writer_completed = true;
        }

        m_data_available.notify_all();
        m_space_available.notify_all();
    }

    void complete_reader() {
        {
            std::scoped_lock lock(m_mutex);
            m_reader_completed = true;
            m_has_pending_read = false;
            m_pending_read_size = 0;
            m_writer_paused = false;
            m_buffered_size = 0;
            m_examined_size = 0;
        }

        m_space_available.notify_all();
        m_data_available.notify_all();
    }
};

inline std::size_t pipe_writer::write(const std::byte* data, std::size_t length, std::stop_token stop_token) {
    return m_state.write(data, length, stop_token);
}

template <typename T>
requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
inline std::size_t pipe_writer::write(const T& value, std::stop_token stop_token) {
    if constexpr (std::convertible_to<const T&, std::string_view>)
    {
        std::string_view strView = static_cast<std::string_view>(value);
        return m_state.write(reinterpret_cast<const std::byte*>(strView.data()), strView.size(), stop_token);
    }
    else
    {
        return m_state.write(reinterpret_cast<const std::byte*>(&value), sizeof(T), stop_token);
    }
}

inline void pipe_writer::complete() {
    m_state.complete_writer();
}

inline read_result pipe_reader::read(std::stop_token stop_token) const {
    return m_state.read_at_least(0, stop_token);
}

inline read_result pipe_reader::read_at_least(const std::size_t min_size, std::stop_token stop_token) const {
    return m_state.read_at_least(min_size, stop_token);
}

inline void pipe_reader::advance(const position& consumed, const position& examined) {
    m_state.advance(consumed, examined);
}

inline void pipe_reader::advance(const position& consumed) {
    advance(consumed, consumed);
}

inline void pipe_reader::advance(const segmented_byte_view& sequence) {
    advance(sequence.begin(), sequence.end());
}

inline void pipe_reader::complete() {
    m_state.complete_reader();
}

} // namespace xtd

#endif // PIPELINE_PIPE_H
