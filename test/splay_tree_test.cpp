#include <functional>
#include <gtest/gtest.h>
#include "splay_tree.h"

bool SplayTree::isvalid() const {
  bool invalid = false;
  EXPECT_TRUE(this->parent == nullptr ||
      this->parent->left == this ||
      this->parent->right == this)
      << (invalid = true, "");
  if (invalid) return false;
  EXPECT_TRUE(this->left == nullptr || this->left->parent == this)
      << (invalid = true, "");
  if (invalid) return false;
  EXPECT_TRUE(this->right == nullptr || this->right->parent == this)
      << (invalid = true, "");
  if (invalid) return false;
  return true;
}

const SplayTree* SplayTree::next() const {
  const SplayTree* ret;
  if (this->right == nullptr) {
    // Follow parent up until we are a left child
    ret = this;
    while (ret->parent != nullptr && ret->parent->right == ret) {
      ret = ret->parent;
    }
    ret = ret->parent;
  } else {
    // Follow right child all the way to the left
    ret = this->right;
    while (ret->left != nullptr) {
      ret = ret->left;
    }
  }
  return ret;
}

std::ostream& operator<<(std::ostream& os, const SplayTree& tree) {
  const SplayTree* root = &tree;
  while (root->parent != nullptr) {
    root = root->parent;
  }
  std::function<void(const SplayTree* ptr, int depth)> inorder_dump =
      [&os, &tree, &inorder_dump](auto ptr, int depth) {
        if (ptr != nullptr) {
          inorder_dump(ptr->left, depth + 1);
          os << (ptr == &tree ? '>' : ' ') << std::string(depth, ' ') << ptr
              << std::endl;
          inorder_dump(ptr->right, depth + 1);
        }
      };
  inorder_dump(root, 0);
  return os;
}

TEST(SplayTreeSuite, links_and_cuts) {

	
}

TEST(SplayTreeSuite, random_splays) {
  SplayTree* tree = new SplayTree();
  //number of nodes
  int n = 10000;
  //number of random splays
  int rsplays = 10000000;
  //configure the sketch globally

  for (int i = 0; i < n; i++)
  {
    tree->link_right(new SplayTree());
    tree = tree->right;
  }

  SplayTree* root = tree;
  for (int i = 0; i < rsplays; i++)
  {
    root = root->splay_random_child();
  }
  // This validates both the parent-child pointer relations and the number of nodes in the tree
  long nnodes = root->count_children();
  if(nnodes != n+1)
    FAIL() << "Expected " << n << " nodes, found " << nnodes << std::endl;
}




