#include "chat/server.h"
#include "logging/logging.h"
#include "logging/sinks/file_sink.h"
#include "logging/sinks/otel_sink.h"
#include <filesystem>
#include <stop_token>
#include <thread>

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

        xtd::logger::instance().add_sink<xtd::otel_sink>(xtd::otel_sink_opts {
            .min_log_level = xtd::log_level::trace,

            
            // Grafana Loki -- ./docker-compose/grafana.yaml
            // RUN -> docker compose -f grafana.yaml up
            .endpoint = "http://localhost:4318/v1/logs", 

            // OpenObserve -- ./docker-compose/openobserve.yaml
            // RUN -> docker compose -f openobserve.yaml up
            //.endpoint = "http://localhost:5080/api/default/v1/logs", 
            //.auth_token = "Basic cm9vdEBleGFtcGxlLmNvbTpBZG1pbiMxMjM0",

            .service_namespace = "chat-app",
            .service_name = "chat-app",
            .service_version = "1.0.0",
            .service_instance_id = "instance-1",
            .environment_name = "development",
        });

        std::jthread hb{[](std::stop_token token) {
            using namespace std::chrono_literals;
            std::mutex mutex;
            std::condition_variable_any cv;
            std::unique_lock lock{mutex};

            while (!token.stop_requested()) {
                cv.wait_for(lock, token, 1s, [] { return false; });

                if (token.stop_requested())
                    break;

                xtd::LOG_INFO("Heartbeat -- DateTime: {}", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
            }
        }};

        xtd::LOG_INFO("Starting chat server...");

        const int port = std::stoi(argc > 1 ? argv[1] : "9090");

        if (port < 1 || port > 65535) {
            throw std::invalid_argument("invalid port");
        }

        {
            server server(static_cast<std::uint16_t>(port));
            server.run();
            hb.request_stop();
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