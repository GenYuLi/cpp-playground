#pragma once

#include <memory>
#include <vector>

namespace ds {

// 一個最小的二元搜尋樹 (BST)，header-only，當作 ds/ playground 的範例結構。
// 重點不在效能，而在「實作 + tests/ 下的 tree_test.cpp + ctest 一鍵跑」的流程。
template <typename T>
class BinarySearchTree {
public:
  void insert(const T& value) { root_ = insert(std::move(root_), value); }

  bool contains(const T& value) const {
    const Node* cur = root_.get();
    while (cur) {
      if (value < cur->value) {
        cur = cur->left.get();
      } else if (cur->value < value) {
        cur = cur->right.get();
      } else {
        return true;
      }
    }
    return false;
  }

  // 中序走訪 → 由小到大的排序序列。
  std::vector<T> inorder() const {
    std::vector<T> out;
    inorder(root_.get(), out);
    return out;
  }

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

private:
  struct Node {
    explicit Node(T v) : value(std::move(v)) {}
    T value;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
  };

  std::unique_ptr<Node> insert(std::unique_ptr<Node> node, const T& value) {
    if (!node) {
      ++size_;
      return std::make_unique<Node>(value);
    }
    if (value < node->value) {
      node->left = insert(std::move(node->left), value);
    } else if (node->value < value) {
      node->right = insert(std::move(node->right), value);
    }
    // 相等 → 視為已存在，不重複插入。
    return node;
  }

  void inorder(const Node* node, std::vector<T>& out) const {
    if (!node) return;
    inorder(node->left.get(), out);
    out.push_back(node->value);
    inorder(node->right.get(), out);
  }

  std::unique_ptr<Node> root_;
  std::size_t size_ = 0;
};

}  // namespace ds
