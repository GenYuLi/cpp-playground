#pragma once

#include <list>
#include <map>
#include <optional>
#include <vector>

#include "../core/types.hpp"

namespace matching_engine::matching {

// Simple order representation
struct Order {
	OrderId id;
	Price price;
	Quantity quantity;
	Quantity filled{0};
	Side side;
	OrderType type;
	Timestamp timestamp;
};

// Market depth level
struct DepthLevel {
	Price price;
	Quantity quantity;
};

// Market depth snapshot
class MarketDepth {
	std::vector<DepthLevel> bids_;
	std::vector<DepthLevel> asks_;

 public:
	void add_bid(Price price, Quantity qty) { bids_.push_back({price, qty}); }

	void add_ask(Price price, Quantity qty) { asks_.push_back({price, qty}); }

	size_t bid_levels() const { return bids_.size(); }
	size_t ask_levels() const { return asks_.size(); }

	std::optional<DepthLevel> bid(size_t level) const {
		if (level >= bids_.size())
			return std::nullopt;
		return bids_[level];
	}

	std::optional<DepthLevel> ask(size_t level) const {
		if (level >= asks_.size())
			return std::nullopt;
		return asks_[level];
	}

	std::optional<Price> spread() const {
		if (bids_.empty() || asks_.empty())
			return std::nullopt;
		return Price{asks_[0].price.ticks - bids_[0].price.ticks};
	}

	std::optional<Price> mid_price() const {
		if (bids_.empty() || asks_.empty())
			return std::nullopt;
		return Price{(bids_[0].price.ticks + asks_[0].price.ticks) / 2};
	}
};

// Simple orderbook implementation
class OrderBook {
 private:
	// Price -> list of orders at that price (FIFO for time priority)
	std::map<Price, std::list<Order>, std::greater<>> bids_;	// Descending
	std::map<Price, std::list<Order>, std::less<>> asks_;			// Ascending

	size_t order_count_ = 0;
	uint64_t next_order_id_ = 1;

	std::vector<OrderEvent> pending_events_;

 public:
	// Add order and match
	Result<OrderEvent> add_order(Price price, Quantity quantity, Side side,
															 OrderType type = OrderType::Limit) {
		Order order{.id = OrderId{next_order_id_++},
								.price = price,
								.quantity = quantity,
								.filled = Quantity{0},
								.side = side,
								.type = type,
								.timestamp = Timestamp::now()};

		OrderEvent event{.type = OrderEventType::New,
										 .order_id = order.id,
										 .price = order.price,
										 .quantity = order.quantity,
										 .side = order.side,
										 .order_type = order.type,
										 .timestamp = order.timestamp};

		// Try to match
		if (side == Side::Buy) {
			match_buy_order(order, event);
		} else {
			match_sell_order(order, event);
		}

		// If not fully filled, add to book
		if (order.filled.value < order.quantity.value && type == OrderType::Limit) {
			if (side == Side::Buy) {
				bids_[order.price].push_back(order);
			} else {
				asks_[order.price].push_back(order);
			}
			++order_count_;
		}

		return Result<OrderEvent>::Ok(event);
	}

	std::optional<Price> get_best_bid() const {
		if (bids_.empty())
			return std::nullopt;
		return bids_.begin()->first;
	}

	std::optional<Price> get_best_ask() const {
		if (asks_.empty())
			return std::nullopt;
		return asks_.begin()->first;
	}

	MarketDepth get_market_depth(size_t max_levels = 10) const {
		MarketDepth depth;

		size_t count = 0;
		for (const auto& [price, orders] : bids_) {
			if (count >= max_levels)
				break;

			Quantity total{0};
			for (const auto& order : orders) {
				total.value += (order.quantity.value - order.filled.value);
			}

			if (total.value > 0) {
				depth.add_bid(price, total);
				++count;
			}
		}

		count = 0;
		for (const auto& [price, orders] : asks_) {
			if (count >= max_levels)
				break;

			Quantity total{0};
			for (const auto& order : orders) {
				total.value += (order.quantity.value - order.filled.value);
			}

			if (total.value > 0) {
				depth.add_ask(price, total);
				++count;
			}
		}

		return depth;
	}

	size_t order_count() const { return order_count_; }
	size_t bid_levels() const { return bids_.size(); }
	size_t ask_levels() const { return asks_.size(); }

	std::vector<OrderEvent> take_events() { return std::move(pending_events_); }

 private:
	void match_buy_order(Order& order, OrderEvent& event) {
		while (!asks_.empty() && order.filled.value < order.quantity.value) {
			auto& [ask_price, ask_orders] = *asks_.begin();

			// Check if we can match
			if (order.type == OrderType::Market || order.price.ticks >= ask_price.ticks) {
				if (ask_orders.empty()) {
					asks_.erase(asks_.begin());
					continue;
				}

				auto& resting_order = ask_orders.front();
				Quantity resting_qty{resting_order.quantity.value - resting_order.filled.value};
				Quantity incoming_qty{order.quantity.value - order.filled.value};
				Quantity fill_qty{std::min(resting_qty.value, incoming_qty.value)};

				// Execute trade at resting order price
				order.filled.value += fill_qty.value;
				resting_order.filled.value += fill_qty.value;

				// Record fill event
				OrderEvent fill_event{
						.type = OrderEventType::Fill,
						.order_id = order.id,
						.price = ask_price,
						.quantity = fill_qty,
						.side = order.side,
						.order_type = order.type,
						.timestamp = Timestamp::now(),
						.fill_info =
								FillInfo{.filled_quantity = fill_qty,
												 .remaining_quantity = Quantity{order.quantity.value - order.filled.value},
												 .fill_price = ask_price,
												 .fill_time = Timestamp::now()}};

				pending_events_.push_back(fill_event);
				event.fill_info.filled_quantity.value += fill_qty.value;
				event.fill_info.fill_price = ask_price;

				// Remove if fully filled
				if (resting_order.filled.value >= resting_order.quantity.value) {
					ask_orders.pop_front();
					--order_count_;
					if (ask_orders.empty()) {
						asks_.erase(asks_.begin());
					}
				}
			} else {
				break;	// No more matches possible
			}
		}

		event.fill_info.remaining_quantity = Quantity{order.quantity.value - order.filled.value};
	}

	void match_sell_order(Order& order, OrderEvent& event) {
		while (!bids_.empty() && order.filled.value < order.quantity.value) {
			auto& [bid_price, bid_orders] = *bids_.begin();

			// Check if we can match
			if (order.type == OrderType::Market || order.price.ticks <= bid_price.ticks) {
				if (bid_orders.empty()) {
					bids_.erase(bids_.begin());
					continue;
				}

				auto& resting_order = bid_orders.front();
				Quantity resting_qty{resting_order.quantity.value - resting_order.filled.value};
				Quantity incoming_qty{order.quantity.value - order.filled.value};
				Quantity fill_qty{std::min(resting_qty.value, incoming_qty.value)};

				// Execute trade at resting order price
				order.filled.value += fill_qty.value;
				resting_order.filled.value += fill_qty.value;

				// Record fill event
				OrderEvent fill_event{
						.type = OrderEventType::Fill,
						.order_id = order.id,
						.price = bid_price,
						.quantity = fill_qty,
						.side = order.side,
						.order_type = order.type,
						.timestamp = Timestamp::now(),
						.fill_info =
								FillInfo{.filled_quantity = fill_qty,
												 .remaining_quantity = Quantity{order.quantity.value - order.filled.value},
												 .fill_price = bid_price,
												 .fill_time = Timestamp::now()}};

				pending_events_.push_back(fill_event);
				event.fill_info.filled_quantity.value += fill_qty.value;
				event.fill_info.fill_price = bid_price;

				// Remove if fully filled
				if (resting_order.filled.value >= resting_order.quantity.value) {
					bid_orders.pop_front();
					--order_count_;
					if (bid_orders.empty()) {
						bids_.erase(bids_.begin());
					}
				}
			} else {
				break;	// No more matches possible
			}
		}

		event.fill_info.remaining_quantity = Quantity{order.quantity.value - order.filled.value};
	}
};

}	 // namespace matching_engine::matching
