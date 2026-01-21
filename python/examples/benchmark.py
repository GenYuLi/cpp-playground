#!/usr/bin/env python3
"""
Performance benchmark for orderbook_py Python bindings.

Compares:
- Individual order additions vs batch operations
- Different order patterns (all buy, all sell, mixed)
- Matching scenarios
"""

import sys
import time
import random
from typing import List, Tuple

sys.path.insert(0, '../../build')

try:
    import orderbook_py as ob
except ImportError:
    print("ERROR: orderbook_py module not found!")
    print("Build it first with: cd build && make orderbook_py")
    sys.exit(1)


def benchmark_individual_orders(n: int = 10000) -> Tuple[float, int]:
    """Benchmark adding orders one by one."""
    book = ob.OrderBook()

    start = time.perf_counter()

    for i in range(n):
        price = 100.0 + random.uniform(-10, 10)
        qty = random.randint(1, 100)
        side = ob.Side.Buy if i % 2 == 0 else ob.Side.Sell
        book.add_order(price=price, quantity=qty, side=side)

    elapsed = time.perf_counter() - start

    return elapsed, book.total_trades()


def benchmark_batch_orders(n: int = 10000) -> Tuple[float, int]:
    """Benchmark adding orders in batch."""
    book = ob.OrderBook()

    # Prepare orders
    orders = []
    for i in range(n):
        price = 100.0 + random.uniform(-10, 10)
        qty = random.randint(1, 100)
        side = 'buy' if i % 2 == 0 else 'sell'
        orders.append({'price': price, 'qty': qty, 'side': side})

    start = time.perf_counter()
    results = book.add_orders_batch(orders)
    elapsed = time.perf_counter() - start

    return elapsed, book.total_trades()


def benchmark_matching_scenarios(n: int = 10000) -> dict:
    """Benchmark different matching scenarios."""
    results = {}

    # Scenario 1: No matching (wide spread)
    book = ob.OrderBook()
    start = time.perf_counter()
    for i in range(n):
        side = ob.Side.Buy if i < n//2 else ob.Side.Sell
        price = 90.0 if side == ob.Side.Buy else 110.0
        book.add_order(price=price, quantity=10, side=side)
    results['no_match'] = time.perf_counter() - start

    # Scenario 2: Full matching (aggressive orders)
    book = ob.OrderBook()
    # Pre-populate with resting orders
    for i in range(n//2):
        book.add_passive_order(price=100.0, quantity=10, side=ob.Side.Sell)

    start = time.perf_counter()
    for i in range(n//2):
        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
    results['full_match'] = time.perf_counter() - start

    # Scenario 3: Mixed (50% match)
    book = ob.OrderBook()
    start = time.perf_counter()
    for i in range(n):
        if i % 4 == 0:
            # Aggressive order
            book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        elif i % 4 == 1:
            # Resting sell
            book.add_passive_order(price=100.0, quantity=10, side=ob.Side.Sell)
        elif i % 4 == 2:
            # Aggressive order
            book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)
        else:
            # Resting buy
            book.add_passive_order(price=100.0, quantity=10, side=ob.Side.Buy)
    results['mixed'] = time.perf_counter() - start

    return results


def benchmark_market_depth(n_orders: int = 10000, n_queries: int = 1000) -> float:
    """Benchmark market depth queries."""
    book = ob.OrderBook()

    # Populate orderbook
    for i in range(n_orders):
        price = 100.0 + random.uniform(-50, 50)
        qty = random.randint(1, 100)
        side = ob.Side.Buy if i % 2 == 0 else ob.Side.Sell
        book.add_passive_order(price=price, quantity=qty, side=side)

    # Benchmark depth queries
    start = time.perf_counter()
    for _ in range(n_queries):
        depth = book.get_market_depth(levels=10)
    elapsed = time.perf_counter() - start

    return elapsed


def format_throughput(n: int, elapsed: float) -> str:
    """Format throughput as orders/sec."""
    ops_per_sec = n / elapsed if elapsed > 0 else 0
    if ops_per_sec >= 1_000_000:
        return f"{ops_per_sec/1_000_000:.2f}M ops/sec"
    elif ops_per_sec >= 1_000:
        return f"{ops_per_sec/1_000:.2f}K ops/sec"
    else:
        return f"{ops_per_sec:.2f} ops/sec"


def main():
    print("=" * 70)
    print("OrderBook Python Bindings - Performance Benchmark")
    print("=" * 70)
    print()

    # Set random seed for reproducibility
    random.seed(42)

    # Test 1: Individual vs Batch
    print("--- Test 1: Individual vs Batch Operations ---")
    n = 50000

    print(f"Adding {n:,} orders individually...")
    time_individual, trades_individual = benchmark_individual_orders(n)
    print(f"  Time: {time_individual:.3f}s")
    print(f"  Throughput: {format_throughput(n, time_individual)}")
    print(f"  Trades: {trades_individual:,}")
    print()

    print(f"Adding {n:,} orders in batch...")
    time_batch, trades_batch = benchmark_batch_orders(n)
    print(f"  Time: {time_batch:.3f}s")
    print(f"  Throughput: {format_throughput(n, time_batch)}")
    print(f"  Trades: {trades_batch:,}")
    print(f"  Speedup: {time_individual/time_batch:.2f}x faster")
    print()

    # Test 2: Matching scenarios
    print("--- Test 2: Matching Scenarios ---")
    n = 20000
    scenarios = benchmark_matching_scenarios(n)

    print(f"No matching ({n:,} orders):")
    print(f"  Time: {scenarios['no_match']:.3f}s")
    print(f"  Throughput: {format_throughput(n, scenarios['no_match'])}")
    print()

    print(f"Full matching ({n//2:,} matches):")
    print(f"  Time: {scenarios['full_match']:.3f}s")
    print(f"  Throughput: {format_throughput(n//2, scenarios['full_match'])}")
    print()

    print(f"Mixed matching ({n:,} orders):")
    print(f"  Time: {scenarios['mixed']:.3f}s")
    print(f"  Throughput: {format_throughput(n, scenarios['mixed'])}")
    print()

    # Test 3: Market depth queries
    print("--- Test 3: Market Depth Queries ---")
    n_orders = 10000
    n_queries = 1000
    print(f"Orderbook with {n_orders:,} orders, querying depth {n_queries:,} times...")
    time_depth = benchmark_market_depth(n_orders, n_queries)
    print(f"  Time: {time_depth:.3f}s")
    print(f"  Per query: {time_depth/n_queries*1000:.2f}ms")
    print(f"  Throughput: {format_throughput(n_queries, time_depth)}")
    print()

    print("=" * 70)
    print("Benchmark complete!")
    print()
    print("Note: These are Python binding overheads + C++ execution.")
    print("Pure C++ benchmarks show ~7.4M orders/sec throughput.")
    print("=" * 70)


if __name__ == '__main__':
    main()
