#ifndef CHANNEL_CHANNEL_H
#define CHANNEL_CHANNEL_H

#include "block_strategy.h"
#include "channel_reader.h"
#include "channel_writer.h"

#include <condition_variable>
#include <cstddef>
#include <queue>
#include <mutex>
#include <optional>
#include <utility>

namespace xtd
{
    template<typename T>
    class channel
    {
    friend class channel_writer<T>;
    friend class channel_reader<T>;
    public:
        explicit channel(const std::size_t capacity = 0)
            : m_capacity(capacity)
            , m_writer(*this)
            , m_reader(*this)
        {
        }

        ~channel()
        {
            complete_writer();
        }

        channel(const channel&) = delete;
        channel& operator=(const channel&) = delete;
        channel(channel&&) = delete;
        channel& operator=(channel&&) = delete;

        [[nodiscard]]
        channel_writer<T>& writer() noexcept
        {
            return m_writer;
        }

        [[nodiscard]]
        channel_reader<T>& reader() noexcept
        {
            return m_reader;
        }
    private:
        [[nodiscard]]
        bool full() const noexcept
        {
            return m_capacity > 0 && m_queue.size() >= m_capacity;
        }

        template<typename... Args>
        requires std::constructible_from<T, Args...>
        bool emplace(block_strategy strategy, Args&&... args)
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT && !m_writer_completed && full())
            {
                ++m_write_waiters;

                m_not_full.wait(lock, [this] {
                    return m_writer_completed || !full();
                });

                --m_write_waiters;
            }

            if (m_writer_completed || full()) {
                return false;
            }

            m_queue.emplace(std::forward<Args>(args)...);

            const bool notify_reader = m_read_waiters != 0;

            lock.unlock();

            if (notify_reader) {
                m_not_empty.notify_one();
            }

            return true;
        }

        void complete_writer()
        {
            {
                std::lock_guard lock(m_mutex);
                m_writer_completed = true;
            }

            m_not_empty.notify_all();
            m_not_full.notify_all();
        }

        [[nodiscard]]
        std::optional<T> read(block_strategy strategy)
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT && !m_writer_completed && m_queue.empty())
            {
                ++m_read_waiters;

                m_not_empty.wait(lock, [this] {
                    return m_writer_completed || !m_queue.empty();
                });

                --m_read_waiters;
            }

            if (m_queue.empty()) {
                return std::nullopt;
            }

            std::optional<T> result(std::in_place, std::move(m_queue.front()));
            m_queue.pop();

            const bool notify_writer = m_write_waiters != 0;

            lock.unlock();

            if (notify_writer) {
                m_not_full.notify_one();
            }

            return result;
        }

        [[nodiscard]]
        std::size_t size() const
        {
            std::lock_guard lock(m_mutex);
            return m_queue.size();
        }

        const std::size_t m_capacity;

        mutable std::mutex m_mutex;
        std::condition_variable m_not_full;
        std::condition_variable m_not_empty;

        std::size_t m_read_waiters = 0;
        std::size_t m_write_waiters = 0;

        std::queue<T> m_queue;
        bool m_writer_completed = false;

        channel_writer<T> m_writer;
        channel_reader<T> m_reader;
    };
}

#endif // CHANNEL_CHANNEL_H