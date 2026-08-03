#ifndef CHANNEL_WRITER_H
#define CHANNEL_WRITER_H

#include <stop_token>

namespace xtd
{
    template<typename T>
    class channel;
    
    template<typename T>
    class channel_writer
    {   
    private:
        friend class channel<T>;
        channel<T>& m_channel;
        
    public:
        explicit channel_writer(channel<T>& channel) noexcept;

        [[nodiscard]]
        bool push(const T& value);

        [[nodiscard]]
        bool push(std::stop_token stop_token, const T& value);

        [[nodiscard]]
        bool push(T&& value);

        [[nodiscard]]
        bool push(std::stop_token stop_token, T&& value);

        template<typename... Args>
        [[nodiscard]]
        bool emplace(Args&&... args);

        template<typename... Args>
        [[nodiscard]]
        bool emplace(std::stop_token stop_token, Args&&... args);

        [[nodiscard]]
        bool try_push(const T& value);

        [[nodiscard]]
        bool try_push(T&& value);

        template<typename... Args>
        [[nodiscard]]
        bool try_emplace(Args&&... args);

        void complete();
    };
}

#endif // CHANNEL_WRITER_H
