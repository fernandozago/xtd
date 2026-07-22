#ifndef PIPELINE_FIXED_BUFFER_POOL_H
#define PIPELINE_FIXED_BUFFER_POOL_H

#include <cstddef>
#include <memory>
#include <vector>

namespace xtd
{

class fixed_buffer_pool
{
public:
    struct unique_ptr_releaser
    {
        fixed_buffer_pool* pool = nullptr;
        void operator()(std::byte* buffer) const noexcept
        {
            if (buffer == nullptr) return;
            pool->release(buffer);
        }
    };

    using fixed_buffer_ptr = std::unique_ptr<std::byte[], unique_ptr_releaser>;

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
        return fixed_buffer_ptr{m_available_buffers.empty()
                ? std::make_unique_for_overwrite<std::byte[]>(m_buffer_size).release()
                : get_buffer_from_pool().release(), 
            unique_ptr_releaser{this}};
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
    inline void release(std::byte* buffer) noexcept
    {
        std::unique_ptr<std::byte[]> buffer_ptr(buffer);
        if (m_available_buffers.size() < m_max_pool_size) {
            m_available_buffers.push_back(std::move(buffer_ptr));
        }

        // If the pool is full, `buffer_ptr` automatically frees the memory.
    }

    inline std::unique_ptr<std::byte[]> get_buffer_from_pool() noexcept
    {
        std::unique_ptr<std::byte[]> buffer = std::move(m_available_buffers.back());
        m_available_buffers.pop_back();
        return buffer;
    }

    const std::size_t m_buffer_size;
    const std::size_t m_max_pool_size;
    std::vector<std::unique_ptr<std::byte[]>> m_available_buffers;
};

} // namespace xtd

#endif