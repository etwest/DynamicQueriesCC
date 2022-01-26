#pragma once

#include <iostream>
#include <gtest/gtest.h>

class EulerTourTree;

class SplayTree {

  // Test helpers
  FRIEND_TEST(SplayTreeSuite, random_splays);
  FRIEND_TEST(SplayTreeSuite, links_and_cuts);
  SplayTree* splay_random_child();
  long count_children();

  SplayTree* left, *right;
  SplayTree* parent;

  void rotate_up();
  void splay();
public:
  EulerTourTree* node;

  SplayTree(): left(nullptr), right(nullptr), parent(nullptr) {};
  SplayTree(EulerTourTree& node);
  void link_left(SplayTree* other);
  void link_right(SplayTree* other);
  SplayTree* traverse_right();
  SplayTree* split_left();
  SplayTree* split_right();

  bool isvalid() const;
  const SplayTree* next() const;

  friend std::ostream& operator<<(std::ostream& os, const SplayTree& tree);
};
