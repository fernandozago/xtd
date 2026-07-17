#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <print>

#include "channel/bounded_channel.h"
#include "channel/unbounded_channel.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

void run_push_read_benchmark(std::string name, xtd::channel_writer<int>& writer, xtd::channel_reader<int>& reader)
{
    std::uint64_t total_messages_transferred = 0;
    
    ankerl::nanobench::Bench bench;
    bench
        .title("single-thread channel push/read")
        .unit("message").batch(1) // One message transferred per benchmark invocation.
        .epochs(21)
        .warmup(2)
        .minEpochTime(2s).maxEpochTime(5s)
        .performanceCounters(true)
        .run(name, 
            [&writer, &reader, &total_messages_transferred] {
                if (writer.push(0)) {
                    if (auto value = reader.read()) {
                        total_messages_transferred += 1;
                        ankerl::nanobench::doNotOptimizeAway(*value);
                        return;
                    }
                }

                // If we reach this point
                // Stop the benchmark
                std::abort();
            });

    std::println("| | | | | **Total messages transferred: {}**", total_messages_transferred);
}

int main()
{
    print_machine_spec();

    xtd::bounded_channel<int, 32> bounded_stack_allocated;
    xtd::unbounded_channel<int> unbounded_stack_allocated;
    std::unique_ptr<xtd::bounded_channel<int, 32>> bounded_heap_allocated = std::make_unique<xtd::bounded_channel<int, 32>>();
    std::unique_ptr<xtd::unbounded_channel<int>> unbounded_heap_allocated = std::make_unique<xtd::unbounded_channel<int>>();

    run_push_read_benchmark(
        "(STACK OBJECT, INLINE STORAGE) bounded_channel / push / read",
        bounded_stack_allocated.writer(),
        bounded_stack_allocated.reader());

    run_push_read_benchmark(
        "(HEAP OBJECT, INLINE STORAGE) bounded_channel / push / read",
        bounded_heap_allocated->writer(),
        bounded_heap_allocated->reader());

    run_push_read_benchmark(
        "(STACK OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read",
        unbounded_stack_allocated.writer(),
        unbounded_stack_allocated.reader());

    run_push_read_benchmark(
        "(HEAP OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read",
        unbounded_heap_allocated->writer(),
        unbounded_heap_allocated->reader());

    return 0;
}