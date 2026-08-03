#ifndef PIPELINE_PIPE_WRITER_H
#define PIPELINE_PIPE_WRITER_H

#include <stop_token>

namespace xtd {
// forward declaration of pipeline class
class pipeline; 

class pipe_writer {
private:
    friend pipeline;

public:
    pipeline& m_state;

    explicit pipe_writer(pipeline& state)  noexcept;
    
    // Writes binary data into the pipeline.
    // data: Pointer to the bytes to write.
    // length: Number of bytes to write.
    std::size_t write(const std::byte* data, std::size_t length, std::stop_token stop_token = {});

    // Writes a trivially copyable value into the pipeline.
    // T: The value type.
    // value: The value to write.
    template <typename T>
    requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
    std::size_t write(const T& value, std::stop_token stop_token = {});

    // Marks the writer as complete and wakes waiting readers.
    void complete();
};

} // namespace xtd

#endif // PIPELINE_PIPE_WRITER_H
