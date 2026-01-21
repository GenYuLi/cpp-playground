#pragma once

#include <algorithm>
#include <map>
#include <unordered_map>

#include "../allocator/slab_allocator.hpp"
#include "../concurrent/spinlock.hpp"
#include "storage_policy.hpp"

namespace orderbook {

// Intrusive list-based storage with slab allocator
// Best for: ultra-low latency, known capacity upper bound
// Features:
// - Zero allocations for list operations (intrusive pointers)
// - Slab allocator for predictable Order allocation
// - Best cache locality
// - Spinlock-protected for thread safety
template <size_t SlabSize = 4096>
class IntrusiveStorage : public StoragePolicyBase<IntrusiveStorage<SlabSize>> {
 public:
	IntrusiveStorage() = default;

	~IntrusiveStorage() { clear_impl(); }

	// Non-copyable, movable
	IntrusiveStorage(const IntrusiveStorage&) = delete;
	IntrusiveStorage& operator=(const IntrusiveStorage&) = delete;

	IntrusiveStorage(IntrusiveStorage&& other) noexcept
			: allocator_(std::move(other.allocator_)),
				price_levels_buy_(std::move(other.price_levels_buy_)),
				price_levels_sell_(std::move(other.price_levels_sell_)),
				order_index_(std::move(other.order_index_)),
				total_orders_(other.total_orders_) {
		other.total_orders_ = 0;
	}

	IntrusiveStorage& operator=(IntrusiveStorage&& other) noexcept {
		if (this != &other) {
			clear_impl();

			allocator_ = std::move(other.allocator_);
			price_levels_buy_ = std::move(other.price_levels_buy_);
			price_levels_sell_ = std::move(other.price_levels_sell_);
			order_index_ = std::move(other.order_index_);
			total_orders_ = other.total_orders_;

			other.total_orders_ = 0;
		}
		return *this;
	}

	// Implementation methods (called via CRTP)
	bool add_order_impl(const Order& order) {
		SpinlockGuard lock(lock_);

		// Check if order already exists
		if (order_index_.find(order.order_id) != order_index_.end()) {
			return false;
		}

		// Allocate and construct order
		Order* new_order = allocator_.construct(order);
		if (new_order == nullptr) {
			return false;	 // Allocation failed
		}

		// Add to price level
		if (order.side == Side::Buy) {
			add_to_price_level(price_levels_buy_, new_order);
		} else {
			add_to_price_level(price_levels_sell_, new_order);
		}

		// Add to index
		order_index_[order.order_id] = new_order;
		++total_orders_;

		return true;
	}

	bool remove_order_impl(uint64_t order_id) {
		SpinlockGuard lock(lock_);

		auto it = order_index_.find(order_id);
		if (it == order_index_.end()) {
			return false;
		}

		Order* order = it->second;

		// Remove from price level
		if (order->side == Side::Buy) {
			remove_from_price_level(price_levels_buy_, order);
		} else {
			remove_from_price_level(price_levels_sell_, order);
		}

		// Remove from index
		order_index_.erase(it);
		--total_orders_;

		// Destroy and deallocate
		allocator_.destroy(order);

		return true;
	}

	Order* find_order_impl(uint64_t order_id) {
		SpinlockGuard lock(lock_);

		auto it = order_index_.find(order_id);
		if (it == order_index_.end()) {
			return nullptr;
		}

		return it->second;
	}

	Order* get_best_bid_impl() {
		SpinlockGuard lock(lock_);

		if (price_levels_buy_.empty()) {
			return nullptr;
		}

		// Best bid is highest price (rbegin)
		auto& level = price_levels_buy_.rbegin()->second;
		return level.head;
	}

	Order* get_best_ask_impl() {
		SpinlockGuard lock(lock_);

		if (price_levels_sell_.empty()) {
			return nullptr;
		}

		// Best ask is lowest price (begin)
		auto& level = price_levels_sell_.begin()->second;
		return level.head;
	}

	std::vector<Order*> get_orders_at_price_impl(double price, Side side) {
		SpinlockGuard lock(lock_);

		std::vector<Order*> orders;

		auto& price_levels = (side == Side::Buy) ? price_levels_buy_ : price_levels_sell_;
		auto it = price_levels.find(price);

		if (it == price_levels.end()) {
			return orders;
		}

		// Traverse intrusive list
		Order* current = it->second.head;
		while (current != nullptr) {
			orders.push_back(current);
			current = current->next;
		}

		return orders;
	}

	std::vector<std::tuple<double, uint64_t, size_t>> get_depth_impl(size_t levels) {
		SpinlockGuard lock(lock_);

		std::vector<std::tuple<double, uint64_t, size_t>> result;
		result.reserve(levels * 2);

		// Get bid depth (highest prices first)
		size_t count = 0;
		for (auto it = price_levels_buy_.rbegin(); it != price_levels_buy_.rend() && count < levels;
				 ++it, ++count) {
			double price = it->first;
			uint64_t total_qty = calculate_level_quantity(it->second);
			size_t order_count = it->second.count;
			result.emplace_back(price, total_qty, order_count);
		}

		// Get ask depth (lowest prices first)
		count = 0;
		for (auto it = price_levels_sell_.begin(); it != price_levels_sell_.end() && count < levels;
				 ++it, ++count) {
			double price = it->first;
			uint64_t total_qty = calculate_level_quantity(it->second);
			size_t order_count = it->second.count;
			result.emplace_back(price, total_qty, order_count);
		}

		return result;
	}

	void clear_impl() {
		SpinlockGuard lock(lock_);

		// Destroy all orders
		for (auto& [order_id, order] : order_index_) {
			allocator_.destroy(order);
		}

		price_levels_buy_.clear();
		price_levels_sell_.clear();
		order_index_.clear();
		total_orders_ = 0;
	}

	[[nodiscard]] size_t size_impl() const { return total_orders_; }

	// Statistics
	[[nodiscard]] size_t allocator_capacity() const { return allocator_.total_capacity(); }

	[[nodiscard]] size_t num_price_levels() const {
		SpinlockGuard lock(lock_);
		return price_levels_buy_.size() + price_levels_sell_.size();
	}

 private:
	// Price level with intrusive list head
	struct PriceLevelList {
		Order* head = nullptr;
		Order* tail = nullptr;
		size_t count = 0;
	};

	// Add order to price level (maintains time priority)
	void add_to_price_level(std::map<double, PriceLevelList>& levels, Order* order) {
		auto& level = levels[order->price];

		if (level.head == nullptr) {
			// First order at this price
			level.head = order;
			level.tail = order;
			order->next = nullptr;
			order->prev = nullptr;
		} else {
			// Append to tail (time priority)
			order->prev = level.tail;
			order->next = nullptr;
			level.tail->next = order;
			level.tail = order;
		}

		++level.count;
	}

	// Remove order from price level
	void remove_from_price_level(std::map<double, PriceLevelList>& levels, Order* order) {
		auto it = levels.find(order->price);
		if (it == levels.end()) {
			return;
		}

		auto& level = it->second;

		// Unlink from doubly-linked list
		if (order->prev != nullptr) {
			order->prev->next = order->next;
		} else {
			// Removing head
			level.head = order->next;
		}

		if (order->next != nullptr) {
			order->next->prev = order->prev;
		} else {
			// Removing tail
			level.tail = order->prev;
		}

		--level.count;

		// Remove price level if empty
		if (level.count == 0) {
			levels.erase(it);
		}
	}

	// Calculate total quantity at price level
	static uint64_t calculate_level_quantity(const PriceLevelList& level) {
		uint64_t total = 0;
		Order* current = level.head;

		while (current != nullptr) {
			total += current->remaining_quantity();
			current = current->next;
		}

		return total;
	}

	SlabAllocator<Order, SlabSize> allocator_;

	// Price levels (std::map for automatic ordering)
	// Buy: higher price = better (use rbegin)
	// Sell: lower price = better (use begin)
	std::map<double, PriceLevelList> price_levels_buy_;
	std::map<double, PriceLevelList> price_levels_sell_;

	// Fast O(1) lookup by order ID
	std::unordered_map<uint64_t, Order*> order_index_;

	size_t total_orders_ = 0;

	mutable Spinlock lock_;
};

}	 // namespace orderbook
