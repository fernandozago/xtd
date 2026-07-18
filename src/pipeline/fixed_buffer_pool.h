#ifndef PIPELINE_FIXED_BUFFER_POOL_H
#define PIPELINE_FIXED_BUFFER_POOL_H

#include <cstddef>
#include <memory>
#include <vector>

namespace xtd
{

class fixed_buffer_pool
{
private:
    using buffer_ref = std::unique_ptr<std::byte[]>;

public:
    struct buffer_deleter
    {
        fixed_buffer_pool* pool = nullptr;
        void operator()(std::byte* const buffer) const noexcept
        {
            if (buffer == nullptr) return;
            pool->release(buffer_ref(buffer));
        }
    };

    using fixed_buffer_ptr = std::unique_ptr<std::byte[], buffer_deleter>;

    explicit fixed_buffer_pool(const std::size_t buffer_size, const std::size_t max_pool_size)
        : m_buffer_size(buffer_size)
        , m_max_pool_size(max_pool_size)
    {
        // Ensures release() never reallocates and can remain noexcept.
        m_available_buffers.reserve(max_pool_size);
    }

    [[nodiscard]]
    fixed_buffer_ptr get_buffer()
    {
        if (m_available_buffers.empty()) {
            return {
                new std::byte[m_buffer_size], // new here is ok, because we are using a unique_ptr to manage the memory and ensure it is freed.
                buffer_deleter{this} // this custom deleter will relate the buffer back to the pool when it is no longer needed.
            };
        }

        buffer_ref buffer = std::move(m_available_buffers.back());
        m_available_buffers.pop_back();

        return {
            buffer.release(),
            buffer_deleter{this}
        };
    }

    [[nodiscard]]
    std::size_t buffer_size() const noexcept
    {
        return m_buffer_size;
    }

    [[nodiscard]]
    std::size_t pool_size() const noexcept
    {
        return m_available_buffers.size();
    }

    fixed_buffer_pool(const fixed_buffer_pool&) = delete;
    fixed_buffer_pool& operator=(const fixed_buffer_pool&) = delete;
    fixed_buffer_pool(fixed_buffer_pool&&) = delete;
    fixed_buffer_pool& operator=(fixed_buffer_pool&&) = delete;

private:
    void release(buffer_ref buffer) noexcept
    {
        if (m_available_buffers.size() < m_max_pool_size) {
            m_available_buffers.push_back(std::move(buffer));
        }

        // If the pool is full, `buffer` automatically frees the memory.
    }

    const std::size_t m_buffer_size;
    const std::size_t m_max_pool_size;

    std::vector<buffer_ref> m_available_buffers;
};

} // namespace xtd

#endif