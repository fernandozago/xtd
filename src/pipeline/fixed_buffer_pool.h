#ifndef PIPELINE_FIXED_BUFFER_POOL_H
#define PIPELINE_FIXED_BUFFER_POOL_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace xtd
{

    class fixed_buffer_pool
    {
    private:
        std::vector<std::byte*> m_available_buffers;
        const std::size_t m_buffer_size;
        const std::size_t m_max_pool_size;

        struct buffer_releaser
        {
            fixed_buffer_pool& pool;

            void operator()(std::byte* buffer) const noexcept
            {
                assert(buffer != nullptr);

                if (pool.m_available_buffers.size() < pool.m_max_pool_size) {
                    pool.m_available_buffers.push_back(buffer);
                }
                else {
                    delete[] buffer;
                }
            }
        };

    public:
        using fixed_buffer_ptr = std::unique_ptr<std::byte[], buffer_releaser>;

        fixed_buffer_pool(const fixed_buffer_pool&) = delete;
        fixed_buffer_pool& operator=(const fixed_buffer_pool&) = delete;
        fixed_buffer_pool(fixed_buffer_pool&&) = delete;
        fixed_buffer_pool& operator=(fixed_buffer_pool&&) = delete;

        explicit fixed_buffer_pool(
            const std::size_t buffer_size,
            const std::size_t max_pool_size)
            : m_buffer_size{buffer_size}
            , m_max_pool_size{max_pool_size}
        {
            assert(buffer_size > 0);
            m_available_buffers.reserve(max_pool_size);
        }

        ~fixed_buffer_pool()
        {
            for (std::byte* buffer : m_available_buffers) {
                delete[] buffer;
            }
        }

        [[nodiscard]]
        fixed_buffer_ptr get_buffer()
        {
            if (m_available_buffers.empty()) {
                return fixed_buffer_ptr{new std::byte[m_buffer_size], buffer_releaser{*this}};
            }

            std::byte* buffer = m_available_buffers.back();
            m_available_buffers.pop_back();

            return fixed_buffer_ptr{buffer, buffer_releaser{*this}};
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
    };

} // namespace xtd

#endif