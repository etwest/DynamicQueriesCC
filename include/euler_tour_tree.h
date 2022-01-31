#pragma once
#include <iostream>
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  using Sptr = std::shared_ptr<SplayTreeNode>;
  std::unordered_map<EulerTourTree*, Sptr> edges;
  

  SplayTreeNode* allowed_caller = nullptr;
  Sketch* sketch = nullptr;

  //TODO: implement these
  void make_edge();
  void delete_edge();
public:
  EulerTourTree();
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  Sketch* get_sketch(SplayTreeNode* caller){return (caller == allowed_caller) ? sketch : nullptr;};

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
