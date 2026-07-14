#ifndef CHANNEL_BOUNDED_H
#define CHANNEL_BOUNDED_H

#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>
#include "block_strategy.h"
#include "channel_writer.h"
#include "channel_reader.h"

namespace xtd
{
	// Thread-safe MPMC channel with compile-time fixed capacity.
	//
	// Key behavior:
	// - Multiple producers and consumers can push/read concurrently.
	// - After complete(), further push/emplace attempts return false.
	//
	// Storage is embedded in the channel object as std::array<std::optional<T>, capacity>.
	// For very large capacities, prefer heap allocation to avoid large stack frames.
	//
	// Examples:
	// xtd::bounded_channel<int, 256> small_on_stack;
	// auto large_on_heap = std::make_unique<xtd::bounded_channel<int, 1'000'000>>();
	template<typename T, std::size_t capacity>
	class bounded_channel
	{
	public:
		static_assert(capacity > 0, "bounded_channel capacity must be greater than zero" );

		bounded_channel()
			: m_writer(*this)
			, m_reader(*this)
		{
		}

		bounded_channel(const bounded_channel&) = delete;
		bounded_channel& operator=(const bounded_channel&) = delete;

		bounded_channel(bounded_channel&&) = delete;
		bounded_channel& operator=(bounded_channel&&) = delete;

		xtd::channel_writer<T>& writer() noexcept {
			return m_writer;
		}

		xtd::channel_reader<T>& reader() noexcept {
			return m_reader;
		}

	private:
		friend class xtd::channel_writer_impl<T, bounded_channel<T, capacity>>;
		friend class xtd::channel_reader_impl<T, bounded_channel<T, capacity>>;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_not_full;
		std::condition_variable m_not_empty;
		std::optional<T> m_buffer[capacity];

		std::size_t m_head = 0;
		std::size_t m_tail = 0;
		std::size_t m_size = 0;

		bool m_completed = false;
		channel_writer_impl<T, bounded_channel<T, capacity>> m_writer;
		channel_reader_impl<T, bounded_channel<T, capacity>> m_reader;

		// #Writer Operations
		bool push(auto&& value, block_strategy strategy) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (strategy == block_strategy::WAIT && !m_completed && m_size == capacity) {
					m_not_full.wait(lock, [this] {
						return m_completed || m_size < capacity;
					});
				}
				
				if (m_completed || m_size == capacity) return false;
				m_buffer[m_tail].emplace(std::forward<decltype(value)>(value));
				
				// advance tail
				m_tail = (m_tail + 1) % capacity;
				++m_size;
			}
			
			m_not_empty.notify_one();
			return true;
		}

		void complete() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_completed = true;
			}

			m_not_empty.notify_all();
			m_not_full.notify_all();
		}
		// #End Writer Operations
		
		// #Reader Operations
		std::optional<T> read(block_strategy strategy) {
			std::optional<T> result;
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (strategy == block_strategy::WAIT && !m_completed && m_size == 0) {
					m_not_empty.wait(lock, [this] {
						return m_completed || m_size != 0;
					});
				}
				
				if (m_size == 0) return std::nullopt;
				auto& slot = m_buffer[m_head];
				
				result.emplace(std::move(*slot));
				slot.reset();
				
				// Advance head
				m_head = (m_head + 1) % capacity;
				--m_size;
			}
			
			m_not_full.notify_one();
			return result;
		}

		[[nodiscard]] 
		std::size_t size() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_size;
		}
		// #End Reader Operations
	};
}

#endif // CHANNEL_BOUNDED_H