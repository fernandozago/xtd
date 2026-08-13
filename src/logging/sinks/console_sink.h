#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include <unistd.h>

#include "log_sink.h"

struct console_sink_opts {
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool show_timezone = true;
    bool use_structured_log = false;
    bool use_colors = true;
};

class console_sink : public log_sink {
public:
    explicit console_sink(const console_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = STDOUT_FILENO,
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .show_timezone = opts.show_timezone,
              .use_structured_log = opts.use_structured_log,
              .use_colors = opts.use_colors,
          })
    {
    }
};

#endif