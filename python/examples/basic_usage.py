#!/usr/bin/env python3
"""
Basic usage example for the C++ OrderBook Python bindings.

This demonstrates:
- Adding orders (with automatic matching)
- Querying market data (best bid/ask, spread)
- Batch operations for efficiency
- Market depth snapshots
"""

import sys
sys.path.insert(0, '../../build')  # Adjust if your build directory differs

try:
    import orderbook_py as ob
except ImportError:
    print("ERROR: orderbook_py module not found!")
    print("Build it first with: cd build && make orderbook_py")
    sys.exit(1)


def main():
    print("=" * 60)
    print("OrderBook Python Bindings - Basic Usage Example")
    print("=" * 60)
    print()

    # Create orderbook
    book = ob.OrderBook()
    print(f"Created empty orderbook: {book}")
    print()

    # Example 1: Add limit orders
    print("--- Example 1: Adding Limit Orders ---")
    result1 = book.add_order(price=100.0, quantity=50, side=ob.Side.Buy)
    print(f"Added buy order @ 100.0: {result1}")

    result2 = book.add_order(price=101.0, quantity=30, side=ob.Side.Sell)
    print(f"Added sell order @ 101.0: {result2}")

    result3 = book.add_order(price=99.5, quantity=20, side=ob.Side.Buy)
    print(f"Added buy order @ 99.5: {result3}")
    print()

    # Example 2: Query market data
    print("--- Example 2: Market Data Queries ---")
    best_bid = book.get_best_bid_price()
    best_ask = book.get_best_ask_price()
    spread = book.get_spread()
    mid_price = book.get_mid_price()

    print(f"Best Bid: {best_bid:.2f}")
    print(f"Best Ask: {best_ask:.2f}")
    print(f"Spread: {spread:.2f}")
    print(f"Mid Price: {mid_price:.2f}")
    print()

    # Example 3: Aggressive order that matches
    print("--- Example 3: Aggressive Order (Matching) ---")
    result = book.add_order(price=101.5, quantity=25, side=ob.Side.Buy)
    print(f"Match result: {result}")
    print(f"Trades executed: {result.num_trades()}")

    for i, trade in enumerate(result.trades):
        print(f"  Trade {i+1}: {trade}")
    print()

    # Example 4: Batch operations (efficient for many orders)
    print("--- Example 4: Batch Operations ---")
    orders = [
        {'price': 98.0, 'qty': 100, 'side': 'buy'},
        {'price': 99.0, 'qty': 150, 'side': 'buy'},
        {'price': 102.0, 'qty': 80, 'side': 'sell'},
        {'price': 103.0, 'qty': 120, 'side': 'sell'},
    ]

    print(f"Adding {len(orders)} orders in batch...")
    results = book.add_orders_batch(orders)
    print(f"Batch complete: {len(results)} results")

    # Count how many had matches
    matched = sum(1 for r in results if r.has_trades())
    print(f"Orders that matched: {matched}/{len(results)}")
    print()

    # Example 5: Market depth (L2 data)
    print("--- Example 5: Market Depth (L2) ---")
    depth = book.get_market_depth(levels=5)
    print(f"Market depth: {depth}")
    print()

    print("Bids (top 5):")
    for i, level in enumerate(depth.bids[:5]):
        print(f"  {i+1}. {level}")

    print("\nAsks (top 5):")
    for i, level in enumerate(depth.asks[:5]):
        print(f"  {i+1}. {level}")
    print()

    # Example 6: Order cancellation
    print("--- Example 6: Order Cancellation ---")
    result = book.add_order(price=97.0, quantity=200, side=ob.Side.Buy)
    print(f"Added order: {result}")

    # In real usage, you'd track order IDs returned from add_order
    # For demo purposes, we'll just show the API
    # success = book.cancel_order(order_id=12345)
    # print(f"Cancellation: {'Success' if success else 'Failed'}")
    print("(Order cancellation requires tracking order IDs from add_order results)")
    print()

    # Final statistics
    print("--- Final Statistics ---")
    print(f"Total orders in book: {book.size()}")
    print(f"Total trades executed: {book.total_trades()}")
    print(f"Total volume traded: {book.total_volume()}")
    print(f"Final orderbook state: {book}")
    print()

    print("=" * 60)
    print("Example complete! Check out tests/test_orderbook.py for more.")
    print("=" * 60)


if __name__ == '__main__':
    main()
