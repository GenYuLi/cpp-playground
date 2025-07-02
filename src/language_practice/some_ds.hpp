#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <utility>

// --------------------------------------------------------------
// Utility traits & comparator
// --------------------------------------------------------------

template <typename T>
struct Less {
  constexpr bool operator()(const T &a, const T &b) const noexcept {
    return a < b;
  }
};

#ifdef __GNUC__
#define QI_LIKELY(x) (__builtin_expect(!!(x), 1))
#define QI_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define QI_LIKELY(x) (x)
#define QI_UNLIKELY(x) (x)
#endif

// --------------------------------------------------------------
// 1. Fixed‑capacity binary heap (single‑thread)
// --------------------------------------------------------------

/**
 * @tparam Key       priority key (ordered by Comp)
 * @tparam Value     payload stored alongside key
 * @tparam Capacity  maximum number of elements (compile‑time fixed)
 * @tparam Comp      strict weak ordering for Key (default: Less)
 */

template <typename Key, typename Value, std::size_t Capacity,
          typename Comp = Less<Key>>
class FixedBinaryHeap {
  static_assert(Capacity > 0, "Capacity must be positive");

  struct Node {
    Key key;
    Value val;
  };

  // index 1‑based to avoid (i‑1)/2 divisions
  std::array<Node, Capacity + 1> buf_{};
  std::size_t sz_ = 0;
  Comp comp_{};

  // -------- helper ----------
  inline void sift_up(std::size_t idx) {
    while (idx > 1) {
      std::size_t parent = idx >> 1;
      if (QI_LIKELY(!comp_(buf_[idx].key, buf_[parent].key))) break;
      std::swap(buf_[idx], buf_[parent]);
      idx = parent;
    }
  }

  inline void sift_down(std::size_t idx) {
    for (;;) {
      std::size_t left = idx << 1;
      std::size_t right = left + 1;
      if (left > sz_) return;
      std::size_t best =
          (right <= sz_ && comp_(buf_[right].key, buf_[left].key)) ? right
                                                                   : left;
      if (!comp_(buf_[best].key, buf_[idx].key)) return;
      std::swap(buf_[idx], buf_[best]);
      idx = best;
    }
  }

 public:
  static constexpr std::size_t capacity = Capacity;

  [[nodiscard]] bool empty() const noexcept { return sz_ == 0; }
  [[nodiscard]] std::size_t size() const noexcept { return sz_; }

  /** push – returns false on overflow (asserts in NDEBUG off). */
  bool push(const Key &k, const Value &v) {
    if (QI_UNLIKELY(sz_ >= Capacity)) {
      assert(false && "FixedBinaryHeap overflow");
      return false;
    }
    buf_[++sz_] = Node{k, v};
    sift_up(sz_);
    return true;
  }

  /** pop – extracts min‑key element, writes to out. Returns false if empty. */
  bool pop(Value &out) {
    if (QI_UNLIKELY(sz_ == 0)) return false;
    out = std::move(buf_[1].val);
    buf_[1] = std::move(buf_[sz_--]);
    if (sz_) sift_down(1);
    return true;
  }

  /** top – peek without removal. */
  [[nodiscard]] const Value &top() const {
    assert(sz_ && "top() on empty heap");
    return buf_[1].val;
  }
};

// --------------------------------------------------------------
// 2. Concurrent binary heap (spin‑lock wrapper)
//     – suitable for low‑contention writer/reader threads. For
//       very high contention, prefer sharded heaps or lock‑free
//       skiplist‑based priority queues.
// --------------------------------------------------------------

template <typename Key, typename Value, std::size_t Capacity,
          typename Comp = Less<Key>>
class ConcurrentBinaryHeap {
  FixedBinaryHeap<Key, Value, Capacity, Comp> heap_;
  mutable std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

  struct ScopedLock {
    std::atomic_flag &f;
    ScopedLock(std::atomic_flag &fl) : f(fl) {
      while (f.test_and_set(std::memory_order_acquire));
    }
    ~ScopedLock() { f.clear(std::memory_order_release); }
  };

 public:
  bool push(const Key &k, const Value &v) {
    ScopedLock g(lock_);
    return heap_.push(k, v);
  }

  bool pop(Value &out) {
    ScopedLock g(lock_);
    return heap_.pop(out);
  }

  [[nodiscard]] bool empty() const {
    ScopedLock g(lock_);
    return heap_.empty();
  }

  [[nodiscard]] std::size_t size() const {
    ScopedLock g(lock_);
    return heap_.size();
  }
};

// --------------------------------------------------------------
// 3. Fixed‑capacity iterative segment tree
//    * Range query in O(log N) using associative Op.
//    * No lazy propagation (add if necessary).
// --------------------------------------------------------------

template <typename T, std::size_t N, typename Op = std::plus<>,
          T Identity = T{}>
class FixedSegmentTree {
  static_assert(N > 0 && (N & (N - 1)) == 0,
                "N must be a (compile‑time) power of two for simplicity");

  std::array<T, 2 * N> tree_{};
  Op op_{};

 public:
  constexpr FixedSegmentTree() {
    for (auto &e : tree_) e = Identity;
  }

  /** point update */
  void set(std::size_t idx, const T &v) {
    assert(idx < N);
    idx += N;
    tree_[idx] = v;
    for (idx >>= 1; idx; idx >>= 1)
      tree_[idx] = op_(tree_[idx << 1], tree_[idx << 1 | 1]);
  }

  /** range query [l, r] inclusive */
  [[nodiscard]] T query(std::size_t l, std::size_t r) const {
    assert(l <= r && r < N);
    l += N;
    r += N;
    T res_left = Identity;
    T res_right = Identity;
    while (l <= r) {
      if (l & 1) res_left = op_(res_left, tree_[l++]);
      if (!(r & 1)) res_right = op_(tree_[r--], res_right);
      l >>= 1;
      r >>= 1;
    }
    return op_(res_left, res_right);
  }
};

// -----------------------------------------------------------------------------
// 4. Lazy‑prop segment tree – range‑add / range‑sum (extendable)
// -----------------------------------------------------------------------------

template <typename T, std::size_t N>
class FixedLazySegmentTree {
  static_assert(N > 0 && (N & (N - 1)) == 0, "N must be power‑of‑two");

  // Node values (segment sums) & lazy pending additions
  std::array<T, 2 * N> seg_{};
  std::array<T, 2 * N> lazy_{};

  // ---------------- helpers ----------------
  inline void apply(std::size_t node, std::size_t len, T add) {
    seg_[node] += add * static_cast<T>(len);
    lazy_[node] += add;
  }

  inline void push(std::size_t node, std::size_t len) {
    if (lazy_[node] != T{}) {
      std::size_t half = len >> 1;
      apply(node << 1, half, lazy_[node]);
      apply(node << 1 | 1, half, lazy_[node]);
      lazy_[node] = T{};
    }
  }

  // range add
  void update(std::size_t node, std::size_t l, std::size_t r, std::size_t ql,
              std::size_t qr, T val) {
    if (ql <= l && r <= qr) {
      apply(node, r - l + 1, val);
      return;
    }
    push(node, r - l + 1);
    std::size_t mid = (l + r) >> 1;
    if (ql <= mid) update(node << 1, l, mid, ql, qr, val);
    if (qr > mid) update(node << 1 | 1, mid + 1, r, ql, qr, val);
    seg_[node] = seg_[node << 1] + seg_[node << 1 | 1];
  }

  // range query
  T query_impl(std::size_t node, std::size_t l, std::size_t r, std::size_t ql,
               std::size_t qr) {
    if (ql <= l && r <= qr) return seg_[node];
    push(node, r - l + 1);
    std::size_t mid = (l + r) >> 1;
    T res = 0;
    if (ql <= mid) res += query_impl(node << 1, l, mid, ql, qr);
    if (qr > mid) res += query_impl(node << 1 | 1, mid + 1, r, ql, qr);
    return res;
  }

 public:
  constexpr FixedLazySegmentTree() {
    seg_.fill(0);
    lazy_.fill(0);
  }

  /** Range add: add `val` to every element in [l, r] inclusive. */
  void range_add(std::size_t l, std::size_t r, T val) {
    assert(l <= r && r < N);
    update(1, 0, N - 1, l, r, val);
  }

  /** Range sum query over [l, r] inclusive. */
  [[nodiscard]] T range_sum(std::size_t l, std::size_t r) {
    assert(l <= r && r < N);
    return query_impl(1, 0, N - 1, l, r);
  }
};

// -----------------------------------------------------------------------------
// 5. Fenwick Tree / Binary Indexed Tree – prefix & range sums
// -----------------------------------------------------------------------------

template <typename T, std::size_t N>
class FixedFenwickTree {
  static_assert(N > 0, "N must be positive");

  std::array<T, N + 1> bit_{};  // 1‑based indexing

 public:
  constexpr FixedFenwickTree() { bit_.fill(0); }

  /** add `delta` to element at index `idx` (0‑based). */
  void add(std::size_t idx, T delta) {
    assert(idx < N);
    for (++idx; idx <= N; idx += idx & -idx) bit_[idx] += delta;
  }

  /** prefix sum [0, idx] inclusive. */
  [[nodiscard]] T prefix_sum(std::size_t idx) const {
    assert(idx < N);
    T res = 0;
    for (++idx; idx; idx -= idx & -idx) res += bit_[idx];
    return res;
  }

  /** range sum [l, r] inclusive. */
  [[nodiscard]] T range_sum(std::size_t l, std::size_t r) const {
    assert(l <= r && r < N);
    return prefix_sum(r) - (l ? prefix_sum(l - 1) : 0);
  }
};
