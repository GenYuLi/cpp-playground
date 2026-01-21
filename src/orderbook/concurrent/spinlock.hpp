#pragma once

#include <atomic>
#include <thread>

namespace orderbook {

// Minimal spinlock implementation
// Cache-line aligned to prevent false sharing
class alignas(64) Spinlock {
 public:
	Spinlock() noexcept = default;

	// Non-copyable, non-movable
	Spinlock(const Spinlock&) = delete;
	Spinlock& operator=(const Spinlock&) = delete;
	Spinlock(Spinlock&&) = delete;
	Spinlock& operator=(Spinlock&&) = delete;

	void lock() noexcept {
		// Exponential backoff for better performance under contention
		int spin_count = 0;
		constexpr int MAX_SPINS = 16;

		while (flag_.test_and_set(std::memory_order_acquire)) {
			// Exponential backoff with pause
			if (spin_count < MAX_SPINS) {
				for (int i = 0; i < (1 << spin_count); ++i) {
// CPU pause instruction for better power efficiency
#if defined(__x86_64__) || defined(_M_X64)
					__builtin_ia32_pause();
#elif defined(__aarch64__)
					asm volatile("yield" ::: "memory");
#else
					std::this_thread::yield();
#endif
				}
				++spin_count;
			} else {
				// After MAX_SPINS, yield to OS scheduler
				std::this_thread::yield();
			}
		}
	}

	bool try_lock() noexcept { return !flag_.test_and_set(std::memory_order_acquire); }

	void unlock() noexcept { flag_.clear(std::memory_order_release); }

 private:
	alignas(64) std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

// RAII lock guard for Spinlock
// Follows std::lock_guard interface
class SpinlockGuard {
 public:
	explicit SpinlockGuard(Spinlock& lock) noexcept : lock_(lock) { lock_.lock(); }

	~SpinlockGuard() noexcept { lock_.unlock(); }

	// Non-copyable, non-movable
	SpinlockGuard(const SpinlockGuard&) = delete;
	SpinlockGuard& operator=(const SpinlockGuard&) = delete;
	SpinlockGuard(SpinlockGuard&&) = delete;
	SpinlockGuard& operator=(SpinlockGuard&&) = delete;

 private:
	Spinlock& lock_;
};

// Unique lock for Spinlock (similar to std::unique_lock)
// Provides deferred locking and manual lock/unlock
class SpinlockUniqueLock {
 public:
	explicit SpinlockUniqueLock(Spinlock& lock) noexcept : lock_(&lock), owns_lock_(true) {
		lock_->lock();
	}

	// Deferred lock constructor
	struct defer_lock_t {};
	static constexpr defer_lock_t defer_lock{};

	SpinlockUniqueLock(Spinlock& lock, defer_lock_t) noexcept : lock_(&lock), owns_lock_(false) {}

	~SpinlockUniqueLock() noexcept {
		if (owns_lock_) {
			lock_->unlock();
		}
	}

	// Non-copyable, movable
	SpinlockUniqueLock(const SpinlockUniqueLock&) = delete;
	SpinlockUniqueLock& operator=(const SpinlockUniqueLock&) = delete;

	SpinlockUniqueLock(SpinlockUniqueLock&& other) noexcept
			: lock_(other.lock_), owns_lock_(other.owns_lock_) {
		other.lock_ = nullptr;
		other.owns_lock_ = false;
	}

	SpinlockUniqueLock& operator=(SpinlockUniqueLock&& other) noexcept {
		if (this != &other) {
			if (owns_lock_) {
				lock_->unlock();
			}
			lock_ = other.lock_;
			owns_lock_ = other.owns_lock_;
			other.lock_ = nullptr;
			other.owns_lock_ = false;
		}
		return *this;
	}

	void lock() noexcept {
		lock_->lock();
		owns_lock_ = true;
	}

	bool try_lock() noexcept {
		owns_lock_ = lock_->try_lock();
		return owns_lock_;
	}

	void unlock() noexcept {
		if (owns_lock_) {
			lock_->unlock();
			owns_lock_ = false;
		}
	}

	[[nodiscard]] bool owns_lock() const noexcept { return owns_lock_; }

	[[nodiscard]] explicit operator bool() const noexcept { return owns_lock_; }

 private:
	Spinlock* lock_;
	bool owns_lock_;
};

}	 // namespace orderbook
