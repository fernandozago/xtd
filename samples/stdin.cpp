#include <future>
#include <iostream>
#include <print>
#include <string>

#include "pipeline/pipeline.h"

std::future<void> start_stdin_reader(xtd::pipeline& pipeline);
void consume(xtd::pipeline& pipeline);

// This sample is designed to be run in a terminal or console that supports standard input and output. 
// It demonstrates the use of a xtd::pipeline for inter-thread communication, where one thread writes 
//   messages to the xtd::pipeline and another thread reads and writes them to the console. 
// The program will exit when an empty line is submitted.

// FYI: Due console limitations, any data > 4096 bytes will be truncated when using std::cin and console output. 
// This is a limitation of the console, not the xtd::pipeline implementation.
int main()
{
    std::println("Enter text lines. Submit an empty line to exit.");

    xtd::pipeline pipeline({.buffer_size = 64});

    // Secondary thread reads stdin and produces pipeline data.
    auto stdin_reader_task = start_stdin_reader(pipeline);

    // Main thread consumes pipeline data.
    consume(pipeline);

    // Propagate any exception raised by the stdin thread.
    stdin_reader_task.get();
    return 0;
}

std::future<void> start_stdin_reader(xtd::pipeline& pipeline)
{
    return std::async(std::launch::async, [&pipeline]()
    {
        xtd::pipe_writer writer(pipeline);
        std::string line;
        while (std::getline(std::cin, line))
        {
            // An empty line indicates the user wants to quit the program.
            // So we exit this loop
            if (line.empty()) {
                break;
            }

            // std::getline does not include the newline character.
            writer.write(line + "\n");
        }

        // Signal the pipeline that no more data will be written,
        //   allowing the reader to complete its work when no more data is available.
        writer.complete();

        std::println("Stdin thread completed writing.");
    });
}

void consume(xtd::pipeline& pipeline)
{
    xtd::pipe_reader reader(pipeline);
    while (const xtd::read_result result = reader.read())
    {
        xtd::segmented_byte_view buffer = result.buffer();

        /* Uncomment this to get debug output */
        /*std::println(
            "-----\tReadOnlySequence (size: {}, Segments: {})",
            buffer.size(),
            buffer.segment_count());*/

        while (xtd::position pos = buffer.position_of('\n'))
        {
            const auto lineBytes = buffer.slice(pos);
            std::println("\tECHO: {}", lineBytes.to_string());

            // Advance the buffer to the next line.
            buffer.slice_in_place(pos + 1, buffer.end());
        }

        reader.advance(buffer.end(), buffer.end());

        if (result.completed()) {
            break;
        }
    }

    reader.complete();
    std::println("Main thread completed reading.");
}
