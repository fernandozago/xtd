#pragma once

#include <format>
#include <thread>

#include "pipeline/pipeline.h"
#include "../utils/utils.h"

/*
Accepts any type of server, as long as it implements the following methods:
- void reply(const int client_id, const std::string_view message)
- void broadcast(const std::string_view message)
*/

static constexpr const char* MOTD = R"(
[✨ Welcome `{}`!]
[💬 Type a message and press Enter to chat.]
[🏷️ Change your name with `/name <your_name>`.]
[🌸 Be kind, have fun, and enjoy your stay!]
)""\n";

template<typename TServer>
concept server_requirements =
    requires(int unique_id, TServer& server, std::string_view message)
    {
        server.reply(unique_id, message);
        server.broadcast(message);
    };

template<typename TServer>
class connection_handler
{
private:
    TServer& m_server;
    const int m_unique_id;
    std::string m_name;
    xtd::pipeline m_pipeline;
    xtd::pipe_writer& m_writer;
    std::jthread m_thread;
    bool m_closed = false;

public:
    connection_handler(int unique_id, TServer& server) requires server_requirements<TServer>
        : m_server(server)
        , m_unique_id(unique_id)
        , m_name(std::to_string(unique_id))
        , m_pipeline()
        , m_writer(m_pipeline.writer())
        , m_thread(&connection_handler::process_incoming_data, this)
    {
        m_server.reply(unique_id, std::format(MOTD, m_name ));
        m_server.broadcast(std::format("[📣: `{}` joined the chat! Say hello!]\n", m_name));
    }
    
    connection_handler(const connection_handler&) = delete;
    connection_handler& operator=(const connection_handler&) = delete;

    ~connection_handler()
    {
        m_server.broadcast(std::format("[📣: `{}` has left the chat.]\n", m_name));
        close();
    }

    void receive_data(const std::byte* data, const std::size_t size)
    {
        if (!m_closed) {
            m_writer.write(data, size);
        }
    }

    void close() noexcept
    {
        m_writer.complete();
        m_closed = true;
    }
private:
    void process_incoming_data() noexcept requires server_requirements<TServer>
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
                            std::string old_name = m_name;
                            m_name = message.substr(6);
                            m_server.broadcast(std::format("[📣: `{}` is now known as `{}`]\n", old_name, m_name));
                        }
                        else {
                            m_server.broadcast(std::format("[{}]: {}\n", m_name, message));
                        }
                    }

                    data = data.slice(++newline, data.end());
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