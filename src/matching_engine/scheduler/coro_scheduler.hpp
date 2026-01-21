#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

namespace matching_engine::coro {

// Forward declaration
template <typename T>
class Task;

// Yield control back to caller
struct Yield {
	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<>) const noexcept {}
	void await_resume() const noexcept {}
};

// Promise type for Task<T>
template <typename T>
class TaskPromise {
	std::optional<T> result_;
	std::exception_ptr exception_;

 public:
	Task<T> get_return_object() noexcept;

	std::suspend_always initial_suspend() noexcept { return {}; }
	std::suspend_always final_suspend() noexcept { return {}; }

	void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>) {
		result_ = std::move(value);
	}

	void unhandled_exception() noexcept { exception_ = std::current_exception(); }

	T result() {
		if (exception_) {
			std::rethrow_exception(exception_);
		}
		return std::move(*result_);
	}
};

// Specialization for void
template <>
class TaskPromise<void> {
	std::exception_ptr exception_;

 public:
	Task<void> get_return_object() noexcept;

	std::suspend_always initial_suspend() noexcept { return {}; }
	std::suspend_always final_suspend() noexcept { return {}; }

	void return_void() noexcept {}

	void unhandled_exception() noexcept { exception_ = std::current_exception(); }

	void result() {
		if (exception_) {
			std::rethrow_exception(exception_);
		}
	}
};

// Task<T> represents a coroutine that returns T
template <typename T = void>
class Task {
 public:
	using promise_type = TaskPromise<T>;
	using handle_type = std::coroutine_handle<promise_type>;

 private:
	handle_type handle_;

 public:
	explicit Task(handle_type handle) noexcept : handle_(handle) {}

	Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

	Task& operator=(Task&& other) noexcept {
		if (this != &other) {
			if (handle_) {
				handle_.destroy();
			}
			handle_ = std::exchange(other.handle_, {});
		}
		return *this;
	}

	~Task() {
		if (handle_) {
			handle_.destroy();
		}
	}

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	// Resume the coroutine
	void resume() {
		if (handle_ && !handle_.done()) {
			handle_.resume();
		}
	}

	// Check if coroutine is done
	bool done() const noexcept { return !handle_ || handle_.done(); }

	// Get the result (only valid after done())
	T get_result() {
		if (!handle_) {
			throw std::runtime_error("Task has no coroutine handle");
		}
		if (!handle_.done()) {
			throw std::runtime_error("Task is not complete");
		}
		return handle_.promise().result();
	}

	// Awaitable interface (allows co_await on Task)
	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> caller) noexcept {
		// When this task is awaited, we need to resume it
		// For simplicity, we just mark it as ready to run
	}

	T await_resume() {
		if (!handle_) {
			throw std::runtime_error("Task has no coroutine handle");
		}

		// Run the coroutine to completion
		while (!handle_.done()) {
			handle_.resume();
		}

		return handle_.promise().result();
	}
};

// Specialization for void
template <>
class Task<void> {
 public:
	using promise_type = TaskPromise<void>;
	using handle_type = std::coroutine_handle<promise_type>;

 private:
	handle_type handle_;

 public:
	explicit Task(handle_type handle) noexcept : handle_(handle) {}

	Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

	Task& operator=(Task&& other) noexcept {
		if (this != &other) {
			if (handle_) {
				handle_.destroy();
			}
			handle_ = std::exchange(other.handle_, {});
		}
		return *this;
	}

	~Task() {
		if (handle_) {
			handle_.destroy();
		}
	}

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	void resume() {
		if (handle_ && !handle_.done()) {
			handle_.resume();
		}
	}

	bool done() const noexcept { return !handle_ || handle_.done(); }

	void get_result() {
		if (!handle_) {
			throw std::runtime_error("Task has no coroutine handle");
		}
		if (!handle_.done()) {
			throw std::runtime_error("Task is not complete");
		}
		handle_.promise().result();
	}

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> caller) noexcept {}

	void await_resume() {
		if (!handle_) {
			throw std::runtime_error("Task has no coroutine handle");
		}

		while (!handle_.done()) {
			handle_.resume();
		}

		handle_.promise().result();
	}
};

// Implementation of get_return_object
template <typename T>
inline Task<T> TaskPromise<T>::get_return_object() noexcept {
	return Task<T>{std::coroutine_handle<TaskPromise<T>>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
	return Task<void>{std::coroutine_handle<TaskPromise<void>>::from_promise(*this)};
}

}	 // namespace matching_engine::coro

// Alias for backward compatibility
namespace matching_engine {
namespace scheduler = coro;
}
