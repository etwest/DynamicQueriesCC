#pragma once
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  std::unordered_map<EulerTourTree*, SplayTree> edges;

public:
  EulerTourTree();
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  void validate() const;

  friend bool isvalid(const EulerTourTree& ett);
  friend void dump(const EulerTourTree& ett);
};
