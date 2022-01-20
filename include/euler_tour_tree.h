#pragma once
#include <iostream>
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  std::unordered_map<EulerTourTree*, SplayTree> edges;

public:
  EulerTourTree();
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
