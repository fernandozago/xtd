#ifndef CHANNEL_CHANNEL_WRITER_H
#define CHANNEL_CHANNEL_WRITER_H

#include "block_strategy.h"

#include <concepts>
#include <utility>

namespace xtd
{
    template<typename T>
    class channel_writer
    {
    public:
        // Enqueues a copy.
        //
        // Returns false when the channel has been completed.
        //
        // bounded_channel: blocks while full.
        // unbounded_channel: does not block.
        [[nodiscard]]
        virtual bool push(const T& value) = 0;

        // Enqueues by move.
        //
        // Returns false when the channel has been completed.
        //
        // bounded_channel: blocks while full.
        // unbounded_channel: does not block.
        [[nodiscard]]
        virtual bool push(T&& value) = 0;

        // Attempts to enqueue a copy without waiting.
        //
        // Returns false when the channel is full or completed.
        [[nodiscard]]
        virtual bool try_push(const T& value) = 0;

        // Attempts to enqueue by move without waiting.
        //
        // Returns false when the channel is full or completed.
        [[nodiscard]]
        virtual bool try_push(T&& value) = 0;

        // Constructs T and enqueues it.
        template<typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]]
        bool emplace(Args&&... args) {
            return push(T(std::forward<Args>(args)...));
        }

        // Constructs T and attempts to enqueue it without waiting.
        template<typename... Args>
        requires std::constructible_from<T, Args...>
        [[nodiscard]]
        bool try_emplace(Args&&... args) {
            return try_push(T(std::forward<Args>(args)...));
        }

        // Completes the channel for writing.
        virtual void complete() = 0;

    };


    // Implementation detail
    template<typename T, typename TChannel>
    class channel_writer_impl final : public channel_writer<T>
    {
    public:
        explicit channel_writer_impl(TChannel& channel) noexcept
            : m_channel(channel)
        {
        }

    protected:
        [[nodiscard]]
        bool push(const T& value) override {
            return m_channel.push(value, block_strategy::WAIT);
        }

        [[nodiscard]]
        bool push(T&& value) override {
            return m_channel.push(std::move(value), block_strategy::WAIT);
        }

        [[nodiscard]]
        bool try_push(const T& value) override {
            return m_channel.push(value, block_strategy::TRY);
        }

        [[nodiscard]]
        bool try_push(T&& value) override {
            return m_channel.push(std::move(value), block_strategy::TRY);
        }

        void complete() override {
            m_channel.complete();
        }

    private:
        TChannel& m_channel;
    };
}

#endif // CHANNEL_CHANNEL_WRITER_H