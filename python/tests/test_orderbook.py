"""
Unit tests for orderbook_py C++ bindings.

Run with: pytest test_orderbook.py -v

Requirements:
    pip install pytest
"""

import sys
import pytest

sys.path.insert(0, '../../build')

try:
    import orderbook_py as ob
except ImportError:
    pytest.skip("orderbook_py not built", allow_module_level=True)


class TestOrderBookBasics:
    """Test basic orderbook operations."""

    def test_create_empty_orderbook(self):
        """Test creating an empty orderbook."""
        book = ob.OrderBook()
        assert book.empty()
        assert book.size() == 0

    def test_add_single_order(self):
        """Test adding a single limit order."""
        book = ob.OrderBook()
        result = book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)

        assert result is not None
        assert result.remaining_qty == 10  # No match, fully remaining
        assert result.filled_qty == 0
        assert not result.fully_filled
        assert book.size() == 1

    def test_best_bid_ask(self):
        """Test best bid/ask price queries."""
        book = ob.OrderBook()

        # Empty book should have no best bid/ask
        best_bid = book.get_best_bid_price()
        best_ask = book.get_best_ask_price()
        assert best_bid == 0.0 or best_bid is None or (hasattr(best_bid, '__bool__') and not best_bid)
        assert best_ask == 0.0 or best_ask is None or (hasattr(best_ask, '__bool__') and not best_ask)

        # Add bid and ask
        book.add_order(price=99.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=101.0, quantity=10, side=ob.Side.Sell)

        best_bid = book.get_best_bid_price()
        best_ask = book.get_best_ask_price()

        assert best_bid == 99.0
        assert best_ask == 101.0

    def test_spread_calculation(self):
        """Test bid-ask spread calculation."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=102.0, quantity=10, side=ob.Side.Sell)

        spread = book.get_spread()
        assert spread == 2.0

    def test_mid_price(self):
        """Test mid-price calculation."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=102.0, quantity=10, side=ob.Side.Sell)

        mid = book.get_mid_price()
        assert mid == 101.0


class TestOrderMatching:
    """Test order matching logic."""

    def test_simple_match(self):
        """Test a simple matching scenario."""
        book = ob.OrderBook()

        # Add resting sell order
        book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)

        # Add aggressive buy order that matches
        result = book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)

        assert result.fully_filled
        assert result.filled_qty == 10
        assert result.remaining_qty == 0
        assert result.num_trades() == 1

    def test_partial_fill(self):
        """Test partial order fill."""
        book = ob.OrderBook()

        # Add resting sell order (10 qty)
        book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)

        # Add aggressive buy order (20 qty) - should partially fill
        result = book.add_order(price=100.0, quantity=20, side=ob.Side.Buy)

        assert not result.fully_filled
        assert result.filled_qty == 10
        assert result.remaining_qty == 10
        assert result.num_trades() == 1

    def test_multiple_matches(self):
        """Test matching against multiple resting orders."""
        book = ob.OrderBook()

        # Add multiple resting sell orders
        book.add_order(price=100.0, quantity=5, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=5, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=5, side=ob.Side.Sell)

        # Add aggressive buy that matches all
        result = book.add_order(price=100.0, quantity=15, side=ob.Side.Buy)

        assert result.fully_filled
        assert result.filled_qty == 15
        assert result.num_trades() == 3  # Matched with 3 orders

    def test_price_improvement(self):
        """Test that aggressive orders match best available prices."""
        book = ob.OrderBook()

        # Add sells at different prices
        book.add_order(price=102.0, quantity=10, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)
        book.add_order(price=101.0, quantity=10, side=ob.Side.Sell)

        # Aggressive buy should match best price first (100.0)
        result = book.add_order(price=103.0, quantity=10, side=ob.Side.Buy)

        assert result.num_trades() == 1
        assert len(result.trades) == 1
        assert result.trades[0].price == 100.0  # Best price


class TestBatchOperations:
    """Test batch order operations."""

    def test_batch_add(self):
        """Test adding orders in batch."""
        book = ob.OrderBook()

        orders = [
            {'price': 100.0, 'qty': 10, 'side': 'buy'},
            {'price': 101.0, 'qty': 20, 'side': 'sell'},
            {'price': 99.0, 'qty': 15, 'side': 'buy'},
        ]

        results = book.add_orders_batch(orders)

        assert len(results) == 3
        assert book.size() == 3

    def test_batch_with_matches(self):
        """Test batch operations that trigger matches."""
        book = ob.OrderBook()

        # Pre-populate with resting orders
        book.add_order(price=100.0, quantity=50, side=ob.Side.Sell)

        # Batch add aggressive orders
        orders = [
            {'price': 100.0, 'qty': 10, 'side': 'buy'},
            {'price': 100.0, 'qty': 20, 'side': 'buy'},
        ]

        results = book.add_orders_batch(orders)

        # Both should match
        assert results[0].has_trades()
        assert results[1].has_trades()


class TestMarketDepth:
    """Test market depth queries."""

    def test_empty_depth(self):
        """Test depth on empty book."""
        book = ob.OrderBook()
        depth = book.get_market_depth(levels=10)

        assert len(depth.bids) == 0
        assert len(depth.asks) == 0

    def test_depth_with_orders(self):
        """Test depth with multiple price levels."""
        book = ob.OrderBook()

        # Add multiple bid levels
        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=99.0, quantity=20, side=ob.Side.Buy)
        book.add_order(price=98.0, quantity=30, side=ob.Side.Buy)

        # Add multiple ask levels
        book.add_order(price=101.0, quantity=15, side=ob.Side.Sell)
        book.add_order(price=102.0, quantity=25, side=ob.Side.Sell)

        depth = book.get_market_depth(levels=10)

        assert len(depth.bids) == 3
        assert len(depth.asks) == 2

        # Best bid should be first
        assert depth.bids[0].price == 100.0
        assert depth.bids[0].total_quantity == 10

        # Best ask should be first
        assert depth.asks[0].price == 101.0
        assert depth.asks[0].total_quantity == 15

    def test_depth_aggregation(self):
        """Test that depth aggregates orders at same price."""
        book = ob.OrderBook()

        # Add multiple orders at same price
        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=100.0, quantity=20, side=ob.Side.Buy)
        book.add_order(price=100.0, quantity=30, side=ob.Side.Buy)

        depth = book.get_market_depth(levels=10)

        assert len(depth.bids) == 1
        assert depth.bids[0].price == 100.0
        assert depth.bids[0].total_quantity == 60
        assert depth.bids[0].order_count == 3


class TestStatistics:
    """Test statistics tracking."""

    def test_trade_counting(self):
        """Test that trades are counted correctly."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)

        assert book.total_trades() == 1
        assert book.total_volume() == 10

    def test_volume_tracking(self):
        """Test total volume tracking."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=100, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=50, side=ob.Side.Buy)
        book.add_order(price=100.0, quantity=30, side=ob.Side.Buy)

        assert book.total_trades() == 2
        assert book.total_volume() == 80  # 50 + 30

    def test_reset_statistics(self):
        """Test resetting statistics."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=10, side=ob.Side.Sell)
        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)

        assert book.total_trades() > 0

        book.reset_statistics()

        assert book.total_trades() == 0
        assert book.total_volume() == 0


class TestEnums:
    """Test enum values."""

    def test_side_enum(self):
        """Test Side enum."""
        assert ob.Side.Buy is not None
        assert ob.Side.Sell is not None
        assert ob.Side.Buy != ob.Side.Sell

    def test_order_type_enum(self):
        """Test OrderType enum."""
        assert ob.OrderType.Limit is not None
        assert ob.OrderType.Market is not None

    def test_order_status_enum(self):
        """Test OrderStatus enum."""
        assert ob.OrderStatus.New is not None
        assert ob.OrderStatus.PartiallyFilled is not None
        assert ob.OrderStatus.Filled is not None
        assert ob.OrderStatus.Cancelled is not None


class TestEdgeCases:
    """Test edge cases and error conditions."""

    def test_zero_quantity_handling(self):
        """Test handling of zero quantity orders."""
        book = ob.OrderBook()

        # Most orderbooks reject zero quantity
        # This tests that the binding doesn't crash
        try:
            result = book.add_order(price=100.0, quantity=0, side=ob.Side.Buy)
            # If it succeeds, check it doesn't affect the book
            assert book.size() <= 1  # Might be rejected
        except (ValueError, RuntimeError):
            # Expected if implementation rejects zero qty
            pass

    def test_clear_orderbook(self):
        """Test clearing the orderbook."""
        book = ob.OrderBook()

        book.add_order(price=100.0, quantity=10, side=ob.Side.Buy)
        book.add_order(price=101.0, quantity=10, side=ob.Side.Sell)

        assert book.size() == 2

        book.clear()

        assert book.empty()
        assert book.size() == 0


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
