#pragma once

#include <iostream>
#include <gtest/gtest.h>
#include "sketch.h"

class EulerTourTree;
class SplayTree;

class SplayTreeNode :public std::enable_shared_from_this<SplayTreeNode> {
  using Wptr = std::weak_ptr<SplayTreeNode>;
  using Sptr = std::shared_ptr<SplayTreeNode>;
  // Test helpers
  FRIEND_TEST(SplayTreeSuite, random_splays);
  FRIEND_TEST(EulerTourTreeSuite, random_links_and_cuts);
  FRIEND_TEST(EulerTourTreeSuite, get_aggregate);
  Sptr splay_random_child();
  long count_children();

  Sptr left, right;
  Wptr parent;
  
  Sptr get_parent() {return parent.lock();};
  Sptr get_cparent() const {return parent.lock();};

  std::shared_ptr<Sketch> sketch_agg = nullptr;
  uint32_t size = 1;

  Sketch* get_sketch();
  void rotate_up();
  void link_left(const Sptr& other);
  void link_right(const Sptr& other);

  bool needs_rebuilding = true;
  void rebuild_one();

public:
  EulerTourTree* node = nullptr;

  SplayTreeNode();

  SplayTreeNode(EulerTourTree& node);
  SplayTreeNode(EulerTourTree* node);

  void splay();

  uint32_t get_root_size();
  
  //Rebuilds our aggregate, then recursively rebuilds our parents aggregate
  void rebuild_agg();
  //Update all the agg sketches from the current node to the root
  void update_path_agg(vec_t update_idx);

  std::shared_ptr<Sketch> get_root_aggregate();

  bool isvalid() const;
  const SplayTreeNode* next() const;

  static std::set<EulerTourTree*> get_component(SplayTreeNode* node);
  static void inorder(SplayTreeNode* node, std::set<EulerTourTree*>& nodes);

  friend class SplayTree;

  friend std::ostream& operator<<(std::ostream& os, const SplayTreeNode& tree);
};

class SplayTree {
  using Wptr = std::weak_ptr<SplayTreeNode>;
  using Sptr = std::shared_ptr<SplayTreeNode>;
    SplayTree();
  public:
    static const Sptr& join(const Sptr& left, const Sptr& right);
    template <typename... T>
    static const Sptr& join(const Sptr& head, const T&... tail);
    static Sptr split_left(const Sptr& node);
    static Sptr split_right(const Sptr& node);
    static Sptr get_last(Sptr node);
};

template <typename... T>
const std::shared_ptr<SplayTreeNode>& SplayTree::join(const std::shared_ptr<SplayTreeNode>& head, const T&... tail) {
  return join(head, join(tail...));
}
