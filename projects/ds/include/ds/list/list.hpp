#pragma once

// 單向鏈結串列 (singly linked list)。骨架而已，實作留給你。
//
// 建議的 API（自己增刪）：
//   void               push_front(const T& value);
//   void               push_back(const T& value);
//   bool               empty() const;
//   std::size_t        size() const;
//   std::vector<T>     to_vector() const;   // 方便寫測試比對
//
// 建議的節點：struct Node { T value; std::unique_ptr<Node> next; };
//   head_ 持有第一個節點，注意 unique_ptr 的所有權轉移。

namespace ds {

template <typename T>
class List {
public:
  // TODO: 你的 public API

private:
  // TODO: 你的 Node 結構與 head_ / size_
};

}  // namespace ds
