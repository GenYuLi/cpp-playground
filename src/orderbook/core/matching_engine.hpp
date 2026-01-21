#pragma once

#include <atomic>
#include <vector>

#include "../storage/storage_policy.hpp"
#include "order.hpp"
#include "price_level.hpp"

namespace orderbook {

// Match result for a single order
struct MatchResult {
	std::vector<Trade> trades;
	bool fully_filled;
	uint64_t remaining_qty;
	uint64_t filled_qty;

	MatchResult() : fully_filled(false), remaining_qty(0), filled_qty(0) {}

	[[nodiscard]] bool has_trades() const noexcept { return !trades.empty(); }

	[[nodiscard]] size_t num_trades() const noexcept { return trades.size(); }
};

// Matching engine with price-time priority
// Template parameter allows different storage policies
template <StoragePolicy Storage>
class MatchingEngine {
 public:
	MatchingEngine() = default;

	// Match an incoming order against the book
	// Returns match result with executed trades
	[[nodiscard]] MatchResult match_order(Order& incoming_order, Storage& storage) {
		MatchResult result;
		result.remaining_qty = incoming_order.quantity;
		result.filled_qty = 0;

		if (incoming_order.type == OrderType::Market) {
			match_market_order(incoming_order, storage, result);
		} else {
			match_limit_order(incoming_order, storage, result);
		}

		result.fully_filled = (result.remaining_qty == 0);
		incoming_order.filled_quantity = result.filled_qty;

		if (incoming_order.is_fully_filled()) {
			incoming_order.status = OrderStatus::Filled;
		} else if (result.filled_qty > 0) {
			incoming_order.status = OrderStatus::PartiallyFilled;
		}

		return result;
	}

	// Statistics
	[[nodiscard]] uint64_t total_trades() const noexcept {
		return trade_count_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] uint64_t total_volume() const noexcept {
		return total_volume_.load(std::memory_order_relaxed);
	}

	void reset_statistics() noexcept {
		trade_count_.store(0, std::memory_order_relaxed);
		total_volume_.store(0, std::memory_order_relaxed);
	}

 private:
	void match_market_order(Order& incoming, Storage& storage, MatchResult& result) {
		// Market order matches at any price
		while (result.remaining_qty > 0) {
			Order* resting =
					(incoming.side == Side::Buy) ? storage.get_best_ask() : storage.get_best_bid();

			if (resting == nullptr) {
				// No more orders to match
				break;
			}

			execute_trade(incoming, *resting, storage, result);
		}
	}

	void match_limit_order(Order& incoming, Storage& storage, MatchResult& result) {
		// Limit order only matches if price is acceptable
		while (result.remaining_qty > 0) {
			Order* resting =
					(incoming.side == Side::Buy) ? storage.get_best_ask() : storage.get_best_bid();

			if (resting == nullptr) {
				// No more orders on opposite side
				break;
			}

			// Check if prices cross
			if (!incoming.can_match_with(*resting)) {
				// Price doesn't match
				break;
			}

			execute_trade(incoming, *resting, storage, result);
		}
	}

	void execute_trade(Order& incoming, Order& resting, Storage& storage, MatchResult& result) {
		// Trade quantity is minimum of remaining quantities
		uint64_t trade_qty = std::min(result.remaining_qty, resting.remaining_quantity());

		// Trade executes at resting order price (price-time priority)
		double trade_price = resting.price;

		// Create trade
		uint64_t trade_id = generate_trade_id();
		Trade trade;

		if (incoming.side == Side::Buy) {
			trade = Trade(trade_id, incoming.order_id, resting.order_id, trade_price, trade_qty);
		} else {
			trade = Trade(trade_id, resting.order_id, incoming.order_id, trade_price, trade_qty);
		}

		// Update order quantities
		incoming.filled_quantity += trade_qty;
		resting.filled_quantity += trade_qty;

		result.remaining_qty -= trade_qty;
		result.filled_qty += trade_qty;
		result.trades.push_back(trade);

		// Update statistics
		trade_count_.fetch_add(1, std::memory_order_relaxed);
		total_volume_.fetch_add(trade_qty, std::memory_order_relaxed);

		// Remove resting order if fully filled
		if (resting.is_fully_filled()) {
			resting.status = OrderStatus::Filled;
			storage.remove_order(resting.order_id);
		} else {
			resting.status = OrderStatus::PartiallyFilled;
		}
	}

	uint64_t generate_trade_id() noexcept {
		return next_trade_id_.fetch_add(1, std::memory_order_relaxed);
	}

	std::atomic<uint64_t> next_trade_id_{1};
	std::atomic<uint64_t> trade_count_{0};
	std::atomic<uint64_t> total_volume_{0};
};

}	 // namespace orderbook
