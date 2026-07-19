#ifndef CHANNEL_CHANNEL_WRITER_H
#define CHANNEL_CHANNEL_WRITER_H

#include "block_strategy.h"

#include <concepts>
#include <utility>

namespace xtd
{
    template<typename T>
    class channel;

    template<typename T>
    class channel_writer
    {
    public:
        channel_writer(const channel_writer&) = delete;
        channel_writer& operator=(const channel_writer&) = delete;
        channel_writer(channel_writer&&) = delete;
        channel_writer& operator=(channel_writer&&) = delete;

        [[nodiscard]]
        bool push(const T& value)
        {
            return m_channel.emplace(block_strategy::WAIT, value);
        }

        [[nodiscard]]
        bool push(T&& value)
        {
            return m_channel.emplace(block_strategy::WAIT, std::move(value));
        }

        [[nodiscard]]
        bool try_push(const T& value)
        {
            return m_channel.emplace(block_strategy::TRY, value);
        }

        [[nodiscard]]
        bool try_push(T&& value)
        {
            return m_channel.emplace(block_strategy::TRY, std::move(value));
        }

        template<typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]]
        bool emplace(Args&&... args)
        {
            return m_channel.emplace(block_strategy::WAIT, std::forward<Args>(args)...);
        }

        template<typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]]
        bool try_emplace(Args&&... args)
        {
            return m_channel.emplace(block_strategy::TRY, std::forward<Args>(args)...);
        }

        void complete()
        {
            m_channel.complete();
        }

    private:
        friend class channel<T>;

        explicit channel_writer(channel<T>& channel) noexcept
            : m_channel(channel)
        {
        }

        channel<T>& m_channel;
    };
}

#endif