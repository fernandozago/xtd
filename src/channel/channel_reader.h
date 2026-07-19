#ifndef CHANNEL_CHANNEL_READER_H
#define CHANNEL_CHANNEL_READER_H

#include "block_strategy.h"

#include <cstddef>
#include <optional>

namespace xtd
{
	template<typename T>
	class channel;

    template<typename T>
    class channel_reader
    {
    public:
        channel_reader(const channel_reader&) = delete;
        channel_reader& operator=(const channel_reader&) = delete;
        channel_reader(channel_reader&&) = delete;
        channel_reader& operator=(channel_reader&&) = delete;

        [[nodiscard]]
        std::optional<T> try_read()
        {
            return m_channel.read(block_strategy::TRY);
        }

        [[nodiscard]]
        std::optional<T> read()
        {
            return m_channel.read(block_strategy::WAIT);
        }

        [[nodiscard]]
        std::size_t size() const
        {
            return m_channel.size();
        }

    private:
        friend class channel<T>;

        explicit channel_reader(
            channel<T>& channel) noexcept
            : m_channel(channel)
        {
        }

        channel<T>& m_channel;
    };
}

#endif