#ifndef CHANNEL_CHANNEL_CORE_H
#define CHANNEL_CHANNEL_CORE_H

#include "block_strategy.h"
#include "channel_reader.h"
#include "channel_writer.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace xtd
{
    template<typename T>
    class channel
    {
    public:
        explicit channel(const std::size_t capacity = 0)
            : m_capacity(capacity)
            , m_writer(*this)
            , m_reader(*this)
        {
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
        friend class xtd::channel_writer<T>;
        friend class xtd::channel_reader<T>;

        [[nodiscard]]
        bool full() const noexcept
        {
            return m_capacity > 0 && m_queue.size() >= m_capacity;
        }

        template<typename U>
        bool push(U&& value, block_strategy strategy)
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT && !m_completed && full())
            {
                ++m_write_waiters;

                m_not_full.wait(lock, [this] {
                    return m_completed || !full();
                });

                --m_write_waiters;
            }

            if (m_completed || full()) {
                return false;
            }

            m_queue.emplace_back(std::forward<U>(value));

            const bool notify_reader = m_read_waiters != 0;

            lock.unlock();

            if (notify_reader) {
                m_not_empty.notify_one();
            }

            return true;
        }

        void complete()
        {
            {
                std::lock_guard lock(m_mutex);
                m_completed = true;
            }

            m_not_empty.notify_all();
            m_not_full.notify_all();
        }

        [[nodiscard]]
        std::optional<T> read(block_strategy strategy)
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT && !m_completed && m_queue.empty())
            {
                ++m_read_waiters;

                m_not_empty.wait(lock, [this] {
                    return m_completed || !m_queue.empty();
                });

                --m_read_waiters;
            }

            if (m_queue.empty()) {
                return std::nullopt;
            }

            std::optional<T> result(std::in_place, std::move(m_queue.front()));
            m_queue.pop_front();

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

        std::deque<T> m_queue;
        bool m_completed = false;

        channel_writer<T> m_writer;
        channel_reader<T> m_reader;
    };
}

#endif