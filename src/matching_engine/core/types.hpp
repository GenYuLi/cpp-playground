#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <optional>

namespace matching_engine {

// Strong type wrappers for type safety
struct OrderId {
	uint64_t value;

	auto operator<=>(const OrderId&) const = default;
};

struct Timestamp {
	uint64_t nanoseconds;

	static Timestamp now() noexcept;

	auto operator<=>(const Timestamp&) const = default;
};

// Fixed-point price representation (avoids floating-point precision issues)
struct Price {
	int64_t ticks;														 // Price in minimum tick units
	static constexpr int64_t TICK_SIZE = 100;	 // 0.01 = 1 cent

	static Price from_double(double price) noexcept {
		return Price{static_cast<int64_t>(price * TICK_SIZE)};
	}

	double to_double() const noexcept { return static_cast<double>(ticks) / TICK_SIZE; }

	auto operator<=>(const Price&) const = default;
};

struct Quantity {
	uint64_t value;

	auto operator<=>(const Quantity&) const = default;

	Quantity& operator+=(const Quantity& other) {
		value += other.value;
		return *this;
	}

	Quantity& operator-=(const Quantity& other) {
		value -= other.value;
		return *this;
	}

	Quantity operator+(const Quantity& other) const { return Quantity{value + other.value}; }

	Quantity operator-(const Quantity& other) const { return Quantity{value - other.value}; }
};

// Order side
enum class Side : uint8_t { Buy, Sell };

// Order type
enum class OrderType : uint8_t { Limit, Market };

// Order event type
enum class OrderEventType : uint8_t {
	New,		 // New order submission
	Cancel,	 // Order cancellation
	Fill,		 // Order fill (full or partial)
	Reject	 // Order rejection
};

// Fill information
struct FillInfo {
	Quantity filled_quantity{0};
	Quantity remaining_quantity{0};
	Price fill_price{0};
	Timestamp fill_time{0};
};

// Order event (used for both input and output)
struct OrderEvent {
	OrderEventType type;
	OrderId order_id{0};
	Price price{0};
	Quantity quantity{0};
	Side side{Side::Buy};
	OrderType order_type{OrderType::Limit};
	Timestamp timestamp{0};

	// For fill events
	FillInfo fill_info;

	// For rejection
	std::optional<const char*> reject_reason;
};

// Result wrapper for operations
template <typename T = void>
class Result {
	bool success_;
	T value_;

 public:
	Result(bool success, T value = T{}) : success_(success), value_(std::move(value)) {}

	bool is_ok() const { return success_; }
	bool is_err() const { return !success_; }

	const T& value() const { return value_; }
	T& value() { return value_; }

	static Result Ok(T value = T{}) { return Result(true, std::move(value)); }

	static Result Err() { return Result(false); }
};

template <>
class Result<void> {
	bool success_;

 public:
	explicit Result(bool success) : success_(success) {}

	bool is_ok() const { return success_; }
	bool is_err() const { return !success_; }

	static Result Ok() { return Result(true); }

	static Result Err() { return Result(false); }
};

}	 // namespace matching_engine
