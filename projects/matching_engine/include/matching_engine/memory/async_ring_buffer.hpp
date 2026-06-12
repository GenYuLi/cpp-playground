#pragma once

#include <array>
#include <atomic>
#include <coroutine>
#include <optional>

#include "../scheduler/coro_scheduler.hpp"

namespace matching_engine::memory {

template <typename T, size_t Capacity>
class AsyncRingBuffer {
	static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

 private:
	std::array<T, Capacity> buffer_{};
	alignas(64) std::atomic<size_t> write_pos_{0};
	alignas(64) std::atomic<size_t> read_pos_{0};

	static constexpr size_t INDEX_MASK = Capacity - 1;

 public:
	AsyncRingBuffer() = default;

	// Non-copyable, non-movable
	AsyncRingBuffer(const AsyncRingBuffer&) = delete;
	AsyncRingBuffer& operator=(const AsyncRingBuffer&) = delete;

	size_t size() const noexcept {
		auto write = write_pos_.load(std::memory_order_acquire);
		auto read = read_pos_.load(std::memory_order_acquire);
		return write - read;
	}

	bool empty() const noexcept { return size() == 0; }

	bool full() const noexcept { return size() >= Capacity; }

	// Synchronous push
	bool push(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>) {
		auto write = write_pos_.load(std::memory_order_relaxed);
		auto read = read_pos_.load(std::memory_order_acquire);

		if (write - read >= Capacity) {
			return false;	 // Buffer full
		}

		buffer_[write & INDEX_MASK] = value;
		write_pos_.store(write + 1, std::memory_order_release);
		return true;
	}

	bool push(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>) {
		auto write = write_pos_.load(std::memory_order_relaxed);
		auto read = read_pos_.load(std::memory_order_acquire);

		if (write - read >= Capacity) {
			return false;	 // Buffer full
		}

		buffer_[write & INDEX_MASK] = std::move(value);
		write_pos_.store(write + 1, std::memory_order_release);
		return true;
	}

	// Synchronous pop
	std::optional<T> pop() noexcept(std::is_nothrow_move_constructible_v<T>) {
		auto read = read_pos_.load(std::memory_order_relaxed);
		auto write = write_pos_.load(std::memory_order_acquire);

		if (read >= write) {
			return std::nullopt;	// Buffer empty
		}

		T value = std::move(buffer_[read & INDEX_MASK]);
		read_pos_.store(read + 1, std::memory_order_release);
		return value;
	}

	// Async push awaitable
	struct PushAwaitable {
		AsyncRingBuffer& buffer;
		T value;
		bool result = false;

		bool await_ready() {
			result = buffer.push(std::move(value));
			return result;	// If successful, don't suspend
		}

		void await_suspend(std::coroutine_handle<> handle) {
			// In a real implementation, we would queue this coroutine
			// For now, we just resume immediately
			handle.resume();
		}

		bool await_resume() {
			if (!result) {
				result = buffer.push(std::move(value));
			}
			return result;
		}
	};

	// Async pop awaitable
	struct PopAwaitable {
		AsyncRingBuffer& buffer;
		std::optional<T> result;

		bool await_ready() {
			result = buffer.pop();
			return result.has_value();
		}

		void await_suspend(std::coroutine_handle<> handle) {
			// In a real implementation, we would queue this coroutine
			// For now, we just resume immediately
			handle.resume();
		}

		std::optional<T> await_resume() {
			if (!result.has_value()) {
				result = buffer.pop();
			}
			return std::move(result);
		}
	};

	// Async interface
	PushAwaitable push_async(T value) { return PushAwaitable{*this, std::move(value)}; }

	PopAwaitable pop_async() { return PopAwaitable{*this}; }
};

}	 // namespace matching_engine::memory
