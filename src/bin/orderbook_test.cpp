#include "../orderbook/orderbook.hpp"

#include <fmt/core.h>

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace orderbook;
using namespace std::chrono;

// Benchmark helper
template <typename Func>
auto measure_time(const std::string& name, Func&& func) {
	auto start = high_resolution_clock::now();
	auto result = func();
	auto end = high_resolution_clock::now();

	auto duration = duration_cast<microseconds>(end - start).count();
	fmt::print("{}: {} μs\n", name, duration);

	return result;
}

// Test basic orderbook operations
void test_basic_operations() {
	fmt::print("\n=== Test: Basic Operations ===\n");

	IntrusiveOrderBook book;

	// Add some buy orders
	auto r1 = book.add_order(100.0, 10, Side::Buy);
	auto r2 = book.add_order(99.5, 15, Side::Buy);
	auto r3 = book.add_order(99.0, 20, Side::Buy);

	// Add some sell orders
	auto r4 = book.add_order(101.0, 10, Side::Sell);
	auto r5 = book.add_order(101.5, 15, Side::Sell);
	auto r6 = book.add_order(102.0, 20, Side::Sell);

	fmt::print("Total orders: {}\n", book.size());
	fmt::print("Best bid: {}\n", book.get_best_bid_price().value_or(0.0));
	fmt::print("Best ask: {}\n", book.get_best_ask_price().value_or(0.0));
	fmt::print("Spread: {}\n", book.get_spread().value_or(0.0));
	fmt::print("Mid price: {}\n", book.get_mid_price().value_or(0.0));

	// Test matching
	fmt::print("\nAdding aggressive buy order at 101.5...\n");
	auto match_result = book.add_order(101.5, 25, Side::Buy);

	fmt::print("Trades executed: {}\n", match_result.num_trades());
	fmt::print("Filled quantity: {}\n", match_result.filled_qty);
	fmt::print("Remaining quantity: {}\n", match_result.remaining_qty);
	fmt::print("Fully filled: {}\n", match_result.fully_filled);

	for (const auto& trade : match_result.trades) {
		fmt::print("  Trade: price={}, qty={}\n", trade.price, trade.quantity);
	}

	fmt::print("\nAfter matching:\n");
	fmt::print("Total orders: {}\n", book.size());
	fmt::print("Total trades: {}\n", book.total_trades());
	fmt::print("Total volume: {}\n", book.total_volume());
}

// Test market depth
void test_market_depth() {
	fmt::print("\n=== Test: Market Depth ===\n");

	IntrusiveOrderBook book;

	// Build a simple book
	book.add_order(100.0, 100, Side::Buy);
	book.add_order(99.5, 150, Side::Buy);
	book.add_order(99.0, 200, Side::Buy);
	book.add_order(98.5, 250, Side::Buy);

	book.add_order(101.0, 100, Side::Sell);
	book.add_order(101.5, 150, Side::Sell);
	book.add_order(102.0, 200, Side::Sell);
	book.add_order(102.5, 250, Side::Sell);

	auto depth = book.get_market_depth(5);

	fmt::print("\nBids:\n");
	for (const auto& level : depth.bids) {
		fmt::print("  {:.2f}: {}\n", level.price, level.total_quantity);
	}

	fmt::print("\nAsks:\n");
	for (const auto& level : depth.asks) {
		fmt::print("  {:.2f}: {}\n", level.price, level.total_quantity);
	}

	fmt::print("\nBest bid: {:.2f}\n", depth.best_bid_price().value_or(0.0));
	fmt::print("Best ask: {:.2f}\n", depth.best_ask_price().value_or(0.0));
	fmt::print("Spread: {:.2f}\n", depth.spread().value_or(0.0));
	fmt::print("Mid: {:.2f}\n", depth.mid_price().value_or(0.0));
}

// Benchmark throughput
void benchmark_throughput() {
	fmt::print("\n=== Benchmark: Throughput ===\n");

	constexpr size_t NUM_ORDERS = 100000;

	IntrusiveOrderBook book;
	std::mt19937 rng(42);
	std::uniform_real_distribution<double> price_dist(95.0, 105.0);
	std::uniform_int_distribution<uint64_t> qty_dist(1, 100);
	std::uniform_int_distribution<int> side_dist(0, 1);

	// Pre-generate orders
	std::vector<Order> orders;
	orders.reserve(NUM_ORDERS);

	for (size_t i = 0; i < NUM_ORDERS; ++i) {
		double price = price_dist(rng);
		uint64_t qty = qty_dist(rng);
		Side side = (side_dist(rng) == 0) ? Side::Buy : Side::Sell;

		orders.emplace_back(i + 1, price, qty, side);
	}

	// Benchmark add operations
	auto start = high_resolution_clock::now();

	for (auto& order : orders) {
		book.add_order(order);
	}

	auto end = high_resolution_clock::now();
	auto duration_us = duration_cast<microseconds>(end - start).count();

	double throughput = (NUM_ORDERS * 1000000.0) / duration_us;

	fmt::print("Added {} orders in {} μs\n", NUM_ORDERS, duration_us);
	fmt::print("Throughput: {:.2f} orders/sec\n", throughput);
	fmt::print("Average latency: {:.3f} μs/order\n", static_cast<double>(duration_us) / NUM_ORDERS);
	fmt::print("\nFinal state:\n");
	fmt::print("  Orders in book: {}\n", book.size());
	fmt::print("  Trades executed: {}\n", book.total_trades());
	fmt::print("  Volume: {}\n", book.total_volume());
}

// Benchmark latency (percentiles)
void benchmark_latency() {
	fmt::print("\n=== Benchmark: Latency Percentiles ===\n");

	constexpr size_t NUM_SAMPLES = 10000;

	IntrusiveOrderBook book;
	std::vector<uint64_t> latencies;
	latencies.reserve(NUM_SAMPLES);

	std::mt19937 rng(42);
	std::uniform_real_distribution<double> price_dist(95.0, 105.0);
	std::uniform_int_distribution<uint64_t> qty_dist(1, 100);

	// Warm up the book
	for (size_t i = 0; i < 1000; ++i) {
		book.add_order(price_dist(rng), qty_dist(rng), (i % 2 == 0) ? Side::Buy : Side::Sell);
	}

	// Measure individual order latencies
	for (size_t i = 0; i < NUM_SAMPLES; ++i) {
		auto start = high_resolution_clock::now();
		book.add_order(price_dist(rng), qty_dist(rng), (i % 2 == 0) ? Side::Buy : Side::Sell);
		auto end = high_resolution_clock::now();

		latencies.push_back(duration_cast<nanoseconds>(end - start).count());
	}

	// Calculate percentiles
	std::sort(latencies.begin(), latencies.end());

	auto percentile = [&latencies](double p) {
		size_t idx = static_cast<size_t>(p * latencies.size());
		return latencies[std::min(idx, latencies.size() - 1)];
	};

	fmt::print("p50: {} ns\n", percentile(0.50));
	fmt::print("p90: {} ns\n", percentile(0.90));
	fmt::print("p95: {} ns\n", percentile(0.95));
	fmt::print("p99: {} ns\n", percentile(0.99));
	fmt::print("p99.9: {} ns\n", percentile(0.999));
	fmt::print("max: {} ns\n", latencies.back());
}

// Test cancel operations
void test_cancel() {
	fmt::print("\n=== Test: Order Cancellation ===\n");

	IntrusiveOrderBook book;

	// Add orders and track IDs
	std::vector<uint64_t> order_ids;

	for (int i = 0; i < 10; ++i) {
		auto result = book.add_order(100.0 + i, 10, Side::Buy);
		// Extract order ID from internal counter (this is a simplification)
	}

	fmt::print("Orders before cancel: {}\n", book.size());

	// Cancel some orders (Note: in real usage, you'd track the order IDs)
	// For now, just demonstrate the API
	bool canceled = book.cancel_order(5);
	fmt::print("Cancel order 5: {}\n", canceled);

	fmt::print("Orders after cancel: {}\n", book.size());
}

int main() {
	fmt::print("╔════════════════════════════════════════╗\n");
	fmt::print("║   OrderBook Performance Tests          ║\n");
	fmt::print("╚════════════════════════════════════════╝\n");

	try {
		test_basic_operations();
		test_market_depth();
		test_cancel();
		benchmark_throughput();
		benchmark_latency();

		fmt::print("\n✓ All tests completed successfully!\n");
		return 0;

	} catch (const std::exception& e) {
		fmt::print("\n✗ Error: {}\n", e.what());
		return 1;
	}
}
