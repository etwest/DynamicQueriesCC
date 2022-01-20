#pragma once
#include "sketch.h"
#include <cassert>

// General idea: information is not stored in the leaves and brought up
// but may exist anywhere. Therefore, we need to store both the aggregate
// and the information this node adds, if any
// The only aggregate we care about is the root's aggregate
class SplayTree;

class SplayTreeNode{
  private:
    using Sptr = std::shared_ptr<SplayTreeNode>;
    using Wptr = std::weak_ptr<SplayTreeNode>;
    using Rptr = SplayTreeNode*;
    Sptr left;
    Sptr right;
    Wptr parent;
    void zig(bool leftm);
    void zigzig(bool leftm);
    void zigzag(bool leftm);
    void rebuild_agg();
    Sptr get_parent() {assert(!parent.expired()); return parent.lock();}
    Rptr get_rightmost() { Rptr node = this; while(node->right) node = node->right.get(); return node;};
    Rptr get_leftmost() { Rptr node = this; while(node->left) node = node->left.get(); return node;};
    friend class SplayTree;
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
  using Rptr = SplayTreeNode*;
  Sptr head;
  void splay(Sptr node);

  void splay_largest() {
    assert(head); Sptr node = head; 
    while(node->right) node = node->right; 
    splay(node); assert(head->right);
  };
  Rptr get_rightmost() {if (head) return head->get_rightmost(); return nullptr;};
  Rptr get_leftmost() {if (head) return head->get_leftmost(); return nullptr;};

  SplayTree(SplayTreeNode* node) {head.reset(node);};
  SplayTree(Sptr node) {head = node;};

 public:
  SplayTree()  {};
  SplayTree(Sketch* sketch) {head.reset(new SplayTreeNode(sketch));};
  ~SplayTree() {};

  // Join other to the right of us
  void join(SplayTree* other);
  // returns the heads right node and splits 
  SplayTree* split();
  void insert(Sketch* node);
  void remove(Sketch* node);

  Sketch* aggregate() { if (head) return head->sketch; return nullptr; };
};
