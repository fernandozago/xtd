//#define ANKERL_NANOBENCH_LOG_ENABLED
#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"


#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <future>
#include <memory>
#include <random>
#include <string>
#include "pipeline/pipeline.h"
#include "utils/utils.h"

static_assert(
    sizeof(std::size_t) == 8,
    "Benchmark requires a 64-bit size_t");

using namespace std::chrono_literals;

constexpr std::size_t bytes_per_kib = 1024;
constexpr std::size_t bytes_per_mib = 1024 * bytes_per_kib;
constexpr std::size_t bytes_per_gib = 1024 * bytes_per_mib;

static double bytes_to_gib(const std::size_t bytes)
{
    return static_cast<double>(bytes) /
           static_cast<double>(bytes_per_gib);
}

void benchmark_writer_throughput(ankerl::nanobench::Bench& bench, const std::size_t write_chunk_size)
{
    assert(write_chunk_size > 0);

    xtd::pipeline pipeline;

    std::size_t total_bytes_read = 0;

    std::future<void> reader_task = std::async(
        std::launch::async,
        [&pipeline, &total_bytes_read]
        {
            xtd::pipe_reader& reader = pipeline.reader();

            while (const xtd::read_result result = reader.read())
            {
                const xtd::segmented_byte_view buffer =
                    result.buffer();

                total_bytes_read += buffer.size();
                reader.advance(buffer.end());

                if (result.completed())
                {
                    break;
                }
            }

            reader.complete();
        });

    xtd::pipe_writer& writer = pipeline.writer();

    std::size_t total_bytes_written = 0;

    const auto payload =
        std::make_unique<std::byte[]>(write_chunk_size);

    std::mt19937 generator{std::random_device{}()};

    std::uniform_int_distribution<unsigned int> distribution{
        0,
        255
    };

    std::generate_n(
        payload.get(),
        write_chunk_size,
        [&generator, &distribution]
        {
            return static_cast<std::byte>(
                distribution(generator));
        });

    bench
        .batch(bytes_to_gib(write_chunk_size))
        .run(
            std::to_string(
                write_chunk_size / bytes_per_kib) +
                " KiB writes",
            [&writer,
             &total_bytes_written,
             write_chunk_size,
             &payload]
            {
                total_bytes_written += writer.write(
                    payload.get(),
                    write_chunk_size);
            });

    writer.complete();
    reader_task.get();

    assert(total_bytes_read == total_bytes_written);
}

int main()
{
    print_machine_spec();

    constexpr std::size_t chunks[]{
        1 * bytes_per_kib,
        2 * bytes_per_kib,
        4 * bytes_per_kib,
        8 * bytes_per_kib,
        16 * bytes_per_kib
    };

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd::pipeline throughput")
        .timeUnit(1ms, "ms")
        .epochs(20)
        .warmup(10)
        .minEpochTime(100ms)
        .maxEpochTime(1s)
        .performanceCounters(true)
        .unit("GiB");

    for (const std::size_t write_chunk_size : chunks)
    {
        benchmark_writer_throughput(bench, write_chunk_size);
    }

    std::ofstream output(
        "./benchmarks/results/pipelines.json",
        std::ios::out | std::ios::trunc);

    assert(output.is_open());

    bench.render(
        ankerl::nanobench::templates::json(),
        output);

    return 0;
}