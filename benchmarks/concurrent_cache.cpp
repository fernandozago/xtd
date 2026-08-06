#define ANKERL_NANOBENCH_IMPLEMENT
#include "third_party/nanobench.h"

#include <chrono>
#include <cstddef>
#include <format>
#include <vector>

#include "cache/concurrent_cache.h"
#include "utils/utils.h"


namespace
{


void run_insert_or_assign_benchmark(ankerl::nanobench::Bench& bench, const std::vector<std::size_t>& keys)
{
    std::size_t key_index = 0;
	xtd::concurrent_cache<std::size_t, std::size_t> cache;
	bench
		.batch(1)
		.run(
			"insert_or_assign",
			[&cache, &keys, &key_index]
			{
				const std::size_t key = keys[key_index++ % keys.size()];
				ankerl::nanobench::doNotOptimizeAway(cache.insert_or_assign(key, 0));
			});
}

void run_get_or_create_benchmark(ankerl::nanobench::Bench& bench, const std::vector<std::size_t>& keys, const std::chrono::nanoseconds ttl = std::chrono::nanoseconds::zero())
{
	xtd::concurrent_cache<std::size_t, std::size_t> cache;
    std::size_t key_index = 0;
	bench
		.batch(1)
		.run(
			std::format("get_or_create(ttl: {}ms)", std::chrono::duration_cast<std::chrono::milliseconds>(ttl).count()),
			[&cache, &ttl, &keys, &key_index]
			{
				ankerl::nanobench::doNotOptimizeAway(cache.get_or_create(
                    keys[key_index++ % keys.size()], 
                    {ttl}, []([[maybe_unused]] const std::size_t current_key) {
                        return std::size_t{0};
                    })
                );
			});
}

void run_populated_cache_get_benchmark(ankerl::nanobench::Bench& bench, const std::vector<std::size_t>& keys)
{
    std::size_t key_index = 0;
    xtd::concurrent_cache<std::size_t, std::size_t> cache;
    for (const std::size_t key : keys) {
        (void)cache.insert_or_assign(key, 0);
    }

	bench
		.batch(1)
		.run(
			"get / pre-populated-cache",
			[&cache, &keys, &key_index]
			{
                ankerl::nanobench::doNotOptimizeAway(cache.get(keys[key_index++ % keys.size()]));
			});
}

} // namespace

int main()
{
    using namespace std::chrono_literals;
	print_machine_spec();

	std::vector<std::size_t> keys;
	keys.reserve(1'000'000);
	for (std::size_t key = 1; key <= 1'000'000; ++key) {
        keys.push_back(key);
		ankerl::nanobench::doNotOptimizeAway(keys[key - 1]);
	}

	ankerl::nanobench::Bench bench;

	bench
		.title("xtd::concurrent_cache throughput")
		.unit("items")
		.epochs(35)
		.warmup(10)
		.minEpochTime(300ms)
		.maxEpochTime(2s)
		.performanceCounters(true);

	run_insert_or_assign_benchmark(bench, keys);
	run_get_or_create_benchmark(bench, keys);
    run_get_or_create_benchmark(bench, keys, 1ms);
	run_populated_cache_get_benchmark(bench, keys);

	return 0;
}

