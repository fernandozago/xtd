#ifndef PIPELINE_PIPE_WRITER_H
#define PIPELINE_PIPE_WRITER_H

#include "pipeline_impl.h"

namespace xtd {

    class pipe_writer {
    private:
        pipeline& m_state;

    public:
        explicit pipe_writer(pipeline& state)  noexcept
            : m_state(state) 
        {}
        
        // Writes binary data into the pipeline.
        // data: Pointer to the bytes to write.
        // length: Number of bytes to write.
        std::size_t write(const std::byte* data, std::size_t length, std::stop_token stop_token = {})
        {
            return m_state.write(data, length, stop_token);
        }

        // Writes a trivially copyable value into the pipeline.
        // T: The value type.
        // value: The value to write.
        template <typename T>
        requires (std::convertible_to<const T&, std::string_view> || std::is_trivially_copyable_v<T>)
        std::size_t write(const T& value, std::stop_token stop_token = {})
        {
            if constexpr (std::convertible_to<const T&, std::string_view>)
            {
                std::string_view strView = static_cast<std::string_view>(value);
                return m_state.write(reinterpret_cast<const std::byte*>(strView.data()), strView.size(), stop_token);
            }
            else
            {
                return m_state.write(reinterpret_cast<const std::byte*>(&value), sizeof(T), stop_token);
            }
        }

        // Marks the writer as complete and wakes waiting readers.
        void complete()
        {
            m_state.complete_writer();
        }
    };

} // namespace xtd

#endif // PIPELINE_PIPE_WRITER_H
