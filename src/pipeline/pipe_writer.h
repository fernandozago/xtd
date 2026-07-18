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

public:
    pipe_writer(const pipe_writer&) = delete;
    pipe_writer& operator=(const pipe_writer&) = delete;
    pipe_writer(pipe_writer&&) = delete;
    pipe_writer& operator=(pipe_writer&&) = delete;

    // Writes binary data into the pipeline.
    // data: Pointer to the bytes to write.
    // length: Number of bytes to write.
    std::size_t write(const std::byte* data, std::size_t length);

    // Writes a trivially copyable value into the pipeline.
    // T: The value type.
    // value: The value to write.
    template <typename T>
    requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
    std::size_t write(const T& value);

    // Marks the writer as complete and wakes waiting readers.
    void complete() noexcept;

private:
    pipeline& m_state;
    explicit pipe_writer(pipeline& state)  noexcept
        : m_state(state) 
    {}
};

} // namespace xtd

#endif // PIPELINE_PIPE_WRITER_H
