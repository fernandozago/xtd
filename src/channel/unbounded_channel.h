#ifndef CHANNEL_UNBOUNDED_CHANNEL_H
#define CHANNEL_UNBOUNDED_CHANNEL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>
#include <utility>
#include "block_strategy.h"
#include "channel_writer.h"
#include "channel_reader.h"

namespace xtd
{
	// Thread-safe MPMC channel backed by a dynamically growing queue.
	//
	// Key behavior:
	// - Multiple producers and consumers can push/read concurrently.
	// - After complete(), further push/emplace attempts return false.
	//
	// Storage is managed by std::queue<T> and grows as values are enqueued.
	// This avoids fixed-capacity blocking but can increase memory usage under sustained producer pressure.
	//
	// Example:
	// xtd::unbounded_channel<int> channel;
	template<typename T>
	class unbounded_channel
	{
	public:
		explicit unbounded_channel()
			: m_writer(*this)
			, m_reader(*this)
		{
		}

		unbounded_channel(const unbounded_channel&) = delete;
		unbounded_channel& operator=(const unbounded_channel&) = delete;

		unbounded_channel(unbounded_channel&&) = delete;
		unbounded_channel& operator=(unbounded_channel&&) = delete;

		xtd::channel_writer<T>& writer() noexcept {
			return m_writer;
		}

		xtd::channel_reader<T>& reader() noexcept {
			return m_reader;
		}

	private:
		friend class xtd::channel_writer_impl<T, unbounded_channel<T>>;
		friend class xtd::channel_reader_impl<T, unbounded_channel<T>>;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_not_empty;
		std::queue<T> m_queue;
		
		bool m_completed = false;
		channel_writer_impl<T, unbounded_channel<T>> m_writer;
		channel_reader_impl<T, unbounded_channel<T>> m_reader;

		// #Writer Operations
		bool push(auto&& value, block_strategy unused) {
			(void)unused; // unused parameter to satisfy interface
			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_completed) return false;
			m_queue.emplace(std::forward<decltype(value)>(value));

			lock.unlock();			
			m_not_empty.notify_one();
			return true;
		}

		void complete() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_completed = true;
			}
			
			m_not_empty.notify_all();
		}
		// #End Writer Operations
		
		// #Reader Operations
		std::optional<T> read(block_strategy strategy) 
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (strategy == block_strategy::WAIT && !m_completed && m_queue.empty()) {
				m_not_empty.wait(lock, [this] {
					return m_completed || !m_queue.empty();
				});
			} 
			
			if (m_queue.empty()) {
				return std::nullopt;
			}
			
			std::optional<T> result(std::move(m_queue.front()));
			m_queue.pop();

			lock.unlock();
			return result;
		}
		
		[[nodiscard]]
		std::size_t size() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_queue.size();
		}
		// #End Reader Operations
	};
}

#endif // CHANNEL_UNBOUNDED_CHANNEL_H