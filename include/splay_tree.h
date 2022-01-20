#pragma once

#include <iostream>

class EulerTourTree;

class SplayTree {
  SplayTree* left, *right;
  SplayTree* parent;

  void rotate_up();
  void splay();
public:
  EulerTourTree* node;

  SplayTree() = default;
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
