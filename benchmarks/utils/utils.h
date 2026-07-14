#ifndef BENCHMARKS_UTILS_UTILS_H
#define BENCHMARKS_UTILS_UTILS_H

#include <fstream>
#include <iostream>
#include <string>

inline std::string read_first_proc_value(const char* file_path, const char* key)
{
    std::ifstream file(file_path);

    if (!file) {
        return "unknown";
    }

    std::string line;
    const std::string key_text = key;

    while (std::getline(file, line)) {
        auto separator_pos = line.find(':');

        if (separator_pos == std::string::npos) {
            continue;
        }

        auto current_key = line.substr(0, separator_pos);
        auto key_end = current_key.find_last_not_of(" \t");

        if (key_end == std::string::npos) {
            continue;
        }

        current_key = current_key.substr(0, key_end + 1);

        if (current_key == key_text) {
            auto value = line.substr(separator_pos + 1);
            auto first = value.find_first_not_of(" \t");

            if (first == std::string::npos) {
                return "unknown";
            }

            return value.substr(first);
        }
    }

    return "unknown";
}

inline void print_machine_spec()
{
    const auto cpu_model = read_first_proc_value("/proc/cpuinfo", "model name");
    const auto cpu_cores = read_first_proc_value("/proc/cpuinfo", "cpu cores");
    const auto logical_cpus = read_first_proc_value("/proc/cpuinfo", "siblings");
    const auto memory_kb_text = read_first_proc_value("/proc/meminfo", "MemTotal");

    double memory_gib = 0.0;
    auto split_pos = memory_kb_text.find_first_of(" \t");

    if (split_pos != std::string::npos) {
        try {
            auto memory_kb = std::stoull(memory_kb_text.substr(0, split_pos));
            memory_gib = static_cast<double>(memory_kb) / (1024.0 * 1024.0);
        }
        catch (...) {
            memory_gib = 0.0;
        }
    }

    std::cout << "Machine Spec:\n";
    std::cout << "  CPU model: " << cpu_model << '\n';
    std::cout << "  CPU cores (physical/logical per socket): " << cpu_cores << " / " << logical_cpus << '\n';

    if (memory_gib > 0.0) {
        std::cout << "  Memory: " << memory_gib << " GiB total\n\n";
    }
    else {
        std::cout << "  Memory: " << memory_kb_text << "\n\n";
    }
}

#endif // BENCHMARKS_UTILS_UTILS_H