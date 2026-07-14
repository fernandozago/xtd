#ifndef PIPELINE_SEGMENTED_BYTE_VIEW_H
#define PIPELINE_SEGMENTED_BYTE_VIEW_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
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
friend class test_helper_segmented_byte_view;
private:
    std::vector<std::span<const std::byte>> m_segments;
    std::uint64_t m_sequence_id;
    std::size_t m_first_segment_start;
    std::size_t m_size;
    position m_start;
    position m_end;

    position position_at_offset(const std::size_t& offset) const
    {
        if (m_segments.empty()) {
            return position(offset, 0, m_sequence_id);
        }

        std::size_t segmentStart = m_first_segment_start;
        for (std::size_t i = 0; i < m_segments.size(); ++i) {
            const std::span<const std::byte>& segment = m_segments[i];
            const std::size_t segmentEnd = segmentStart + segment.size();
            const bool isLastSegment = i == m_segments.size() - 1;

            if (offset >= segmentStart &&
                (offset < segmentEnd || (isLastSegment && offset == segmentEnd))) {
                return position(segmentStart, offset - segmentStart, m_sequence_id);
            }

            segmentStart = segmentEnd;
        }

        throw std::out_of_range("position offset is out of range");
    }

    void ensure_position_belongs_to_sequence(const position& position, const char* paramName) const {
        if (position.m_sequence_id != m_sequence_id) {
            throw std::invalid_argument(std::string(paramName) + " must belong to this sequence");
        }
    }

    std::size_t validate_size(const std::size_t& startOffset, const std::size_t& endOffset) const {
        if (startOffset > endOffset) {
            throw std::out_of_range("Sequence range is invalid");
        }

        return endOffset - startOffset;
    }
   
    void validate_slice_range(const std::size_t& sliceStart, const std::size_t& sliceEnd) const {
        const std::size_t currentStart = m_first_segment_start;
        const std::size_t currentEnd = m_end.offset_in_sequence();

        if (sliceStart < currentStart || sliceStart > currentEnd) {
            throw std::out_of_range("slice start is out of range");
        }

        if (sliceEnd < currentStart || sliceEnd > currentEnd) {
            throw std::out_of_range("slice end is out of range");
        }

        if (sliceStart > sliceEnd) {
            throw std::out_of_range("slice range is invalid");
        }
    }

    void validate_relative_slice(const std::size_t& startOffset, const std::size_t& size) const {
        if (startOffset > m_size) {
            throw std::out_of_range("slice start offset is out of range");
        }

        if (size > m_size - startOffset) {
            throw std::out_of_range("slice size is out of range");
        }
    }

    segmented_byte_view slice_range_multi_segments(const std::size_t& sliceStart, const std::size_t& sliceEnd) const {
        std::vector<std::span<const std::byte>> slicedSegments;
        slicedSegments.reserve(m_segments.size());

        std::size_t segmentStart = m_first_segment_start;
        for (const std::span<const std::byte>& segment : m_segments) {
            const std::size_t segmentEnd = segmentStart + segment.size();

            if (segmentEnd <= sliceStart) {
                segmentStart = segmentEnd;
                continue;
            }
            if (segmentStart >= sliceEnd) break;

            const std::size_t overlapStart = std::max(sliceStart, segmentStart);
            const std::size_t readable_size = std::min(sliceEnd, segmentEnd) - overlapStart;
            slicedSegments.emplace_back(segment.subspan(overlapStart - segmentStart, readable_size));
            segmentStart = segmentEnd;
        }

        slicedSegments.shrink_to_fit();
        return {
            std::move(slicedSegments),
            sliceStart,
            sliceEnd,
            m_sequence_id
        };
    }

    segmented_byte_view slice_range(const std::size_t& sliceStart, const std::size_t& sliceEnd) const {
        validate_slice_range(sliceStart, sliceEnd);

        const std::span<const std::byte>& firstSegment = m_segments.front();
        const std::size_t firstSegmentEnd = m_first_segment_start + firstSegment.size();

        // Check if is required to slice across multiple segments
        if (sliceStart < m_first_segment_start || sliceEnd > firstSegmentEnd) {
            return slice_range_multi_segments(sliceStart, sliceEnd);
        }
        
        // Slice is fully contained within the first segment
        return segmented_byte_view(
            firstSegment.subspan(sliceStart - m_first_segment_start, sliceEnd - sliceStart),
            sliceStart,
            sliceEnd,
            m_sequence_id
        );
    }

    std::size_t copy_to_multi_segments(std::byte* destination, const std::size_t& bytesToCopy) const {
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
    segmented_byte_view(std::vector<std::span<const std::byte>>&& segments, const std::size_t& startOffset, const std::size_t& endOffset, const std::uint64_t& sequenceId)
        : m_segments(std::move(segments))
        , m_sequence_id(sequenceId)
        , m_first_segment_start(startOffset)
        , m_size(validate_size(startOffset, endOffset))
        , m_start(position_at_offset(startOffset))
        , m_end(position_at_offset(endOffset))
    {
    }

    // Initializes a read-only sequence with a single segment and explicit range metadata.
    segmented_byte_view(std::span<const std::byte>&& segment, const std::size_t& startOffset, const std::size_t& endOffset, const std::uint64_t& sequenceId) 
    : segmented_byte_view(std::vector{std::move(segment)}, startOffset, endOffset, sequenceId)
    {
    }

public:
    segmented_byte_view() = delete;

    /// <summary>
    /// Gets the inclusive start position of this sequence.
    /// </summary>
    /// <returns>The start position.</returns>
    [[nodiscard]] 
    const position& start() const noexcept { return m_start; }

    /// <summary>
    /// Gets the exclusive end position of this sequence.
    /// </summary>
    /// <returns>The end position.</returns>
    [[nodiscard]] 
    const position& end() const noexcept { return m_end; }

    /// <summary>
    /// Gets the length of this sequence in bytes.
    /// </summary>
    /// <returns>The number of bytes in the sequence range.</returns>
    [[nodiscard]] 
    std::size_t size() const noexcept { return m_size; }

    /// <summary>
    /// Indicates whether this sequence is empty.
    /// </summary>
    /// <returns>True when the sequence length is zero; otherwise, false.</returns>
    [[nodiscard]] 
    bool empty() const noexcept { return m_size == 0; }
    /// <summary>
    /// Counts how many segments intersect the current sequence range.
    /// </summary>
    /// <returns>The number of visible segments.</returns>
    [[nodiscard]] 
    std::size_t segment_count() const noexcept {
        return m_segments.size();
    }

    /// <summary>
    /// Gets a non-owning view over the visible segments.
    /// </summary>
    /// <returns>A span covering this sequence's segments.</returns>
    [[nodiscard]] 
    std::span<const std::span<const std::byte>> segments() const noexcept {
        return m_segments;
    }

    /// <summary>
    /// Forms a slice out of the current segmented_byte_view, from the sequence start up to end (exclusive).
    /// </summary>
    /// <param name="end">The ending position of the slice (exclusive).</param>
    /// <returns>A new segmented_byte_view representing everything before end.</returns>
    [[nodiscard]] 
    segmented_byte_view slice(const position& end) const {
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(m_start.offset_in_sequence(), end.offset_in_sequence());
    }

    /// <summary>
    /// Forms a slice out of the current segmented_byte_view<T>, beginning at start and ending at end (exclusive).
    /// </summary>
    /// <param name="start">The starting position of the slice.</param>
    /// <param name="end">The ending position of the slice.</param>
    /// <returns>A new segmented_byte_view representing the specified slice.</returns>
    [[nodiscard]] 
    segmented_byte_view slice(const position& start, const position& end) const {
        ensure_position_belongs_to_sequence(start, "start");
        ensure_position_belongs_to_sequence(end, "end");
        return slice_range(start.offset_in_sequence(), end.offset_in_sequence());
    }

    /// <summary>
    /// Forms a slice out of the current segmented_byte_view, beginning at start and ending at end (exclusive).
    /// </summary>
    /// <param name="startOffset">The starting offset of the slice relative to the current sequence start.</param>
    /// <param name="end">The ending position of the slice.</param>
    /// <returns>A new segmented_byte_view representing the specified slice.</returns>
    [[nodiscard]] 
    segmented_byte_view slice(std::size_t startOffset, const position& end) const {
        ensure_position_belongs_to_sequence(end, "end");
        validate_relative_slice(startOffset, end.offset_in_sequence() - (m_start.offset_in_sequence() + startOffset));

        return slice_range(m_start.offset_in_sequence() + startOffset, end.offset_in_sequence());
    }

    /// <summary>
    /// Forms a slice out of the current segmented_byte_view, beginning at startOffset and spanning size bytes.
    /// </summary>
    /// <param name="startOffset">The starting offset of the slice relative to the current sequence start.</param>
    /// <param name="size">The number of bytes to include in the slice.</param>
    /// <returns>A new segmented_byte_view representing the specified slice.</returns>
    [[nodiscard]] 
    segmented_byte_view slice(const std::size_t& startOffset, const std::size_t& size) const {
        validate_relative_slice(startOffset, size);

        const std::size_t sliceEnd = m_start.offset_in_sequence() + startOffset + size;
        return slice_range(m_start.offset_in_sequence() + startOffset, sliceEnd);
    }

    /// <summary>
    /// Finds the position of the specified byte value within the sequence.
    /// </summary>
    /// <param name="value">The byte value to search for.</param>
    /// <param name="pos">The position of the found byte, if any.</param>
    /// <returns>True if the byte is found; otherwise, false.</returns>
    [[nodiscard]] 
    bool position_of(const std::byte& value, position& pos) const {
        if (empty()) return false;

        std::size_t segmentStart = m_first_segment_start;
        for (const std::span<const std::byte>& segment : m_segments) {
            const std::size_t count = segment.size();
            if (count == 0) continue;

            const std::byte* first = segment.data();
            const std::byte* last = first + count;

            const std::byte* found = std::find(first, last, value);

            if (found != last) {
                pos = position(
                    segmentStart,
                    static_cast<std::size_t>(found - first),
                    m_sequence_id
                );
                return true;
            }

            segmentStart += count;
        }

        return false;
    }

    /// <summary>
    /// Finds the position of the specified character value within the sequence.
    /// </summary>
    /// <param name="value">The character value to search for.</param>
    /// <param name="pos">The position of the found character, if any.</param>
    /// <returns>True if the character is found; otherwise, false.</returns>
    [[nodiscard]] 
    bool position_of(const char& value, position& pos) const {
        return position_of(std::byte{static_cast<unsigned char>(value)}, pos);
    }

    /// <summary>
    /// Copies the contents of the sequence to the specified destination byte buffer.
    /// </summary>
    /// <param name="destination">The destination byte buffer.</param>
    /// <param name="destinationSize">The size of the destination buffer in bytes.</param>
    /// <returns>The number of bytes copied.</returns>
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

    /// <summary>
    /// Copies bytes from this sequence into the destination buffer, capped by the smaller of
    /// the sequence.size() or destination.size().
    /// </summary>
    /// <param name="destination">The destination buffer to copy into. It is not resized.</param>
    /// <returns>The number of bytes copied.</returns>
    [[nodiscard]] 
    std::size_t copy_to(std::vector<std::byte>& destination) const {
        return copy_to(destination.data(), destination.size());
    }

    /// <summary>
    /// Copies the sequence bytes into a trivially copyable value.
    /// </summary>
    /// <typeparam name="T">The destination value type.</typeparam>
    /// <param name="destination">The destination value.</param>
    /// <returns>True when the full value was copied; otherwise, false.</returns>
    template <typename T>
    requires (!std::is_convertible_v<T, std::string_view>)
    [[nodiscard]] 
    bool copy_to(T& destination) const {
        static_assert(std::is_trivially_copyable_v<T>, "copy_to(T&) requires T to be trivially copyable");

        if (m_size != sizeof(T))
            return false;

        return copy_to(reinterpret_cast<std::byte*>(&destination), sizeof(T)) == sizeof(T);
    }

    /// <summary>
    /// Converts the sequence to a string representation.
    /// </summary>
    /// <returns>A string containing the sequence's contents.</returns>
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
};

// Test helper struct to access private members of segmented_byte_view
class test_helper_segmented_byte_view {
public:
    static segmented_byte_view create_from_segments(std::vector<std::span<const std::byte>>&& segments, std::size_t startOffset, std::size_t endOffset, std::uint64_t sequenceId) {
        return segmented_byte_view(std::move(segments), startOffset, endOffset, sequenceId);
    }

    static std::size_t get_first_segment_start(const segmented_byte_view& seq) {
        return seq.m_first_segment_start;
    }

    static std::uint64_t get_sequence_id(const segmented_byte_view& seq) {
        return seq.m_sequence_id;
    }
};

} // namespace xtd

#endif // PIPELINE_READONLYSEQUENCE_H
