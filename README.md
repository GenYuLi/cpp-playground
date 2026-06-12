# cpp-playground

C++ 語言特性與 library 的小型實驗場 — 每個 `bin/` 下的 `.cpp` 都是一個獨立的 demo,單獨編成 executable。

## Build

```sh
./build.sh                                    # 預設: Release + Ninja, 產物在 build/
BUILD_TYPE=Debug ./build.sh                   # Debug build
BUILD_DIR=build-rel ./build.sh                # 換目錄
```

或直接用 cmake:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

可選 flag:`-DCXX_VERSION=23` 切換 C++ 標準(預設 20,支援 14/17/20/23)。

## 環境

兩種方式擇一:

- **Nix (推薦)**: `nix-shell` 或 `direnv allow` — `flake.nix` 已備齊 fmt / Boost / nlohmann_json / cmake / ninja
- **手動**: `./setup.sh` 會檢查並提示安裝(目前支援 apt-based 系統)

Build 完 `build/compile_commands.json` 會自動產生,clangd / nvim LSP / VSCode 都會用到 — 通常做個 symlink:

```sh
ln -sf build/compile_commands.json .
```

## 目錄結構

```
cpp-playground/
├── CMakeLists.txt              # Top-level: 只做 project setup + find_package + add_subdirectory
│
├── projects/                   # 自成一體的 sub-project,可獨立 build
│   └── matching_engine/        # Header-only async matching engine + 測試
│       ├── CMakeLists.txt      #   (standalone-capable: 自帶 project() 跟 find_package)
│       ├── include/matching_engine/
│       │   ├── core/types.hpp
│       │   ├── matching/{async_engine,orderbook}.hpp
│       │   ├── memory/async_ring_buffer.hpp
│       │   └── scheduler/coro_scheduler.hpp
│       └── tests/
│           └── coro_matching_test.cpp
│
├── pg/                         # 「playground」共用 lib (SHARED),提供給 bin/ 使用
│   ├── CMakeLists.txt
│   ├── include/pg/
│   │   ├── asio_test/
│   │   └── language_practice/  # 各種 idiom / pattern / 練習用 header
│   └── src/
│       ├── asio_test/
│       └── language_practice/
│
└── bin/                        # 每個 .cpp 都會編成同名 executable,連 pg_lib
    ├── CMakeLists.txt          #   (foreach 自動展開,加新檔案不用改 CMake)
    ├── language/               # C++ 語言特性練習
    │   ├── concept_require.cpp
    │   ├── constexpr.cpp
    │   ├── const_string.cpp
    │   ├── init_ways.cpp
    │   ├── lazy_init.cpp
    │   ├── lp.cpp              # CRTP + heap 等綜合練習,用 pg_lib
    │   ├── noexcept.cpp
    │   ├── nrvo.cpp
    │   ├── pack.cpp
    │   ├── perfect_forwarding.cpp
    │   ├── placement_new.cpp
    │   ├── template.cpp
    │   ├── type_erase.cpp
    │   └── universal_reference.cpp
    └── asio/                   # Boost.Asio 練習
        ├── asio_test_bin.cpp
        └── socket_listener.cpp
```

## 設計選擇

- **`projects/matching_engine` 是 standalone-capable 的 sub-project** — 它的 `CMakeLists.txt` 自帶 `cmake_minimum_required` / `project()` / `find_package`,可以單獨 `cd` 進去 `cmake -B build` 編譯,也可以未來輕易切出去當獨立 repo。
- **`pg/` 是 pure subdirectory 風格** — 假設 top-level 已 setup,只負責定義 `pg_lib` target。它本來就只服務這個 playground,不打算單獨發行。
- **`bin/` 用 `file(GLOB ... CONFIGURE_DEPENDS)`** — 加新 `.cpp` 不用動 CMake,但需要 reconfigure(Ninja 會自動 trigger)。Production 大型專案通常顯式列檔 (因為 GLOB 對 IDE 和某些工具不夠友善),playground 用 GLOB 換取方便。

## 加新東西的方法

| 想做什麼 | 做法 |
|---|---|
| 加新的語言特性 demo | 丟一個 `.cpp` 到 `bin/language/`,re-build 即可 |
| 加新的 Asio demo | 丟一個 `.cpp` 到 `bin/asio/` |
| 加新的 `pg_lib` 內容 | header 放 `pg/include/pg/<topic>/`,source 放 `pg/src/<topic>/` |
| 加完全獨立的小專案 | 在 `projects/<name>/` 開新資料夾,寫 standalone-capable `CMakeLists.txt`,再到 top-level 加 `add_subdirectory(projects/<name>)` |
