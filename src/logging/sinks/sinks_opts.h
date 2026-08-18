#ifndef SINKS_OPTS
#define SINKS_OPTS

#include "logging/log_message.h"
#include <stdexcept>
#include <unistd.h>

namespace xtd 
{

    struct log_sink_opts {
        log_level min_log_level = log_level::information;
        int fd = STDOUT_FILENO;
        bool use_local_time = true;
        bool use_structured_log = false;
        bool use_colors = true;
        bool flush_on_write = true;

        void validate() const {
        }
    };

    struct file_sink_opts {
        log_level min_log_level = log_level::information;
        std::string file_path;
        bool use_local_time = true;
        bool flush_on_write = true;

        void validate() const {
            if (file_path.empty()) {
                throw std::invalid_argument{"file path cannot be empty"};
            }
        }
    };

    struct otel_sink_opts {
        log_level min_log_level = log_level::information;
        std::string endpoint;
        std::string auth_token = "";
        std::string service_namespace;
        std::string service_name;
        std::string service_version;
        std::string service_instance_id;
        std::string environment_name;
        std::size_t batch_size = 100;

        bool use_local_time = false;

        void validate() const {
            if (endpoint.empty()) {
                throw std::invalid_argument{"endpoint cannot be empty"};
            }

            if (service_namespace.empty()) {
                throw std::invalid_argument{"service_namespace cannot be empty"};
            }

            if (service_name.empty()) {
                throw std::invalid_argument{"service_name cannot be empty"};
            }

            if (service_version.empty()) {
                throw std::invalid_argument{"service_version cannot be empty"};
            }

            if (service_instance_id.empty()) {
                throw std::invalid_argument{"service_instance_id cannot be empty"};
            }

            if (environment_name.empty()) {
                throw std::invalid_argument{"environment_name cannot be empty"};
            }

            if (batch_size == 0) {
                throw std::invalid_argument{"batch_size must be greater than zero"};
            }
        }
    };

}

#endif