#pragma once
#include <iostream>
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  using Sptr = std::shared_ptr<SplayTreeNode>;
  std::unordered_map<EulerTourTree*, Sptr> edges;
  

  SplayTreeNode* allowed_caller = nullptr;
  std::unique_ptr<Sketch> sketch = nullptr;

  //TODO: implement these
  Sptr make_edge(EulerTourTree* other);
  void delete_edge(EulerTourTree* other);
public:
  EulerTourTree();
  EulerTourTree(Sketch* sketch);
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  Sketch* get_sketch(SplayTreeNode* caller);

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
