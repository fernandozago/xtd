#ifndef FILE_SINK_H
#define FILE_SINK_H

#include <filesystem>
#include "log_sink.h"

namespace xtd 
{

    class file_sink : public log_sink {
    private:
        std::string m_file_path;

    public:
        explicit file_sink(const file_sink_opts& opts)
            : log_sink(log_sink_opts{
                .min_log_level = opts.min_log_level,
                .use_local_time = opts.use_local_time,
                .use_colors = false,
                .flush_on_write = opts.flush_on_write,
            })
            , m_file_path(opts.file_path)
        {
            opts.validate();
            open_file();
        }

        file_sink(const file_sink&) = delete;
        file_sink& operator=(const file_sink&) = delete;
        file_sink(file_sink&&) = default;
        file_sink& operator=(file_sink&&) = default;

        ~file_sink() {
            if (m_fd >= 0) {
                ::close(m_fd);
            }
        }

    private:
        void open_file() {
            std::filesystem::path path{m_file_path};
            std::filesystem::path parent_path = path.parent_path();
            if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
                std::filesystem::create_directories(parent_path);
            }

            const int fd = ::open(m_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                throw std::runtime_error{"failed to open log file: " + m_file_path};
            }

            m_fd = fd;
        }
    };

}

#endif