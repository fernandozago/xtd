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
private:
    friend class pipe_reader_impl<>;
    friend class pipe_writer_impl<>;

     // Larger/aligned objects first.
    xtd::fixed_buffer_pool m_data_segment_pool;
    std::deque<data_segment> m_segments{};

    mutable std::mutex m_mutex{};
    mutable std::condition_variable_any m_data_available{};
    mutable std::condition_variable_any m_space_available{};

    const std::size_t m_buffer_size;
    const std::size_t m_pause_writer_threshold;
    const std::size_t m_resume_writer_threshold;

    std::uint64_t m_pending_read_sequence_id = 0;
    std::size_t m_buffered_size = 0;
    std::size_t m_examined_size = 0;
    std::size_t m_pending_read_size = 0;

    pipe_writer m_writer;
    pipe_reader m_reader;

    bool m_reader_waiting = false;
    bool m_has_pending_read = false;
    bool m_writer_completed = false;
    bool m_reader_completed = false;
    bool m_writer_paused = false;

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
                head.advance_read(remaining_consumed);
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

                // Calculate the maximum number of bytes we can write to the pipeline without exceeding the pause threshold.
                const std::size_t remaining_capacity = m_pause_writer_threshold - m_buffered_size;
                const std::size_t requested_size = std::min(remaining.size(), remaining_capacity);

                /* The data is copied here */
                const std::size_t copy_size = get_segment().copy_from(remaining.data(), requested_size);
                /* The data has been copied */

                remaining = remaining.subspan(copy_size);
                m_buffered_size += copy_size;
                notify_data_available = m_reader_waiting;

                if (m_buffered_size == m_pause_writer_threshold) {
                    m_writer_paused = true;
                }
            }

            if (notify_data_available) {
                lock.unlock();
                m_data_available.notify_one();

                // Optimization: If remaining data is empty, we can return early without reacquiring the lock.
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

    // ----- ctor validation helpers -----

    struct validated_options
    {
        std::size_t buffer_size;
        std::size_t pause_writer_threshold;
        std::size_t resume_writer_threshold;
        std::size_t max_pooled_segments;
    };

    [[nodiscard]]
    static std::size_t validate_buffer_size(const std::size_t size)
    {
        argument_assert(size > 0, "buffer_size must be > 0");
        return size;
    }

    [[nodiscard]]
    static std::size_t validate_pause_threshold(
        const std::size_t pause_threshold)
    {
        argument_assert(
            pause_threshold > 0,
            "pause_writer_threshold must be > 0"
        );

        return pause_threshold;
    }

    [[nodiscard]]
    static std::size_t validate_resume_threshold(
        const std::size_t resume_threshold,
        const std::size_t pause_threshold)
    {
        argument_assert(
            resume_threshold <= pause_threshold,
            "resume_writer_threshold must be <= pause_writer_threshold"
        );

        return resume_threshold;
    }

    [[nodiscard]]
    static validated_options validate_options(const pipe_options& options)
    {
        argument_assert(options.buffer_size > 0,
            "buffer_size must be > 0");

        argument_assert(options.pause_writer_threshold > 0,
            "pause_writer_threshold must be > 0");

        argument_assert(options.resume_writer_threshold <= options.pause_writer_threshold,
            "resume_writer_threshold must be <= pause_writer_threshold");


        // When pause_writer_threshold is not divisible by buffer_size, we need an extra segment to accommodate the remainder.
        const std::size_t max_pooled_segments = options.pause_writer_threshold / options.buffer_size +
            (options.pause_writer_threshold % options.buffer_size != 0); 

        return {
            .buffer_size = options.buffer_size,
            .pause_writer_threshold = options.pause_writer_threshold,
            .resume_writer_threshold = options.resume_writer_threshold,
            .max_pooled_segments = max_pooled_segments,
        };
    }

    // Initializes a pipeline with the provided options.
    // options: pipeline buffering and backpressure options.
    explicit pipeline(const validated_options& options)
        : m_data_segment_pool(options.buffer_size, options.max_pooled_segments)
        , m_buffer_size(options.buffer_size)
        , m_pause_writer_threshold(options.pause_writer_threshold)
        , m_resume_writer_threshold(options.resume_writer_threshold)
        , m_writer{*this}
        , m_reader(*this)
    {
    }
    // ----- ctor validation helpers -----

public:
    pipeline(const pipeline&) = delete;
    pipeline& operator=(const pipeline&) = delete;
    pipeline(pipeline&&) = delete;
    pipeline& operator=(pipeline&&) = delete;

    ~pipeline() {
        complete_writer();
        complete_reader();
    }

    explicit pipeline(pipe_options options = {})
        : pipeline(validate_options(options))
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
};

} // namespace xtd

#endif // PIPELINE_PIPE_H
