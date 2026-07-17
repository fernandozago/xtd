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
    
    std::future<void> reader_task[2];

    if (!single_thread) {
        reader_task[0] = std::async(
            std::launch::async,
            [&reader, &total_messages_received] {
                while (const auto value = reader.read()) {
                    ++total_messages_received;
                    ankerl::nanobench::doNotOptimizeAway(*value);
                }
            });
        reader_task[1] = std::async(
            std::launch::async,
            [&reader, &total_messages_received] {
                while (const auto value = reader.read()) {
                    ++total_messages_received;
                    ankerl::nanobench::doNotOptimizeAway(*value);
                }
            });
    }
    
    std::uint64_t total_messages_enqueued = 0;
    ankerl::nanobench::Bench bench;
    bench
        .title("channel push/read")
        .unit("message").batch(1) // One message transferred per benchmark invocation.
        .epochs(25)
        .warmup(0)
        .minEpochTime(1s).maxEpochTime(2s)
        .performanceCounters(true)
        .run(name, 
            [&writer, &single_thread, &reader, &total_messages_enqueued, &total_messages_received]() {
                auto result = writer.push(0);
                if (!result) { std::abort(); }
                total_messages_enqueued += 1;
                ankerl::nanobench::doNotOptimizeAway(result);

                if (single_thread) {
                    // In single-threaded mode, we must read the value back to avoid deadlock.
                    if (auto value = reader.read()) {
                        total_messages_received += 1;
                        ankerl::nanobench::doNotOptimizeAway(*value);
                    }
                }
            });

    writer.complete();
    if (reader_task[0].valid()) {
        reader_task[0].get();
    }
    if (reader_task[1].valid()) {
        reader_task[1].get();
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