#ifndef CHANNEL_CHANNEL_READER_H
#define CHANNEL_CHANNEL_READER_H

#include <cstddef>
#include <optional>
#include <stop_token>

#include "channel_impl.h"
#include "block_strategy.h"

namespace xtd
{
    template<typename T>
    class channel_reader
    {

    private:
        friend class channel<T>;
        channel<T>& m_channel;

        explicit channel_reader(channel<T>& channel) noexcept
            : m_channel(channel)
        {
        }

    public:
        channel_reader() = delete;
        channel_reader(const channel_reader&) = delete;
        channel_reader& operator=(const channel_reader&) = delete;
        channel_reader(channel_reader&&) = delete;
        channel_reader& operator=(channel_reader&&) = delete;

        [[nodiscard]]
        std::optional<T> try_read()
        {
            return m_channel.read({}, block_strategy::TRY);
        }

        [[nodiscard]]
        std::optional<T> read(std::stop_token stopToken = {})
        {
            return m_channel.read(stopToken, block_strategy::WAIT);
        }

        [[nodiscard]]
        std::size_t size() const
        {
            return m_channel.size();
        }
    };
}

#endif