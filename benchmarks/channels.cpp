#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include "channel/bounded_channel.h"
#include "channel/unbounded_channel.h"
#include "utils/utils.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <future>
#include <memory>

using namespace std::chrono_literals;

void run_push_read_benchmark(
    ankerl::nanobench::Bench& bench,
    const std::string name,
    const bool single_thread,
    xtd::channel_writer<int>& writer,
    xtd::channel_reader<int>& reader)
{
    std::atomic<std::size_t> total_messages_received = 0;

    std::future<void> reader_task;

    if (!single_thread)
    {
        reader_task = std::async(
            std::launch::async,
            [&reader, &total_messages_received]
            {
                while (const auto read = reader.read())
                    ++total_messages_received;
            });
    }

    std::uint64_t total_messages_enqueued = 0;

    bench
        .run(
            name,
            [&writer,
             single_thread,
             &reader,
             &total_messages_enqueued,
             &total_messages_received]
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

    if (reader_task.valid())
    {
        reader_task.get();
    }

    assert(total_messages_enqueued == total_messages_received.load());
}

int main()
{
    print_machine_spec();

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd::channel push/read throughput")
        .unit("message")
        .epochs(21)
        .warmup(10'000)
        .minEpochTime(100ms)
        .maxEpochTime(500ms)
        .performanceCounters(true)
        .batch(1);

    {
        const auto bounded_channel =
            std::make_unique<xtd::bounded_channel<int, 1024>>();

        run_push_read_benchmark(
            bench,
            "single-thread / bounded_channel / push / read",
            true,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        const auto unbounded_channel =
            std::make_unique<xtd::unbounded_channel<int>>();

        run_push_read_benchmark(
            bench,
            "single-thread / unbounded_channel / push / read",
            true,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }

    {
        const auto bounded_channel =
            std::make_unique<xtd::bounded_channel<int, 1024>>();

        run_push_read_benchmark(
            bench,
            "multi-thread / bounded_channel / push / read",
            false,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        const auto unbounded_channel =
            std::make_unique<xtd::unbounded_channel<int>>();

        run_push_read_benchmark(
            bench,
            "multi-thread / unbounded_channel / push / read",
            false,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }

    std::ofstream output(
        "./benchmarks/results/channels.json",
        std::ios::out | std::ios::trunc);

    assert(output.is_open());

    bench.render(
        ankerl::nanobench::templates::json(),
        output);

    return 0;
}