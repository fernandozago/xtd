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

    inline std::size_t validate_buffer_size(const std::size_t& size) const {
        if (size == 0) {
            throw std::invalid_argument("buffer_size must be > 0");
        }

        return size;
    }

    inline std::size_t validate_pause_threshold(const std::size_t& pauseThreshold) const {
        if (pauseThreshold == 0) {
            throw std::invalid_argument("pause_writer_threshold must be > 0");
        }

        return pauseThreshold;
    }

    inline std::size_t validate_resume_threshold(const std::size_t& resumeThreshold, const std::size_t& pauseThreshold) const {
        if (resumeThreshold > pauseThreshold) {
            throw std::invalid_argument("resume_writer_threshold must be <= pause_writer_threshold");
        }

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

    read_result read() {
        std::unique_lock lock(m_mutex);
        if (m_has_pending_read) {
            throw std::runtime_error("advance(consumed, examined) must be called before the next read");
        }

        m_data_available.wait(lock, [this]
        {
            return m_writer_completed || m_reader_completed ||
                (m_wait_for_data_change
                    ? m_buffered_size != m_wait_for_data_change_from_size
                    : m_buffered_size > 0);
        });

        if (m_reader_completed) {
            throw std::runtime_error("pipeline reader is completed");
        }

        if (m_has_pending_read) {
            throw std::runtime_error("advance(consumed, examined) must be called before the next read");
        }

        m_wait_for_data_change = false;
        std::vector<std::span<const std::byte>> readSegments = m_segments
            | std::views::filter([](const data_segment& segment) noexcept {
                return segment.readable_size() > 0;
            })
            | std::views::transform(&data_segment::readable_bytes)
            | std::ranges::to<std::vector>();

        m_pending_read_size = m_buffered_size;
        m_pending_read_sequence_id = next_read_sequence_id();
        m_has_pending_read = true;
        return read_result{segmented_byte_view{
            std::move(readSegments),
            0,
            m_pending_read_size,
            m_pending_read_sequence_id
        }, m_writer_completed};
    }

    bool advance_core(const std::size_t& consumedOffset, const std::size_t& examinedOffset) {
        std::size_t l_consumedOffset = consumedOffset;

        m_has_pending_read = false;

        while (l_consumedOffset > 0 && !m_segments.empty()) {
            data_segment& head = m_segments.front();
            const std::size_t readable = head.readable_size();

            if (readable > l_consumedOffset) {
                head.advance(l_consumedOffset);
                m_buffered_size -= l_consumedOffset;
                l_consumedOffset = 0;
                break;
            }

            m_buffered_size -= readable;
            l_consumedOffset -= readable;

            m_data_segment_pool.return_segment(std::move(m_segments.front()));
            m_segments.pop_front();
        }

        m_examined_size = std::min(
            examinedOffset - consumedOffset,
            m_buffered_size
        );

        if (m_writer_paused && should_resume_writer()) {
            m_writer_paused = false;
        }

        return m_writer_completed;
    }

    void advance(const position& consumed, const position& examined) {
        if (consumed > examined) {
            throw std::invalid_argument("consumed must be <= examined");
        }

        const std::size_t consumedOffset = consumed.offset_in_sequence();
        const std::size_t examinedOffset = examined.offset_in_sequence();
        bool notify = false;
        {
            std::scoped_lock lock(m_mutex);

            if (!m_has_pending_read) {
                throw std::runtime_error("advance called without a pending read");
            }

            if (m_reader_completed) {
                throw std::runtime_error("pipeline reader is completed");
            }

            if (consumed.m_sequence_id != m_pending_read_sequence_id || examined.m_sequence_id != m_pending_read_sequence_id) {
                throw std::invalid_argument("position must belong to the most recent read buffer");
            }

            if (consumedOffset > m_pending_read_size) {
                throw std::invalid_argument("consumed is out of range for the most recent read buffer");
            }

            if (examinedOffset > m_pending_read_size) {
                throw std::invalid_argument("examined exceeds the most recent read buffer length");
            }

            const bool writerCompleted = advance_core(consumedOffset, examinedOffset);

            m_wait_for_data_change = examinedOffset == m_pending_read_size && !writerCompleted;
            m_wait_for_data_change_from_size = consumedOffset < m_pending_read_size ? m_pending_read_size - consumedOffset : 0;
            m_pending_read_size = 0;

            notify = true;
        }

        if (notify) {
            m_space_available.notify_all();
            m_data_available.notify_all();
        }
    }

    std::size_t write(const std::byte* data, const std::size_t& length) {
        if (length > 0 && data == nullptr) {
            throw std::invalid_argument("data must not be null when length > 0");
        }

        std::unique_lock lock(m_mutex);
        if (m_writer_completed) {
            throw std::runtime_error("pipeline writer is completed");
        }

        if (m_reader_completed) {
            throw std::runtime_error("pipeline reader is completed");
        }

        for (std::size_t offset = 0; offset < length;) {
            m_space_available.wait(lock, [this] {
                return m_reader_completed
                    || m_writer_completed
                    || !m_writer_paused
                    || should_resume_writer();
            });

            if (m_writer_completed) {
                throw std::runtime_error("pipeline writer is completed");
            }

            if (m_reader_completed) {
                throw std::runtime_error("pipeline reader is completed");
            }

            if (m_writer_paused && should_resume_writer()) {
                m_writer_paused = false;
            }

            bool notifyDataAvailable = false;
            while (offset < length && !m_writer_paused)
            {
                if (m_segments.empty() || m_segments.back().full()) {
                    m_segments.push_back(m_data_segment_pool.rent_segment());
                }

                const std::byte* beginOffset = data + offset;
                const std::size_t size = length - offset;
                const std::size_t copiedSize = m_segments.back().copy_from(beginOffset, size);

                offset += copiedSize;
                m_buffered_size += copiedSize;

                if (!notifyDataAvailable && copiedSize > 0) {
                    notifyDataAvailable = true;
                }

                if (m_buffered_size >= m_pause_writer_threshold) {
                    m_writer_paused = true;
                }
            }

            if (notifyDataAvailable) {
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

    void complete_reader() noexcept
    {
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
