#ifndef PIPELINE_PIPE_WRITER_H
#define PIPELINE_PIPE_WRITER_H

#include <cstddef>
#include <concepts>
#include <string_view>

namespace xtd 
{

class pipeline;

class pipe_writer
{
friend class pipeline;

private:
    pipeline& m_state;
    pipe_writer(pipeline& state)  noexcept
        : m_state(state) 
    {}

public:
    pipe_writer(const pipe_writer&) = delete;
    pipe_writer& operator=(const pipe_writer&) = delete;
    pipe_writer(pipe_writer&&) = delete;
    pipe_writer& operator=(pipe_writer&&) = delete;

    /// <summary>
    /// Writes binary data into the pipeline.
    /// </summary>
    /// <param name="data">Pointer to the bytes to write.</param>
    /// <param name="length">Number of bytes to write.</param>
    std::size_t write(const std::byte* data, std::size_t length);

    /// <summary>
    /// Writes a trivially copyable value into the pipeline.
    /// </summary>
    /// <typeparam name="T">The value type.</typeparam>
    /// <param name="value">The value to write.</param>
    template <typename T>
    requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
    std::size_t write(const T& value);

    /// <summary>
    /// Marks the writer as complete and wakes waiting readers.
    /// </summary>
    void complete() noexcept;
};

} // namespace xtd

#endif // PIPELINE_PIPE_WRITER_H
