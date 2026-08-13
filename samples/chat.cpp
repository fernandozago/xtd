#include "chat/server.h"
#include "logging/logging.h"
#include "logging/sinks/console_sink.h"
#include "logging/sinks/file_sink.h"
#include "logging/sinks/otel_sink.h"
#include <filesystem>

/*
 Simple chat server
 
 Build and run:
  make run FILE=samples/chat.cpp               #Uses default port 9090
  make run FILE=samples/chat.cpp ARGS="8080"   #Uses port 8080
 
 Connect using netcat (nc):
   nc <server-ip-address> <port>
 
 Inside the chat:
   - Type a message and press Enter to send it.
   - Use `/name <your_name>` to change your name.
   - Press Ctrl+C to disconnect.
 
 Make sure the selected port is allowed through the server's firewall.
 */

int main(int argc, char** argv)
{
    try
    {
        const std::string exe_fullpath = std::filesystem::read_symlink("/proc/self/exe").string();
        logger::instance().add_sink<console_sink>(console_sink_opts {
            .min_log_level = log_level::information,
            .use_local_time = true,
            .use_colors = true,
        });

        logger::instance().add_sink<file_sink>(file_sink_opts {
            .file_path = exe_fullpath + ".structured.log",
            .use_local_time = false,
            .use_structured_log = true,
        });

        logger::instance().add_sink<file_sink>(file_sink_opts {
            .file_path = exe_fullpath + ".plain.log",
        });

        logger::instance().add_sink<file_sink>(file_sink_opts {
            .file_path = exe_fullpath + ".trace.log",
            .min_log_level = log_level::trace,
        });

        logger::instance().add_sink<otel_sink>(otel_sink_opts {
            .endpoint = "http://localhost:5080/api/default/v1/logs",
            .auth_token = "Basic cm9vdEBvdGVsLmNvbTpyb290cHc=",
            .stream_name = "chat-app",
            .service_name = "chat-app-v1",
            .min_log_level = log_level::trace,
            .use_local_time = false,
        });

        LOG_TRACE("Int: {}, Float: {:.2f}", 3, 3.14159);
        LOG_INFO("Starting chat server...");

        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        {
            server server(static_cast<std::uint16_t>(port));
            server.run();
        }

        LOG_INFO("Server stopped");

        return 0;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("error: {}", ex.what());
        return 1;
    }
}