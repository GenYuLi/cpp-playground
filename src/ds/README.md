# ds — 資料結構練習場

對應 Rust playground 的 `src/ds/`。每個資料結構一個資料夾，header-only 實作 +
旁邊的 doctest 測試，`ctest` 一鍵全跑（≈ `cargo test`）。

## 結構

```
src/ds/
├── ds.hpp            # umbrella header（≈ rust 的 ds/mod.rs），#include 全部結構
├── ds_test_main.cpp  # 唯一定義 doctest main 的 TU，別動
├── tree/             # ✅ BinarySearchTree —— 範例，已實作 + 測試
│   ├── tree.hpp
│   └── tree_test.cpp
├── graph/            # 🚧 骨架，待實作
├── trie/             # 🚧 骨架，待實作
└── list/             # 🚧 骨架，待實作
```

## 跑測試

一定要在 nix 環境裡（`direnv` 進目錄會自動載入，或前綴 `direnv exec .`）：

```bash
cmake -S . -B build                       # 第一次 / CMakeLists 改過才需要
cmake --build build --target ds_tests
ctest --test-dir build --output-on-failure
```

只跑某個結構：

```bash
./build/ds_tests --test-case="*BST*"      # doctest 的過濾語法
```

## 新增一個資料結構

1. 開資料夾，寫 `xxx/xxx.hpp`（header-only 實作）。
2. 旁邊寫 `xxx/xxx_test.cpp`，用 doctest 的 `TEST_CASE` / `CHECK` / `REQUIRE`：

   ```cpp
   #include <doctest/doctest.h>
   #include "xxx/xxx.hpp"

   TEST_CASE("xxx: 做了什麼") {
     ds::Xxx<int> x;
     CHECK(x.empty());
   }
   ```

3. （可選）在 `ds.hpp` 加一行 `#include "xxx/xxx.hpp"`。
4. 直接 build + ctest。`CMakeLists.txt` 用 `GLOB_RECURSE ... CONFIGURE_DEPENDS`
   自動收 `*_test.cpp`，不用改 CMake。

## 骨架說明

`graph/` `trie/` `list/` 目前是空殼：header 只有 class 外形 + 預期 API 的 TODO，
測試是 `TEST_CASE(... * doctest::skip())` 先被跳過（所以 ctest 仍是綠的）。
動手時把實作補進 header、把測試的 `* doctest::skip()` 拿掉再填內容即可。
```
