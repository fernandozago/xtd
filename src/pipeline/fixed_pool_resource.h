#ifndef PIPELINE_FIXED_BUFFER_POOL_H
#define PIPELINE_FIXED_BUFFER_POOL_H

#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <new>
#include <print>
#include <vector>

namespace xtd
{

class fixed_pool_resource final : public std::pmr::memory_resource
{
private:
    static constexpr std::size_t buffer_alignment =
        alignof(std::byte);

    std::vector<void*> m_available_buffers;

    const std::size_t m_max_pool_size;

    std::size_t m_reused_buffers{0};
    std::size_t m_discarded_buffers{0};
    std::size_t m_created_buffers{0};
    std::size_t m_active_buffers{0};
    std::size_t m_peak_active_buffers{0};
    std::size_t m_current_total_buffers{0};
    std::size_t m_peak_total_buffers{0};

    [[nodiscard]]
    void* do_allocate(
        const std::size_t bytes,
        const std::size_t alignment) override
    {
        assert(bytes > 0);
        assert(alignment == buffer_alignment);

        if (m_available_buffers.empty()) {
            ++m_created_buffers;
            ++m_active_buffers;
            ++m_current_total_buffers;
            m_peak_active_buffers = std::max(m_peak_active_buffers, m_active_buffers);
            m_peak_total_buffers = std::max(m_peak_total_buffers, m_current_total_buffers);
            return ::operator new(bytes);
        }

        void* buffer = m_available_buffers.back();
        m_available_buffers.pop_back();

        ++m_reused_buffers;
        ++m_active_buffers;
        m_peak_active_buffers = std::max(m_peak_active_buffers, m_active_buffers);

        return buffer;
    }

    void do_deallocate(
        void* const pointer,
        [[maybe_unused]] const std::size_t bytes,
        const std::size_t alignment) noexcept override
    {
        assert(pointer != nullptr);
        assert(bytes > 0);
        assert(alignment == buffer_alignment);
        assert(m_active_buffers > 0);

        --m_active_buffers;

        if (m_available_buffers.size() < m_max_pool_size) {
            m_available_buffers.push_back(pointer);
            return;
        }

        ++m_discarded_buffers;
        assert(m_current_total_buffers > 0);
        --m_current_total_buffers;

        ::operator delete(pointer);
    }

    [[nodiscard]]
    bool do_is_equal(
        const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }

public:
    fixed_pool_resource(const fixed_pool_resource&) = delete;
    fixed_pool_resource& operator=(const fixed_pool_resource&) = delete;

    fixed_pool_resource(fixed_pool_resource&&) = delete;
    fixed_pool_resource& operator=(fixed_pool_resource&&) = delete;

    explicit fixed_pool_resource(
        const std::size_t max_pool_size)
        : m_max_pool_size{max_pool_size}
    {
        assert(max_pool_size > 0);

        m_available_buffers.reserve(max_pool_size);
    }

    ~fixed_pool_resource() override
    {
        #if DEBUG
        try {
            std::println("fixed_pool_resource usage:");
            std::println("  max pool size:      {}", m_max_pool_size);
            std::println("  created buffers:    {}", m_created_buffers);
            std::println("  retained buffers:   {}", m_available_buffers.size());
            std::println("  reused buffers:     {}", m_reused_buffers);
            std::println("  discarded buffers:  {}", m_discarded_buffers);
            std::println("  peak active:        {}", m_peak_active_buffers);
            std::println("  peak total:         {}", m_peak_total_buffers);
        }
        catch (...) {
            // Destructors must not allow logging failures to escape.
        }
        #endif

        release();
    }

    void release() noexcept
    {
        for (void* buffer : m_available_buffers) {
            ::operator delete(buffer);
        }

        m_available_buffers.clear();
    }

    [[nodiscard]]
    std::size_t pool_size() const noexcept
    {
        return m_available_buffers.size();
    }

    [[nodiscard]]
    std::size_t max_pool_size() const noexcept
    {
        return m_max_pool_size;
    }

    [[nodiscard]]
    std::size_t reused_buffers() const noexcept
    {
        return m_reused_buffers;
    }

    [[nodiscard]]
    std::size_t discarded_buffers() const noexcept
    {
        return m_discarded_buffers;
    }
};

} // namespace xtd

#endif