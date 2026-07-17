#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include "pipeline/pipeline.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <print>
#include <future>

// Required for 64-bit size_t assumption in benchmark code below.
static_assert(sizeof(std::size_t) == 8, "Benchmark requires a 64-bit size_t");

using namespace std::chrono_literals;
constexpr std::size_t bytes_per_kib = 1024;
constexpr std::size_t bytes_per_mib = 1024 * bytes_per_kib;
constexpr std::size_t bytes_per_gib = 1024 * bytes_per_mib;

static double bytes_to_gib(std::size_t bytes) {
    return static_cast<double>(bytes) / bytes_per_gib;
}

void benchmark_writer_throughput(std::size_t write_chunk_size) {
    assert(write_chunk_size > 0);

    xtd::pipeline pipeline;

    std::size_t total_bytes_read = 0;

    std::future<void> reader_task = std::async(
        std::launch::async,
        [&pipeline, &total_bytes_read]() {
            xtd::pipe_reader& reader = pipeline.reader();

            while (const xtd::read_result result = reader.read()) {
                const xtd::segmented_byte_view buffer = result.buffer();

                total_bytes_read += buffer.size();
                reader.advance(buffer.end());

                if (result.completed()) {
                    break;
                }
            }

            reader.complete();
        });


    xtd::pipe_writer& writer = pipeline.writer();

    std::size_t total_bytes_write = 0;
    const auto payload = std::make_unique<std::byte[]>(write_chunk_size);

    ankerl::nanobench::Bench bench;
    bench
        .title("xtd::pipeline writer throughput")
        .timeUnit(1ms, "ms")
        .epochs(6)
        .warmup(0)
        .minEpochTime(1s).maxEpochTime(2s)
        .unit("GiB").batch(bytes_to_gib(write_chunk_size))
        .run(std::to_string(write_chunk_size / bytes_per_kib) + " KiB writes", 
            [&writer, &total_bytes_write, write_chunk_size, &payload] {
                total_bytes_write += writer.write(payload.get(), write_chunk_size);
            });

    writer.complete();
    reader_task.get();

    assert(total_bytes_read == total_bytes_write);
    std::println("| | | | | ** Total Bytes Transferred: {:.2f} GiB**", bytes_to_gib(total_bytes_write));
}

int main() {
    const std::size_t chunks[] = { 
        1 * bytes_per_kib, 
        2 * bytes_per_kib, 
        4 * bytes_per_kib, 
        8 * bytes_per_kib, 
        16 * bytes_per_kib 
    };

    for (const auto write_chunk_size : chunks) {
        benchmark_writer_throughput(write_chunk_size);
    }

    return 0;
}