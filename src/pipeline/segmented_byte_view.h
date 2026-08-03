#ifndef PIPELINE_SEGMENTED_BYTE_VIEW_H
#define PIPELINE_SEGMENTED_BYTE_VIEW_H

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "position.h"

namespace xtd
{

struct read_result;
class test_helper_segmented_byte_view;

struct from_end {
    std::size_t m_offset;
    explicit from_end(std::size_t offset) : m_offset(offset) {}
};

struct segmented_byte_view
{
private:
    friend struct read_result;
    friend class test_helper_segmented_byte_view;

    struct slice_range
    {
        std::size_t begin;
        std::size_t end;
    };

    std::vector<std::span<const std::byte>> m_segments;
    std::size_t m_begin_offset;
    std::size_t m_size;

    inline static void argument_assert(bool condition, const char* message)
    {
        if (!condition) {
            throw std::invalid_argument(message);
        }
    }

    inline static void range_assert(bool condition, const char* message)
    {
        if (!condition) {
            throw std::out_of_range(message);
        }
    }

    [[nodiscard]]
    std::size_t end_offset() const noexcept
    {
        return m_begin_offset + m_size;
    }

    void validate_slice_range(const std::size_t slice_begin, const std::size_t slice_end) const
    {
        range_assert(slice_begin <= slice_end,
            "slice begin must be <= slice end");

        range_assert(slice_begin >= m_begin_offset && slice_begin <= end_offset(),
            "slice begin is out of range");

        range_assert(slice_end >= m_begin_offset && slice_end <= end_offset(),
            "slice end is out of range");
    }

    inline void validate_relative_slice(const std::size_t begin_offset, const std::size_t size) const
    {
        range_assert(begin_offset <= m_size,
            "slice begin offset is out of range");

        range_assert(size <= m_size - begin_offset,
            "slice size is out of range");
    }

    [[nodiscard]]
    slice_range resolve_slice_range(const position& end) const
    {
        return {
            m_begin_offset,
            end.sequence_offset()
        };
    }

    [[nodiscard]]
    slice_range resolve_slice_range(const position& begin, const position& end) const
    {
        return {
            begin.sequence_offset(),
            end.sequence_offset()
        };
    }

    [[nodiscard]]
    slice_range resolve_slice_range(const std::size_t begin_offset, const position& end) const
    {
        range_assert(begin_offset <= m_size,
            "slice begin offset is out of range");

        return {
            m_begin_offset + begin_offset, 
            end.sequence_offset()
        };
    }

    [[nodiscard]]
    slice_range resolve_slice_range(const std::size_t begin_offset, const std::size_t size) const
    {
        validate_relative_slice(begin_offset, size);

        const std::size_t absolute_begin = m_begin_offset + begin_offset;
        return {
            absolute_begin, 
            absolute_begin + size
        };
    }

    [[nodiscard]]
    std::vector<std::span<const std::byte>> make_slice_segments(const std::size_t slice_begin, const std::size_t slice_end) const
    {
        validate_slice_range(slice_begin, slice_end);

        std::vector<std::span<const std::byte>> result;

        if (slice_begin == slice_end) {
            return result;
        }

        result.reserve(m_segments.size());

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

            result.emplace_back(
                segment.subspan(
                    overlap_begin - segment_begin,
                    overlap_end - overlap_begin
                )
            );

            segment_begin = segment_end;
        }

        return result;
    }

    void slice_in_place_absolute(const slice_range& range)
    {
        m_segments = make_slice_segments(range.begin, range.end);
        m_begin_offset = range.begin;
        m_size = range.end - range.begin;
    }

    explicit segmented_byte_view(const segmented_byte_view& source, const slice_range& range)
        : m_segments(source.make_slice_segments(range.begin, range.end))
        , m_begin_offset(range.begin)
        , m_size(range.end - range.begin)
    {
    }

    explicit segmented_byte_view(std::vector<std::span<const std::byte>>&& segments, std::size_t size)
        : m_segments(std::move(segments))
        , m_begin_offset(0)
        , m_size(size)
    {
    }

    explicit segmented_byte_view()
        : m_segments()
        , m_begin_offset(0)
        , m_size(0)
    {
    }

public:
    [[nodiscard]]
    position begin() const noexcept
    {
        return position{m_begin_offset};
    }

    [[nodiscard]]
    position end() const noexcept
    {
        return position{end_offset()};
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

    [[nodiscard]]
    segmented_byte_view slice(const position& end) const
    {
        return segmented_byte_view{*this, resolve_slice_range(end)};
    }

    [[nodiscard]]
    segmented_byte_view slice(const position& begin, const position& end) const
    {
        return segmented_byte_view{*this, resolve_slice_range(begin, end)};
    }

    [[nodiscard]]
    segmented_byte_view slice(const std::size_t begin_offset, const position& end) const
    {
        return segmented_byte_view{*this, resolve_slice_range(begin_offset, end)};
    }

    [[nodiscard]]
    segmented_byte_view slice(const std::size_t begin_offset, const std::size_t size) const
    {
        return segmented_byte_view{*this, resolve_slice_range(begin_offset, size)};
    }

    void slice_in_place(const position& end)
    {
        slice_in_place_absolute(resolve_slice_range(end));
    }

    void slice_in_place(const position& begin, const position& end)
    {
        slice_in_place_absolute(resolve_slice_range(begin, end));
    }

    void slice_in_place(const std::size_t begin_offset, const position& end)
    {
        slice_in_place_absolute(resolve_slice_range(begin_offset, end));
    }

    void slice_in_place(const std::size_t begin_offset, const std::size_t size)
    {
        slice_in_place_absolute(resolve_slice_range(begin_offset, size));
    }

    const std::byte& operator[](const xtd::from_end& from_end) const
    {
        range_assert(from_end.m_offset > 0 && from_end.m_offset <= m_size,
            "from_end offset is out of range");

        std::size_t remaining = from_end.m_offset - 1;
        for (auto iterator = m_segments.rbegin(); iterator != m_segments.rend(); ++iterator) {
            const std::span<const std::byte> segment = *iterator;

            if (remaining < segment.size()) {
                return segment[segment.size() - 1 - remaining];
            }

            remaining -= segment.size();
        }

        std::unreachable();
    }

    const std::byte& operator[](const position& pos) const
    {
        const std::size_t absolute_offset = pos.sequence_offset();

        range_assert(absolute_offset >= m_begin_offset && absolute_offset < end_offset(),
            "position is out of range");

        return (*this)[absolute_offset - m_begin_offset];
    }

    const std::byte& operator[](const std::size_t index) const
    {
        range_assert(index < m_size, "index is out of range");

        std::size_t segment_begin = 0;

        for (const std::span<const std::byte> segment : m_segments) {
            const std::size_t segment_end = segment_begin + segment.size();

            if (index < segment_end) {
                return segment[index - segment_begin];
            }

            segment_begin = segment_end;
        }

        std::unreachable();
    }

    [[nodiscard]]
    position position_of(const std::byte value) const
    {
        std::size_t segment_begin = m_begin_offset;

        for (const std::span<const std::byte> segment : m_segments) {
            const auto found = std::ranges::find(segment, value);

            if (found != segment.end()) {
                const auto distance = std::ranges::distance(segment.begin(), found);

                return position{
                    segment_begin + static_cast<std::size_t>(distance)
                };
            }

            segment_begin += segment.size();
        }

        return position{};
    }

    [[nodiscard]]
    position position_of_any(const std::span<const std::byte> values) const
    {
        if (!values.empty()) {
            std::size_t segment_begin = m_begin_offset;

            for (const std::span<const std::byte> segment : m_segments) {
                const auto found = std::ranges::find_first_of(segment, values);

                if (found != segment.end()) {
                    const auto distance =
                        std::ranges::distance(segment.begin(), found);

                    return position{
                        segment_begin + static_cast<std::size_t>(distance)
                    };
                }

                segment_begin += segment.size();
            }
        }

        return position{};
    }

    [[nodiscard]]
    position position_of(const char value) const
    {
        return position_of(static_cast<std::byte>(value));
    }

    [[nodiscard]]
    position position_of_any(const std::string_view values) const
    {
        return position_of_any(std::span<const char>(values.data(), values.size()));
    }

    [[nodiscard]]
    position position_of_any(const std::span<const char> values) const
    {
        return position_of_any(std::span<const std::byte>(reinterpret_cast<const std::byte*>(values.data()), values.size()));
    }

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

            std::ranges::copy(
                segment.first(chunk_size),
                destination.subspan(copied, chunk_size).begin()
            );

            copied += chunk_size;
        }

        return copied;
    }

    // Copies bytes into a pointer and size buffer.
    [[nodiscard]]
    std::size_t copy_to(std::byte* destination, const std::size_t destination_size) const
    {
        argument_assert(destination != nullptr || destination_size == 0,
            "destination must not be null when destination_size > 0");

        return copy_to(std::span<std::byte>{ destination, destination_size});
    }

    // Copies bytes into an existing vector.
    [[nodiscard]]
    std::size_t copy_to(std::vector<std::byte>& destination) const noexcept
    {
        return copy_to(std::span<std::byte>{destination});
    }

    // Copies the complete view into a trivially copyable value.
    template <typename T>
    requires (std::is_trivially_copyable_v<T> && !std::is_convertible_v<T, std::string_view>)
    [[nodiscard]]
    bool copy_to(T& destination) const
    {
        argument_assert(sizeof(T) <= m_size,
            "buffer size is smaller than the size of the destination type");

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

        for (const std::span<const std::byte>& segment : m_segments) {
            result.append(
                reinterpret_cast<const char*>(segment.data()),
                segment.size());
        }

        return result;
    }
};

} // namespace xtd

#endif // PIPELINE_SEGMENTED_BYTE_VIEW_H
