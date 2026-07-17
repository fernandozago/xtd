#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <print>
#include <cassert>
#include <future>

#include "channel/bounded_channel.h"
#include "channel/unbounded_channel.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

void run_push_read_benchmark(std::string&& name, bool single_thread, xtd::channel_writer<int>& writer, xtd::channel_reader<int>& reader)
{
    std::atomic<std::size_t> total_messages_received = 0;
    
    std::future<void> reader_task;

    if (!single_thread) {
        reader_task = std::async(
            std::launch::async,
            [&reader, &total_messages_received] {
                while (const auto value = reader.read()) {
                    ++total_messages_received;
                }
            });
    }
    
    std::uint64_t total_messages_enqueued = 0;
    ankerl::nanobench::Bench bench;
    bench
        .title("channel push/read")
        .unit("message")
        .batch(1)
        .epochs(21)
        .warmup(10'000)
        .minEpochTime(100ms)
        .maxEpochTime(500ms)
        .performanceCounters(true)
        .run(name,
            [&writer, &single_thread, &reader, &total_messages_enqueued, &total_messages_received]() {
                
                auto result = writer.push(0);

                if (!result) {
                    std::abort();
                }

                ++total_messages_enqueued;
                ankerl::nanobench::doNotOptimizeAway(result);

                if (single_thread) {
                    if (auto value = reader.read()) {
                        ++total_messages_received;
                    }
                }
            });

    writer.complete();
    if (reader_task.valid()) {
        reader_task.get();
    }
    assert(total_messages_enqueued == total_messages_received);
    std::println("| | | | | **Total messages transferred: {}**", total_messages_enqueued);
}

int main()
{
    print_machine_spec();
   
    {
        std::unique_ptr<xtd::bounded_channel<int, 1024>> bounded_channel = std::make_unique<xtd::bounded_channel<int, 1024>>();
        run_push_read_benchmark(
            "single-thread / bounded_channel / push / read",
            true,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        std::unique_ptr<xtd::unbounded_channel<int>> unbounded_channel = std::make_unique<xtd::unbounded_channel<int>>();
        run_push_read_benchmark(
            "single-thread / unbounded_channel / push / read",
            true,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }

    {
        std::unique_ptr<xtd::bounded_channel<int, 1024>> bounded_channel = std::make_unique<xtd::bounded_channel<int, 1024>>();
        run_push_read_benchmark(
            "multi-thread / bounded_channel / push / read",
            false,
            bounded_channel->writer(),
            bounded_channel->reader());
    }

    {
        std::unique_ptr<xtd::unbounded_channel<int>> unbounded_channel = std::make_unique<xtd::unbounded_channel<int>>();
        run_push_read_benchmark(
            "multi-thread / unbounded_channel / push / read",
            false,
            unbounded_channel->writer(),
            unbounded_channel->reader());
    }
    return 0;
}