#ifndef PIPELINE_SEGMENTED_BYTE_VIEW_H
#define PIPELINE_SEGMENTED_BYTE_VIEW_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "position.h"

namespace xtd
{

class pipeline;
class test_helper_segmented_byte_view;

struct segmented_byte_view
{
friend class pipeline;
friend class test_helper_segmented_byte_view;

private:
    std::vector<std::span<const std::byte>> m_segments;
    std::uint64_t m_sequence_id;
    std::size_t m_begin_offset;
    std::size_t m_size;

    [[nodiscard]]
    static std::size_t validate_size(std::size_t begin_offset, std::size_t end_offset)
    {
        if (begin_offset > end_offset) {
            throw std::out_of_range("sequence range is invalid");
        }

        return end_offset - begin_offset;
    }

    [[nodiscard]]
    std::size_t end_offset() const noexcept
    {
        return m_begin_offset + m_size;
    }

    void assert_invariants() const noexcept
    {
#ifndef NDEBUG
        std::size_t visible_size = 0;

        for (const std::span<const std::byte> segment : m_segments) {
            visible_size += segment.size();
        }

        assert(visible_size == m_size);
#endif
    }

    void ensure_position_belongs_to_sequence(const position& pos, const char* parameter_name) const
    {
        if (pos.m_sequence_id != m_sequence_id) {
            throw std::invalid_argument(std::string(parameter_name) + " must belong to this sequence");
        }
    }

    void validate_slice_range(std::size_t slice_begin, std::size_t slice_end) const
    {
        const std::size_t current_begin = m_begin_offset;
        const std::size_t current_end = end_offset();

        if (slice_begin < current_begin || slice_begin > current_end) {
            throw std::out_of_range(
                "slice begin is out of range");
        }

        if (slice_end < current_begin || slice_end > current_end) {
            throw std::out_of_range(
                "slice end is out of range");
        }

        if (slice_begin > slice_end) {
            throw std::out_of_range(
                "slice range is invalid");
        }
    }

    void validate_relative_slice(std::size_t begin_offset, std::size_t size) const
    {
        if (begin_offset > m_size) {
            throw std::out_of_range("slice begin offset is out of range");
        }

        if (size > m_size - begin_offset) {
            throw std::out_of_range("slice size is out of range");
        }
    }

    [[nodiscard]]
    segmented_byte_view slice_range(std::size_t slice_begin, std::size_t slice_end) const
    {
        validate_slice_range(slice_begin, slice_end);

        if (slice_begin == slice_end) {
            return segmented_byte_view(
                std::vector<std::span<const std::byte>>{},
                m_sequence_id,
                slice_begin);
        }

        std::vector<std::span<const std::byte>> sliced_segments;
        sliced_segments.reserve(m_segments.size());

        std::size_t segment_begin = m_begin_offset;

        for (const std::span<const std::byte> segment : m_segments) {
            const std::size_t segment_end = segment_begin + segment.size();

            if (segment_end <= slice_begin) {
                segment_begin = segment_end;
                continue;
            }

            if (segment_begin >= slice_end) {
                break;
            }

            const std::size_t overlap_begin = std::max(segment_begin, slice_begin);
            const std::size_t overlap_end = std::min(segment_end, slice_end);
            sliced_segments.emplace_back(segment.subspan(overlap_begin - segment_begin,overlap_end - overlap_begin));
            segment_begin = segment_end;
        }

        return segmented_byte_view(
            std::move(sliced_segments),
            m_sequence_id,
            slice_begin);
    }

    [[nodiscard]]
    static std::size_t calculate_size(
        std::span<const std::span<const std::byte>> segments) noexcept
    {
        return std::ranges::fold_left(segments,
            std::size_t{0},
            [](std::size_t size, std::span<const std::byte> segment) {
                return size + segment.size();
            });
    }

    segmented_byte_view(std::vector<std::span<const std::byte>> segments, std::uint64_t sequence_id, std::size_t begin_offset)
        : m_segments(std::move(segments))
        , m_sequence_id(sequence_id)
        , m_begin_offset(begin_offset)
        , m_size(calculate_size(m_segments))
    {
    }

    segmented_byte_view(std::span<const std::byte> segment, std::uint64_t sequence_id, std::size_t begin_offset)
        : m_segments{segment}
        , m_sequence_id(sequence_id)
        , m_begin_offset(begin_offset)
        , m_size(segment.size())
    {
    }

    segmented_byte_view(std::vector<std::span<const std::byte>> segments, std::uint64_t sequence_id)
        : m_segments(std::move(segments))
        , m_sequence_id(sequence_id)
        , m_begin_offset(0)
        , m_size(calculate_size(m_segments))
    {
    }

public:
    segmented_byte_view() = delete;
    segmented_byte_view(const segmented_byte_view&) = default;
    segmented_byte_view& operator=(const segmented_byte_view&) = default;
    segmented_byte_view(segmented_byte_view&&) noexcept = default;
    segmented_byte_view& operator=(segmented_byte_view&&) noexcept = default;
    ~segmented_byte_view() = default;

    // Gets the inclusive beginning position.
    [[nodiscard]]
    position begin() const noexcept
    {
        return position{m_begin_offset, m_sequence_id};
    }

    // Gets the exclusive ending position.
    [[nodiscard]]
    position end() const noexcept
    {
        return position{end_offset(), m_sequence_id};
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return m_size == 0;
    }

    [[nodiscard]]
    std::size_t segment_count() const noexcept
    {
        return m_segments.size();
    }

    [[nodiscard]]
    std::span<const std::span<const std::byte>> segments() const noexcept
    {
        return {m_segments.data(), m_segments.size()};
    }

    // Creates a slice from this view's beginning up to end.
    [[nodiscard]]
    segmented_byte_view slice(const position& end) const
    {
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(m_begin_offset, end.offset_in_sequence());
    }

    // Creates a slice between two positions.
    [[nodiscard]]
    segmented_byte_view slice(const position& begin, const position& end) const
    {
        ensure_position_belongs_to_sequence(begin, "begin");
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(begin.offset_in_sequence(), end.offset_in_sequence());
    }

    // Creates a slice from a relative offset up to an absolute position.
    [[nodiscard]]
    segmented_byte_view slice(std::size_t begin_offset, const position& end) const
    {
        ensure_position_belongs_to_sequence(end, "end");

        if (begin_offset > m_size) {
            throw std::out_of_range("slice begin offset is out of range");
        }

        const std::size_t absolute_begin = m_begin_offset + begin_offset;
        return slice_range(absolute_begin, end.offset_in_sequence());
    }

    // Creates a slice from a relative offset spanning size bytes.
    [[nodiscard]]
    segmented_byte_view slice(std::size_t begin_offset, std::size_t size) const
    {
        validate_relative_slice(begin_offset, size);
        const std::size_t absolute_begin = m_begin_offset + begin_offset;
        return slice_range(absolute_begin, absolute_begin + size);
    }

    // Finds the first occurrence of value.
    [[nodiscard]]
    bool position_of(std::byte value, position& result) const
    {
        std::size_t segment_begin = m_begin_offset;

        for (const std::span<const std::byte> segment : m_segments) {
            const auto found = std::ranges::find(segment, value);

            if (found != segment.end()) {
                const auto local_offset = 
                    static_cast<std::size_t>(std::ranges::distance(segment.begin(), found));

                result = position{segment_begin + local_offset, m_sequence_id};
                return true;
            }

            segment_begin += segment.size();
        }

        return false;
    }

    [[nodiscard]]
    bool position_of(char value, position& result) const
    {
        return position_of(static_cast<std::byte>(value), result);
    }

    // Copies bytes into the destination span.
    [[nodiscard]]
    std::size_t copy_to(std::span<std::byte> destination) const noexcept
    {
        const std::size_t target_size = std::min(destination.size(), m_size);
        if (target_size == 0) {
            return 0;
        }

        std::size_t copied = 0;

        for (const std::span<const std::byte> segment : m_segments) {
            if (copied == target_size) {
                break;
            }

            const std::size_t chunk_size = std::min(segment.size(), target_size - copied);
            std::ranges::copy_n(segment.begin(), chunk_size, destination.begin() + static_cast<std::ptrdiff_t>(copied));
            copied += chunk_size;
        }

        return copied;
    }

    // Copies bytes into a pointer and size buffer.
    [[nodiscard]]
    std::size_t copy_to(std::byte* destination, const std::size_t destination_size) const
    {
        if (destination == nullptr && destination_size > 0) {
            throw std::invalid_argument("destination must not be null when destination_size > 0");
        }

        return copy_to(std::span<std::byte>{destination, destination_size});
    }

    // Copies bytes into an existing vector.
    [[nodiscard]]
    std::size_t copy_to(std::vector<std::byte>& destination) const noexcept
    {
        return copy_to(std::span<std::byte>{destination});
    }

    // Copies the complete view into a trivially copyable value.
    template <typename T>
    requires (std::is_trivially_copyable_v<T> &&!std::is_convertible_v<T, std::string_view>)
    [[nodiscard]]
    bool copy_to(T& destination) const noexcept
    {
        if (m_size != sizeof(T)) {
            return false;
        }

        const std::span<std::byte> destination_bytes = std::as_writable_bytes(std::span<T, 1>{&destination, 1});
        return copy_to(destination_bytes) == destination_bytes.size();
    }

    [[nodiscard]]
    std::string to_string() const
    {
        if (empty()) {
            return {};
        }

        std::string result;
        result.reserve(m_size);

        for (const std::span<const std::byte> segment : m_segments) {
            result.append(reinterpret_cast<const char*>(segment.data()), segment.size());
        }

        return result;
    }
};

} // namespace xtd

#endif // PIPELINE_SEGMENTED_BYTE_VIEW_H