#pragma once
#include <iostream>
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  using Sptr = std::shared_ptr<SplayTreeNode>;
  std::unordered_map<EulerTourTree*, Sptr> edges;

public:
  EulerTourTree();
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
