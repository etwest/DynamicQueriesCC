#pragma once

class EulerTourTree;

class SplayTree {
  SplayTree* left, *right;
  SplayTree* parent;

public:
  EulerTourTree* node;

  SplayTree() = default;
  SplayTree(EulerTourTree& node);
  void splay();
  void rotate_up();
  void link_left(SplayTree* other);
  void link_right(SplayTree* other);
  SplayTree* traverse_right();
  SplayTree* split_left();
  SplayTree* split_right();

  friend bool isvalid(const EulerTourTree& ett);
  friend void dump(const EulerTourTree& ett);
};
