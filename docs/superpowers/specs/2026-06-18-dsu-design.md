# DSU (Disjoint Set Union) — Design Spec

**Date:** 2026-06-18
**Component:** `projects/ds` data-structure playground
**Status:** Approved, ready for implementation plan

## Goal

Add two disjoint-set-union variants to the `ds` playground:

1. **`ds::DSU`** — a standard single-threaded DSU, left as a **skeleton** (suggested API
   comments + skipped tests), matching the playground's "implement it yourself" style.
2. **`ds::ConcurrentDSU`** — a **fully implemented** lock-free concurrent DSU
   (atomic `parent` + immutable random priority + CAS), with passing tests including a
   multi-threaded stress test.

The concurrent version is fully written because its correctness is subtle (the
immutable-priority invariant that prevents cycles) and not suitable for fill-in-the-blank.

## File Layout

Follows the existing `include/ds/<name>/<name>.hpp` + `tests/<name>_test.cpp` convention.

```
projects/ds/include/ds/dsu/dsu.hpp              ← sequential, skeleton
projects/ds/include/ds/dsu/concurrent_dsu.hpp   ← concurrent, full implementation
projects/ds/tests/dsu_test.cpp                  ← seq: skipped skeleton tests
projects/ds/tests/concurrent_dsu_test.cpp       ← concurrent: passing (functional + stress)
```

Plus: add both headers to the `ds.hpp` umbrella include.

## Component 1: `ds::DSU` (sequential, skeleton)

Same style as `ds/trie/trie.hpp`: `#pragma once`, suggested-API comments, `// TODO`,
implementation left empty. Suggested API (aligned with the concurrent version for easy
side-by-side comparison):

```cpp
explicit DSU(int n);
int  find(int x);              // union by size + path compression
bool unite(int a, int b);      // true = this call actually merged two sets
bool connected(int a, int b);
```

The comment should suggest **union by size** (not rank) and path compression.

## Component 2: `ds::ConcurrentDSU` (concurrent, full)

Lock-free DSU over a fixed universe `[0, n)`:

- `parent_` : `std::vector<std::atomic<int>>` — the only mutable shared state.
- `prio_`   : `std::vector<uint64_t>` — immutable random priority, fixed for life,
  decides link direction (low-prio root links to high-prio root). Index breaks ties.
- `find`    : path halving via best-effort `compare_exchange_weak` (safe whether the CAS
  succeeds, fails, or races).
- `unite`   : re-find both roots, link low-prio root under high-prio root with
  `compare_exchange_strong`; on CAS failure (a concurrent unite stole the root) re-find
  and retry.

Memory ordering: conservative `acquire` loads / `acq_rel` CAS (the documented default;
safe for both pure-connectivity use and component-gated payload publication).

API identical to the sequential version: `find`, `unite`, `connected`, plus
`explicit ConcurrentDSU(int n)`. Constructor explicitly `.store(i)` each parent (do not
rely on default-constructed atomic value) and seeds `prio_` from a fixed-seed
`std::mt19937_64`.

### Correctness invariants (for reference, documented in comments)

- **Acyclicity:** links always go low-priority-root → high-priority-root. For a fixed pair
  of roots the direction is a constant function of immutable priorities, so two threads can
  never write opposing edges → no cycle → `find` always terminates.
- **Leads-to-root:** every node's parent stays within its set and leads to the root. Sets
  only merge, never split, so path-halving (`parent[x] := grandparent`) can never break
  reachability to the root.

## Component 3: Tests

### `dsu_test.cpp` (skeleton)

`TEST_CASE("dsu: find / unite / connected" * doctest::skip())` with `// TODO` body —
mirrors `trie_test.cpp`. Becomes active once the user implements `ds::DSU`.

### `concurrent_dsu_test.cpp` (passing)

**Functional (single-threaded):**
- Initially every element is its own set; `connected(i, i)` true, distinct elements not
  connected.
- `unite` returns `true` on first real merge, `false` when already in the same set.
- Connectivity is transitive after a chain of unites.

**Concurrent stress:**
- Spawn `N` threads (`std::thread::hardware_concurrency()` clamped to a sane range,
  e.g. `[2, 8]`).
- Generate a fixed set of random edges from a **fixed seed**; partition them across
  threads; each thread calls `unite` on its share.
- Join all threads.
- **Oracle:** feed the *same* edge set into a reference sequential DSU (built inside the
  test, independent of `ds::DSU`'s skeleton). Final connectivity is order-independent for
  set union, so for every sampled pair (or every element vs. its reference root)
  `ConcurrentDSU::connected` must match the reference. This oracle does **not** depend on
  thread interleaving → **not flaky**.

**Acyclicity:** verified implicitly — a real cycle would make `find` loop forever and hang
the test (a visible failure). No test-only access to private internals is added; the class
stays clean.

## Component 4: CMake

`projects/ds/CMakeLists.txt`: the concurrent header uses `<thread>`, so add
`find_package(Threads REQUIRED)` and link `Threads::Threads` to the `ds_tests` target.
Everything else is unchanged — the existing `GLOB_RECURSE tests/*_test.cpp` already picks
up the two new test files into the single `ds_tests` executable.

## Decisions / Trade-offs (locked)

- **Sequential = skeleton, concurrent = full** — by design.
- **Sequential suggested strategy: union by size** + path compression.
- **Stress oracle = reference sequential DSU over the same edge set** — deterministic,
  interleaving-independent, not flaky.
- **Acyclicity not actively detected** — relies on `find` termination; no step-cap walk and
  no test-only internal accessor, to keep `ConcurrentDSU` clean.

## Out of Scope

- Dynamic resize / segmented arrays (fixed universe `[0, n)` only).
- `relaxed` memory-ordering optimization (kept conservative `acquire`/`acq_rel`).
- DCAS / packed `(parent, rank)` deterministic-wait-free variant.

## References

- Jayanti, S. V. & Tarjan, R. E. *A Randomized Concurrent Algorithm for Disjoint Set
  Union.* PODC 2016. DOI: 10.1145/2933057.2933108. arXiv:1612.01514.
- Jayanti, S. V. & Tarjan, R. E. *Concurrent Disjoint Set Union.* Distributed Computing
  (2021). arXiv:2003.01203. DOI: 10.1007/s00446-020-00388-x.
