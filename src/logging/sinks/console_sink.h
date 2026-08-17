#ifndef CONSOLE_SINK_H
#define CONSOLE_SINK_H

#include "log_sink.h"

class console_sink : public log_sink {
public:
    explicit console_sink(const console_sink_opts& opts)
        : log_sink(log_sink_opts{
            .min_log_level = opts.min_log_level,
            .fd = opts.fd,
            .use_local_time = opts.use_local_time,
            .use_colors = opts.use_colors,
            .flush_on_write = opts.flush_on_write,
        })
    {
    }
};

#endif