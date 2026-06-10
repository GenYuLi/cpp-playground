#pragma once

// Umbrella header for the data-structure playground (對應 rust 的 ds/mod.rs)。
// 每新增一個資料結構，就在這裡 #include 它的 header，外部只要 #include "ds.hpp"
// 就能拿到全部。測試檔則各自 include 自己那一個即可。

#include "graph/graph.hpp"
#include "list/list.hpp"
#include "tree/tree.hpp"
#include "trie/trie.hpp"
