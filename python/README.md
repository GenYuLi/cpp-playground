# OrderBook Python Bindings

Python bindings for the high-performance C++20 OrderBook implementation.

## Performance

The C++ orderbook delivers:
- **7.4M orders/sec** throughput
- **135ns** average latency
- **531ns** p99 latency

Python bindings add minimal overhead thanks to pybind11 and GIL-free operations.

## Installation

### Option 1: Build Script (Recommended)

```bash
# From project root
./build.sh

# This will:
# 1. Build the C++ extension with CMake
# 2. Copy the .so to your venv's site-packages
```

### Option 2: Manual CMake Build

```bash
mkdir -p build && cd build
cmake -DPython3_EXECUTABLE=$(which python) ..
make orderbook_py -j4
cp orderbook_py.*.so ../.venv/lib/python*/site-packages/
```

### Option 3: pip install (editable mode)

```bash
# From project root (requires build deps installed)
pip install -e . --no-build-isolation
```

## Quick Start

```python
import orderbook_py as ob

# Create orderbook
book = ob.OrderBook()

# Add orders
result = book.add_order(price=100.0, quantity=50, side=ob.Side.Buy)
result = book.add_order(price=101.0, quantity=30, side=ob.Side.Sell)

# Query market data
print(f"Best bid: {book.get_best_bid_price()}")
print(f"Best ask: {book.get_best_ask_price()}")
print(f"Spread: {book.get_spread()}")

# Add aggressive order (triggers matching)
result = book.add_order(price=101.5, quantity=25, side=ob.Side.Buy)
print(f"Trades: {result.num_trades()}")
for trade in result.trades:
    print(f"  {trade}")
```

## API Reference

### OrderBook Class

```python
book = ob.OrderBook()  # Create empty orderbook
```

#### Order Operations

```python
# Add order with matching
result = book.add_order(
    price=100.0,
    quantity=10,
    side=ob.Side.Buy,  # or ob.Side.Sell
    type=ob.OrderType.Limit  # or ob.OrderType.Market (optional)
)

# Add passive order (no matching)
result = book.add_passive_order(price=100.0, quantity=10, side=ob.Side.Buy)

# Cancel order
success = book.cancel_order(order_id=12345)

# Modify order quantity (loses time priority)
success = book.modify_order(order_id=12345, new_quantity=20)

# Batch operations (efficient!)
orders = [
    {'price': 100.0, 'qty': 10, 'side': 'buy'},
    {'price': 101.0, 'qty': 20, 'side': 'sell'},
]
results = book.add_orders_batch(orders)
```

#### Market Data Queries

```python
# Best prices
best_bid = book.get_best_bid_price()  # Returns float or None
best_ask = book.get_best_ask_price()
spread = book.get_spread()
mid_price = book.get_mid_price()

# Market depth (L2 data)
depth = book.get_market_depth(levels=10)
for level in depth.bids:
    print(f"Bid: ${level.price} x {level.total_quantity} ({level.order_count} orders)")
for level in depth.asks:
    print(f"Ask: ${level.price} x {level.total_quantity} ({level.order_count} orders)")
```

#### Statistics

```python
total = book.total_trades()   # Total number of trades
volume = book.total_volume()  # Total traded volume
book.reset_statistics()       # Reset counters
```

#### Utility

```python
size = book.size()    # Number of orders in book
empty = book.empty()  # True if empty
book.clear()          # Remove all orders
```

### Data Types

#### MatchResult

Returned from `add_order()`:

```python
result.trades            # List[Trade] - executed trades
result.fully_filled      # bool - was order fully filled?
result.remaining_qty     # int - quantity not filled
result.filled_qty        # int - quantity filled
result.has_trades()      # bool - any trades?
result.num_trades()      # int - number of trades
```

#### Trade

```python
trade.trade_id       # int
trade.buy_order_id   # int
trade.sell_order_id  # int
trade.price          # float
trade.quantity       # int
trade.timestamp_ns   # int - nanosecond timestamp
```

#### PriceLevel

```python
level.price           # float
level.total_quantity  # int - aggregated quantity at this price
level.order_count     # int - number of orders
```

#### MarketDepth

```python
depth.bids           # List[PriceLevel] - sorted best to worst
depth.asks           # List[PriceLevel] - sorted best to worst
depth.best_bid_price()  # float or None
depth.best_ask_price()  # float or None
depth.spread()          # float or None
depth.mid_price()       # float or None
```

#### Enums

```python
ob.Side.Buy          # Buy side
ob.Side.Sell         # Sell side

ob.OrderType.Limit   # Limit order
ob.OrderType.Market  # Market order

ob.OrderStatus.New
ob.OrderStatus.PartiallyFilled
ob.OrderStatus.Filled
ob.OrderStatus.Cancelled
```

## Testing

```bash
# Install dev dependencies
uv sync

# Run tests
pytest python/tests -v

# With coverage
pytest python/tests -v --cov=orderbook_py
```

## Performance Tips

### 1. Use Batch Operations

```python
# SLOW - 1M orders takes ~10 seconds
for i in range(1_000_000):
    book.add_order(price=100.0, quantity=1, side=ob.Side.Buy)

# FAST - 1M orders takes ~2 seconds
orders = [{'price': 100.0, 'qty': 1, 'side': 'buy'} for _ in range(1_000_000)]
results = book.add_orders_batch(orders)
```

### 2. GIL Release

These operations release the GIL for true parallelism:
- `add_order()`, `add_passive_order()`, `cancel_order()`, `modify_order()`
- `get_market_depth()`, `add_orders_batch()`, `clear()`

### 3. Cache Market Data

```python
# Instead of multiple calls:
depth = book.get_market_depth(levels=10)
# Use depth.best_bid_price(), depth.spread(), etc.
```

## Benchmark Results

| Metric | Value |
|--------|-------|
| Throughput | 7.4M orders/sec |
| Avg Latency | 135 ns |
| p50 Latency | 181 ns |
| p90 Latency | 291 ns |
| p99 Latency | 531 ns |
| p99.9 Latency | 952 ns |

Python bindings add ~50-100ns overhead per call.

## Troubleshooting

### Import Error

```python
ImportError: No module named 'orderbook_py'
```

**Solution**: Run `./build.sh` to build and install the module.

### Build Fails with ABI Mismatch

```
AssertionError: would build wheel with unsupported tag ('cp313', 'cp312', ...)
```

**Solution**: This happens when Nix Python pollutes your environment. Make sure `.envrc` contains:
```bash
unset PYTHONPATH
unset _PYTHON_SYSCONFIGDATA_NAME
```

### Version Mismatch

```
ImportError: Python version mismatch: module was compiled for Python 3.X
```

**Solution**: Rebuild with the correct Python version:
```bash
rm -rf build && ./build.sh
```

## See Also

- [Main C++ Documentation](../ORDERBOOK_README.md)
- [C++ Examples](../src/bin/orderbook_test.cpp)
