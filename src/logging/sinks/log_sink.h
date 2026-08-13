#ifndef LOG_SINK_H
#define LOG_SINK_H

#include "../log_message.h"

class log_sink {
    public:
        virtual void write(const log_message& message) = 0;
        virtual ~log_sink() = default;
};

#endif // LOG_SINK_H