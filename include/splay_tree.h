#pragma once

#include <iostream>
#include <gtest/gtest.h>

class EulerTourTree;
class SplayTree;

class SplayTreeNode {

  // Test helpers
  FRIEND_TEST(SplayTreeSuite, random_splays);
  FRIEND_TEST(SplayTreeSuite, links_and_cuts);
  SplayTreeNode* splay_random_child();
  long count_children();

  SplayTreeNode* left, *right;
  SplayTreeNode* parent;

  void rotate_up();
  void splay();
  void link_left(SplayTreeNode* other);
  void link_right(SplayTreeNode* other);
public:
  EulerTourTree* node;

  SplayTreeNode(): left(nullptr), right(nullptr), parent(nullptr) {};
  SplayTreeNode(EulerTourTree& node);

  bool isvalid() const;
  const SplayTreeNode* next() const;

  friend class SplayTree;

  friend std::ostream& operator<<(std::ostream& os, const SplayTreeNode& tree);
};

class SplayTree {
    SplayTree();
  public:
    static SplayTreeNode* join(SplayTreeNode* left, SplayTreeNode* right);
    static SplayTreeNode* split_left(SplayTreeNode* node);
    static SplayTreeNode* split_right(SplayTreeNode* node);
    static SplayTreeNode* get_last(SplayTreeNode* node);
};
