#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <deque>
#include <memory_resource>
#include <string>

#include "pipeline/custom_allocators/arena_pool_resource.h"
#include "pipeline/custom_allocators/fixed_pool_resource.h"
#include "utils/utils.h"

using namespace std::chrono_literals;

namespace
{

constexpr std::size_t segment_size = 4096;
constexpr std::size_t total_capacity_bytes = 131072;
constexpr std::size_t pool_capacity = total_capacity_bytes / segment_size;
constexpr std::size_t alignment = alignof(std::byte);
constexpr std::size_t bytes_per_kb = 1024;
constexpr std::size_t bytes_per_mb = 1024 * bytes_per_kb;
constexpr std::size_t bytes_per_gb = 1024 * bytes_per_mb;

static_assert(total_capacity_bytes % segment_size == 0, "total capacity must be a multiple of segment size");
static_assert(pool_capacity == 32, "expected 32 pooled segments");

double bytes_to_gb(const std::size_t bytes)
{
    return static_cast<double>(bytes) / static_cast<double>(bytes_per_gb);
}

void run_roundtrip_benchmark(ankerl::nanobench::Bench& bench, const std::string& allocator_name, std::pmr::memory_resource&& resource)
{
    const std::string case_name = allocator_name + " / allocate+deallocate(1)";

    bench
        .batch(bytes_to_gb(segment_size))
        .run(case_name,
            [&resource]
            {
                void* const pointer = resource.allocate(segment_size, alignment);
                ankerl::nanobench::doNotOptimizeAway(pointer);
                resource.deallocate(pointer, segment_size, alignment);
            });
}

void run_deque_fifo_burst_benchmark(ankerl::nanobench::Bench& bench, const std::string& allocator_name, std::pmr::memory_resource&& resource)
{
    const std::string case_name = allocator_name + " / deque-fifo-burst(" + std::to_string(pool_capacity) + ")";
    
    std::deque<void*> segments;
    bench
        .batch(bytes_to_gb(segment_size * pool_capacity))
        .run(case_name,
            [&resource, &segments]
            {

                for (std::size_t i = 0; i < pool_capacity; ++i) {
                    void* const pointer = resource.allocate(segment_size, alignment);
                    segments.push_back(pointer);
                    ankerl::nanobench::doNotOptimizeAway(pointer);
                }

                while (!segments.empty()) {
                    void* const pointer = segments.front();
                    segments.pop_front();
                    resource.deallocate(pointer, segment_size, alignment);
                }
            });
}

void run_allocator_suite(ankerl::nanobench::Bench& bench)
{
    const std::pmr::pool_options pool_options{
        .max_blocks_per_chunk = pool_capacity,
        .largest_required_pool_block = segment_size
    };

    run_roundtrip_benchmark(bench, "fixed_pool_resource", 
        xtd::fixed_pool_resource(segment_size, pool_capacity));
    run_roundtrip_benchmark(bench, "arena_pool_resource", 
        xtd::arena_pool_resource(segment_size, pool_capacity));
    run_roundtrip_benchmark(bench, "unsynchronized_pool_resource", 
        std::pmr::unsynchronized_pool_resource(pool_options));

    run_deque_fifo_burst_benchmark(bench, "fixed_pool_resource", 
        xtd::fixed_pool_resource(segment_size, pool_capacity));
    run_deque_fifo_burst_benchmark(bench, "arena_pool_resource", 
        xtd::arena_pool_resource(segment_size, pool_capacity));
    run_deque_fifo_burst_benchmark(bench, "unsynchronized_pool_resource", 
        std::pmr::unsynchronized_pool_resource(pool_options));
}

} // namespace

int main()
{
    print_machine_spec();

    ankerl::nanobench::Bench bench;

    bench
        .title("xtd custom allocator throughput")
        .unit("GB")
        .timeUnit(1ms, "ms")
        .epochs(30)
        .warmup(10)
        .minEpochTime(300ms)
        .maxEpochTime(2s)
        .performanceCounters(true);

    run_allocator_suite(bench);

    return 0;
}
