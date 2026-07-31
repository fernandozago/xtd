#ifndef CHANNEL_CHANNEL_H
#define CHANNEL_CHANNEL_H

#include <expected>
#include <condition_variable>
#include <cstddef>
#include <queue>
#include <mutex>
#include <utility>
#include <stop_token>

#include "block_strategy.h"

namespace xtd
{
    template<typename T>
    class channel_writer;
    
    template<typename T>
    class channel_reader;
    
    enum class channel_read_errors {
        REQUEST_CANCELLED = 1,
        CHANNEL_EMPTY = 2,
    };

    template<typename T>
    class channel
    {
    private:
        friend class channel_writer<T>;
        friend class channel_reader<T>;

        struct can_read_predicate
        {
            const channel& m_channel;

            [[nodiscard]]
            bool operator()() const noexcept
            {
                return m_channel.m_writer_completed || !m_channel.m_queue.empty();
            }
        };

        struct can_write_predicate
        {
            const channel& m_channel;

            [[nodiscard]]
            bool operator()() const noexcept
            {
                return m_channel.m_writer_completed || !m_channel.full();
            }
        };

        std::queue<T> m_queue{};

        std::condition_variable_any m_not_full{};
        std::condition_variable_any m_not_empty{};
        mutable std::mutex m_mutex{};

        std::size_t m_read_waiters = 0;
        std::size_t m_write_waiters = 0;

        const std::size_t m_capacity;
        channel_writer<T> m_writer;
        channel_reader<T> m_reader;

        bool m_writer_completed = false;

        [[nodiscard]]
        inline bool full() const noexcept
        {
            return m_capacity > 0 && m_queue.size() >= m_capacity;
        }

        bool internal_wait_to_write(std::unique_lock<std::mutex>& lock, const std::stop_token stop_token)
        {
            if (!stop_token.stop_requested() && !m_writer_completed && full())
            {
                ++m_write_waiters;
                m_not_full.wait(lock, stop_token, can_write_predicate{*this});
                --m_write_waiters;
            }

            return !stop_token.stop_requested() && !m_writer_completed && !full();
        }

        template<typename... Args>
        bool emplace(const std::stop_token stop_token, const block_strategy strategy, Args&&... args)
        requires std::constructible_from<T, Args...>
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT)
            {
                if (!internal_wait_to_write(lock, stop_token)) {
                    return false;
                }
            }
            else {
                if (m_writer_completed || full()) {
                    return false;
                }
            }

            m_queue.emplace(std::forward<Args>(args)...);
            lock.unlock();
            if (m_read_waiters > 0) {
                m_not_empty.notify_one();
            }
            return true;
        }

        void complete_writer()
        {
            {
                std::scoped_lock lock(m_mutex);
                m_writer_completed = true;
            }
            m_not_empty.notify_all();
            m_not_full.notify_all();
        }

        bool wait_to_read(const std::stop_token stop_token)
        {
            std::unique_lock lock(m_mutex);
            return internal_wait_to_read(lock, stop_token).has_value();
        }

        std::expected<void, channel_read_errors> internal_wait_to_read(std::unique_lock<std::mutex>& lock, const std::stop_token stop_token)
        {
            if (!stop_token.stop_requested() && !m_writer_completed && m_queue.empty())
            {
                ++m_read_waiters;
                m_not_empty.wait(lock, stop_token, can_read_predicate{*this});
                --m_read_waiters;
            }

            if (stop_token.stop_requested()) {
                return std::unexpected<channel_read_errors>(channel_read_errors::REQUEST_CANCELLED);
            }

            if (m_queue.empty()) {
                return std::unexpected<channel_read_errors>(channel_read_errors::CHANNEL_EMPTY);
            }

            return {};
        }

        [[nodiscard]]
        std::expected<T, channel_read_errors> read(const std::stop_token stop_token, const block_strategy strategy)
        {
            std::unique_lock lock(m_mutex);

            if (strategy == block_strategy::WAIT)
            {
                const auto result = internal_wait_to_read(lock, stop_token);
                if (!result) {
                    return std::unexpected<channel_read_errors>(result.error());
                }
            }
            else {
                if (m_queue.empty()) {
                    return std::unexpected<channel_read_errors>(channel_read_errors::CHANNEL_EMPTY);
                }
            }

            
            std::expected<T, channel_read_errors> result(std::in_place, std::move(m_queue.front()));
            m_queue.pop();
            
            lock.unlock();
            if (m_write_waiters > 0) {
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
    };
}

#endif // CHANNEL_CHANNEL_IMPL_H
