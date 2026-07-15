#ifndef PIPELINE_SEGMENTED_BYTE_VIEW_H
#define PIPELINE_SEGMENTED_BYTE_VIEW_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "position.h"

namespace xtd
{

struct segmented_byte_view
{
friend class pipeline;
private:
    std::vector<std::span<const std::byte>> m_segments;
    std::uint64_t m_sequence_id;
    std::size_t m_first_segment_begin;
    std::size_t m_size;
    position m_begin;
    position m_end;

    position position_at_offset(const std::size_t offset) const
    {
        if (m_segments.empty()) {
            return position(offset, 0, m_sequence_id);
        }

        std::size_t segmentBegin = m_first_segment_begin;
        for (std::size_t i = 0; i < m_segments.size(); ++i) {
            const std::span<const std::byte>& segment = m_segments[i];
            const std::size_t segmentEnd = segmentBegin + segment.size();
            const bool isLastSegment = i == m_segments.size() - 1;

            if (offset >= segmentBegin &&
                (offset < segmentEnd || (isLastSegment && offset == segmentEnd))) {
                return position(segmentBegin, offset - segmentBegin, m_sequence_id);
            }

            segmentBegin = segmentEnd;
        }

        throw std::out_of_range("position offset is out of range");
    }

    void ensure_position_belongs_to_sequence(const position& position, const char* paramName) const {
        if (position.m_sequence_id != m_sequence_id) {
            throw std::invalid_argument(std::string(paramName) + " must belong to this sequence");
        }
    }

    std::size_t validate_size(const std::size_t beginOffset, const std::size_t endOffset) const {
        if (beginOffset > endOffset) {
            throw std::out_of_range("Sequence range is invalid");
        }

        return endOffset - beginOffset;
    }
   
    void validate_slice_range(const std::size_t sliceBegin, const std::size_t sliceEnd) const {
        const std::size_t currentBegin = m_first_segment_begin;
        const std::size_t currentEnd = m_end.offset_in_sequence();

        if (sliceBegin < currentBegin || sliceBegin > currentEnd) {
            throw std::out_of_range("slice begin is out of range");
        }

        if (sliceEnd < currentBegin || sliceEnd > currentEnd) {
            throw std::out_of_range("slice end is out of range");
        }

        if (sliceBegin > sliceEnd) {
            throw std::out_of_range("slice range is invalid");
        }
    }

    void validate_relative_slice(const std::size_t beginOffset, const std::size_t size) const {
        if (beginOffset > m_size) {
            throw std::out_of_range("slice begin offset is out of range");
        }

        if (size > m_size - beginOffset) {
            throw std::out_of_range("slice size is out of range");
        }
    }

    segmented_byte_view slice_range_multi_segments(const std::size_t sliceBegin, const std::size_t sliceEnd) const {
        std::vector<std::span<const std::byte>> slicedSegments;
        slicedSegments.reserve(m_segments.size());

        std::size_t segmentBegin = m_first_segment_begin;
        for (const std::span<const std::byte>& segment : m_segments) {
            const std::size_t segmentEnd = segmentBegin + segment.size();

            if (segmentEnd <= sliceBegin) {
                segmentBegin = segmentEnd;
                continue;
            }
            if (segmentBegin >= sliceEnd) break;

            const std::size_t overlapBegin = std::max(sliceBegin, segmentBegin);
            const std::size_t readable_size = std::min(sliceEnd, segmentEnd) - overlapBegin;
            slicedSegments.emplace_back(segment.subspan(overlapBegin - segmentBegin, readable_size));
            segmentBegin = segmentEnd;
        }

        slicedSegments.shrink_to_fit();
        return segmented_byte_view(
            std::move(slicedSegments),
            sliceBegin,
            sliceEnd,
            m_sequence_id
        );
    }

    segmented_byte_view slice_range(const std::size_t sliceBegin, const std::size_t sliceEnd) const {
        validate_slice_range(sliceBegin, sliceEnd);

        const std::span<const std::byte>& firstSegment = m_segments.front();
        const std::size_t firstSegmentEnd = m_first_segment_begin + firstSegment.size();

        // Check if is required to slice across multiple segments
        if (sliceBegin < m_first_segment_begin || sliceEnd > firstSegmentEnd) {
            return slice_range_multi_segments(sliceBegin, sliceEnd);
        }
        
        // Slice is fully contained within the first segment
        return segmented_byte_view(
            firstSegment.subspan(sliceBegin - m_first_segment_begin, sliceEnd - sliceBegin),
            sliceBegin,
            sliceEnd,
            m_sequence_id
        );
    }

    std::size_t copy_to_multi_segments(std::byte* destination, const std::size_t bytesToCopy) const {
        if (destination == nullptr || bytesToCopy == 0 || empty()) return 0;

        const std::size_t target = std::min(bytesToCopy, m_size);

        std::size_t remaining = target;

        for (const std::span<const std::byte>& segment : m_segments) {
            if (remaining == 0) break;
            const std::size_t chunk = std::min(segment.size(), remaining);

            std::copy_n(segment.data(), chunk, destination);

            destination += chunk;
            remaining -= chunk;
        }

        return target - remaining;
    }

    std::string to_string_multi_segments() const {
        std::string text;
        text.reserve(m_size);
        
        std::size_t remaining = m_size;
        for (const std::span<const std::byte>& segment : m_segments)
        {
            if (remaining == 0) break;
            const std::size_t size = std::min(segment.size(), remaining);

            const char* ptr = reinterpret_cast<const char*>(segment.data());
            text.append(ptr, size);
            remaining -= size;
        }

        return text;
    }

    // Initializes a read-only sequence with explicit segment and range metadata.
    segmented_byte_view(std::vector<std::span<const std::byte>>&& segments, const std::size_t beginOffset, const std::size_t endOffset, const std::uint64_t sequenceId)
        : m_segments(std::move(segments))
        , m_sequence_id(sequenceId)
        , m_first_segment_begin(beginOffset)
        , m_size(validate_size(beginOffset, endOffset))
        , m_begin(position_at_offset(beginOffset))
        , m_end(position_at_offset(endOffset))
    {
    }

    // Initializes a read-only sequence with a single segment and explicit range metadata.
    segmented_byte_view(std::span<const std::byte>&& segment, const std::size_t beginOffset, const std::size_t endOffset, const std::uint64_t sequenceId) 
    : segmented_byte_view(std::vector{std::move(segment)}, beginOffset, endOffset, sequenceId)
    {
    }

public:
    segmented_byte_view() = delete;

    // Gets the inclusive begin position of this sequence.
    // Returns the begin position.
    [[nodiscard]] 
    const position& begin() const noexcept { return m_begin; }

    // Gets the exclusive end position of this sequence.
    // Returns the end position.
    [[nodiscard]] 
    const position& end() const noexcept { return m_end; }

    // Gets the length of this sequence in bytes.
    // Returns the number of bytes in the sequence range.
    [[nodiscard]] 
    std::size_t size() const noexcept { return m_size; }

    // Indicates whether this sequence is empty.
    // Returns true when the sequence length is zero; otherwise, false.
    [[nodiscard]] 
    bool empty() const noexcept { return m_size == 0; }
    // Counts how many segments intersect the current sequence range.
    // Returns the number of visible segments.
    [[nodiscard]] 
    std::size_t segment_count() const noexcept {
        return m_segments.size();
    }

    // Gets a non-owning view over the visible segments.
    // Returns a span covering this sequence's segments.
    [[nodiscard]] 
    std::span<const std::span<const std::byte>> segments() const noexcept {
        return m_segments;
    }

    // Forms a slice out of the current segmented_byte_view, from the sequence begin up to end (exclusive).
    // end: The ending position of the slice (exclusive).
    // Returns a new segmented_byte_view representing everything before end.
    [[nodiscard]] 
    segmented_byte_view slice(const position& end) const {
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(m_begin.offset_in_sequence(), end.offset_in_sequence());
    }

    // Forms a slice out of the current segmented_byte_view<T>, from the sequence begin and ending at end (exclusive).
    // begin: The begin position of the slice.
    // end: The ending position of the slice.
    // Returns a new segmented_byte_view representing the specified slice.
    [[nodiscard]] 
    segmented_byte_view slice(const position& begin, const position& end) const {
        ensure_position_belongs_to_sequence(begin, "begin");
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(begin.offset_in_sequence(), end.offset_in_sequence());
    }

    // Forms a slice out of the current segmented_byte_view, beginning at beginOffset and ending at end (exclusive).
    // beginOffset: The begin offset of the slice relative to the current sequence begin.
    // end: The ending position of the slice.
    // Returns a new segmented_byte_view representing the specified slice.
    [[nodiscard]] 
    segmented_byte_view slice(std::size_t beginOffset, const position& end) const {
        ensure_position_belongs_to_sequence(end, "end");
        validate_relative_slice(beginOffset, end.offset_in_sequence() - (m_begin.offset_in_sequence() + beginOffset));

        return slice_range(m_begin.offset_in_sequence() + beginOffset, end.offset_in_sequence());
    }

    // Forms a slice out of the current segmented_byte_view, beginning at beginOffset and spanning size bytes.
    // beginOffset: The begin offset of the slice relative to the current sequence begin.
    // size: The number of bytes to include in the slice.
    // Returns a new segmented_byte_view representing the specified slice.
    [[nodiscard]] 
    segmented_byte_view slice(const std::size_t beginOffset, const std::size_t size) const {
        validate_relative_slice(beginOffset, size);

        const std::size_t sliceEnd = m_begin.offset_in_sequence() + beginOffset + size;
        return slice_range(m_begin.offset_in_sequence() + beginOffset, sliceEnd);
    }

    // Finds the position of the specified byte value within the sequence.
    // value: The byte value to search for.
    // pos: The position of the found byte, if any.
    // Returns true if the byte is found; otherwise, false.
    [[nodiscard]] 
    bool position_of(const std::byte& value, position& pos) const {
        if (empty()) return false;

        std::size_t segmentBegin = m_first_segment_begin;
        for (const std::span<const std::byte>& segment : m_segments) {
            const std::size_t count = segment.size();
            if (count == 0) continue;

            const std::byte* first = segment.data();
            const std::byte* last = first + count;
            const std::byte* found = std::find(first, last, value);

            if (found != last) {
                pos = position(
                    segmentBegin,
                    std::distance(first, found),
                    m_sequence_id
                );
                return true;
            }

            segmentBegin += count;
        }

        return false;
    }

    // Finds the position of the specified character value within the sequence.
    // value: The character value to search for.
    // pos: The position of the found character, if any.
    // Returns true if the character is found; otherwise, false.
    [[nodiscard]] 
    bool position_of(const char value, position& pos) const {
        return position_of(static_cast<std::byte>(value), pos);
    }

    // Copies the contents of the sequence to the specified destination byte buffer.
    // destination: The destination byte buffer.
    // destinationSize: The size of the destination buffer in bytes.
    // Returns the number of bytes copied.
    [[nodiscard]] 
    std::size_t copy_to(std::byte* destination, const std::size_t destinationSize) const {
        if (destination == nullptr && destinationSize > 0) {
            throw std::invalid_argument("destination must not be null when destinationSize > 0");
        }

        if (empty() || m_segments.empty()) return 0;

        const std::size_t bytesToCopy = std::min(m_size, destinationSize);
        if (bytesToCopy == 0) return 0;

        const std::span<const std::byte>& firstSegment = m_segments.front();
        if (bytesToCopy > firstSegment.size()) {
            return copy_to_multi_segments(destination, bytesToCopy);
        }

        std::copy_n(firstSegment.data(), bytesToCopy, destination);
        return bytesToCopy;
    }

    // Copies bytes from this sequence into the destination buffer, capped by the smaller of
    // the sequence.size() or destination.size().
    // destination: The destination buffer to copy into. It is not resized.
    // Returns the number of bytes copied.
    [[nodiscard]] 
    std::size_t copy_to(std::vector<std::byte>& destination) const {
        return copy_to(destination.data(), destination.size());
    }

    // Copies the sequence bytes into a trivially copyable value.
    // T: The destination value type.
    // destination: The destination value.
    // Returns true when the full value was copied; otherwise, false.
    template <typename T>
    requires (!std::is_convertible_v<T, std::string_view>)
    [[nodiscard]] 
    bool copy_to(T& destination) const {
        static_assert(std::is_trivially_copyable_v<T>, "copy_to(T&) requires T to be trivially copyable");

        if (m_size != sizeof(T))
            return false;

        return copy_to(reinterpret_cast<std::byte*>(&destination), sizeof(T)) == sizeof(T);
    }

    // Converts the sequence to a string representation.
    // Returns a string containing the sequence's contents.
    [[nodiscard]] 
    std::string to_string() const {
        if (empty() || m_segments.empty()) return {};

        const std::span<const std::byte>& firstSegment = m_segments.front();
        if (m_size > firstSegment.size()) {
            return to_string_multi_segments();
        }

        const char* ptr = reinterpret_cast<const char*>(firstSegment.data());
        return std::string(ptr, m_size);
    }

    
friend class test_helper_segmented_byte_view;
};

} // namespace xtd

#endif // PIPELINE_SEGMENTED_BYTE_VIEW_H
