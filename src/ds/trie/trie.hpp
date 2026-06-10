#pragma once

// 前綴樹 (Trie)，存字串。骨架而已，實作留給你。
//
// 建議的 API（自己增刪）：
//   void  insert(const std::string& word);
//   bool  contains(const std::string& word) const;     // 完整單字
//   bool  starts_with(const std::string& prefix) const; // 任意前綴
//
// 建議的節點：每個 Node 有 children (例如 std::array<unique_ptr<Node>, 26>
//   或 std::unordered_map<char, unique_ptr<Node>>) 與 is_end 旗標。

namespace ds {

class Trie {
public:
  // TODO: 你的 public API

private:
  // TODO: 你的 Node 結構與 root_
};

}  // namespace ds
