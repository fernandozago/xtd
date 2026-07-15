#ifndef PIPELINE_PIPELINE_H
#define PIPELINE_PIPELINE_H

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <ranges>

#include "data_segment.h"
#include "data_segment_pool.h"
#include "pipe_reader.h"
#include "pipe_writer.h"
#include "position.h"
#include "segmented_byte_view.h"
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
    const std::size_t m_buffer_size;
    const std::size_t m_pause_writer_threshold;
    const std::size_t m_resume_writer_threshold;
    xtd::data_segment_pool m_data_segment_pool;

    std::deque<data_segment> m_segments;

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_data_available;
    mutable std::condition_variable m_space_available;

    std::uint64_t m_pending_read_sequence_id = 0;

    std::size_t m_buffered_size = 0;
    std::size_t m_examined_size = 0;
    std::size_t m_pending_read_size = 0;
    std::size_t m_wait_for_data_change_from_size = 0;

    bool m_wait_for_data_change = false;
    bool m_has_pending_read = false;
    bool m_writer_completed = false;
    bool m_reader_completed = false;
    bool m_writer_paused = false;

    pipe_writer m_writer;
    pipe_reader m_reader;

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

    bool should_resume_writer() const noexcept {
        return m_buffered_size <= m_resume_writer_threshold ||
            (m_examined_size >= m_buffered_size ? 0 : m_buffered_size - m_examined_size) <= m_resume_writer_threshold;
    }

    read_result read()
    {
        std::unique_lock lock{m_mutex};
        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        m_data_available.wait(lock, [this] {
            return m_reader_completed
                || m_writer_completed
                || (m_wait_for_data_change
                    ? m_buffered_size != m_wait_for_data_change_from_size
                    : m_buffered_size > 0);
        });

        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        /*
        * wait() releases the mutex. Another reader may have completed a read
        * while this thread was waiting, so this second check is necessary if
        * concurrent calls to read() are possible.
        */
        runtime_assert(!m_has_pending_read, "advance(consumed, examined) must be called before the next read");
        m_wait_for_data_change = false;

        std::vector<std::span<const std::byte>> read_segments;
        read_segments.reserve(m_segments.size());

        std::ranges::transform(
            m_segments
                | std::views::filter(
                    [](const data_segment& segment) noexcept {
                        return segment.readable_size() > 0;
                    }),
            std::back_inserter(read_segments),
            &data_segment::readable_bytes);

        segmented_byte_view buffer{
            std::move(read_segments),
            m_pending_read_sequence_id = next_read_sequence_id()
        };

        /*
        * The view calculates its size from its actual spans.
        * Do not duplicate that calculation using m_buffered_size.
        */
        assert(buffer.size() == m_buffered_size);
        m_pending_read_size = buffer.size();
        m_has_pending_read = true;

        return read_result{
            std::move(buffer),
            m_writer_completed
        };
    }

    bool advance_core(std::size_t consumed_offset, std::size_t examined_offset)
    {
        const std::size_t pending_read_size = m_pending_read_size;
        std::size_t remaining_consumed = consumed_offset;

        while (remaining_consumed > 0) {
            assert(!m_segments.empty());

            data_segment& head = m_segments.front();
            const std::size_t readable_size = head.readable_size();

            if (remaining_consumed < readable_size) {
                head.advance(remaining_consumed);
                m_buffered_size -= remaining_consumed;
                remaining_consumed = 0;
                break;
            }

            remaining_consumed -= readable_size;
            m_buffered_size -= readable_size;

            m_data_segment_pool.return_segment(
                std::move(m_segments.front()));

            m_segments.pop_front();
        }

        assert(remaining_consumed == 0);

        m_examined_size = std::min(examined_offset - consumed_offset,m_buffered_size);
        if (m_writer_paused && should_resume_writer()) {
            m_writer_paused = false;
        }

        m_wait_for_data_change = examined_offset == pending_read_size && !m_writer_completed;
        m_wait_for_data_change_from_size = pending_read_size - consumed_offset;
        m_pending_read_size = 0;
        m_pending_read_sequence_id = 0;
        m_has_pending_read = false;

        return m_writer_completed;
    }

    void advance(const position& consumed, const position& examined)
    {
        const std::size_t consumed_offset = consumed.offset_in_sequence();
        const std::size_t examined_offset = examined.offset_in_sequence();
        argument_assert(consumed_offset <= examined_offset, "consumed must be <= examined");

        {
            std::scoped_lock lock{m_mutex};

            runtime_assert(!m_reader_completed, "pipeline reader is completed");
            runtime_assert(m_has_pending_read, "advance called without a pending read");
            argument_assert(consumed.m_sequence_id == m_pending_read_sequence_id, "consumed position must belong to the most recent read buffer");
            argument_assert(examined.m_sequence_id == m_pending_read_sequence_id, "examined position must belong to the most recent read buffer");
            argument_assert(examined_offset <= m_pending_read_size, "examined exceeds the most recent read buffer length");
            advance_core(consumed_offset, examined_offset);
        }

        m_space_available.notify_all();
        m_data_available.notify_all();
    }

    inline data_segment& get_segment() {
        if (m_segments.empty() || m_segments.back().full()) {
            m_segments.push_back(m_data_segment_pool.rent_segment());
        }
        return m_segments.back();
    }

    std::size_t write(const std::byte* data, const std::size_t length)
    {
        if (length > 0 && data == nullptr) {
            throw std::invalid_argument("data must not be null when length > 0");
        }
        
        runtime_assert(!m_writer_completed, "pipeline writer is completed");
        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        std::span<const std::byte> remaining{data, length};
        std::unique_lock lock(m_mutex);
        
        runtime_assert(!m_writer_completed, "pipeline writer is completed");
        runtime_assert(!m_reader_completed, "pipeline reader is completed");

        while (!remaining.empty()) {
            m_space_available.wait(lock, [this] {
                return m_reader_completed
                    || m_writer_completed
                    || !m_writer_paused
                    || should_resume_writer();
            });

            runtime_assert(!m_writer_completed, "pipeline writer is completed");
            runtime_assert(!m_reader_completed, "pipeline reader is completed");

            if (m_writer_paused && should_resume_writer()) {
                m_writer_paused = false;
            }

            bool notify_data_available = false;

            while (!remaining.empty() && !m_writer_paused) {
                const std::size_t copied_size = get_segment()
                    .copy_from(remaining.data(), remaining.size());

                remaining = remaining.subspan(copied_size);
                m_buffered_size += copied_size;

                notify_data_available |= copied_size > 0;

                if (m_buffered_size >= m_pause_writer_threshold) {
                    m_writer_paused = true;
                }
            }

            if (notify_data_available) {
                lock.unlock();
                m_data_available.notify_one();
                lock.lock();
            }
        }

        return length;
    }

    void complete_writer() noexcept {
        {
            std::scoped_lock lock(m_mutex);
            m_writer_completed = true;
        }

        m_data_available.notify_all();
        m_space_available.notify_all();
    }

    void complete_reader() noexcept {
        {
            std::scoped_lock lock(m_mutex);
            m_reader_completed = true;
            m_has_pending_read = false;
            m_pending_read_size = 0;
            m_wait_for_data_change = false;
            m_wait_for_data_change_from_size = 0;
            m_writer_paused = false;
            m_buffered_size = 0;
            m_examined_size = 0;
        }

        m_space_available.notify_all();
        m_data_available.notify_all();
    }

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
        , m_data_segment_pool(m_buffer_size, m_pause_writer_threshold)
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
};

inline std::size_t pipe_writer::write(const std::byte* data, std::size_t length) {
    return m_state.write(data, length);
}

template <typename T>
requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
inline std::size_t pipe_writer::write(const T& value) {
    if constexpr (std::convertible_to<const T&, std::string_view>)
    {
        std::string_view strView = static_cast<std::string_view>(value);
        return m_state.write(reinterpret_cast<const std::byte*>(strView.data()), strView.size());
    }
    else
    {
        return m_state.write(reinterpret_cast<const std::byte*>(&value), sizeof(T));
    }
}

inline void pipe_writer::complete() noexcept {
    m_state.complete_writer();
}

inline read_result pipe_reader::read() {
    return m_state.read();
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

inline void pipe_reader::complete() noexcept {
    m_state.complete_reader();
}

} // namespace xtd

#endif // PIPELINE_PIPE_H
