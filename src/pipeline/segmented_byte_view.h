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

struct read_result;
class test_helper_segmented_byte_view;

struct segmented_byte_view
{
friend struct read_result;
friend class test_helper_segmented_byte_view;

private:
    std::vector<std::span<const std::byte>> m_segments;
    std::uint64_t m_sequence_id;
    std::size_t m_begin_offset;
    std::size_t m_size;

    inline static void argument_assert(bool condition, const char* message) {
        if (!condition) {
            throw std::invalid_argument(message);
        }
    }

    inline static void range_assert(bool condition, const char* message) {
        if (!condition) {
            throw std::out_of_range(message);
        }
    }

    [[nodiscard]]
    static std::size_t validate_size(std::size_t begin_offset, std::size_t end_offset)
    {
        range_assert(begin_offset <= end_offset, "begin_offset must be <= end_offset");
        return end_offset - begin_offset;
    }

    [[nodiscard]]
    std::size_t end_offset() const noexcept
    {
        return m_begin_offset + m_size;
    }

    void validate_slice_range(std::size_t slice_begin, std::size_t slice_end) const
    {
        range_assert(slice_begin <= slice_end,
            "slice begin must be <= slice end");

        range_assert(slice_begin >= m_begin_offset && slice_begin <= end_offset(),
            "slice begin is out of range");

        range_assert(slice_end >= m_begin_offset && slice_end <= end_offset(),
            "slice end is out of range");
    }

    inline void validate_relative_slice(std::size_t begin_offset, std::size_t size) const
    {
        range_assert(begin_offset <= m_size, 
            "slice begin offset is out of range");
        range_assert(size <= m_size - begin_offset, 
            "slice size is out of range");
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
        argument_assert(end.m_sequence_id == m_sequence_id, "end must belong to this sequence");
        return slice_range(m_begin_offset, end.offset_in_sequence());
    }

    // Creates a slice between two positions.
    [[nodiscard]]
    segmented_byte_view slice(const position& begin, const position& end) const
    {
        argument_assert(begin.m_sequence_id == m_sequence_id, "begin must belong to this sequence");
        argument_assert(end.m_sequence_id == m_sequence_id, "end must belong to this sequence");

        return slice_range(begin.offset_in_sequence(), end.offset_in_sequence());
    }

    // Creates a slice from a relative offset up to an absolute position.
    [[nodiscard]]
    segmented_byte_view slice(std::size_t begin_offset, const position& end) const
    {
        argument_assert(end.m_sequence_id == m_sequence_id, "end must belong to this sequence");
        range_assert(begin_offset <= m_size, "slice begin offset is out of range");

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
    position position_of(std::byte value) const
    {
        std::size_t segment_begin = m_begin_offset;

        for (const std::span<const std::byte> segment : m_segments) {
            const auto found = std::ranges::find(segment, value);

            if (found != segment.end()) {
                return position(
                    segment_begin + std::ranges::distance(segment.begin(), found), 
                    m_sequence_id
                );
            }
            else {
                segment_begin += segment.size();
            }
        }

        return {};
    }

    [[nodiscard]]
    position position_of(char value) const
    {
        return position_of(static_cast<std::byte>(value));
    }

    // Copies bytes into the destination span.
    [[nodiscard]]
    std::size_t copy_to(const std::span<std::byte>& destination) const noexcept
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
        argument_assert(destination != nullptr || destination_size == 0, "destination must not be null when destination_size > 0");
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
    bool copy_to(T& destination) const
    {
        argument_assert(sizeof(T) <= m_size, "buffer size is smaller than the size of the destination type");
        const std::span<std::byte> destination_bytes = std::as_writable_bytes(std::span<T, 1>{&destination, 1});
        return copy_to(destination_bytes) == destination_bytes.size();
    }

    [[nodiscard]]
    std::string to_string() const noexcept
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