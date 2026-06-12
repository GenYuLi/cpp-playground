#pragma once

// 無向圖 (adjacency list)。骨架而已，實作留給你。
//
// 建議的 API（自己增刪）：
//   void               add_edge(const T& a, const T& b);
//   bool               has_edge(const T& a, const T& b) const;
//   std::vector<T>     neighbors(const T& v) const;
//   std::size_t        num_vertices() const;
//
// 建議的儲存結構：std::unordered_map<T, std::vector<T>> adj_;

namespace ds {

template <typename T>
class Graph {
public:
  // TODO: 你的 public API

private:
  // TODO: 你的儲存結構
};

}  // namespace ds
