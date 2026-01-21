#pragma once

#include <algorithm>
#include <vector>

#include "order.hpp"

namespace orderbook {

// Price level utilities
namespace price_level {

// Compare orders by price-time priority
struct PriceTimePriority {
	bool operator()(const Order* a, const Order* b) const noexcept { return (*a) < (*b); }
};

// Aggregate orders at a price level
inline uint64_t aggregate_quantity(const std::vector<Order*>& orders) noexcept {
	uint64_t total = 0;
	for (const Order* order : orders) {
		if (order != nullptr) {
			total += order->remaining_quantity();
		}
	}
	return total;
}

// Find the best (first) order at a price level
inline Order* get_best_order(std::vector<Order*>& orders) noexcept {
	if (orders.empty())
		return nullptr;

	// Orders should be sorted by time priority (FIFO)
	// Return the first non-filled order
	for (auto* order : orders) {
		if (order != nullptr && !order->is_fully_filled()) {
			return order;
		}
	}

	return nullptr;
}

// Remove filled orders from a price level
inline void cleanup_filled_orders(std::vector<Order*>& orders) {
	orders.erase(std::remove_if(
									 orders.begin(), orders.end(),
									 [](const Order* order) { return order == nullptr || order->is_fully_filled(); }),
							 orders.end());
}

// Sort orders by price-time priority
inline void sort_by_priority(std::vector<Order*>& orders) {
	std::sort(orders.begin(), orders.end(), PriceTimePriority{});
}

// Check if price level is empty (no remaining quantity)
inline bool is_empty(const std::vector<Order*>& orders) noexcept {
	for (const Order* order : orders) {
		if (order != nullptr && !order->is_fully_filled()) {
			return false;
		}
	}
	return true;
}

// Get total order count at price level
inline size_t count_orders(const std::vector<Order*>& orders) noexcept {
	return std::count_if(orders.begin(), orders.end(), [](const Order* order) {
		return order != nullptr && !order->is_fully_filled();
	});
}

}	 // namespace price_level

}	 // namespace orderbook
