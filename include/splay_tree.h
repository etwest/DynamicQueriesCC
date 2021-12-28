#include <Supernode.h>

class SplayTreeNode{
  private:
    using Sptr = std::shared_ptr<SplayTreeNode>;
    Supernode sketch_agg;
    Sptr left;
    Sptr right;
    Sptr parent;
    void zig(bool left);
    void zigzig(bool left);
    void zigzag(bool left);
    void rebuild_agg();
  public:
    void splay();
    SplayTreeNode() = default;
    SplayTreeNode(Supernode& sketch) : sketch_agg(sketch){};
}

class SplayTree {
 private:
  using Sptr = std::shared_ptr<SplayTreeNode>;
  Sptr head;
  void splay(Sptr node);

 public:
  SplayTree();
  ~SplayTree();

  void join(SplayTree& other);
  void split(SplayTree& left, SplayTree& right);
  void insert(Supernode& node);
  void remove(Supernode& node);

  Supernode* aggregate() { if (head) return head->sketch_agg; return nullptr; };
};
