#pragma once

#include <cstddef>
#include <stop_token>
#include <string>
#include <thread>

#include "commands_parser.h"
#include "pipeline/pipeline.h"
#include "../utils/utils.h"
#include "pipeline/segmented_byte_view.h"

/*
Accepts any server type implementing:

- void reply(int client_id, std::string_view message) noexcept
- void broadcast(std::string_view message) noexcept
*/

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
    xtd::pipe_writer m_writer;

    std::atomic_bool m_closed = false;
    std::jthread m_thread;

public:
    connection_handler(int unique_id, TServer& server)
        : m_unique_id(unique_id)
        , m_server(server)
        , m_name(std::to_string(unique_id))
        , m_pipeline()
        , m_writer(m_pipeline)
        , m_thread([this](std::stop_token stopToken) {
            process_incoming_data(stopToken);
        })
    {
        m_server.reply(unique_id, std::format(MOTD, m_name));
        m_server.broadcast(std::format("[📣: `{}` joined the chat! Say hello!]\n", m_name));
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
        println_locked("handler for client fd {} (AKA: `{}`) destroyed", m_unique_id, m_name);
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
    void process_incoming_data(std::stop_token stop_token) noexcept
    {
        xtd::pipe_reader reader(m_pipeline);

        try
        {
            while (const xtd::read_result result = reader.read())
            {
                xtd::segmented_byte_view data = result.buffer();
                while (xtd::position newLine = data.position_of('\n')) {
                    if (stop_token.stop_requested()) break;

                    // Extract the exact line of data up to (excluding) the newline character
                    xtd::segmented_byte_view line_bytes = data.slice(newLine);
                    
                    if (!line_bytes.empty()) {
                        //check the last byte of the line for carriage return (\r) and remove it if present
                        if (line_bytes[xtd::from_end(1)] == std::byte('\r')) {
                            line_bytes = line_bytes.slice(newLine - 1);
                        }
    
                        process_command(line_bytes);
                    }

                    // Advance the data view to exclude the processed line and the newline character
                    data.slice_in_place(newLine + 1, data.end());
                }

                reader.advance(data.begin(), data.end());

                if (result.completed() || stop_token.stop_requested()) {
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

    void process_command(const xtd::segmented_byte_view& line_bytes)
    {
        const auto [cmd_type, argument] = commands_parser::parse_command(line_bytes);

        switch (cmd_type)
        {
            case command_type::message:
                m_server.broadcast(std::format("[💬 {}]: {}\n", m_name, argument));
                break;

            case command_type::name:
            {
                if (argument.empty() || argument.size() > 32) {
                    m_server.reply(m_unique_id, "[⚠️: Name must be between 1 and 32 characters.]\n");
                    break;
                }

                std::string old_name = std::exchange(m_name, std::move(argument));
                m_server.broadcast(std::format("[📣: `{}` is now known as `{}`]\n", old_name, m_name));
                break;
            }

            case command_type::help:
                m_server.reply(m_unique_id, HELP_RESPONSE);
                break;

            case command_type::quit:
                if (!argument.empty()) {
                    m_server.broadcast(std::format("[💬 {}]: {}\n", m_name, argument));
                }
                m_server.reply(m_unique_id, "[📣: Disconnecting... Bye! 👋]\n");
                m_server.user_quit(m_unique_id);
                break;

            case command_type::unknown:
            default:
                m_server.reply(m_unique_id, UNKNOWN_COMMAND_RESPONSE);
                break;
        }
    }
};