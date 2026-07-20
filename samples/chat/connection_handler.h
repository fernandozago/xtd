#pragma once

#include <thread>

#include "pipeline/pipeline.h"
#include "../utils/utils.h"

template<typename TServer>
concept chat_server =
    requires(TServer& server, int client_id, std::string_view message)
    {
        server.reply(client_id, message);
        server.set_name(client_id, message);
        server.broadcast(client_id, message);
    };

template<typename TServer>
class connection_handler
{
public:
    connection_handler(int unique_id, TServer& server) requires chat_server<TServer>
        : m_server(server)
        , m_pipeline()
        , m_writer(m_pipeline.writer())
        , fd(unique_id)
        , m_thread([this] {
            process_incoming_data();
        })
    {
        m_server.reply(
            unique_id,
            std::format(
                "Hello `{}`! Welcome to the chat server. "
                "Type `/name <your_name>` to set your name.",
                unique_id
            )
        );
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
            m_closed = true;
        }
    }

private:
    TServer& m_server;
    xtd::pipeline m_pipeline;
    xtd::pipe_writer& m_writer;
    const int fd;

    std::jthread m_thread;
    bool m_closed = false;

    void process_incoming_data() noexcept requires chat_server<TServer>
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
                            m_server.set_name(fd, message.substr(6));
                        }
                        else {
                            m_server.broadcast(fd, std::move(message));
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