#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <optional>

namespace orderbook {

// Lock-free Multi-Producer Single-Consumer (MPSC) queue
// Based on Dmitry Vyukov's intrusive MPSC queue algorithm
// http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
//
// Properties:
// - Lock-free for producers (multiple threads can enqueue concurrently)
// - Wait-free for consumer (single thread dequeues)
// - Linearizable
// - Requires elements to be default-constructible
template <typename T>
class MPSCQueue {
 private:
	struct alignas(64) Node {
		std::atomic<Node*> next{nullptr};
		T data;

		Node() = default;

		explicit Node(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
				: data(std::move(value)) {}

		explicit Node(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) : data(value) {}
	};

 public:
	MPSCQueue() {
		// Initialize with a dummy node
		// This simplifies the algorithm by ensuring head always points to a valid node
		Node* dummy = new Node();
		head_.store(dummy, std::memory_order_relaxed);
		tail_ = dummy;
	}

	~MPSCQueue() {
		// Drain the queue and delete all nodes
		while (Node* node = tail_) {
			tail_ = node->next.load(std::memory_order_relaxed);
			delete node;
		}
	}

	// Non-copyable, movable
	MPSCQueue(const MPSCQueue&) = delete;
	MPSCQueue& operator=(const MPSCQueue&) = delete;

	MPSCQueue(MPSCQueue&& other) noexcept
			: head_(other.head_.load(std::memory_order_relaxed)), tail_(other.tail_) {
		Node* dummy = new Node();
		other.head_.store(dummy, std::memory_order_relaxed);
		other.tail_ = dummy;
	}

	MPSCQueue& operator=(MPSCQueue&& other) noexcept {
		if (this != &other) {
			// Drain current queue
			while (tail_) {
				Node* node = tail_;
				tail_ = node->next.load(std::memory_order_relaxed);
				delete node;
			}

			head_.store(other.head_.load(std::memory_order_relaxed), std::memory_order_relaxed);
			tail_ = other.tail_;

			Node* dummy = new Node();
			other.head_.store(dummy, std::memory_order_relaxed);
			other.tail_ = dummy;
		}
		return *this;
	}

	// Enqueue (producer side, thread-safe for multiple producers)
	void enqueue(T&& value) {
		Node* node = new Node(std::move(value));
		enqueue_node(node);
	}

	void enqueue(const T& value) {
		Node* node = new Node(value);
		enqueue_node(node);
	}

	// Try to dequeue (consumer side, single consumer only!)
	// Returns std::nullopt if queue is empty
	[[nodiscard]] std::optional<T> try_dequeue() noexcept {
		// tail_ points to the last dequeued node (or dummy)
		Node* tail = tail_;
		Node* next = tail->next.load(std::memory_order_acquire);

		if (next == nullptr) {
			// Queue is empty
			return std::nullopt;
		}

		// Move data out of next node
		T value = std::move(next->data);

		// Advance tail
		tail_ = next;

		// Delete old tail node
		delete tail;

		return value;
	}

	// Dequeue with spinning (blocks until data available)
	// Consumer side only!
	[[nodiscard]] T dequeue_spin() noexcept {
		while (true) {
			if (auto value = try_dequeue()) {
				return std::move(*value);
			}

// Pause CPU for better power efficiency
#if defined(__x86_64__) || defined(_M_X64)
			__builtin_ia32_pause();
#elif defined(__aarch64__)
			asm volatile("yield" ::: "memory");
#endif
		}
	}

	// Check if queue is empty (approximate, not linearizable)
	[[nodiscard]] bool empty() const noexcept {
		Node* tail = tail_;
		Node* next = tail->next.load(std::memory_order_acquire);
		return next == nullptr;
	}

	// Get approximate size (expensive operation, iterates through queue)
	[[nodiscard]] size_t approx_size() const noexcept {
		size_t count = 0;
		Node* current = tail_->next.load(std::memory_order_acquire);

		while (current != nullptr) {
			++count;
			current = current->next.load(std::memory_order_acquire);
		}

		return count;
	}

 private:
	// Enqueue a node (internal, lock-free for multiple producers)
	void enqueue_node(Node* node) noexcept {
		assert(node != nullptr);
		assert(node->next.load(std::memory_order_relaxed) == nullptr);

		// Link the node at the head of the list
		// Multiple producers compete here via CAS
		Node* prev_head = head_.exchange(node, std::memory_order_acq_rel);

		// Connect the previous head to this node
		// This makes the node visible to the consumer
		prev_head->next.store(node, std::memory_order_release);
	}

	// Cache-line aligned to prevent false sharing
	alignas(64) std::atomic<Node*> head_;	 // Producers modify (CAS)
	alignas(64) Node* tail_;							 // Consumer only (no atomics needed)
};

// Bounded MPSC queue with fixed capacity
// Uses a ring buffer for cache efficiency
template <typename T, size_t Capacity>
class BoundedMPSCQueue {
	static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

 public:
	BoundedMPSCQueue() noexcept : buffer_(std::make_unique<Slot[]>(Capacity)) {
		// Initialize sequence numbers
		for (size_t i = 0; i < Capacity; ++i) {
			buffer_[i].sequence.store(i, std::memory_order_relaxed);
		}
		enqueue_pos_.store(0, std::memory_order_relaxed);
		dequeue_pos_.store(0, std::memory_order_relaxed);
	}

	~BoundedMPSCQueue() = default;

	// Non-copyable, non-movable
	BoundedMPSCQueue(const BoundedMPSCQueue&) = delete;
	BoundedMPSCQueue& operator=(const BoundedMPSCQueue&) = delete;
	BoundedMPSCQueue(BoundedMPSCQueue&&) = delete;
	BoundedMPSCQueue& operator=(BoundedMPSCQueue&&) = delete;

	// Try to enqueue (returns false if queue is full)
	bool try_enqueue(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
		size_t pos = enqueue_pos_.load(std::memory_order_relaxed);

		while (true) {
			Slot& slot = buffer_[pos & (Capacity - 1)];
			size_t seq = slot.sequence.load(std::memory_order_acquire);
			intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

			if (diff == 0) {
				// Slot is available, try to claim it
				if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed,
																							 std::memory_order_relaxed)) {
					// Successfully claimed slot, write data
					slot.data = std::move(value);
					slot.sequence.store(pos + 1, std::memory_order_release);
					return true;
				}
			} else if (diff < 0) {
				// Queue is full
				return false;
			} else {
				// Another producer claimed this slot, try next position
				pos = enqueue_pos_.load(std::memory_order_relaxed);
			}
		}
	}

	bool try_enqueue(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
		T copy = value;
		return try_enqueue(std::move(copy));
	}

	// Try to dequeue (consumer side only!)
	[[nodiscard]] std::optional<T> try_dequeue() noexcept {
		size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
		Slot& slot = buffer_[pos & (Capacity - 1)];
		size_t seq = slot.sequence.load(std::memory_order_acquire);
		intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

		if (diff == 0) {
			// Data available
			T value = std::move(slot.data);
			slot.sequence.store(pos + Capacity, std::memory_order_release);
			dequeue_pos_.store(pos + 1, std::memory_order_release);
			return value;
		}

		// Queue is empty
		return std::nullopt;
	}

	// Get approximate size
	[[nodiscard]] size_t approx_size() const noexcept {
		size_t enqueue_pos = enqueue_pos_.load(std::memory_order_relaxed);
		size_t dequeue_pos = dequeue_pos_.load(std::memory_order_relaxed);
		return enqueue_pos - dequeue_pos;
	}

	[[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }

 private:
	struct alignas(64) Slot {
		std::atomic<size_t> sequence;
		T data;
	};

	std::unique_ptr<Slot[]> buffer_;
	alignas(64) std::atomic<size_t> enqueue_pos_;
	alignas(64) std::atomic<size_t> dequeue_pos_;
};

}	 // namespace orderbook
