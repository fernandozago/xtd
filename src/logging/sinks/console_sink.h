#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include "log_sink.h"

struct console_sink_opts {
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool use_structured_log = false;
    bool use_colors = true;
    bool flush_on_write = true;
};

class console_sink : public log_sink {
public:
    explicit console_sink(const console_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = STDOUT_FILENO,
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .use_structured_log = opts.use_structured_log,
              .use_colors = opts.use_colors,
              .flush_on_write = opts.flush_on_write,
          })
    {
    }
};

#endif