#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>

#include "utils/utils.h"
#include "channel/bounded_channel.h"
#include "channel/unbounded_channel.h"

namespace
{
constexpr std::size_t kMessageCount = 2'000'000;
constexpr std::size_t kCapacity = 1024;

constexpr std::uint64_t kExpectedChecksum =
    static_cast<std::uint64_t>(kMessageCount) *
    static_cast<std::uint64_t>(kMessageCount - 1) / 2;

template<typename TWriter, typename TReader>
void run_push_read_benchmark(TWriter& writer, TReader& reader)
{
    std::uint64_t checksum = 0;

    for (std::size_t i = 0; i < kMessageCount; ++i) {
        if (!writer.push(i)) {
            std::abort();
        }

        auto value = reader.read();

        if (!value) {
            std::abort();
        }

        checksum += static_cast<std::uint64_t>(*value);
    }

    if (checksum != kExpectedChecksum) {
        std::abort();
    }

    ankerl::nanobench::doNotOptimizeAway(checksum);
}
}

int main()
{
    using namespace std::chrono_literals;

    ankerl::nanobench::Bench bench;

    bench
        .title("single-thread channel push/read")
        .unit("message")
        .batch(kMessageCount)
        .epochs(21)
        .warmup(2)
        .minEpochTime(500ms)
        .maxEpochTime(2s)
        .performanceCounters(true);

    print_machine_spec();

        
    auto boundedStackAllocated = xtd::bounded_channel<std::size_t, kCapacity>();
    auto unboundedStackAllocated = xtd::unbounded_channel<std::size_t>();
    
    auto boundedHeapAllocated = std::make_unique<xtd::bounded_channel<std::size_t, kCapacity>>();
    auto unboundedHeapAllocated = std::make_unique<xtd::unbounded_channel<std::size_t>>();

    bench.run("(STACK OBJECT, INLINE STORAGE) bounded_channel / push / read", [&boundedStackAllocated] {
        run_push_read_benchmark(
            boundedStackAllocated.writer(),
            boundedStackAllocated.reader());
    });


    bench.run("(HEAP OBJECT, INLINE STORAGE) bounded_channel / push / read", [&boundedHeapAllocated] {
        run_push_read_benchmark(
            boundedHeapAllocated->writer(),
            boundedHeapAllocated->reader());
    });

    bench.run("(STACK OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read", [&unboundedStackAllocated] {

        run_push_read_benchmark(
            unboundedStackAllocated.writer(),
            unboundedStackAllocated.reader());
    });

    bench.run("(HEAP OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read", [&unboundedHeapAllocated] {
        run_push_read_benchmark(
            unboundedHeapAllocated->writer(),
            unboundedHeapAllocated->reader());
    });

    return 0;
}