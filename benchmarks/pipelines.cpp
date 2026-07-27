//#define ANKERL_NANOBENCH_LOG_ENABLED
#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <format>
#include <latch>
#include <thread>
#include <print>
#include "pipeline/pipeline.h"
#include "utils/utils.h"

static_assert(sizeof(std::size_t) == 8, "Benchmark requires a 64-bit size_t");

using namespace std::chrono_literals;

constexpr std::size_t bytes_per_kb = 1024;
constexpr std::size_t bytes_per_mb = 1024 * bytes_per_kb;
constexpr std::size_t bytes_per_gb = 1024 * bytes_per_mb;
static std::vector<std::string> results;

static double bytes_to_gb(const std::size_t bytes)
{
    return static_cast<double>(bytes) / static_cast<double>(bytes_per_gb);
}

static double bytes_to_mb(const std::size_t bytes)
{
    return static_cast<double>(bytes) / static_cast<double>(bytes_per_mb);
}

struct consumer {
private:
    std::size_t total_bytes_read = 0;
    xtd::pipe_reader& m_reader;
    std::latch& m_latch;
    std::jthread reader_task;

    void consume() {
        m_latch.count_down();
        while (const xtd::read_result result = m_reader.read())
        {
            xtd::segmented_byte_view buffer = result.buffer();
            total_bytes_read += buffer.size();
            buffer.slice_in_place(buffer.end(), buffer.end());
            m_reader.advance(buffer);

            if (result.completed()) {
                break;
            }
        }

        m_reader.complete();
    }

public:
    consumer(xtd::pipe_reader& reader, std::latch& latch) 
        : m_reader(reader) 
        , m_latch(latch)
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
    std::latch latch(1);
    consumer consumer(pipeline.reader(), latch);
    latch.wait();

    xtd::pipe_writer& writer = pipeline.writer();
    std::size_t total_bytes_written = 0;
    std::string bench_name = std::format("{} KB writes", write_chunk_size / (double)bytes_per_kb);
    bench
        .batch(bytes_to_mb(write_chunk_size))
        .run(bench_name,
            [&writer, &total_bytes_written, write_chunk_size, &payload]
            {
                total_bytes_written += writer.write(payload.get(), write_chunk_size);
            });

    writer.complete();
    const std::size_t total_bytes_read = consumer.get_total_bytes_read();
    assert(total_bytes_read == total_bytes_written);
    results.push_back(std::format("| {:>16.2f} GB | `{}`", bytes_to_gb(total_bytes_written), bench_name));
    std::fflush(stdout);
}

int main()
{
    print_machine_spec();

    static constexpr std::size_t chunks[] {
        1 * bytes_per_kb, // 1 KB
        2 * bytes_per_kb, // 2 KB
        4 * bytes_per_kb, // 4 KB
        8 * bytes_per_kb, // 8 KB
        16 * bytes_per_kb, // 16 KB
    };

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd::pipeline throughput")
        .timeUnit(1us, "μs")
        .epochs(25)
        .warmup(10)
        .minEpochTime(250ms)
        .maxEpochTime(2s)
        .performanceCounters(true)
        .unit("MB");

    for (const std::size_t write_chunk_size : chunks) {
        benchmark(bench, write_chunk_size);
    }

    // Uncomment for detailed JSON output
    // std::ofstream output("./benchmarks/results/pipelines.json", std::ios::out | std::ios::trunc);
    // assert(output.is_open());
    // bench.render(ankerl::nanobench::templates::json(), output);

    std::println();
    std::println("|   Total Transferred | xtd::pipeline throughput ");
    std::println("|--------------------:|:-------------------------");
    for (const std::string& result : results) {
        std::println("{}", result);
    }

    return 0;
}