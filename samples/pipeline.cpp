#include <string>
#include <thread>
#include <print>

#include "pipeline/pipeline.h"
#include "pipeline/position.h"


int main()
{
	// See pipe_options.h for more info about default values
	// This xtd::pipeline is configured with a 1 KB buffer size, a resume threshold of 128 KB, and a pause threshold of 256 KB for testing...
	xtd::pipeline pipeline{xtd::pipe_options
	{
		.buffer_size = 1024,
		.resume_writer_threshold = 1024 * 128,
		.pause_writer_threshold = 1024 * 256
	}};

	auto& reader = pipeline.reader();
	
	// Start a producer thread that writes messages to the pipeline
	std::thread producer([&]() {
		auto& writer = pipeline.writer();
		const int randSize = 1024 * 2;
		writer.write("First message\n");
		for (int i = 1; i <= 100'000; ++i)
		{
			// Generate random sized messages between (256 and randSize) + 1 char (newLine), followed by a newline character
			writer.write(std::string((std::rand() % randSize) + 256, 'x') + "\n");
		}

		std::println("Producer thread completed writing messages.");
		// Wait for 250 milliseconds before completing the writer to simulate a delay
		// std::this_thread::sleep_for(std::chrono::milliseconds(250));
		writer.complete();
	});

	int lineCount = 0;
	size_t byteCount = 0;
	while (true)
	{
		const xtd::read_result result = reader.read(); // Read data from the pipeline
		xtd::segmented_byte_view readOnlySequence = result.buffer(); // Get the read-only sequence from the result
		const size_t size = readOnlySequence.size(); // Get the size of the read-only sequence (total number of bytes available to read)
		const size_t segments_size = readOnlySequence.segment_count(); // Get the number of segments in the read-only sequence (total number of contiguous memory blocks)
		
		xtd::position newLinePos{}; // Initialize a xtd::position to track the location of the next newline character
		while (readOnlySequence.position_of('\n', newLinePos)) // Find the xtd::position of the next newline character (exclusive) and update newLinePos
		{
			newLinePos += 1; // +1 includes the offset of the newline character itself, so that the slice includes the newline character in the lineBytes
			const xtd::segmented_byte_view lineBytes = readOnlySequence.slice(newLinePos); // Slice the read-only sequence up to the newline xtd::position to get the line bytes
			++lineCount;
			byteCount += lineBytes.size();
			
			// Uncomment line below to print each line read from the pipeline
			// std::println("{}", lineBytes.to_string()); 

			readOnlySequence = readOnlySequence.slice(newLinePos, readOnlySequence.end()); // Slice the read-only sequence to remove the processed line and continue searching for the next newline character
		}
	
		std::println("segmented_byte_view (size: {}) (segments: {})", size, segments_size);
		reader.advance(readOnlySequence.start(), readOnlySequence.end()); // Advance the reader to the end of the processed read-only sequence, marking it as consumed and examined

		// Uncomment line below to simulate processing time for each read operation
		// and force backpressure on xtd::pipe_writer when full
		// std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 5) + 1)); 

		if (result.completed()) {
			break;
		}
	}

	reader.complete();
	producer.join();

	printf("xtd::pipeline test completed successfully with %d lines read.\n", lineCount);
	printf("Total bytes read: %zu MB\n", byteCount / (1024 * 1024));
	return 0;
}