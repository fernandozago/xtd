#ifndef CHANNEL_READER_H
#define CHANNEL_READER_H

#include <expected>
#include <stop_token>

namespace xtd
{
    template<typename T>
    class channel;

    enum class channel_read_errors {
        REQUEST_CANCELLED = 1,
        CHANNEL_EMPTY = 2,
    };

    template<typename T>
    class channel_reader
    {

    private:
        friend class channel<T>;
        channel<T>& m_channel;
        
        public:
        explicit channel_reader(channel<T>& channel) noexcept;

        [[nodiscard]]
        std::expected<T, channel_read_errors> try_read();

        [[nodiscard]]
        std::expected<T, channel_read_errors> read(std::stop_token stopToken = {});

        [[nodiscard]]
        bool wait_to_read(std::stop_token stopToken = {});

        [[nodiscard]]
        std::size_t size() const;
    };
}

#endif // CHANNEL_READER_H
