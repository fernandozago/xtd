#ifndef PIPELINE_SEGMENTED_BYTE_VIEW_H
#define PIPELINE_SEGMENTED_BYTE_VIEW_H

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <ranges>
#include <cstring>

#include "position.h"

namespace xtd
{

struct read_result;
class test_helper_segmented_byte_view;

struct from_end {
    std::size_t m_offset;
    constexpr from_end(std::size_t offset) 
        : m_offset(offset) 
    {}
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

    [[nodiscard]]
    std::size_t end_offset() const noexcept
    {
        return m_begin_offset + m_size;
    }

    void validate_slice_range(const std::size_t slice_begin, const std::size_t slice_end) const
    {
        if (slice_begin > slice_end) {
            throw std::out_of_range("slice begin must be <= slice end");
        }

        if (!(slice_begin >= m_begin_offset && slice_begin <= end_offset())) {
            throw std::out_of_range("slice begin is out of range");
        }

        if (!(slice_end >= m_begin_offset && slice_end <= end_offset())) {
            throw std::out_of_range("slice end is out of range");
        }
    }

    void validate_relative_slice(const std::size_t begin_offset, const std::size_t size) const
    {
        if (begin_offset > m_size) {
            throw std::out_of_range("slice begin offset is out of range");
        }

        if (size > m_size - begin_offset) {
            throw std::out_of_range("slice size is out of range");
        }
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
        if (begin_offset > m_size) {
            throw std::out_of_range("slice begin offset is out of range");
        }

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

        for (const std::span<const std::byte>& segment : m_segments) {
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
    bool is_single_segment() const noexcept
    {
        return m_segments.size() == 1;
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
        if (!(from_end.m_offset > 0 && from_end.m_offset <= m_size)) {
            throw std::out_of_range("from_end offset is out of range");
        }

        std::size_t remaining = from_end.m_offset - 1;
        for (auto iterator = m_segments.rbegin(); iterator != m_segments.rend(); ++iterator) {
            const std::span<const std::byte>& segment = *iterator;

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

        if (!(absolute_offset >= m_begin_offset && absolute_offset < end_offset())) {
            throw std::out_of_range("position is out of range");
        }

        return (*this)[absolute_offset - m_begin_offset];
    }

    const std::byte& operator[](const std::size_t index) const
    {
        if (index >= m_size) {
            throw std::out_of_range("index is out of range");
        }

        std::size_t segment_begin = 0;

        for (const std::span<const std::byte>& segment : m_segments) {
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
        if (m_size == 0) {
            return position{};
        }
        
        const unsigned char target = std::to_integer<unsigned char>(value);
       
        std::size_t offset = m_begin_offset;
        for (const std::span<const std::byte>& segment : m_segments) {
            const void* found = std::memchr(segment.data(), target, segment.size());
            if (found != nullptr) {
                return position{
                    offset + static_cast<std::size_t>(static_cast<const std::byte*>(found) - segment.data())
                };
            }

            offset += segment.size();
        }

        return position{};
    }

    [[nodiscard]]
    position position_of(const char value) const
    {
        return position_of(static_cast<std::byte>(value));
    }

    [[nodiscard]]
    position position_of_any(const std::span<const std::byte> values) const
    {
        if (values.empty()) {
            return position{};
        }

        if (values.size() == 1) {
            return position_of(values[0]);
        }

        std::size_t segment_begin = m_begin_offset;
        for (const std::span<const std::byte>& segment : m_segments) {
            const auto found = std::ranges::find_first_of(segment, values);

            if (found != segment.end()) {
                const auto distance = static_cast<std::size_t>(
                    std::ranges::distance(segment.begin(), found));

                return position{segment_begin + distance};
            }

            segment_begin += segment.size();
        }

        return position{};
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

    // Use with caution for large sequences or consider copying to a contiguous buffer first. (benchmark it)
    // This is ok when is_single_segment() is true, but for multiple segments, it will be slow.
    [[nodiscard]]
    position position_of_sequence(const std::span<const std::byte> sequence) const
    {
        if (sequence.empty() || sequence.size() > m_size) {
            return position{};
        }

        if (sequence.size() == 1) {
            return position_of(sequence.front());
        }

        std::size_t segment_offset = 0;
        auto joined_segments = m_segments | std::views::join;
        for (const std::span<const std::byte>& segment : m_segments) {
            /*
            * Fast check: sequence contained entirely in this segment.
            */
            if (const auto found = std::ranges::search(segment, sequence); !found.empty()) {
                return position{
                    m_begin_offset +
                    segment_offset +
                    static_cast<std::size_t>(
                        found.begin() - segment.begin()
                    )
                };
            }

            // no more work to do if this is the last segment
            if (is_single_segment()) {
                break;
            }

            /*
            * Slow check around the boundary:
            *
            *   last sequence.size() - 1 bytes of this segment
            *   +
            *   first sequence.size() - 1 following bytes
            */
            const std::size_t tail_size = std::min(segment.size(), sequence.size() - 1);

            if (tail_size != 0) {
                const std::size_t boundary_offset = segment_offset + segment.size() - tail_size;
                const std::size_t boundary_size = tail_size + sequence.size() - 1;

                auto boundary = joined_segments
                    | std::views::drop(boundary_offset)
                    | std::views::take(boundary_size);

                if (const auto boundary_found = std::ranges::search(boundary, sequence); !boundary_found.empty()) {
                    const auto offset_in_boundary = static_cast<std::size_t>(
                        std::ranges::distance(boundary.begin(),boundary_found.begin())
                    );

                    return position{m_begin_offset + boundary_offset + offset_in_boundary};
                }
            }

            segment_offset += segment.size();
        }

        return position{};
    }

    [[nodiscard]]
    position position_of_sequence(const std::string_view sequence) const {
        
        return position_of_sequence(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(sequence.data()),
            sequence.size()
        ));
    }

    [[nodiscard]]
    std::size_t copy_to(std::span<std::byte> destination) const noexcept
    {
        const std::size_t target_size = std::min(destination.size(), m_size);

        if (target_size == 0) {
            return 0;
        }
        
        if (is_single_segment()) {
            std::ranges::copy(m_segments.front().subspan(0, target_size), destination.begin());
            return target_size;
        }
        
        std::size_t copied = 0;
        for (const std::span<const std::byte>& segment : m_segments) {
            if (copied == target_size) {
                break;
            }

            const std::size_t chunk_size = std::min(segment.size(), target_size - copied);

            std::ranges::copy(
                segment.subspan(0, chunk_size),
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
        if (!(destination != nullptr || destination_size == 0)) {
            throw std::invalid_argument(
                "destination must not be null when destination_size > 0");
        }

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
        const std::size_t destination_size = sizeof(T);
        if (destination_size > m_size) {
            throw std::out_of_range(
                "buffer size is smaller than the size of the destination type");
        }

        return copy_to(std::as_writable_bytes(std::span<T, 1>{&destination, 1})) == destination_size;
    }

    std::span<const std::byte> first_segment() const noexcept
    {
        return m_segments.front();
    }

    [[nodiscard]]

    std::string_view as_string_view() const
    {
        if (!is_single_segment()) {
            throw std::logic_error(
                "buffer must be a single segment to convert to string_view");
        }
        return std::string_view{reinterpret_cast<const char*>(m_segments.front().data()), m_size};
    }

    [[nodiscard]]
    std::string to_string() const
    {
        if (empty()) return {};
        
        if (is_single_segment()) {
            return std::string{reinterpret_cast<const char*>(m_segments.front().data()), m_size};
        }
        
        std::string result;
        result.reserve(m_size);
        for (const std::span<const std::byte>& segment : m_segments) {
            result.append(reinterpret_cast<const char*>(segment.data()), segment.size());
        }

        return result;
    }
};

} // namespace xtd

#endif // PIPELINE_SEGMENTED_BYTE_VIEW_H
