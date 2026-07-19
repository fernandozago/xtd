#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <print>
#include <memory>
#include <vector>

#include "channel/channel.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

static std::vector<std::string> results;

class consumer {
public:
    explicit consumer(xtd::channel_reader<int>& reader)
        : m_reader(reader)
        , m_thread(&consumer::consume, this)
    {
    }

    ~consumer()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    std::size_t get_received_messages()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }

        return m_received_messages;
    }

private:
    xtd::channel_reader<int>& m_reader;
    std::thread m_thread;
    std::size_t m_received_messages = 0;

    void consume()
    {
        while (m_reader.read()) {
            ++m_received_messages;
        }
    }
};

void benchmark(ankerl::nanobench::Bench& bench, const std::string name, const bool single_thread, xtd::channel<int>&& channel)
{
    std::size_t total_messages_received = 0;

    // Consumer creates its own thread to read from the channel.
    // To make it simple, consumer must be in a stable location in memory.
    // So we use a vector of unique_ptr to manage the lifetime of the consumer objects.
    xtd::channel_reader<int>& reader = channel.reader();
    std::vector<std::unique_ptr<consumer>> consumers;
    if (!single_thread) {
        consumers.emplace_back(std::make_unique<consumer>(reader));
        consumers.emplace_back(std::make_unique<consumer>(reader));
    }

    std::uint64_t total_messages_enqueued = 0;

    xtd::channel_writer<int>& writer = channel.writer();
    bench.run(name,
            [&reader, &writer, single_thread, &total_messages_enqueued, &total_messages_received]
            {
                if (const auto write = writer.push(0); write) {
                    ++total_messages_enqueued;
                }

                if (single_thread)
                {
                    if (const auto read = reader.read(); read) {
                        ++total_messages_received;
                    }
                }
            });

    writer.complete();

    for (auto& consumer : consumers)
    {
        total_messages_received += consumer->get_received_messages();
    }

    assert(total_messages_enqueued == total_messages_received);
    results.push_back(std::format(std::locale("en_US.UTF-8"), "| {:>23L} | `{}`", total_messages_enqueued, name));
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