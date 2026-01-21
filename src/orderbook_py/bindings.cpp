#include <fmt/format.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../orderbook/orderbook.hpp"

namespace py = pybind11;
using namespace orderbook;

// Helper to convert Side enum to/from string
std::string side_to_string(Side side) {
	return (side == Side::Buy) ? "buy" : "sell";
}

Side string_to_side(const std::string& s) {
	if (s == "buy" || s == "BUY" || s == "Buy") {
		return Side::Buy;
	} else if (s == "sell" || s == "SELL" || s == "Sell") {
		return Side::Sell;
	}
	throw std::invalid_argument("Invalid side: " + s + " (must be 'buy' or 'sell')");
}

PYBIND11_MODULE(orderbook_py, m) {
	m.doc() = "Low-latency orderbook implementation with C++20";

	// Enums
	py::enum_<Side>(m, "Side").value("Buy", Side::Buy).value("Sell", Side::Sell).export_values();

	py::enum_<OrderType>(m, "OrderType")
			.value("Limit", OrderType::Limit)
			.value("Market", OrderType::Market)
			.export_values();

	py::enum_<OrderStatus>(m, "OrderStatus")
			.value("New", OrderStatus::New)
			.value("PartiallyFilled", OrderStatus::PartiallyFilled)
			.value("Filled", OrderStatus::Filled)
			.value("Cancelled", OrderStatus::Cancelled)
			.export_values();

	// Order class
	py::class_<Order>(m, "Order")
			.def(py::init<>())
			.def(py::init<uint64_t, double, uint64_t, Side, OrderType>(), py::arg("order_id"),
					 py::arg("price"), py::arg("quantity"), py::arg("side"),
					 py::arg("type") = OrderType::Limit)
			.def_readonly("order_id", &Order::order_id)
			.def_readonly("timestamp_ns", &Order::timestamp_ns)
			.def_readonly("price", &Order::price)
			.def_readonly("quantity", &Order::quantity)
			.def_readonly("filled_quantity", &Order::filled_quantity)
			.def_readonly("side", &Order::side)
			.def_readonly("type", &Order::type)
			.def_readonly("status", &Order::status)
			.def("remaining_quantity", &Order::remaining_quantity)
			.def("is_fully_filled", &Order::is_fully_filled)
			.def("can_match_with", &Order::can_match_with)
			.def("__repr__", [](const Order& o) {
				return fmt::format("Order(id={}, price={}, qty={}/{}, side={}, status={})", o.order_id,
													 o.price, o.filled_quantity, o.quantity, side_to_string(o.side),
													 static_cast<int>(o.status));
			});

	// Trade class
	py::class_<Trade>(m, "Trade")
			.def(py::init<>())
			.def_readonly("trade_id", &Trade::trade_id)
			.def_readonly("buy_order_id", &Trade::buy_order_id)
			.def_readonly("sell_order_id", &Trade::sell_order_id)
			.def_readonly("price", &Trade::price)
			.def_readonly("quantity", &Trade::quantity)
			.def_readonly("timestamp_ns", &Trade::timestamp_ns)
			.def("__repr__", [](const Trade& t) {
				return fmt::format("Trade(id={}, price={}, qty={}, buy={}, sell={})", t.trade_id, t.price,
													 t.quantity, t.buy_order_id, t.sell_order_id);
			});

	// MatchResult class
	py::class_<MatchResult>(m, "MatchResult")
			.def(py::init<>())
			.def_readonly("trades", &MatchResult::trades)
			.def_readonly("fully_filled", &MatchResult::fully_filled)
			.def_readonly("remaining_qty", &MatchResult::remaining_qty)
			.def_readonly("filled_qty", &MatchResult::filled_qty)
			.def("has_trades", &MatchResult::has_trades)
			.def("num_trades", &MatchResult::num_trades)
			.def("__repr__", [](const MatchResult& r) {
				return fmt::format("MatchResult(filled={}/{}, trades={}, fully_filled={})", r.filled_qty,
													 r.filled_qty + r.remaining_qty, r.num_trades(), r.fully_filled);
			});

	// PriceLevel class
	py::class_<PriceLevel>(m, "PriceLevel")
			.def(py::init<>())
			.def(py::init<double, uint64_t, size_t>())
			.def_readonly("price", &PriceLevel::price)
			.def_readonly("total_quantity", &PriceLevel::total_quantity)
			.def_readonly("order_count", &PriceLevel::order_count)
			.def("__repr__", [](const PriceLevel& p) {
				return fmt::format("PriceLevel(price={}, qty={}, orders={})", p.price, p.total_quantity,
													 p.order_count);
			});

	// MarketDepth class
	py::class_<MarketDepth>(m, "MarketDepth")
			.def(py::init<>())
			.def_readonly("bids", &MarketDepth::bids)
			.def_readonly("asks", &MarketDepth::asks)
			.def("best_bid_price", &MarketDepth::best_bid_price)
			.def("best_ask_price", &MarketDepth::best_ask_price)
			.def("spread", &MarketDepth::spread)
			.def("mid_price", &MarketDepth::mid_price)
			.def("__repr__", [](const MarketDepth& d) {
				return fmt::format("MarketDepth(bids={}, asks={}, spread={})", d.bids.size(), d.asks.size(),
													 d.spread().value_or(0.0));
			});

	// OrderBook class (IntrusiveOrderBook variant)
	py::class_<IntrusiveOrderBook>(m, "OrderBook")
			.def(py::init<>())
			// Basic operations
			.def("add_order",
					 py::overload_cast<double, uint64_t, Side, OrderType>(&IntrusiveOrderBook::add_order),
					 py::arg("price"), py::arg("quantity"), py::arg("side"),
					 py::arg("type") = OrderType::Limit, py::call_guard<py::gil_scoped_release>(),
					 "Add a new order with matching")
			.def("add_passive_order", &IntrusiveOrderBook::add_passive_order, py::arg("price"),
					 py::arg("quantity"), py::arg("side"), py::call_guard<py::gil_scoped_release>(),
					 "Add order without matching (passive)")
			.def("cancel_order", &IntrusiveOrderBook::cancel_order, py::arg("order_id"),
					 py::call_guard<py::gil_scoped_release>(), "Cancel an order by ID")
			.def("modify_order", &IntrusiveOrderBook::modify_order, py::arg("order_id"),
					 py::arg("new_quantity"), py::call_guard<py::gil_scoped_release>(),
					 "Modify order quantity (loses time priority)")
			// Market data queries
			.def("get_best_bid_price", &IntrusiveOrderBook::get_best_bid_price, "Get best bid price")
			.def("get_best_ask_price", &IntrusiveOrderBook::get_best_ask_price, "Get best ask price")
			.def("get_spread", &IntrusiveOrderBook::get_spread, "Get bid-ask spread")
			.def("get_mid_price", &IntrusiveOrderBook::get_mid_price, "Get mid price")
			.def("get_market_depth", &IntrusiveOrderBook::get_market_depth, py::arg("levels") = 10,
					 py::call_guard<py::gil_scoped_release>(), "Get L2 market depth snapshot")
			// Batch operations
			.def(
					"add_orders_batch",
					[](IntrusiveOrderBook& book, const std::vector<py::dict>& orders) {
						std::vector<Order> order_vec;
						order_vec.reserve(orders.size());

						for (const auto& order_dict : orders) {
							double price = order_dict["price"].cast<double>();
							uint64_t qty = order_dict["qty"].cast<uint64_t>();
							std::string side_str = order_dict["side"].cast<std::string>();
							Side side = string_to_side(side_str);

							OrderType type = OrderType::Limit;
							if (order_dict.contains("type")) {
								std::string type_str = order_dict["type"].cast<std::string>();
								if (type_str == "market" || type_str == "Market") {
									type = OrderType::Market;
								}
							}

							// Generate order ID internally
							Order order(0, price, qty, side, type);
							order_vec.push_back(order);
						}

						// Release GIL for batch processing
						py::gil_scoped_release release;
						return book.add_orders_batch(order_vec);
					},
					py::arg("orders"), "Add multiple orders in batch (efficient)")
			// Utility
			.def("clear", &IntrusiveOrderBook::clear, py::call_guard<py::gil_scoped_release>(),
					 "Clear all orders")
			.def("size", &IntrusiveOrderBook::size, "Get total number of orders")
			.def("empty", &IntrusiveOrderBook::empty, "Check if orderbook is empty")
			// Statistics
			.def("total_trades", &IntrusiveOrderBook::total_trades, "Get total number of trades executed")
			.def("total_volume", &IntrusiveOrderBook::total_volume, "Get total trading volume")
			.def("reset_statistics", &IntrusiveOrderBook::reset_statistics, "Reset trade statistics")
			.def("__repr__", [](const IntrusiveOrderBook& book) {
				return fmt::format("OrderBook(orders={}, trades={}, volume={})", book.size(),
													 book.total_trades(), book.total_volume());
			});

	// Version info
	m.attr("__version__") = "1.0.0";

	// Convenience function to create orders from dict
	m.def(
			"create_order",
			[](const py::dict& order_dict) {
				double price = order_dict["price"].cast<double>();
				uint64_t qty = order_dict["qty"].cast<uint64_t>();
				std::string side_str = order_dict["side"].cast<std::string>();
				Side side = string_to_side(side_str);

				uint64_t order_id =
						order_dict.contains("order_id") ? order_dict["order_id"].cast<uint64_t>() : 0;

				OrderType type = OrderType::Limit;
				if (order_dict.contains("type")) {
					std::string type_str = order_dict["type"].cast<std::string>();
					if (type_str == "market" || type_str == "Market") {
						type = OrderType::Market;
					}
				}

				return Order(order_id, price, qty, side, type);
			},
			py::arg("order_dict"), "Create an Order from a dictionary");
}
