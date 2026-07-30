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

        struct notifier
        {
            std::condition_variable_any& m_cv;
            bool m_should_notify = false;

            explicit notifier(std::condition_variable_any& cv, bool should_notify = false) noexcept
                : m_cv(cv)
                , m_should_notify(should_notify)
            {
            }

            notifier(const notifier&) = delete;
            notifier& operator=(const notifier&) = delete;

            void arm() noexcept
            {
                m_should_notify = true;
            }

            ~notifier()
            {
                if (m_should_notify) {
                    m_cv.notify_one();
                }
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

        template<typename... Args>
        bool emplace(const std::stop_token stop_token, const block_strategy strategy, Args&&... args)
        requires std::constructible_from<T, Args...>
        {
            notifier notify_reader(m_not_empty);
            std::unique_lock lock(m_mutex);

            if (stop_token.stop_requested()) {
                return false;
            }

            if (strategy == block_strategy::WAIT && !m_writer_completed && full())
            {
                ++m_write_waiters;
                m_not_full.wait(lock, stop_token, [this] {
                    return m_writer_completed || !full();
                });
                --m_write_waiters;

                if (stop_token.stop_requested()) {
                    return false;
                }
            }

            if (m_writer_completed || full()) {
                return false;
            }

            if (m_read_waiters > 0) {
                notify_reader.arm();
            }
            
            m_queue.emplace(std::forward<Args>(args)...);
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
            if (stop_token.stop_requested()) {
                return std::unexpected<channel_read_errors>(channel_read_errors::REQUEST_CANCELLED);
            }

            if (!m_writer_completed && m_queue.empty())
            {
                ++m_read_waiters;
                m_not_empty.wait(lock, stop_token, [this] {
                    return m_writer_completed || !m_queue.empty();
                });
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
            notifier notify_writer(m_not_full);
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

            if (m_write_waiters > 0) {
                notify_writer.arm();
            }

            std::expected<T, channel_read_errors> result(std::in_place, std::move(m_queue.front()));
            m_queue.pop();
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
