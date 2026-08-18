#ifndef SINKS_OPTS
#define SINKS_OPTS

#include "logging/log_message.h"
#include <cctype>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace xtd 
{
    namespace utils {
        static void validate_required_string(std::string_view name, const std::string& value) {
            if (value.empty()) {
                throw std::invalid_argument{std::string{name} + " cannot be empty"};
            }

            bool has_non_whitespace = false;
            for (const char ch : value) {
                if (!std::isspace(static_cast<unsigned char>(ch))) {
                    has_non_whitespace = true;
                    break;
                }
            }

            if (!has_non_whitespace) {
                throw std::invalid_argument{std::string{name} + " cannot be empty or whitespace"};
            }
        }

        static void validate_optional_non_whitespace_string(std::string_view name, const std::string& value) {
            if (value.empty()) {
                return;
            }

            bool has_non_whitespace = false;
            for (const char ch : value) {
                if (!std::isspace(static_cast<unsigned char>(ch))) {
                    has_non_whitespace = true;
                    break;
                }
            }

            if (!has_non_whitespace) {
                throw std::invalid_argument{std::string{name} + " cannot be empty or whitespace"};
            }
        }
    }

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
            utils::validate_required_string("file_path", file_path);
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
            utils::validate_required_string("endpoint", endpoint);
            utils::validate_required_string("service_namespace", service_namespace);
            utils::validate_required_string("service_name", service_name);
            utils::validate_required_string("service_version", service_version);
            utils::validate_required_string("service_instance_id", service_instance_id);
            utils::validate_required_string("environment_name", environment_name);
            utils::validate_optional_non_whitespace_string("auth_token", auth_token);

            if (batch_size == 0) {
                throw std::invalid_argument{"batch_size must be greater than zero"};
            }
        }
    };

}

#endif