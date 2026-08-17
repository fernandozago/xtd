#ifndef CHANNEL_READER_H
#define CHANNEL_READER_H

#include "channel/channel_enums.h"
#include "channel_impl.h"
#include <expected>
#include <stop_token>
#include <generator>

namespace xtd
{
    template<typename T>
    class channel_reader
    {

    private:
        channel<T>& m_channel;
        
    public:
        explicit channel_reader(channel<T>& channel) noexcept
            : m_channel(channel)
        {
        }

        [[nodiscard]]
        std::expected<T, channel_read_errors> try_read()
        {
            return m_channel.read({}, block_strategy::TRY);
        }

        [[nodiscard]]
        std::expected<T, channel_read_errors> read(std::stop_token stop_token = {})
        {
            return m_channel.read(stop_token, block_strategy::WAIT);
        }

        std::generator<T> read_all(std::stop_token stop_token = {}) {
            return m_channel.read_all(stop_token);
        }

        [[nodiscard]]
        bool wait_to_read(std::stop_token stop_token = {})
        {
            return m_channel.wait_to_read(stop_token);
        }

        [[nodiscard]]
        std::size_t size() const
        {
            return m_channel.size();
        }
    };
}

#endif // CHANNEL_READER_H
