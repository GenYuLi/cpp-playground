#include <doctest/doctest.h>

#include <vector>

#include "tree/tree.hpp"

TEST_CASE("BST: insert 後 contains 找得到") {
  ds::BinarySearchTree<int> bst;
  CHECK(bst.empty());

  bst.insert(5);
  bst.insert(3);
  bst.insert(8);

  CHECK(bst.size() == 3);
  CHECK(bst.contains(5));
  CHECK(bst.contains(3));
  CHECK(bst.contains(8));
  CHECK_FALSE(bst.contains(42));
}

TEST_CASE("BST: 重複插入不會增加 size") {
  ds::BinarySearchTree<int> bst;
  bst.insert(1);
  bst.insert(1);
  bst.insert(1);
  CHECK(bst.size() == 1);
}

TEST_CASE("BST: 中序走訪是排序好的") {
  ds::BinarySearchTree<int> bst;
  for (int v : {5, 3, 8, 1, 4, 7, 9}) bst.insert(v);

  std::vector<int> expected{1, 3, 4, 5, 7, 8, 9};
  CHECK(bst.inorder() == expected);
}
