#pragma once

#include <concepts>
#include <optional>
#include <vector>

#include "../core/order.hpp"

namespace orderbook {

// Storage policy concept (C++20 concept for type checking)
template <typename T>
concept StoragePolicy = requires(T storage, const Order& order, uint64_t order_id, Side side) {
	// Add order to storage
	{ storage.add_order(order) } -> std::same_as<bool>;

	// Remove order by ID
	{ storage.remove_order(order_id) } -> std::same_as<bool>;

	// Find order by ID
	{ storage.find_order(order_id) } -> std::same_as<Order*>;

	// Get best bid (highest buy price)
	{ storage.get_best_bid() } -> std::same_as<Order*>;

	// Get best ask (lowest sell price)
	{ storage.get_best_ask() } -> std::same_as<Order*>;

	// Get all orders at a price level
	{ storage.get_orders_at_price(0.0, side) } -> std::same_as<std::vector<Order*>>;

	// Get market depth (L2 data) - returns (price, quantity, order_count)
	{ storage.get_depth(10) } -> std::same_as<std::vector<std::tuple<double, uint64_t, size_t>>>;

	// Clear all orders
	{ storage.clear() } -> std::same_as<void>;

	// Get total number of orders
	{ storage.size() } -> std::same_as<size_t>;
};

// Base storage policy using CRTP
// Derived classes should implement the actual storage logic
template <typename Derived>
class StoragePolicyBase {
 public:
	// Add order
	bool add_order(const Order& order) { return static_cast<Derived*>(this)->add_order_impl(order); }

	// Remove order
	bool remove_order(uint64_t order_id) {
		return static_cast<Derived*>(this)->remove_order_impl(order_id);
	}

	// Find order
	Order* find_order(uint64_t order_id) {
		return static_cast<Derived*>(this)->find_order_impl(order_id);
	}

	// Get best bid
	Order* get_best_bid() { return static_cast<Derived*>(this)->get_best_bid_impl(); }

	// Get best ask
	Order* get_best_ask() { return static_cast<Derived*>(this)->get_best_ask_impl(); }

	// Get orders at price level
	std::vector<Order*> get_orders_at_price(double price, Side side) {
		return static_cast<Derived*>(this)->get_orders_at_price_impl(price, side);
	}

	// Get market depth - returns (price, quantity, order_count) tuples
	// First `levels` entries are bids (highest to lowest), next are asks (lowest to highest)
	std::vector<std::tuple<double, uint64_t, size_t>> get_depth(size_t levels) {
		return static_cast<Derived*>(this)->get_depth_impl(levels);
	}

	// Clear all orders
	void clear() { static_cast<Derived*>(this)->clear_impl(); }

	// Get total size
	size_t size() const { return static_cast<const Derived*>(this)->size_impl(); }

 protected:
	~StoragePolicyBase() = default;
};

// Helper struct for market depth (price level aggregation)
struct PriceLevel {
	double price;
	uint64_t total_quantity;
	size_t order_count;

	constexpr PriceLevel() noexcept : price(0.0), total_quantity(0), order_count(0) {}

	constexpr PriceLevel(double p, uint64_t qty, size_t count) noexcept
			: price(p), total_quantity(qty), order_count(count) {}
};

// Market depth snapshot (L2 data)
struct MarketDepth {
	std::vector<PriceLevel> bids;	 // Sorted by price descending
	std::vector<PriceLevel> asks;	 // Sorted by price ascending

	MarketDepth() = default;

	MarketDepth(std::vector<PriceLevel> b, std::vector<PriceLevel> a)
			: bids(std::move(b)), asks(std::move(a)) {}

	// Get best bid price
	[[nodiscard]] std::optional<double> best_bid_price() const noexcept {
		if (bids.empty())
			return std::nullopt;
		return bids.front().price;
	}

	// Get best ask price
	[[nodiscard]] std::optional<double> best_ask_price() const noexcept {
		if (asks.empty())
			return std::nullopt;
		return asks.front().price;
	}

	// Get spread
	[[nodiscard]] std::optional<double> spread() const noexcept {
		auto bid = best_bid_price();
		auto ask = best_ask_price();
		if (!bid || !ask)
			return std::nullopt;
		return *ask - *bid;
	}

	// Get mid price
	[[nodiscard]] std::optional<double> mid_price() const noexcept {
		auto bid = best_bid_price();
		auto ask = best_ask_price();
		if (!bid || !ask)
			return std::nullopt;
		return (*bid + *ask) / 2.0;
	}
};

}	 // namespace orderbook
