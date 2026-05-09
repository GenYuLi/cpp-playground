#include <fmt/color.h>
#include <fmt/core.h>

#include <chrono>
#include <vector>

#include "matching_engine/matching/async_engine.hpp"
#include "matching_engine/scheduler/coro_scheduler.hpp"

using namespace matching_engine;
using namespace matching_engine::matching;
using namespace matching_engine::scheduler;

// Helper function to get current timestamp
Timestamp Timestamp::now() noexcept {
	auto now = std::chrono::high_resolution_clock::now();
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	return Timestamp{static_cast<uint64_t>(nanos)};
}

// Test 1: Basic async order submission with co_await
coro::Task<void> test_async_order_submission() {
	fmt::print(fg(fmt::color::cyan), "\n=== Test 1: Async Order Submission ===\n");

	AsyncMatchingEngine<> engine;

	// Create buy order
	OrderEvent buy_order;
	buy_order.type = OrderEventType::New;
	buy_order.order_id = OrderId{1};
	buy_order.price = Price::from_double(100.50);
	buy_order.quantity = Quantity{100};
	buy_order.side = Side::Buy;
	buy_order.order_type = OrderType::Limit;

	// Submit with co_await
	auto result = co_await engine.submit_order_async(buy_order);

	if (result.is_ok()) {
		fmt::print(fg(fmt::color::green), "✓ Order submitted successfully\n");
	} else {
		fmt::print(fg(fmt::color::red), "✗ Order submission failed\n");
	}

	// Get best bid
	auto best_bid = co_await engine.get_best_bid_async();
	if (best_bid) {
		fmt::print("  Best bid: ${:.2f}\n", best_bid->to_double());
	}

	co_return;
}

// Test 2: Async matching with producer-consumer pattern
coro::Task<size_t> async_order_producer(AsyncMatchingEngine<>& engine, int count) {
	size_t submitted = 0;

	for (int i = 0; i < count; ++i) {
		OrderEvent order;
		order.type = OrderEventType::New;
		order.order_id = OrderId{static_cast<uint64_t>(i + 1)};
		order.price = Price::from_double(100.0 + i * 0.1);
		order.quantity = Quantity{10};
		order.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
		order.order_type = OrderType::Limit;

		auto result = co_await engine.submit_order_async(order);

		if (result.is_ok()) {
			++submitted;
		}

		// Yield to allow other coroutines to run
		co_await coro::Yield{};
	}

	co_return submitted;
}

coro::Task<size_t> async_event_consumer(AsyncMatchingEngine<>& engine, int max_events) {
	size_t consumed = 0;

	for (int i = 0; i < max_events; ++i) {
		auto event = co_await engine.get_event_async();

		if (event) {
			consumed++;

			if (event->type == OrderEventType::Fill) {
				fmt::print("  Trade executed: Qty={}, Price=${:.2f}\n",
									 event->fill_info.filled_quantity.value, event->fill_info.fill_price.to_double());
			}
		}

		co_await coro::Yield{};
	}

	co_return consumed;
}

coro::Task<void> test_producer_consumer() {
	fmt::print(fg(fmt::color::cyan), "\n=== Test 2: Producer-Consumer Pattern ===\n");

	AsyncMatchingEngine<> engine;

	// Start producer and consumer coroutines
	auto producer_task = async_order_producer(engine, 10);
	auto consumer_task = async_event_consumer(engine, 20);

	// Run both until completion
	while (!producer_task.done() || !consumer_task.done()) {
		if (!producer_task.done()) {
			producer_task.resume();
		}
		if (!consumer_task.done()) {
			consumer_task.resume();
		}
	}

	size_t produced = producer_task.get_result();
	size_t consumed = consumer_task.get_result();

	fmt::print(fg(fmt::color::green), "✓ Produced: {}, Consumed: {}\n", produced, consumed);

	co_return;
}

// Test 3: Batch processing with co_await
coro::Task<void> test_batch_processing() {
	fmt::print(fg(fmt::color::cyan), "\n=== Test 3: Batch Order Processing ===\n");

	AsyncMatchingEngine<> engine;

	// Create batch of orders
	std::vector<OrderEvent> orders;
	for (int i = 0; i < 50; ++i) {
		OrderEvent order;
		order.type = OrderEventType::New;
		order.order_id = OrderId{static_cast<uint64_t>(i + 1)};
		order.price = Price::from_double(99.0 + (i % 10) * 0.5);
		order.quantity = Quantity{static_cast<uint64_t>(5 + i % 15)};
		order.side = (i % 3 == 0) ? Side::Buy : Side::Sell;
		order.order_type = OrderType::Limit;
		orders.push_back(order);
	}

	// Process batch
	auto start = std::chrono::high_resolution_clock::now();
	size_t processed = co_await engine.process_batch_async(std::span(orders));
	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	fmt::print(fg(fmt::color::green), "✓ Processed {} orders in {} μs\n", processed,
						 duration.count());
	fmt::print("  Average: {:.2f} μs per order\n", static_cast<double>(duration.count()) / processed);

	// Check orderbook state
	auto& book = engine.engine().orderbook();
	fmt::print("  Orderbook: {} orders, {} bid levels, {} ask levels\n", book.order_count(),
						 book.bid_levels(), book.ask_levels());

	co_return;
}

// Test 4: Market depth monitoring with async
coro::Task<void> test_market_depth_monitoring() {
	fmt::print(fg(fmt::color::cyan), "\n=== Test 4: Market Depth Monitoring ===\n");

	AsyncMatchingEngine<> engine;

	// Add some liquidity
	for (int i = 0; i < 5; ++i) {
		OrderEvent bid;
		bid.type = OrderEventType::New;
		bid.order_id = OrderId{static_cast<uint64_t>(i + 1)};
		bid.price = Price::from_double(100.0 - i * 0.1);
		bid.quantity = Quantity{100};
		bid.side = Side::Buy;
		bid.order_type = OrderType::Limit;

		co_await engine.submit_order_async(bid);
	}

	for (int i = 0; i < 5; ++i) {
		OrderEvent ask;
		ask.type = OrderEventType::New;
		ask.order_id = OrderId{static_cast<uint64_t>(i + 10)};
		ask.price = Price::from_double(100.1 + i * 0.1);
		ask.quantity = Quantity{100};
		ask.side = Side::Sell;
		ask.order_type = OrderType::Limit;

		co_await engine.submit_order_async(ask);
	}

	// Get market depth
	auto depth = co_await engine.get_market_depth_async();

	fmt::print(fg(fmt::color::yellow), "\n  Market Depth:\n");
	fmt::print("  {:>10} {:>10} | {:>10} {:>10}\n", "Bid Size", "Bid Price", "Ask Price", "Ask Size");
	fmt::print("  {:-^44}\n", "");

	size_t levels = std::min(depth.bid_levels(), depth.ask_levels());
	for (size_t i = 0; i < levels; ++i) {
		auto bid = depth.bid(i);
		auto ask = depth.ask(i);

		if (bid && ask) {
			fmt::print("  {:>10} ${:>9.2f} | ${:>9.2f} {:>10}\n", bid->quantity.value,
								 bid->price.to_double(), ask->price.to_double(), ask->quantity.value);
		}
	}

	// Check spread
	if (auto spread = depth.spread()) {
		fmt::print(fg(fmt::color::green), "\n  Spread: ${:.2f}\n", spread->to_double());
	}

	if (auto mid = depth.mid_price()) {
		fmt::print("  Mid Price: ${:.2f}\n", mid->to_double());
	}

	co_return;
}

// Test 5: Async ring buffer stress test
coro::Task<void> test_ring_buffer_async() {
	fmt::print(fg(fmt::color::cyan), "\n=== Test 5: Async Ring Buffer ===\n");

	memory::AsyncRingBuffer<int, 1024> buffer;

	// Producer coroutine
	auto producer = [&buffer]() -> coro::Task<int> {
		int sent = 0;
		for (int i = 0; i < 1000; ++i) {
			bool success = co_await buffer.push_async(i);
			if (success) {
				++sent;
			}
			if (i % 100 == 0) {
				co_await coro::Yield{};
			}
		}
		co_return sent;
	}();

	// Consumer coroutine
	auto consumer = [&buffer]() -> coro::Task<int> {
		int received = 0;
		for (int i = 0; i < 1000; ++i) {
			auto value = co_await buffer.pop_async();
			if (value) {
				++received;
			}
			if (i % 100 == 0) {
				co_await coro::Yield{};
			}
		}
		co_return received;
	}();

	// Run both coroutines
	while (!producer.done() || !consumer.done()) {
		if (!producer.done())
			producer.resume();
		if (!consumer.done())
			consumer.resume();
	}

	int sent = producer.get_result();
	int received = consumer.get_result();

	fmt::print(fg(fmt::color::green), "✓ Sent: {}, Received: {}\n", sent, received);

	co_return;
}

int main() {
	fmt::print(fg(fmt::color::magenta) | fmt::emphasis::bold,
						 "\n╔══════════════════════════════════════╗\n");
	fmt::print(fg(fmt::color::magenta) | fmt::emphasis::bold,
						 "║  Coroutine Matching Engine Tests    ║\n");
	fmt::print(fg(fmt::color::magenta) | fmt::emphasis::bold,
						 "╚══════════════════════════════════════╝\n");

	// Run all tests
	auto test1 = test_async_order_submission();
	while (!test1.done()) {
		test1.resume();
	}

	auto test2 = test_producer_consumer();
	while (!test2.done()) {
		test2.resume();
	}

	auto test3 = test_batch_processing();
	while (!test3.done()) {
		test3.resume();
	}

	auto test4 = test_market_depth_monitoring();
	while (!test4.done()) {
		test4.resume();
	}

	auto test5 = test_ring_buffer_async();
	while (!test5.done()) {
		test5.resume();
	}

	fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "\n✓ All tests completed!\n\n");

	return 0;
}
