#ifndef CHANNEL_CHANNEL_READER_H
#define CHANNEL_CHANNEL_READER_H

#include "block_strategy.h"

#include <cstddef>
#include <optional>

namespace xtd
{
	template<typename T>
	class channel_reader
	{
	public:
		virtual ~channel_reader() = default;

		// Attempts to read one value without waiting for data.
		//
		// Returns:
		// - std::optional<T> containing a value when data is available.
		// - std::nullopt when the channel is empty.
		//
		// Blocking behavior:
		// - bounded_channel: never blocks.
		// - unbounded_channel: never blocks.
		[[nodiscard]]
		virtual std::optional<T> try_read() = 0;

		// Reads one value.
		//
		// Returns:
		// - std::optional<T> containing a value when data is available.
		// - std::nullopt when the channel is completed and no buffered
		//   data remains.
		//
		// Blocking behavior:
		// - bounded_channel: blocks while empty until data arrives or
		//   the channel is completed.
		// - unbounded_channel: blocks while empty until data arrives or
		//   the channel is completed.
		[[nodiscard]]
		virtual std::optional<T> read() = 0;

		// Gets the current number of buffered items.
		[[nodiscard]]
		virtual std::size_t size() const = 0;

	protected:
		channel_reader() = default;
	};


	// Implementation detail
	template<typename T, typename TChannel>
	class channel_reader_impl final : public channel_reader<T>
	{
	public:
		explicit channel_reader_impl(TChannel& channel) noexcept
			: m_channel(channel)
		{
		}

		[[nodiscard]]
		std::optional<T> try_read() override {
			return m_channel.read(block_strategy::TRY);
		}

		[[nodiscard]]
		std::optional<T> read() override {
			return m_channel.read(block_strategy::WAIT);
		}

		[[nodiscard]]
		std::size_t size() const override {
			return m_channel.size();
		}

	private:
		TChannel& m_channel;
	};

} // namespace xtd

#endif // CHANNEL_CHANNEL_READER_H