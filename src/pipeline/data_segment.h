#ifndef PIPELINE_DATA_SEGMENT_H
#define PIPELINE_DATA_SEGMENT_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include "fixed_buffer_pool.h"

namespace xtd
{

struct data_segment
{
private:
    fixed_buffer_pool::fixed_buffer_ptr m_buffer;
    std::span<std::byte> m_writable_span;
    std::span<const std::byte> m_readable_span;
    const std::size_t m_capacity;

public:
    explicit data_segment(fixed_buffer_pool& resource)
        : m_buffer(resource.get_buffer())
        , m_writable_span(m_buffer.get(), resource.buffer_size())
        , m_readable_span(m_buffer.get(), std::size_t{0})
        , m_capacity(resource.buffer_size())
    {
    }

    data_segment(const data_segment&) = delete;
    data_segment& operator=(const data_segment&) = delete;

    data_segment(data_segment&&) = delete;
    data_segment& operator=(data_segment&&) = delete;

    [[nodiscard]]
    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    [[nodiscard]]
    std::size_t readable_size() const noexcept
    {
        return m_readable_span.size();
    }

    [[nodiscard]]
    std::span<const std::byte> readable_bytes() const noexcept
    {
        return m_readable_span;
    }

    [[nodiscard]]
    std::size_t writable_size() const noexcept
    {
        return m_writable_span.size();
    }

    [[nodiscard]]
    const std::span<std::byte>& writable_bytes() noexcept
    {
        return m_writable_span;
    }

    void advance_read(const std::size_t size)
    {
        if (size > m_readable_span.size()) {
            throw std::out_of_range("consume size exceeds readable size");
        }

        m_readable_span = m_readable_span.subspan(size);
    }

    [[nodiscard]]
    bool full() const noexcept
    {
        return m_writable_span.empty();
    }
    
    [[nodiscard]]
    std::size_t copy_from(const std::byte* const source, const std::size_t size) noexcept
    {
        const std::size_t to_copy_size = std::min(m_writable_span.size(), size);
        std::memcpy(m_writable_span.data(), source, to_copy_size);
        m_readable_span = {m_readable_span.data(), m_readable_span.size() + to_copy_size};
        m_writable_span = m_writable_span.subspan(to_copy_size);
        return to_copy_size;
    }
};

} // namespace xtd

#endif