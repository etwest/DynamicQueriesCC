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
  FRIEND_TEST(SplayTreeSuite, links_and_cuts);
  Sptr splay_random_child();
  long count_children();

  Sptr left, right;
  Wptr parent;
  
  Sptr get_parent() {return parent.lock();};
  Sptr get_cparent() const {return parent.lock();};


  Sketch* sketch_agg = nullptr;
  Sketch* get_sketch();
  void rotate_up();
  void splay();
  void link_left(const Sptr& other);
  void link_right(const Sptr& other);

public:
  EulerTourTree* node;

  SplayTreeNode()
  {
    sketch_agg = (Sketch*) new char[Sketch::sketchSizeof()];
    Sketch::makeSketch((char*)sketch_agg, 0);
  };
  
  SplayTreeNode(EulerTourTree& node) : node(&node)
  {
    SplayTreeNode();
  };

  ~SplayTreeNode() 
  {
    free(sketch_agg);
  };

  /* Rebuilds our aggregate, then recursively rebuilds our parents aggregate
   */ 
  void rebuild_agg();

  bool isvalid() const;
  const SplayTreeNode* next() const;

  friend class SplayTree;

  friend std::ostream& operator<<(std::ostream& os, const SplayTreeNode& tree);
};

class SplayTree {
  using Wptr = std::weak_ptr<SplayTreeNode>;
  using Sptr = std::shared_ptr<SplayTreeNode>;
    SplayTree();
  public:
    static const Sptr& join(const Sptr& left, const Sptr& right);
    static Sptr split_left(const Sptr& node);
    static Sptr split_right(const Sptr& node);
    static Sptr get_last(Sptr node);
};
