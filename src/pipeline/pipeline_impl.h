#ifndef PIPELINE_PIPELINE_H
#define PIPELINE_PIPELINE_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <span>
#include <stdexcept>
#include <stop_token>

#include "data_segment.h"
#include "position.h"
#include "read_result.h"
#include "custom_allocators/fixed_pool_resource.h"
#include "custom_allocators/arena_pool_resource.h"

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
        friend class pipe_writer;
        friend class pipe_reader;
        using memory_resource_ptr = std::unique_ptr<std::pmr::memory_resource>;

        struct data_available_predicate
        {
            const pipeline& m_pipeline;
            const std::size_t& m_min_size;

            [[nodiscard]]
            bool operator()() const noexcept
            {
                return m_pipeline.m_writer_completed
                    || (
                        m_pipeline.m_buffered_size > m_pipeline.m_examined_size 
                        && (m_min_size == 0 || m_pipeline.m_buffered_size >= m_min_size)
                    ); 
            }
        };
        struct space_available_predicate
        {
            const pipeline& m_pipeline;

            [[nodiscard]]
            bool operator()() const noexcept
            {
                return !m_pipeline.m_writer_paused;
            }
        };

        // Larger/aligned objects first.
        //xtd::fixed_buffer_pool m_data_segment_pool;
        //std::pmr::unsynchronized_pool_resource m_allocator;
        memory_resource_ptr m_allocator;
        std::deque<data_segment> m_segments{};

        mutable std::mutex m_mutex{};
        mutable std::condition_variable_any m_data_available{};
        mutable std::condition_variable_any m_space_available{};

        const std::size_t m_pause_writer_threshold;
        const std::size_t m_resume_writer_threshold;

        std::size_t m_buffer_size = 0;
        std::size_t m_buffered_size = 0;
        std::size_t m_actual_buffered_size = 0;
        std::size_t m_examined_size = 0;
        std::size_t m_pending_read_size = 0;

        bool m_reader_waiting = false;
        bool m_has_pending_read = false;
        bool m_writer_completed = false;
        bool m_reader_completed = false;
        bool m_writer_paused = false;

        static void runtime_assert(bool condition, const char* message) {
            if (!condition) {
                throw std::runtime_error(message);
            }
        }

        static void argument_assert(bool condition, const char* message) {
            if (!condition) {
                throw std::invalid_argument(message);
            }
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
        bool should_resume_writer() const noexcept
        {
            return m_buffer_size <= m_resume_writer_threshold
                && m_actual_buffered_size < m_pause_writer_threshold;
        }

        [[nodiscard]]
        bool has_writable_tail() const noexcept
        {
            return !m_segments.empty() && !m_segments.back().full();
        }

        [[nodiscard]]
        std::size_t segment_allocation_capacity() const noexcept
        {
            return m_actual_buffered_size < m_pause_writer_threshold
                ? m_pause_writer_threshold - m_actual_buffered_size
                : 0;
        }

        [[nodiscard]]
        std::size_t writer_available_capacity() const noexcept
        {
            if (has_writable_tail()) {
                return m_segments.back().writable_size();
            }

            return segment_allocation_capacity() >= m_buffer_size
                ? m_buffer_size
                : 0;
        }

        [[nodiscard]]
        bool should_pause_writer() const noexcept
        {
            return !has_writable_tail()
                && segment_allocation_capacity() < m_buffer_size;
        }

        bool is_any_completed() const noexcept
        {
            return m_writer_completed || m_reader_completed;
        }
        
        read_result read_at_least(const std::size_t min_size, std::stop_token stop_token)
        {
            std::unique_lock lock{m_mutex};
                
            runtime_assert(!m_has_pending_read, 
                "advance(consumed, examined) must be called before the next read");
                
            runtime_assert(!m_reader_completed, 
                "pipeline reader is completed");

            if (!m_writer_completed) {
                m_reader_waiting = true;
                m_data_available.wait(lock, stop_token, data_available_predicate{*this, min_size});
                m_reader_waiting = false;
            }
        
            if (stop_token.stop_requested()) {
                return read_result(true);
            }

            m_has_pending_read = true;

            return xtd::read_result (
                m_segments,
                m_writer_completed,
                m_pending_read_size
            );
        }

        void advance(const position& consumed, const position& examined)
        {
            const std::size_t consumed_offset = consumed.sequence_offset();
            const std::size_t examined_offset = examined.sequence_offset();

            argument_assert(consumed_offset <= examined_offset, 
                "consumed must be <= examined");

            std::unique_lock lock{m_mutex};

            runtime_assert(!m_reader_completed, 
                "pipeline reader is completed");

            runtime_assert(m_has_pending_read, 
                "advance called without a pending read");

            argument_assert( examined_offset <= m_pending_read_size,
                "examined exceeds the most recent read buffer length");

            const bool was_paused = m_writer_paused;
            std::size_t remaining_consumed = consumed_offset;

            while (remaining_consumed > 0) {
                data_segment& head = m_segments.front();
                const std::size_t readable_size = head.readable_size();

                if (remaining_consumed < readable_size) {
                    head.advance_read(remaining_consumed);
                    m_buffered_size -= remaining_consumed;
                    break;
                }

                m_segments.pop_front();

                remaining_consumed -= readable_size;
                m_buffered_size -= readable_size;
                m_actual_buffered_size -= m_buffer_size;
            }

            m_examined_size = examined_offset - consumed_offset;

            if (m_writer_paused && should_resume_writer()) {
                m_writer_paused = false;
            }

            m_pending_read_size = 0;
            m_has_pending_read = false;

            const bool notify = (was_paused && !m_writer_paused);
            lock.unlock();
            if (notify) {
                m_space_available.notify_one();
            }
        }

        data_segment& get_segment() {
            if (m_segments.empty() || m_segments.back().full()) {
                m_segments.emplace_back(m_buffer_size, *m_allocator);
                m_actual_buffered_size += m_buffer_size;
            }
            return m_segments.back();
        }

        std::size_t write(std::span<const std::byte> data, std::stop_token stop_token)
        {
            if (data.size() == 0) {
                return 0;
            }

            std::size_t copied = 0;

            while (!data.empty()) {
                bool notify_data_available = false;

                std::unique_lock lock{m_mutex};
                runtime_assert(!is_any_completed(), "pipeline is completed");
                m_space_available.wait(lock, stop_token, space_available_predicate{*this});

                
                while (!data.empty() && !m_writer_paused) {
                    runtime_assert(!is_any_completed(),
                        "pipeline is completed");
        
                    if (stop_token.stop_requested()) {
                        return copied;
                    }

                    const std::size_t available_capacity = writer_available_capacity();

                    if (available_capacity == 0) {
                        m_writer_paused = true;
                        break;
                    }

                    const std::size_t requested_size = std::min(data.size(), available_capacity);
                    const std::size_t copy_size = get_segment().copy_from(data.data(), requested_size);

                    data = data.subspan(copy_size);
                    m_buffered_size += copy_size;
                    copied += copy_size;

                    if (m_reader_waiting) {
                        notify_data_available = true;
                    }

                    if (should_pause_writer()) {
                        m_writer_paused = true;
                    }
                }

                lock.unlock();
                if (notify_data_available) {
                    m_data_available.notify_one();
                }
            }

            return copied;
        }

        void complete_writer() {
            {
                std::scoped_lock lock(m_mutex);
                m_writer_paused = false;
                m_writer_completed = true;
            }

            m_data_available.notify_all();
            m_space_available.notify_all();
        }

        void complete_reader() {
            {
                std::scoped_lock lock{m_mutex};
                m_reader_completed = true;
                m_has_pending_read = false;
                m_writer_paused = false;
                m_pending_read_size = 0;
                m_buffered_size = 0;
                m_actual_buffered_size = 0;
                m_examined_size = 0;
            }

            m_space_available.notify_all();
            m_data_available.notify_all();
        }

        // ----- ctor validation helpers -----
        struct validated_options_tag
        {
        };

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
            argument_assert(size > 0, 
                "buffer_size must be > 0");
            return size;
        }

        [[nodiscard]]
        static std::size_t validate_pause_threshold(const std::size_t pause_threshold)
        {
            argument_assert(pause_threshold > 0,
                "pause_writer_threshold must be > 0");

            return pause_threshold;
        }

        [[nodiscard]]
        static std::size_t validate_resume_threshold(const std::size_t resume_threshold, const std::size_t pause_threshold)
        {
            argument_assert(resume_threshold <= pause_threshold,
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

            const std::size_t max_pooled_segments =
                options.pause_writer_threshold / options.buffer_size
                + static_cast<std::size_t>(options.pause_writer_threshold % options.buffer_size != 0);

            return {
                .buffer_size = options.buffer_size,
                .pause_writer_threshold = options.pause_writer_threshold,
                .resume_writer_threshold = options.resume_writer_threshold,
                .max_pooled_segments = max_pooled_segments,
            };
        }

        enum class allocator_kind {
            fixed_pool_resource, /* Original -- Still best throughput */
            arena_pool_resource, /* Experimental -- Using std::pmr::arena_pool_resource */
            unsynchronized_pool_resource, /* Experimental -- Using std::pmr::unsynchronized_pool_resource */
        };


        [[nodiscard]]
        memory_resource_ptr make_allocator(const allocator_kind kind, const validated_options& options)
        {
            switch (kind) {
                case allocator_kind::fixed_pool_resource:
                    return std::make_unique<xtd::fixed_pool_resource>(options.buffer_size, options.max_pooled_segments);

                case allocator_kind::arena_pool_resource:
                    return std::make_unique<xtd::arena_pool_resource>(options.buffer_size, options.max_pooled_segments);
                
                case allocator_kind::unsynchronized_pool_resource:
                    return std::make_unique<std::pmr::unsynchronized_pool_resource>(std::pmr::pool_options{
                        .max_blocks_per_chunk = options.max_pooled_segments,
                        .largest_required_pool_block = options.buffer_size
                    });

                default:
                    throw std::invalid_argument{"unsupported allocator kind"};
            }
        }

        explicit pipeline(const validated_options& options, validated_options_tag)
            : m_allocator(make_allocator(allocator_kind::fixed_pool_resource, options))
            , m_pause_writer_threshold(options.pause_writer_threshold)
            , m_resume_writer_threshold(options.resume_writer_threshold)
            , m_buffer_size(options.buffer_size)
        {
        }


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
            : pipeline(validate_options(options), validated_options_tag{})
        {
        }
    };

} // namespace xtd

#endif // PIPELINE_PIPE_H
