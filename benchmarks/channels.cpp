#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <chrono>
#include <latch>
#include <thread>
#include <cassert>
#include <print>
#include "channel/channel.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

static std::vector<std::string> results;

class consumer {
private:
    std::size_t m_received_messages = 0;
    xtd::channel_reader<int>& m_reader;
    std::latch& m_latch;
    std::jthread m_thread;

    void consume()
    {
        m_latch.count_down();
        while (const auto value = m_reader.read()) {
            ++m_received_messages;
        }
    }

public:
    explicit consumer(xtd::channel_reader<int>& reader, std::latch& latch)
        : m_reader(reader)
        , m_latch(latch)
        , m_thread(&consumer::consume, this)
    {
    }
        
    std::size_t get_received_messages()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }

        return m_received_messages;
    }
};

void benchmark(ankerl::nanobench::Bench& bench, const std::string name, const bool single_thread, xtd::channel<int>&& channel)
{
    std::size_t total_messages_received = 0;
        
    // Consumer creates its own thread to read from the channel.
    // To make it simple, consumer must be in a stable location in memory.
    // So we use a vector of unique_ptr to manage the lifetime of the consumer objects.
    xtd::channel_reader reader(channel);
    std::vector<std::unique_ptr<consumer>> consumers;

    if (!single_thread) {
        constexpr std::size_t consumer_count = 2;
        std::latch started{consumer_count};
        for (std::size_t i = 0; i < consumer_count; ++i) {
            consumers.emplace_back(
                std::make_unique<consumer>(reader, started)
            );
        }
        started.wait();
    }

    std::uint64_t total_messages_enqueued = 0;
    xtd::channel_writer<int> writer(channel);
    bench.run(name,
            [&reader, &writer, single_thread, &total_messages_enqueued, &total_messages_received]
            {
                ankerl::nanobench::doNotOptimizeAway(writer.push(0));
                ++total_messages_enqueued;

                if (single_thread)
                {
                    ankerl::nanobench::doNotOptimizeAway(reader.read());
                    ++total_messages_received;
                }
            });

    writer.complete();

    // auto chrono_start = std::chrono::high_resolution_clock::now();
    // std::println("Total messages enqueued: {}", total_messages_enqueued);
    if (!single_thread) {
        for (auto& consumer : consumers) {
            total_messages_received += consumer->get_received_messages();
        }
        // auto chrono_end = std::chrono::high_resolution_clock::now();
        // auto chrono_duration = std::chrono::duration_cast<std::chrono::microseconds>(chrono_end - chrono_start).count();
        // std::println("Time taken for consumers to finish reading: {} μs", chrono_duration);
        // std::fflush(stdout);
    }

    assert(total_messages_enqueued == total_messages_received);
    results.push_back(std::format(std::locale("en_US.UTF-8"), "| {:>23L} | `{}`", total_messages_enqueued, name));
    std::fflush(stdout);
}

int main()
{
    print_machine_spec();

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd::channel throughput")
        .unit("message")
        .epochs(25)
        .warmup(10)
        .minEpochTime(500ms)
        .maxEpochTime(2s)
        .performanceCounters(true)
        .batch(1);

    benchmark(bench, "single-thread / bounded_channel", true,
        xtd::channel<int>(1024));

    benchmark(bench, "single-thread / unbounded_channel", true,
        xtd::channel<int>());

    benchmark(bench, "multi-thread / bounded_channel", false,
        xtd::channel<int>(1024));

    benchmark(bench, "multi-thread / unbounded_channel", false,
        xtd::channel<int>());

    // Uncomment for detailed JSON output
    // std::ofstream output("./benchmarks/results/channels.json", std::ios::out | std::ios::trunc);
    // assert(output.is_open());
    // bench.render(ankerl::nanobench::templates::json(), output);

    std::println();
    std::println("| Total Messages Enqueued | xtd::channel throughput ");
    std::println("|------------------------:|:-------------------------");
    for (const std::string& result : results)
    {
        std::println("{}", result);
    }

    return 0;
}