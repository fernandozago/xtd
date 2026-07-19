#include <cassert>
#include <future>
#include <thread>
#include <barrier>
#include <string>
#include "utils/utils.h"
#include "channel/channel.h"


struct TestData {
	int value;
	float z;

	TestData(int value, float z)
		: value(value)
		, z(z)
	{}
};

static std::barrier barrier{ 5 };

template<typename TChannel>
int run_sample(TChannel& channel, const char* channel_name);

int main(int argc, char* argv[])
{
	if (argc < 2) {
		println_locked("Usage: {} [bounded OR unbounded]", argv[0]);
		return 1;
	}

	std::string mode = argv[1];
	if (is_equal_ignore_case(mode, "bounded")) {
		xtd::channel<TestData> channel(500);
		return run_sample(channel, "Bounded");
	}
	else if (is_equal_ignore_case(mode, "unbounded")) {
		xtd::channel<TestData> channel;
		return run_sample(channel, "Unbounded");
	}
	
	println_locked("Invalid mode: {}. Use 'bounded' or 'unbounded'.", mode);
	return 1;
}


template<typename TChannel>
int run_sample(TChannel& channel, const char* channel_name)
{
    std::thread writer_a([&channel]() {
        xtd::channel_writer<TestData>& writer = channel.writer();
        barrier.arrive_and_wait();
        for (int i = 0; i < 1000; ++i) {
            (void)writer.emplace(i, static_cast<float>(i) * 0.1f);
        }
        println_locked("Writer A finished writing.");
    });

    auto writer_b = std::async(std::launch::async, [&channel]() {
        xtd::channel_writer<TestData>& writer = channel.writer();
        barrier.arrive_and_wait();
        for (int i = 0; i < 1000; ++i) {
            (void)writer.emplace(i, static_cast<float>(i) * 0.1f);
        }
        println_locked("Writer B finished writing.");
    });

    std::thread slow_consumer([&channel]() {
        xtd::channel_reader<TestData>& reader = channel.reader();
        barrier.arrive_and_wait();
        while (auto value = reader.read()) {
            println_locked("Consumer 1 read value: {}, {}", value->value, value->z);
            //std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        println_locked("Consumer 1 finished reading.");
    });

    auto faster_consumer = std::async(std::launch::async, [&channel]() {
        xtd::channel_reader<TestData>& reader = channel.reader();
        barrier.arrive_and_wait();
        while (auto value = reader.read()) {
            println_locked("Consumer 2 read value: {}, {}", value->value, value->z);
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        println_locked("Consumer 2 finished reading.");
    });

    barrier.arrive_and_wait(); // Start writers/readers at the same time

    writer_a.join();
    writer_b.get();
    channel.writer().complete();
    assert(channel.writer().emplace(0, 0.0f) == false);

    slow_consumer.join();
    faster_consumer.get();
    assert(channel.reader().size() == 0);

    println_locked("{} channel test completed successfully.", channel_name);
    return 0;
}