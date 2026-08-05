#ifndef PIPELINE_ARENA_POOL_RESOURCE_H
#define PIPELINE_ARENA_POOL_RESOURCE_H

//#define PRINT_STATS

#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <new>

#ifdef PRINT_STATS
#include <print>
#endif

namespace xtd
{

class arena_pool_resource final : public std::pmr::memory_resource
{
private:
    static constexpr std::size_t buffer_alignment = alignof(std::byte);

    const std::size_t m_segment_size;
    const std::size_t m_max_pool_size;
    const std::size_t m_total_size;

    std::byte* const m_storage;
    std::byte* const m_end;

    std::byte* m_head;
    std::byte* m_tail;

    std::size_t m_active_buffers{0};

#ifdef PRINT_STATS
    std::size_t m_total_allocations{0};
    std::size_t m_total_deallocations{0};
    std::size_t m_reused_buffers{0};
    std::size_t m_failed_allocations{0};
    std::size_t m_peak_active_buffers{0};
#endif

    [[nodiscard]]
    static std::size_t calculate_total_size(const std::size_t buffer_size, const std::size_t max_pool_size)
    {
        assert(buffer_size > 0);
        assert(max_pool_size > 0);
        return buffer_size * max_pool_size;
    }

    [[nodiscard]]
    std::byte* advance(std::byte* pointer) const noexcept
    {
        pointer += m_segment_size;
        if (pointer == m_end) {
            return m_storage;
        }
        return pointer;
    }

    [[nodiscard]]
    void* do_allocate(const std::size_t bytes, const std::size_t alignment) override
    {
        assert(bytes == m_segment_size);
        assert(alignment == buffer_alignment);
        assert(m_active_buffers != m_max_pool_size);

        std::byte* const buffer = m_head;
        m_head = advance(m_head);
        ++m_active_buffers;

#ifdef PRINT_STATS
        ++m_total_allocations;

        if (m_total_allocations > m_max_pool_size) {
            ++m_reused_buffers;
        }

        m_peak_active_buffers =
            std::max(m_peak_active_buffers, m_active_buffers);
#endif

        return buffer;
    }

    void do_deallocate([[maybe_unused]] void* const pointer, [[maybe_unused]] const std::size_t bytes, [[maybe_unused]] const std::size_t alignment) noexcept override
    {
        assert(m_active_buffers > 0);
        m_tail = advance(m_tail);
        --m_active_buffers;

#ifdef PRINT_STATS
        ++m_total_deallocations;
#endif
    }

    [[nodiscard]]
    bool do_is_equal(
        const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }

public:
    arena_pool_resource(const arena_pool_resource&) = delete;
    arena_pool_resource& operator=(const arena_pool_resource&) = delete;

    arena_pool_resource(arena_pool_resource&&) = delete;
    arena_pool_resource& operator=(arena_pool_resource&&) = delete;

    explicit arena_pool_resource(const std::size_t buffer_size, const std::size_t max_pool_size)
        : m_segment_size{buffer_size}
        , m_max_pool_size{max_pool_size}
        , m_total_size{calculate_total_size(buffer_size,max_pool_size)}
        , m_storage{static_cast<std::byte*>(::operator new(m_total_size))}
        , m_end{m_storage + m_total_size}
        , m_head{m_storage}
        , m_tail{m_storage}
    {
    }

    ~arena_pool_resource() override
    {
#ifdef PRINT_STATS
        try {
            std::println("arena_pool_resource usage:");
            std::println(
                "  buffer size:         {}",
                m_segment_size);
            std::println(
                "  arena capacity:      {}",
                m_max_pool_size);
            std::println(
                "  arena bytes:         {}",
                m_total_size);
            std::println(
                "  total allocations:   {}",
                m_total_allocations);
            std::println(
                "  total deallocations: {}",
                m_total_deallocations);
            std::println(
                "  reused buffers:      {}",
                m_reused_buffers);
            std::println(
                "  failed allocations:  {}",
                m_failed_allocations);
            std::println(
                "  peak active:         {}",
                m_peak_active_buffers);
            std::println(
                "  active at shutdown:  {}",
                m_active_buffers);
        }
        catch (...) {
        }
#endif

        assert(m_active_buffers == 0);
        ::operator delete(m_storage);
    }
};

} // namespace xtd

#endif