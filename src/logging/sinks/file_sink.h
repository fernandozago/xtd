#ifndef FILE_SINK_H
#define FILE_SINK_H

#include <filesystem>
#include "log_sink.h"

struct file_sink_opts {
    std::string file_path;
    log_level min_log_level = log_level::information;
    bool use_local_time = true;
    bool show_timezone = true;
    bool use_structured_log = false;
    bool flush_on_write = true;
};

class file_sink : public log_sink {
private:
    std::string m_file_path;

public:
    explicit file_sink(const file_sink_opts& opts)
        : log_sink(log_sink_opts{
              .fd = open_file(opts.file_path),
              .min_log_level = opts.min_log_level,
              .use_local_time = opts.use_local_time,
              .show_timezone = opts.show_timezone,
              .use_structured_log = opts.use_structured_log,
              .use_colors = false,
              .flush_on_write = opts.flush_on_write,
          })
        , m_file_path(opts.file_path)
    {
    }

    file_sink(const file_sink&) = delete;
    file_sink& operator=(const file_sink&) = delete;
    file_sink(file_sink&&) = default;
    file_sink& operator=(file_sink&&) = default;

    ~file_sink() {
        const int fd = m_fd();
        if (fd >= 0) {
            ::close(fd);
        }
    }

private:
    static int open_file(const std::string& file_path) {
        if (file_path.empty()) {
            throw std::invalid_argument{"file path cannot be empty"};
        }

        std::filesystem::path path{file_path};
        std::filesystem::path parent_path = path.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
        }

        const int fd = ::open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            throw std::runtime_error{"failed to open log file: " + file_path};
        }

        return fd;
    }
};

#endif