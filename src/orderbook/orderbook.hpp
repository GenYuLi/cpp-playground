#pragma once

#include <atomic>
#include <span>
#include <string>

#include "core/matching_engine.hpp"
#include "core/order.hpp"
#include "core/price_level.hpp"
#include "storage/intrusive_storage.hpp"
#include "storage/storage_policy.hpp"

namespace orderbook {

// Main OrderBook template class
// Template parameter allows switching storage implementations
template <StoragePolicy Storage>
class OrderBook {
 public:
	OrderBook() = default;

	// Add a new order (with matching)
	[[nodiscard]] MatchResult add_order(double price, uint64_t quantity, Side side,
																			OrderType type = OrderType::Limit) {
		uint64_t order_id = generate_order_id();
		Order order(order_id, price, quantity, side, type);
		return add_order(order);
	}

	// Add an order object (with matching)
	// If order.order_id is 0, auto-generate a new ID
	[[nodiscard]] MatchResult add_order(Order order) {
		// Auto-generate order ID if not provided
		if (order.order_id == 0) {
			order.order_id = generate_order_id();
		}

		// Try to match the order first
		MatchResult result = matcher_.match_order(order, storage_);

		// If not fully filled, add remainder to book
		if (!result.fully_filled && order.type == OrderType::Limit) {
			storage_.add_order(order);
		}

		return result;
	}

	// Add order without matching (passive order)
	[[nodiscard]] bool add_passive_order(double price, uint64_t quantity, Side side) {
		uint64_t order_id = generate_order_id();
		Order order(order_id, price, quantity, side, OrderType::Limit);
		return storage_.add_order(order);
	}

	// Cancel an order
	[[nodiscard]] bool cancel_order(uint64_t order_id) {
		Order* order = storage_.find_order(order_id);
		if (order == nullptr) {
			return false;
		}

		order->status = OrderStatus::Cancelled;
		return storage_.remove_order(order_id);
	}

	// Modify order quantity (cancel and replace)
	[[nodiscard]] bool modify_order(uint64_t order_id, uint64_t new_quantity) {
		Order* order = storage_.find_order(order_id);
		if (order == nullptr) {
			return false;
		}

		// Save order details
		double price = order->price;
		Side side = order->side;
		OrderType type = order->type;

		// Cancel old order
		if (!cancel_order(order_id)) {
			return false;
		}

		// Add new order with same ID (loses time priority!)
		Order new_order(order_id, price, new_quantity, side, type);
		return storage_.add_order(new_order);
	}

	// Find order by ID
	[[nodiscard]] Order* find_order(uint64_t order_id) { return storage_.find_order(order_id); }

	// Get best bid order
	[[nodiscard]] Order* get_best_bid() { return storage_.get_best_bid(); }

	// Get best ask order
	[[nodiscard]] Order* get_best_ask() { return storage_.get_best_ask(); }

	// Get best bid price
	[[nodiscard]] std::optional<double> get_best_bid_price() {
		Order* order = storage_.get_best_bid();
		if (order == nullptr)
			return std::nullopt;
		return order->price;
	}

	// Get best ask price
	[[nodiscard]] std::optional<double> get_best_ask_price() {
		Order* order = storage_.get_best_ask();
		if (order == nullptr)
			return std::nullopt;
		return order->price;
	}

	// Get spread
	[[nodiscard]] std::optional<double> get_spread() {
		auto bid = get_best_bid_price();
		auto ask = get_best_ask_price();
		if (!bid || !ask)
			return std::nullopt;
		return *ask - *bid;
	}

	// Get mid price
	[[nodiscard]] std::optional<double> get_mid_price() {
		auto bid = get_best_bid_price();
		auto ask = get_best_ask_price();
		if (!bid || !ask)
			return std::nullopt;
		return (*bid + *ask) / 2.0;
	}

	// Get market depth snapshot (L2 data)
	[[nodiscard]] MarketDepth get_market_depth(size_t levels = 10) {
		auto depth_data = storage_.get_depth(levels);

		std::vector<PriceLevel> bids;
		std::vector<PriceLevel> asks;

		// Storage returns bids first (up to `levels`), then asks (up to `levels`)
		// We need to split them based on the specified levels count
		size_t bid_count = 0;

		for (const auto& [price, qty, order_count] : depth_data) {
			// First `levels` entries are bids, rest are asks
			if (bid_count < levels && bids.size() < levels) {
				// Check if this could be a bid (we don't have more than levels bids yet)
				// Since storage returns bids first, we take them until we hit asks
				auto best_ask = get_best_ask_price();
				if (!best_ask || price < *best_ask) {
					bids.emplace_back(price, qty, order_count);
					++bid_count;
					continue;
				}
			}
			// This must be an ask
			asks.emplace_back(price, qty, order_count);
		}

		return MarketDepth(std::move(bids), std::move(asks));
	}

	// Batch add orders (for efficiency)
	[[nodiscard]] std::vector<MatchResult> add_orders_batch(std::span<Order> orders) {
		std::vector<MatchResult> results;
		results.reserve(orders.size());

		for (auto& order : orders) {
			results.push_back(add_order(order));
		}

		return results;
	}

	// Clear all orders
	void clear() { storage_.clear(); }

	// Get total number of orders in book
	[[nodiscard]] size_t size() const { return storage_.size(); }

	// Check if book is empty
	[[nodiscard]] bool empty() const { return storage_.size() == 0; }

	// Statistics
	[[nodiscard]] uint64_t total_trades() const { return matcher_.total_trades(); }

	[[nodiscard]] uint64_t total_volume() const { return matcher_.total_volume(); }

	void reset_statistics() { matcher_.reset_statistics(); }

	// Access storage (for advanced use)
	[[nodiscard]] Storage& storage() { return storage_; }

	[[nodiscard]] const Storage& storage() const { return storage_; }

 private:
	uint64_t generate_order_id() noexcept {
		return next_order_id_.fetch_add(1, std::memory_order_relaxed);
	}

	Storage storage_;
	MatchingEngine<Storage> matcher_;
	std::atomic<uint64_t> next_order_id_{1};
};

// Type aliases for common configurations
using IntrusiveOrderBook = OrderBook<IntrusiveStorage<>>;

}	 // namespace orderbook
