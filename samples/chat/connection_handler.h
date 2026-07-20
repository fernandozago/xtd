#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <functional>

#include "pipeline/pipeline.h"
#include "../utils/utils.h"

using reply_fn = std::function<void(const int fd, const std::string_view)>;
using broadcast_fn = std::function<void(const int fd, const std::string_view)>;
using setname_fn = std::function<void(const int fd, const std::string_view)>;

class connection_handler
{
public:
    connection_handler(int unique_id, reply_fn sendFn, broadcast_fn broadcastFn, setname_fn setnameFn)
        : m_pipeline()
        , m_writer(m_pipeline.writer())
        , fd(unique_id)
        , m_reply(std::move(sendFn))
        , m_broadcast(std::move(broadcastFn))
        , m_setname(std::move(setnameFn))
        , m_thread(&connection_handler::process_incoming_data, this)
    {
        m_reply(unique_id, std::format("Hello `{}`! Welcome to the chat server. Type `/name <your_name>` to set your name.", unique_id));
    }

    ~connection_handler()
    {
        close();
    }

    connection_handler(const connection_handler&) = delete;
    connection_handler& operator=(const connection_handler&) = delete;

    void receive_data(const std::byte* data, std::size_t size)
    {
        if (!m_closed) {
            m_writer.write(data, size);
        }
    }

    void close() noexcept
    {
        if (!m_closed) {
            m_writer.complete();
            if (m_thread.joinable()) {
                m_thread.join();
            }
            m_closed = true;
        }
    }

private:
    xtd::pipeline m_pipeline;
    xtd::pipe_writer& m_writer;
    const int fd;

    reply_fn m_reply;
    broadcast_fn m_broadcast;
    setname_fn m_setname;
    std::jthread m_thread;
    bool m_closed = false;

    void process_incoming_data() noexcept
    {
        auto& reader = m_pipeline.reader();
        try
        {
            while (const xtd::read_result result = reader.read())
            {
                xtd::segmented_byte_view data = result.buffer();

                while (xtd::position newline = data.position_of('\n'))
                {
                    std::string message = data.slice(newline).to_string();
                    if (!message.empty()) {
                        if (message.starts_with("/name ")) {
                            m_setname(fd, message.substr(6));
                        }
                        else {
                            m_broadcast(fd, message);
                        }
                    }
                    data = data.slice(newline + 1, data.end());
                }

                reader.advance(data.begin(), data.end());

                if (result.completed()) {
                    break;
                }
            }
        }
        catch (const std::exception& ex)
        {
            println_locked("client error: {}", ex.what());
        }
        catch (...)
        {
            println_locked("client error: unknown exception");
        }

        reader.complete();
        println_locked("client disconnected");
    }
};