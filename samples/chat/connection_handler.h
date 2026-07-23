#pragma once

#include <cstddef>
#include <stop_token>
#include <string>
#include <thread>

#include "pipeline/pipeline.h"
#include "../utils/utils.h"

/*
Accepts any server type implementing:

- void reply(int client_id, std::string_view message) noexcept
- void broadcast(std::string_view message) noexcept
*/

inline constinit std::string_view COMMAND_NAME = "/name ";
inline constinit std::string_view COMMAND_HELP = "/help";
inline constinit std::string_view COMMAND_QUIT = "/quit";

inline constinit std::string_view UNKNOWN_COMMAND_RESPONSE = "[⚠️: Unknown command. -- Send `/help` to see more options]\n";

inline constinit std::string_view HELP_RESPONSE = R"(
[ℹ️: Use `/help` to see this help message.]
[ℹ️: Use `/name <your_name>` to change your name.]
[ℹ️: Use `/quit` to disconnect from the chat.]
)""\n";

inline constexpr std::string_view MOTD = R"(
[✨ Welcome `{}`]
[💬 Type a message and press Enter to chat]
[🌸 Be kind, have fun, and enjoy your stay]
[ℹ️: Send `/help` for more information]
)""\n";

inline static bool start_with_ingnore_case(const std::string_view& prefix, const std::string_view& str) {
    if (str.size() < prefix.size()) return false;
    return is_equal_ignore_case(str.substr(0, prefix.size()), prefix);
}

template<class TServer>
class connection_handler
{
private: 
    const int m_unique_id;
    TServer& m_server;
    std::string m_name;

    xtd::pipeline m_pipeline;
    xtd::pipe_writer& m_writer;

    std::atomic_bool m_closed = false;
    std::jthread m_thread;

public:
    connection_handler(int unique_id, TServer& server)
        : m_unique_id(unique_id)
        , m_server(server)
        , m_name(std::to_string(unique_id))
        , m_writer(m_pipeline.writer())
    {
        m_server.reply(unique_id, std::format(MOTD, m_name));
        m_server.broadcast(std::format("[📣: `{}` joined the chat! Say hello!]\n", m_name));
        m_thread = std::jthread([this](std::stop_token stopToken) {
            process_incoming_data(stopToken);
        });
    }

    connection_handler(const connection_handler&) = delete;
    connection_handler& operator=(const connection_handler&) = delete;

    ~connection_handler()
    {
        close();
        if (m_thread.joinable()) {
            m_thread.join();
        }
        m_server.broadcast(std::format("[📣: `{}` has left the chat.]\n", m_name));
        println_locked("handler for client fd {} (Name: `{}`) destroyed", m_unique_id, m_name);
    }

    std::string& name() noexcept { return m_name; }

    void receive_data(const std::byte* data, std::size_t size)
    {
        if (!m_closed) {
            m_writer.write(data, size);
        }
    }

    void close() noexcept
    {
        if (!m_closed.exchange(true)) {
            m_thread.request_stop();
            m_writer.complete();
        }
    }

private:
    void process_incoming_data(std::stop_token stopToken) noexcept
    {
        auto& reader = m_pipeline.reader();

        try
        {
            while (const xtd::read_result result = reader.read())
            {
                xtd::segmented_byte_view data = result.buffer();
                while (xtd::position newLine = data.position_of('\n')) {
                    if (stopToken.stop_requested()) break;

                    // Extract the exact line of data up to (excluding) the newline character
                    auto line_bytes = data.slice(newLine);
                    
                    //check the last byte of the line for carriage return (\r) and remove it if present
                    if (xtd::position carriege_return_pos = line_bytes.position_of('\r')) {
                        line_bytes = line_bytes.slice(carriege_return_pos);
                    }

                    process_message(line_bytes);
                    data = data.slice(newLine + 1, data.end());
                }

                reader.advance(data.begin(), data.end());

                if (result.completed())
                {
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
    }

    void process_message(const xtd::segmented_byte_view& data) noexcept
    {
        if (data.empty()) return;
        const std::string message = data.to_string();

        if (message.starts_with('/')) {
            process_command(message);
            return;
        }
        m_server.broadcast(std::format("[💬 {}]: {}\n", m_name, message));
    }

    void process_command(const std::string& command) noexcept
    {
        if (start_with_ingnore_case(COMMAND_NAME, command))
        {
            const std::string new_name = command.substr(COMMAND_NAME.size());
            if (new_name.empty() || new_name.size() > 32) {
                m_server.reply(m_unique_id, "[⚠️: Name must be between 1 and 32 characters.]\n");
                return;
            }
            
            const std::string old_name = std::exchange(m_name, std::move(new_name));
            m_server.broadcast(std::format("[📣: `{}` is now known as `{}`]\n", old_name, m_name));
            return;
        }

        if (start_with_ingnore_case(COMMAND_HELP, command))
        {
            m_server.reply(m_unique_id, HELP_RESPONSE);
            return;
        }

        if (start_with_ingnore_case(COMMAND_QUIT, command))
        {
            m_server.reply(m_unique_id, "[📣: Disconnecting... Bye! 👋]\n");
            m_server.user_quit(m_unique_id);
            return;
        }

        m_server.reply(m_unique_id, UNKNOWN_COMMAND_RESPONSE);
    }
};