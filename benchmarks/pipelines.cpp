//#define ANKERL_NANOBENCH_LOG_ENABLED
#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <print>

#include "pipeline/pipeline.h"
#include "utils/utils.h"

static_assert(sizeof(std::size_t) == 8, "Benchmark requires a 64-bit size_t");

using namespace std::chrono_literals;

constexpr std::size_t bytes_per_kib = 1024;
constexpr std::size_t bytes_per_mib = 1024 * bytes_per_kib;
constexpr std::size_t bytes_per_gib = 1024 * bytes_per_mib;
static std::vector<std::string> results;

static double bytes_to_gib(const std::size_t bytes)
{
    return static_cast<double>(bytes) /
           static_cast<double>(bytes_per_gib);
}

static double bytes_to_mib(const std::size_t bytes)
{
    return static_cast<double>(bytes) /
           static_cast<double>(bytes_per_mib);
}

struct consumer {
public:
    consumer(xtd::pipe_reader& reader) 
        : m_reader(reader) 
        , reader_task(&consumer::consume, this)
    {
    }

    ~consumer() {
        if (reader_task.joinable()) {
            reader_task.join();
        }
    }

    std::size_t get_total_bytes_read() {
        if (reader_task.joinable()) {
            reader_task.join();
        }
        return total_bytes_read;
    }
private:
    xtd::pipe_reader& m_reader;
    std::thread reader_task;
    std::size_t total_bytes_read = 0;

    void consume() {
        while (const xtd::read_result result = m_reader.read())
        {
            const xtd::segmented_byte_view buffer = result.buffer();

            total_bytes_read += buffer.size();
            m_reader.advance(buffer.end());

            if (result.completed()) {
                break;
            }
        }

        m_reader.complete();
    }
};

void benchmark(ankerl::nanobench::Bench& bench, const std::size_t write_chunk_size)
{
    assert(write_chunk_size > 0);

    const auto payload = std::make_unique<std::byte[]>(write_chunk_size);

    std::generate_n(
        payload.get(),
        write_chunk_size,
        []
        {
            std::mt19937 generator{std::random_device{}()};
            std::uniform_int_distribution<unsigned int> distribution{
                0,
                255
            };
            return static_cast<std::byte>(distribution(generator));
        });


    // Create Pipeline
    xtd::pipeline pipeline;

    // Start consumer thread before starting the benchmark
    consumer consumer(pipeline.reader());

    xtd::pipe_writer& writer = pipeline.writer();
    std::size_t total_bytes_written = 0;
    std::string bench_name = std::to_string(write_chunk_size / bytes_per_kib) + " KiB writes";
    bench
        .batch(bytes_to_mib(write_chunk_size))
        .run(bench_name,
            [&writer, &total_bytes_written, write_chunk_size, &payload]
            {
                total_bytes_written += writer.write(payload.get(), write_chunk_size);
            });

    writer.complete();
    const std::size_t total_bytes_read = consumer.get_total_bytes_read();
    assert(total_bytes_read == total_bytes_written);
    results.push_back(std::format("| {:>15.2f} GiB | {}", bytes_to_gib(total_bytes_written), bench_name));
}

int main()
{
    print_machine_spec();

    constexpr std::size_t chunks[]{
        1 * bytes_per_kib,
        2 * bytes_per_kib,
        4 * bytes_per_kib,
        8 * bytes_per_kib,
        16 * bytes_per_kib,
        32 * bytes_per_kib
    };

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd::pipeline throughput")
        .timeUnit(1us, "μs")
        .epochs(20)
        .warmup(10)
        .minEpochTime(250ms)
        .maxEpochTime(2s)
        .performanceCounters(true)
        .unit("MiB");

    for (const std::size_t write_chunk_size : chunks)
    {
        benchmark(bench, write_chunk_size);
    }

    // Uncomment for detailed JSON output
    // std::ofstream output("./benchmarks/results/pipelines.json", std::ios::out | std::ios::trunc);
    // assert(output.is_open());
    // bench.render(ankerl::nanobench::templates::json(), output);

    std::println();
    std::println("|   Total Transferred | xtd::pipeline throughput ");
    std::println("|--------------------:|:-------------------------");
    for (const std::string& result : results)
    {
        std::println("{}", result);
    }

    return 0;
}