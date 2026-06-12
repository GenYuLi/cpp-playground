# ds — 資料結構練習場

對應 Rust playground 的 `src/ds/`。header-only 實作 + doctest 測試，
`ctest` 一鍵全跑（≈ `cargo test`）。與 `projects/matching_engine` 同構：
public headers 在 `include/ds/...`，測試在 `tests/`。

## 結構

```
projects/ds/
├── CMakeLists.txt
├── include/ds/
│   ├── ds.hpp          # umbrella header（≈ rust 的 ds/mod.rs）
│   ├── tree/tree.hpp   # ✅ BinarySearchTree —— 範例，已實作
│   ├── graph/graph.hpp # 🚧 骨架，待實作
│   ├── trie/trie.hpp   # 🚧 骨架，待實作
│   └── list/list.hpp   # 🚧 骨架，待實作
└── tests/
    ├── ds_test_main.cpp # 唯一定義 doctest main 的 TU，別動
    ├── tree_test.cpp    # ✅ 對應 BST 的測試
    ├── graph_test.cpp   # 🚧 skip() 跳過，待填
    ├── trie_test.cpp    # 🚧
    └── list_test.cpp    # 🚧
```

消費端一律寫 `#include <ds/tree/tree.hpp>`（include 路徑停在 `include/`）。

## 跑測試

在 nix flake 環境裡（`direnv` 進目錄會自動載入，或前綴 `direnv exec .`）：

```bash
cmake -B build -G Ninja        # 第一次 / CMakeLists 改過才需要
cmake --build build --target ds_tests
ctest --test-dir build --output-on-failure
```

只跑某個結構：

```bash
./build/projects/ds/ds_tests --test-case="*BST*"   # doctest 過濾語法
```

## 新增一個資料結構

1. 寫 `include/ds/xxx/xxx.hpp`（header-only 實作）。
2. 在 `tests/xxx_test.cpp` 用 doctest 寫 `TEST_CASE` / `CHECK` / `REQUIRE`：

   ```cpp
   #include <doctest/doctest.h>
   #include <ds/xxx/xxx.hpp>

   TEST_CASE("xxx: 做了什麼") {
     ds::Xxx<int> x;
     CHECK(x.empty());
   }
   ```

3. （可選）在 `include/ds/ds.hpp` 加一行 `#include <ds/xxx/xxx.hpp>`。
4. 直接 build + ctest。`tests/*_test.cpp` 用 `GLOB_RECURSE ... CONFIGURE_DEPENDS`
   自動收進來，不用改 CMake。

## 骨架說明

`graph/` `trie/` `list/` 目前是空殼：header 只有 class 外形 + 預期 API 的 TODO，
測試是 `TEST_CASE(... * doctest::skip())` 先被跳過（所以 ctest 仍是綠的）。
動手時把實作補進 header、把測試的 `* doctest::skip()` 拿掉再填內容即可。
