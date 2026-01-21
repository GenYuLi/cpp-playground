#pragma once

#include <chrono>
#include <compare>
#include <cstdint>

namespace orderbook {

enum class Side : uint8_t { Buy = 0, Sell = 1 };

enum class OrderType : uint8_t { Limit = 0, Market = 1 };

enum class OrderStatus : uint8_t { New = 0, PartiallyFilled = 1, Filled = 2, Cancelled = 3 };

// Cache-line aligned order structure
// Designed to fit in a single 64-byte cache line
struct alignas(64) Order {
	// Hot path data - first 64 bytes
	uint64_t order_id;				 // 8 bytes
	uint64_t timestamp_ns;		 // 8 bytes - nanoseconds since epoch
	double price;							 // 8 bytes
	uint64_t quantity;				 // 8 bytes
	uint64_t filled_quantity;	 // 8 bytes

	Side side;						// 1 byte
	OrderType type;				// 1 byte
	OrderStatus status;		// 1 byte
	uint8_t padding1[5];	// 5 bytes padding

	// Intrusive list pointers
	Order* next;	// 8 bytes
	Order* prev;	// 8 bytes
	// Total: 64 bytes exactly

	// Default constructor
	constexpr Order() noexcept
			: order_id(0),
				timestamp_ns(0),
				price(0.0),
				quantity(0),
				filled_quantity(0),
				side(Side::Buy),
				type(OrderType::Limit),
				status(OrderStatus::New),
				padding1{},
				next(nullptr),
				prev(nullptr) {}

	// Parameterized constructor
	Order(uint64_t id, double p, uint64_t qty, Side s, OrderType t = OrderType::Limit) noexcept
			: order_id(id),
				timestamp_ns(get_timestamp()),
				price(p),
				quantity(qty),
				filled_quantity(0),
				side(s),
				type(t),
				status(OrderStatus::New),
				padding1{},
				next(nullptr),
				prev(nullptr) {}

	// Get remaining unfilled quantity
	[[nodiscard]] constexpr uint64_t remaining_quantity() const noexcept {
		return quantity - filled_quantity;
	}

	// Check if order is fully filled
	[[nodiscard]] constexpr bool is_fully_filled() const noexcept {
		return filled_quantity >= quantity;
	}

	// Check if order can match with another order
	[[nodiscard]] constexpr bool can_match_with(const Order& other) const noexcept {
		if (side == other.side)
			return false;

		if (side == Side::Buy) {
			// Buy order matches if our price >= sell price
			return price >= other.price;
		} else {
			// Sell order matches if our price <= buy price
			return price <= other.price;
		}
	}

	// Spaceship operator for price-time priority
	[[nodiscard]] std::partial_ordering operator<=>(const Order& other) const noexcept {
		// First compare by price (reversed for buy orders)
		if (side == Side::Buy) {
			// Higher price has higher priority for buy orders
			auto price_cmp = other.price <=> price;
			if (price_cmp != 0)
				return price_cmp;
		} else {
			// Lower price has higher priority for sell orders
			auto price_cmp = price <=> other.price;
			if (price_cmp != 0)
				return price_cmp;
		}

		// Then compare by timestamp (earlier is better)
		if (timestamp_ns < other.timestamp_ns)
			return std::partial_ordering::less;
		if (timestamp_ns > other.timestamp_ns)
			return std::partial_ordering::greater;
		return std::partial_ordering::equivalent;
	}

	[[nodiscard]] bool operator==(const Order& other) const noexcept {
		return order_id == other.order_id;
	}

 private:
	// Get current timestamp in nanoseconds
	static uint64_t get_timestamp() noexcept {
		using namespace std::chrono;
		return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
};

// Verify size is exactly 64 bytes
static_assert(sizeof(Order) == 64, "Order must be exactly 64 bytes (one cache line)");
static_assert(alignof(Order) == 64, "Order must be 64-byte aligned");

// Trade result structure
struct Trade {
	uint64_t trade_id;
	uint64_t buy_order_id;
	uint64_t sell_order_id;
	double price;
	uint64_t quantity;
	uint64_t timestamp_ns;

	constexpr Trade() noexcept
			: trade_id(0), buy_order_id(0), sell_order_id(0), price(0.0), quantity(0), timestamp_ns(0) {}

	Trade(uint64_t tid, uint64_t bid, uint64_t sid, double p, uint64_t qty) noexcept
			: trade_id(tid),
				buy_order_id(bid),
				sell_order_id(sid),
				price(p),
				quantity(qty),
				timestamp_ns(get_timestamp()) {}

 private:
	static uint64_t get_timestamp() noexcept {
		using namespace std::chrono;
		return duration_cast<std::chrono::nanoseconds>(
							 std::chrono::high_resolution_clock::now().time_since_epoch())
				.count();
	}
};

}	 // namespace orderbook
