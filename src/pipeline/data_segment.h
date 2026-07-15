#ifndef PIPELINE_DATA_SEGMENT_H
#define PIPELINE_DATA_SEGMENT_H

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <cassert>

namespace xtd
{

struct data_segment
{
private:
    const std::size_t m_capacity;
    std::unique_ptr<std::byte[]> m_buffer;
    std::span<std::byte> m_writable_span;
    std::span<const std::byte> m_readable_span;

public:
    explicit data_segment(std::size_t capacity)
        : m_capacity(capacity)
        , m_buffer(std::make_unique<std::byte[]>(capacity))
        , m_writable_span(m_buffer.get(), capacity)
        , m_readable_span(m_buffer.get(), std::size_t{0})
    {
    }

    data_segment(const data_segment&) = delete;
    data_segment& operator=(const data_segment&) = delete;
    data_segment& operator=(data_segment&&) noexcept = delete;

    data_segment(data_segment&& other) noexcept
        : m_capacity(other.m_capacity)
        , m_buffer(std::move(other.m_buffer))
        , m_writable_span(other.m_writable_span)
        , m_readable_span(other.m_readable_span)
    {
        assert(m_capacity == other.m_capacity);
        assert(m_buffer != nullptr);

        // Resets the other instance's spans to empty, as the buffer has been moved.
        other.m_writable_span = {};
        other.m_readable_span = {};
    }


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
    std::span<std::byte> writable_bytes() noexcept
    {
        return m_writable_span;
    }

    void advance(const std::size_t& size)
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
    std::size_t copy_from(const std::byte* source, const std::size_t& size) noexcept
    {
        const std::size_t copied = std::min(m_writable_span.size(), size);
        std::copy_n(source, copied, m_writable_span.begin());
        
        m_readable_span = {m_readable_span.data(), m_readable_span.size() + copied};
        m_writable_span = m_writable_span.subspan(copied);
        return copied;
    }

    void reset() noexcept
    {
        assert(m_buffer != nullptr);
        m_readable_span = {m_buffer.get(), std::size_t{0}};
        m_writable_span = {m_buffer.get(), m_capacity};
    }
};

} // namespace xtd

#endif // PIPELINE_DATA_SEGMENT_H