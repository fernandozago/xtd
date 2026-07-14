#ifndef PIPELINE_PIPE_UTILS_H
#define PIPELINE_PIPE_UTILS_H

#include <cstddef>
#include <cerrno>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>

#include "pipe_writer.h"

namespace xtd
{

struct pipe_utils
{
    /// <summary>
    /// Starts a background thread that copies file bytes into the given pipeline writer and completes the writer.
    /// </summary>
    /// <param name="path">Path to the source file.</param>
    /// <param name="writer">Destination pipeline writer.</param>
    /// <param name="chunkSize">Read chunk size in bytes.</param>
    /// <returns>A joinable thread performing the file copy.</returns>
    static std::thread threaded_copy_file_from_path(const std::string& path, pipe_writer& writer, const std::size_t chunkSize = 4096) {
        if (chunkSize == 0) {
            throw std::invalid_argument("chunk size must be greater than zero");
        }

        {
            std::ifstream probe(path, std::ios::binary);
            if (!probe) {
                throw std::runtime_error("failed to open file");
            }
        }

        return std::thread([path, &writer, chunkSize]() {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                writer.complete();
                return;
            }

            std::vector<std::byte> buffer(chunkSize);

            while (true) {
                in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
                const auto readCount = static_cast<std::size_t>(in.gcount());

                if (readCount > 0) {
                    writer.write(buffer.data(), readCount);
                }

                if (in.eof() || in.fail()) break;
            }

            writer.complete();
        });
    }

    /// <summary>
    /// Starts a background thread that copies bytes from a socket into the given pipeline writer and completes the writer.
    /// </summary>
    /// <param name="socketFd">Source socket file descriptor (expected to be readable).</param>
    /// <param name="writer">Destination pipeline writer.</param>
    /// <param name="chunkSize">Read chunk size in bytes.</param>
    /// <returns>A joinable thread performing the socket copy.</returns>
    static std::thread threaded_copy_from_socket(int socketFd, pipe_writer& writer, std::size_t chunkSize = 4096)
    {
        if (socketFd < 0) {
            throw std::invalid_argument("socketFd must be a valid descriptor");
        }

        if (chunkSize == 0) {
            throw std::invalid_argument("chunk size must be greater than zero");
        }

        return std::thread([socketFd, &writer, chunkSize]() {
            std::vector<std::byte> buffer(chunkSize);

            while (true) {
                const ssize_t readCount = ::recv(socketFd, buffer.data(), buffer.size(), 0);

                if (readCount > 0) {
                    writer.write(buffer.data(), static_cast<std::size_t>(readCount));
                    continue;
                }

                if (readCount == 0) break;
                if (errno == EINTR) continue;
                break;
            }

            writer.complete();
        });
    }
};

} // namespace xtd

#endif // PIPELINE_PIPE_UTILS_H