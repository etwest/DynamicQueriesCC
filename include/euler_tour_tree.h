#pragma once
#include <iostream>
#include <unordered_map>

#include <splay_tree.h>

class EulerTourTree {
  FRIEND_TEST(SplayTreeSuite, random_splays);
  FRIEND_TEST(EulerTourTreeSuite, random_links_and_cuts);
  FRIEND_TEST(EulerTourTreeSuite, get_aggregate);
  using Sptr = std::shared_ptr<SplayTreeNode>;
  std::unordered_map<EulerTourTree*, Sptr> edges;
  

  SplayTreeNode* allowed_caller = nullptr;
  long seed = 0;
  std::unique_ptr<Sketch> sketch = nullptr;

  //TODO: implement these
  Sptr make_edge(EulerTourTree* other);
  void delete_edge(EulerTourTree* other);
public:
  EulerTourTree(long seed);
  EulerTourTree(Sketch* sketch, long seed);
  bool link(EulerTourTree& other);
  bool cut(EulerTourTree& other);

  bool isvalid() const;

  Sketch* get_sketch(SplayTreeNode* caller);
  void update_sketch(vec_t update_idx);
  std::shared_ptr<Sketch> get_aggregate();

  long get_seed() {return seed;};

  friend std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett);
};
