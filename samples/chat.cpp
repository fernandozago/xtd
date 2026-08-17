#include "chat/server.h"
#include "logging/logging.h"
#include "logging/sinks/file_sink.h"
#include "logging/sinks/curl_otel_sink.h"
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
        xtd::logger::instance().add_sink<xtd::log_sink>(xtd::log_sink_opts {
            .min_log_level = xtd::log_level::information,
        });

        xtd::logger::instance().add_sink<xtd::file_sink>(xtd::file_sink_opts {
            .file_path = exe_fullpath + ".plain.log",
        });

        xtd::logger::instance().add_sink<xtd::file_sink>(xtd::file_sink_opts {
            .min_log_level = xtd::log_level::trace,
            .file_path = exe_fullpath + ".trace.log",
            .use_local_time = false
        });

        xtd::logger::instance().add_sink<xtd::curl_otel_sink>(xtd::otel_sink_opts {
            .min_log_level = xtd::log_level::trace,
            .endpoint = "http://localhost:5080/api/default/v1/logs",
            .auth_token = "Basic cm9vdEBvdGVsLmNvbTpyb290cHc=",
            .service_namespace = "chat-app",
            .service_name = "chat-app",
            .service_version = "1.0.0",
            .service_instance_id = "instance-1",
            .environment_name = "development",
        });
       
        xtd::LOG_INFO("Starting chat server...");

        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        {
            server server(static_cast<std::uint16_t>(port));
            server.run();
        }

        xtd::LOG_INFO("Server stopped");

        return 0;
    }
    catch (const std::exception& ex)
    {
        xtd::LOG_FATAL_EX(ex, "Error on main");
        return 1;
    }
}