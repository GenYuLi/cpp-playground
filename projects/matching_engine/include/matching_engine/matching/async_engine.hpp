#pragma once

#include <span>

#include "../core/types.hpp"
#include "../memory/async_ring_buffer.hpp"
#include "../scheduler/coro_scheduler.hpp"
#include "orderbook.hpp"

namespace matching_engine::matching {

// SyncMatchingEngine wrapper for tests
class SyncMatchingEngine {
 private:
	OrderBook orderbook_;

 public:
	SyncMatchingEngine() = default;

	Result<OrderEvent> add_order(Price price, Quantity quantity, Side side,
															 OrderType type = OrderType::Limit) {
		return orderbook_.add_order(price, quantity, side, type);
	}

	std::optional<Price> get_best_bid() const { return orderbook_.get_best_bid(); }

	std::optional<Price> get_best_ask() const { return orderbook_.get_best_ask(); }

	MarketDepth get_market_depth(size_t max_levels = 10) const {
		return orderbook_.get_market_depth(max_levels);
	}

	const OrderBook& orderbook() const { return orderbook_; }
	OrderBook& orderbook() { return orderbook_; }

	std::vector<OrderEvent> take_events() { return orderbook_.take_events(); }
};

// Wrapper that provides async interface for synchronous orderbook
template <size_t EventQueueSize = 4096>
class AsyncMatchingEngine {
 private:
	SyncMatchingEngine engine_;
	memory::AsyncRingBuffer<OrderEvent, EventQueueSize> event_queue_;

 public:
	AsyncMatchingEngine() = default;

	// Async order submission
	struct SubmitOrderAwaitable {
		AsyncMatchingEngine& async_engine;
		OrderEvent order_event;
		Result<OrderEvent> result{false};

		bool await_ready() {
			// Submit order immediately
			result = async_engine.engine_.add_order(order_event.price, order_event.quantity,
																							order_event.side, order_event.order_type);

			// Queue any events generated
			if (result.is_ok()) {
				auto events = async_engine.engine_.take_events();
				for (auto& evt : events) {
					async_engine.event_queue_.push(std::move(evt));
				}
			}

			return true;	// Always ready (synchronous execution)
		}

		void await_suspend(std::coroutine_handle<>) {}

		Result<OrderEvent> await_resume() { return result; }
	};

	coro::Task<Result<OrderEvent>> submit_order_async(const OrderEvent& order) {
		co_return co_await SubmitOrderAwaitable{*this, order};
	}

	// Async best bid query
	struct BestBidAwaitable {
		AsyncMatchingEngine& async_engine;
		std::optional<Price> result;

		bool await_ready() {
			result = async_engine.engine_.get_best_bid();
			return true;
		}

		void await_suspend(std::coroutine_handle<>) {}

		std::optional<Price> await_resume() { return result; }
	};

	coro::Task<std::optional<Price>> get_best_bid_async() {
		co_return co_await BestBidAwaitable{*this};
	}

	// Async best ask query
	struct BestAskAwaitable {
		AsyncMatchingEngine& async_engine;
		std::optional<Price> result;

		bool await_ready() {
			result = async_engine.engine_.get_best_ask();
			return true;
		}

		void await_suspend(std::coroutine_handle<>) {}

		std::optional<Price> await_resume() { return result; }
	};

	coro::Task<std::optional<Price>> get_best_ask_async() {
		co_return co_await BestAskAwaitable{*this};
	}

	// Async event retrieval
	coro::Task<std::optional<OrderEvent>> get_event_async() {
		co_return co_await event_queue_.pop_async();
	}

	// Async batch processing
	struct BatchAwaitable {
		AsyncMatchingEngine& async_engine;
		std::span<OrderEvent> orders;
		size_t processed = 0;

		bool await_ready() { return false; }

		void await_suspend(std::coroutine_handle<>) {
			// Process all orders
			for (auto& order : orders) {
				auto result = async_engine.engine_.add_order(order.price, order.quantity, order.side,
																										 order.order_type);

				if (result.is_ok()) {
					++processed;

					// Queue events
					auto events = async_engine.engine_.take_events();
					for (auto& evt : events) {
						async_engine.event_queue_.push(std::move(evt));
					}
				}
			}
		}

		size_t await_resume() { return processed; }
	};

	coro::Task<size_t> process_batch_async(std::span<OrderEvent> orders) {
		co_return co_await BatchAwaitable{*this, orders};
	}

	// Async market depth query
	struct MarketDepthAwaitable {
		AsyncMatchingEngine& async_engine;
		size_t max_levels;
		MarketDepth result;

		bool await_ready() {
			result = async_engine.engine_.get_market_depth(max_levels);
			return true;
		}

		void await_suspend(std::coroutine_handle<>) {}

		MarketDepth await_resume() { return std::move(result); }
	};

	coro::Task<MarketDepth> get_market_depth_async(size_t max_levels = 10) {
		co_return co_await MarketDepthAwaitable{*this, max_levels};
	}

	// Access to underlying engine (for inspection)
	const SyncMatchingEngine& engine() const { return engine_; }
	SyncMatchingEngine& engine() { return engine_; }
};

}	 // namespace matching_engine::matching
