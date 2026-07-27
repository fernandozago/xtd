#ifndef CHANNEL_CHANNEL_READER_H
#define CHANNEL_CHANNEL_READER_H

#include "channel_impl.h"

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