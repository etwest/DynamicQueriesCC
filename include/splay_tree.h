#pragma once
#include "sketch.h"
#include <cassert>

// General idea: information is not stored in the leaves and brought up
// but may exist anywhere. Therefore, we need to store both the aggregate
// and the information this node adds, if any
// The only aggregate we care about is the root's aggregate
class SplayTreeNode{
  private:
    using Sptr = std::shared_ptr<SplayTreeNode>;
    using Wptr = std::weak_ptr<SplayTreeNode>;
    Sptr left;
    Sptr right;
    Wptr parent;
    void zig(bool leftm);
    void zigzig(bool leftm);
    void zigzag(bool leftm);
    void rebuild_agg();
    Sptr get_parent() {assert(!parent.expired()); return parent.lock();}
  public:
    void splay();
    Sketch* sketch = nullptr;
    SplayTreeNode() = default;
    SplayTreeNode(Sketch* sketch) : sketch(sketch)
    {
      rebuild_agg();
    };
};

class SplayTree {
 private:
  using Sptr = std::shared_ptr<SplayTreeNode>;
  Sptr head;
  void splay(Sptr node);

 public:
  SplayTree()  {};
  ~SplayTree() {};

  void join(SplayTree& other);
  void split(SplayTree& left, SplayTree& right);
  void insert(Sketch* node);
  void remove(Sketch* node);

  Sketch* aggregate() { if (head) return head->sketch; return nullptr; };
};
