#include <iostream>
#include <future>
#include <string>
#include <print>

#include "pipeline/pipeline.h"
#include "pipeline/position.h"


// This sample is designed to be run in a terminal or console that supports standard input and output. 
// It demonstrates the use of a xtd::pipeline for inter-thread communication, where one thread writes messages to the xtd::pipeline and another thread reads and echoes them back. 
// The program will exit when an empty line is submitted.

// FYI: Due console limitations, any data > 4096 bytes will be truncated when using std::cin and console output. This is a limitation of the console, not the xtd::pipeline implementation.
int main()
{
    std::future<void> startConsumer(xtd::pipeline& pipeline);

    std::println("Enter text lines. Submit an empty line to exit.");

    xtd::pipeline pipeline({.buffer_size = 128});
    auto consumer = startConsumer(pipeline);

    auto& writer = pipeline.writer();
    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty()) {
            break;
        }
        writer.write(line + "\n");
    }

    writer.complete();
    consumer.get();

    std::println("Exiting.");

    return 0;
}


std::future<void> startConsumer(xtd::pipeline& pipeline)
{
    return std::async(std::launch::async, [&]() {
        xtd::pipe_reader& reader = pipeline.reader();

        while (true)
        {
            const xtd::read_result result = reader.read();
            xtd::segmented_byte_view buffer = result.buffer();
            std::println("-----\tReadOnlySequence (size: {}, Segments: {})", buffer.size(), buffer.segment_count());

            xtd::position pos;
            while (buffer.position_of('\n', pos)) {
                std::println("\tECHO: {}", buffer.slice(pos).to_string());
                std::println("-----");
                buffer = buffer.slice(pos + 1, buffer.end());
            }

            reader.advance(buffer.end(), buffer.end());

            if (result.completed()) {
                break;
            }
        }

        reader.complete();
        std::println("Consumer thread completed reading.");
    });
}