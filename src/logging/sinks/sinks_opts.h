#ifndef SINKS_OPTS
#define SINKS_OPTS

#include "logging/log_message.h"
#include <unistd.h>

struct log_sink_opts {
    log_level min_log_level = log_level::information;
    int fd = STDOUT_FILENO;
    bool use_local_time = true;
    bool use_structured_log = false;
    bool use_colors = true;
    bool flush_on_write = true;
};

struct file_sink_opts {
    log_level min_log_level = log_level::information;
    std::string file_path;
    bool use_local_time = true;
    bool flush_on_write = true;
};

struct otel_sink_opts {
    log_level min_log_level = log_level::information;
    std::string endpoint;
    std::string auth_token;
    std::string stream_name;
    std::string service_name;

    bool use_local_time = false;
};

#endif