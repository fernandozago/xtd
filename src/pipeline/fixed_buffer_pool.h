#ifndef PIPELINE_FIXED_BUFFER_POOL_H
#define PIPELINE_FIXED_BUFFER_POOL_H

#include <cstddef>
#include <memory>

namespace xtd
{

class fixed_buffer_pool
{
public:
    struct fixed_buffer_ptr_deleter
    {
        fixed_buffer_pool* pool = nullptr;
        void operator()(std::byte* const buffer) const noexcept
        {
            pool->release(buffer);
        }
    };

    using fixed_buffer_ptr = std::unique_ptr<std::byte, fixed_buffer_pool::fixed_buffer_ptr_deleter>;

     [[nodiscard]]
    fixed_buffer_ptr allocate_buffer()
    {
        return fixed_buffer_ptr(acquire(), fixed_buffer_ptr_deleter{ .pool = this });
    }

    explicit fixed_buffer_pool(const std::size_t buffer_size, const std::size_t max_pool_size)
        : m_buffer_size(buffer_size)
        , m_max_pool_size(max_pool_size)
        , m_buffers(max_pool_size > 0 ? new std::byte*[max_pool_size] : nullptr)
    {
    }

    ~fixed_buffer_pool()
    {
        for (std::size_t i = 0; i < m_pool_size; ++i) {
            delete[] m_buffers[i];
        }

        delete[] m_buffers;
    }

    fixed_buffer_pool(const fixed_buffer_pool&) = delete;
    fixed_buffer_pool& operator=(const fixed_buffer_pool&) = delete;
    fixed_buffer_pool(fixed_buffer_pool&&) = delete;
    fixed_buffer_pool& operator=(fixed_buffer_pool&&) = delete;

    [[nodiscard]]
    std::size_t buffer_size() const noexcept
    {
        return m_buffer_size;
    }

    [[nodiscard]]
    std::size_t pool_size() const noexcept
    {
        return m_pool_size;
    }

private:
    const std::size_t m_buffer_size;
    const std::size_t m_max_pool_size;

    std::byte** m_buffers;
    std::size_t m_pool_size = 0;

    [[nodiscard]]
    std::byte* acquire()
    {
        if (m_pool_size == 0) {
            return new std::byte[m_buffer_size];
        }

        return m_buffers[--m_pool_size];
    }

    void release(std::byte* const buffer) noexcept
    {
        if (buffer == nullptr) {
            return;
        }

        if (m_pool_size >= m_max_pool_size) {
            delete[] buffer;
            return;
        }

        m_buffers[m_pool_size++] = buffer;
    }
};

} // namespace xtd

#endif