#include "chat/server.h"
#include "logging/logging.h"
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
        logger::instance().add_sink<log_sink>(log_sink_opts {
            .min_log_level = log_level::information,
        });

        logger::instance().add_sink<file_sink>(file_sink_opts {
            .file_path = exe_fullpath + ".plain.log",
        });

        logger::instance().add_sink<file_sink>(file_sink_opts {
            .min_log_level = log_level::trace,
            .file_path = exe_fullpath + ".trace.log",
            .use_local_time = false
        });

        logger::instance().add_sink<otel_sink>(otel_sink_opts {
            .min_log_level = log_level::trace,
            .endpoint = "http://localhost:5080/api/default/v1/logs",
            .auth_token = "Basic cm9vdEBvdGVsLmNvbTpyb290cHc=",
            .stream_name = "chat-app",
            .service_name = "chat-app-v1",
        });

        LOG_TRACE("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);
        LOG_DEBUG("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);
        LOG_INFO("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);
        LOG_WARN("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);
        LOG_ERROR("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);
        LOG_FATAL("Int: {0}, Float: {1:.2f} -- {1}", 3, 3.14159);

        try {
            throw std::runtime_error{"This is a test exception"};
        }
        catch (const std::exception& ex) {
            LOG_WARN_EX(ex, "Caught an exception: {}", ex.what());
        }
        
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
        LOG_FATAL_EX(ex, "Error on main");
        return 1;
    }
}