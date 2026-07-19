#define ANKERL_NANOBENCH_IMPLEMENT

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <print>

#include "third_party/nanobench.h"
#include "channel/channel.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

static std::vector<std::string> results;

void run_push_read_benchmark(ankerl::nanobench::Bench& bench, const std::string name, const bool single_thread, 
    xtd::channel_writer<int>& writer, xtd::channel_reader<int>& reader)
{
    std::size_t total_messages_received = 0;

    std::future<std::size_t> reader_task[2];

    if (!single_thread)
    {
        reader_task[0] = std::async(std::launch::async,
            [&reader] {
                std::size_t received_message = 0;
                while (const auto read = reader.read())
                    ++received_message;

                return received_message;
            });
        
        reader_task[1] = std::async(std::launch::async,
            [&reader] {
                std::size_t received_message = 0;
                while (const auto read = reader.read())
                    ++received_message;

                return received_message;
            });
    }

    std::uint64_t total_messages_enqueued = 0;

    bench
        .run(name,
            [&writer, single_thread, &reader, &total_messages_enqueued, &total_messages_received]
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

    for (auto& task : reader_task)
    {
        if (task.valid())
            total_messages_received += task.get();
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

    {
        const auto bounded_channel = std::make_unique<xtd::channel<int>>(1024);
        run_push_read_benchmark(
            bench,
            "single-thread / bounded_channel",
            true,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        const auto unbounded_channel = std::make_unique<xtd::channel<int>>();
        run_push_read_benchmark(
            bench,
            "single-thread / unbounded_channel",
            true,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }

    {
        const auto bounded_channel = std::make_unique<xtd::channel<int>>(1024);

        run_push_read_benchmark(
            bench,
            "multi-thread / bounded_channel",
            false,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        const auto unbounded_channel = std::make_unique<xtd::channel<int>>();
        run_push_read_benchmark(
            bench,
            "multi-thread / unbounded_channel",
            false,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }

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