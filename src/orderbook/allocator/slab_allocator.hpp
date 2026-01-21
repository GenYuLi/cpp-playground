#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <memory>
#include <new>
#include <vector>

namespace orderbook {

// Lock-free slab allocator for fixed-size objects
// Uses a free list approach for O(1) allocation/deallocation
template <typename T, size_t SlabSize = 4096>
class SlabAllocator {
	static_assert(SlabSize > 0, "SlabSize must be positive");
	static_assert(sizeof(T) >= sizeof(void*), "T must be at least pointer-sized");

 public:
	// Each slab contains SlabSize objects
	struct alignas(64) Slab {
		// Storage for objects (not constructed)
		alignas(T) std::array<std::byte, sizeof(T) * SlabSize> storage;

		Slab() = default;

		T* get_object(size_t index) noexcept {
			assert(index < SlabSize);
			return reinterpret_cast<T*>(storage.data() + index * sizeof(T));
		}
	};

	SlabAllocator() noexcept {
		// Pre-allocate one slab
		allocate_new_slab();
	}

	~SlabAllocator() noexcept {
		// Destroy all slabs
		// Note: Assumes all allocated objects have been deallocated
		slabs_.clear();
	}

	// Non-copyable, movable
	SlabAllocator(const SlabAllocator&) = delete;
	SlabAllocator& operator=(const SlabAllocator&) = delete;

	SlabAllocator(SlabAllocator&& other) noexcept
			: slabs_(std::move(other.slabs_)),
				free_list_head_(other.free_list_head_.load(std::memory_order_relaxed)),
				total_allocated_(other.total_allocated_.load(std::memory_order_relaxed)) {
		other.free_list_head_.store(nullptr, std::memory_order_relaxed);
		other.total_allocated_.store(0, std::memory_order_relaxed);
	}

	SlabAllocator& operator=(SlabAllocator&& other) noexcept {
		if (this != &other) {
			slabs_ = std::move(other.slabs_);
			free_list_head_.store(other.free_list_head_.load(std::memory_order_relaxed),
														std::memory_order_relaxed);
			total_allocated_.store(other.total_allocated_.load(std::memory_order_relaxed),
														 std::memory_order_relaxed);

			other.free_list_head_.store(nullptr, std::memory_order_relaxed);
			other.total_allocated_.store(0, std::memory_order_relaxed);
		}
		return *this;
	}

	// Allocate an object
	// Returns pointer to uninitialized memory
	[[nodiscard]] T* allocate() {
		// Try to pop from free list first (lock-free)
		Node* old_head = free_list_head_.load(std::memory_order_acquire);
		while (old_head != nullptr) {
			Node* next = old_head->next;
			if (free_list_head_.compare_exchange_weak(old_head, next, std::memory_order_release,
																								std::memory_order_acquire)) {
				// Successfully popped from free list
				return reinterpret_cast<T*>(old_head);
			}
			// CAS failed, retry with updated old_head
		}

		// Free list is empty, allocate from slab
		return allocate_from_slab();
	}

	// Deallocate an object
	// Object must have been allocated by this allocator
	void deallocate(T* ptr) noexcept {
		if (ptr == nullptr)
			return;

		// Push to free list (lock-free)
		Node* node = reinterpret_cast<Node*>(ptr);
		Node* old_head = free_list_head_.load(std::memory_order_relaxed);

		do {
			node->next = old_head;
		} while (!free_list_head_.compare_exchange_weak(old_head, node, std::memory_order_release,
																										std::memory_order_relaxed));
	}

	// Construct object in-place
	template <typename... Args>
	[[nodiscard]] T* construct(Args&&... args) {
		T* ptr = allocate();
		try {
			return new (ptr) T(std::forward<Args>(args)...);
		} catch (...) {
			deallocate(ptr);
			throw;
		}
	}

	// Destroy and deallocate object
	void destroy(T* ptr) noexcept {
		if (ptr == nullptr)
			return;
		ptr->~T();
		deallocate(ptr);
	}

	// Statistics
	[[nodiscard]] size_t total_capacity() const noexcept { return slabs_.size() * SlabSize; }

	[[nodiscard]] size_t total_allocated() const noexcept {
		return total_allocated_.load(std::memory_order_relaxed);
	}

 private:
	// Free list node (reuses object memory)
	struct Node {
		Node* next;
	};

	std::vector<std::unique_ptr<Slab>> slabs_;
	alignas(64) std::atomic<Node*> free_list_head_{nullptr};
	alignas(64) std::atomic<size_t> total_allocated_{0};

	// Allocate from current slab or create new slab
	T* allocate_from_slab() {
		// Thread-safe slab allocation using atomic counter
		size_t index = total_allocated_.fetch_add(1, std::memory_order_relaxed);
		size_t slab_index = index / SlabSize;
		size_t object_index = index % SlabSize;

		// Ensure slab exists
		while (slab_index >= slabs_.size()) {
			allocate_new_slab();
		}

		return slabs_[slab_index]->get_object(object_index);
	}

	// Allocate a new slab
	void allocate_new_slab() {
		auto slab = std::make_unique<Slab>();
		slabs_.push_back(std::move(slab));
	}
};

// Pool allocator with pre-allocated capacity
// Non-growing variant for predictable performance
template <typename T, size_t Capacity>
class FixedPoolAllocator {
	static_assert(Capacity > 0, "Capacity must be positive");
	static_assert(sizeof(T) >= sizeof(void*), "T must be at least pointer-sized");

 public:
	FixedPoolAllocator() noexcept {
		// Initialize free list with all slots
		for (size_t i = 0; i < Capacity; ++i) {
			Node* node = reinterpret_cast<Node*>(&storage_[i]);
			node->next = (i + 1 < Capacity) ? reinterpret_cast<Node*>(&storage_[i + 1]) : nullptr;
		}
		free_list_head_.store(reinterpret_cast<Node*>(&storage_[0]), std::memory_order_relaxed);
	}

	~FixedPoolAllocator() noexcept = default;

	// Non-copyable, non-movable
	FixedPoolAllocator(const FixedPoolAllocator&) = delete;
	FixedPoolAllocator& operator=(const FixedPoolAllocator&) = delete;
	FixedPoolAllocator(FixedPoolAllocator&&) = delete;
	FixedPoolAllocator& operator=(FixedPoolAllocator&&) = delete;

	// Allocate an object (returns nullptr if pool exhausted)
	[[nodiscard]] T* allocate() noexcept {
		Node* old_head = free_list_head_.load(std::memory_order_acquire);
		while (old_head != nullptr) {
			Node* next = old_head->next;
			if (free_list_head_.compare_exchange_weak(old_head, next, std::memory_order_release,
																								std::memory_order_acquire)) {
				allocated_count_.fetch_add(1, std::memory_order_relaxed);
				return reinterpret_cast<T*>(old_head);
			}
		}
		return nullptr;	 // Pool exhausted
	}

	// Deallocate an object
	void deallocate(T* ptr) noexcept {
		if (ptr == nullptr)
			return;

		Node* node = reinterpret_cast<Node*>(ptr);
		Node* old_head = free_list_head_.load(std::memory_order_relaxed);

		do {
			node->next = old_head;
		} while (!free_list_head_.compare_exchange_weak(old_head, node, std::memory_order_release,
																										std::memory_order_relaxed));

		allocated_count_.fetch_sub(1, std::memory_order_relaxed);
	}

	// Construct object in-place
	template <typename... Args>
	[[nodiscard]] T* construct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		T* ptr = allocate();
		if (ptr == nullptr)
			return nullptr;

		if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
			return new (ptr) T(std::forward<Args>(args)...);
		} else {
			try {
				return new (ptr) T(std::forward<Args>(args)...);
			} catch (...) {
				deallocate(ptr);
				return nullptr;
			}
		}
	}

	// Destroy and deallocate object
	void destroy(T* ptr) noexcept {
		if (ptr == nullptr)
			return;
		ptr->~T();
		deallocate(ptr);
	}

	// Statistics
	[[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }

	[[nodiscard]] size_t allocated_count() const noexcept {
		return allocated_count_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] size_t available_count() const noexcept { return Capacity - allocated_count(); }

 private:
	struct Node {
		Node* next;
	};

	alignas(T) std::array<std::byte, sizeof(T) * Capacity> storage_;
	alignas(64) std::atomic<Node*> free_list_head_;
	alignas(64) std::atomic<size_t> allocated_count_{0};
};

}	 // namespace orderbook
