#ifndef XTD_TESTS_PIPELINE_TEST_COMMON_H
#define XTD_TESTS_PIPELINE_TEST_COMMON_H

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <netinet/in.h>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../src/pipeline/segmented_byte_view.h"

namespace xtd {
class test_helper_segmented_byte_view {
public:
    static segmented_byte_view create_from_segments(std::vector<std::span<const std::byte>>&& segments, std::uint64_t sequence_id) {
        std::vector<std::span<const std::byte>> readable_segments;
        readable_segments.reserve(segments.size());

        std::size_t total_size = 0;

        for (const std::span<const std::byte>& segment : segments) {
            const std::size_t readable_size = segment.size();

            if (readable_size != 0) {
                readable_segments.emplace_back(segment);
                total_size += readable_size;
            }
        }

        return segmented_byte_view{
            std::move(readable_segments),
            sequence_id,
            total_size
        };
    }

    static std::size_t get_first_segment_begin(const segmented_byte_view& seq) {
        return seq.m_begin_offset;
    }

    static std::uint64_t get_sequence_id(const segmented_byte_view& seq) {
        return seq.m_sequence_id;
    }
};
}

inline std::size_t readCurrentRssKb()
{
    std::ifstream status("/proc/self/status");
    if (!status)
        throw std::runtime_error("failed to read /proc/self/status");

    std::string key;
    while (status >> key)
    {
        if (key == "VmRSS:")
        {
            std::size_t valueKb = 0;
            status >> valueKb;
            return valueKb;
        }

        std::string restOfLine;
        std::getline(status, restOfLine);
    }

    throw std::runtime_error("VmRSS not found in /proc/self/status");
}

#endif
