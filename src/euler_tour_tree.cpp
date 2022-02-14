#include <cassert>

#include <euler_tour_tree.h>

using Sptr = std::shared_ptr<SplayTreeNode>;

EulerTourTree::EulerTourTree() :
    sketch((Sketch *) ::operator new(Sketch::sketchSizeof())) {
  // Initialize sentinel
  this->make_edge(nullptr);
  // Initialize sketch
  Sketch::makeSketch((char*)sketch.get(), 0);
}

EulerTourTree::EulerTourTree(Sketch* sketch) :
  sketch(sketch) {
  // Initialize sentinel
  this->make_edge(nullptr);
}

Sptr EulerTourTree::make_edge(EulerTourTree* other) {
  Sptr node = std::make_shared<SplayTreeNode>(*this);
  if (allowed_caller == nullptr) {
    allowed_caller = node.get();
  }
  return this->edges.emplace(std::make_pair(other, std::move(node))).first->second;
}

void EulerTourTree::delete_edge(EulerTourTree* other) {
  bool deleting_allowed = this->edges[other].get() == allowed_caller;
  this->edges.erase(other);
  if (deleting_allowed) {
    if (this->edges.empty()) {
      allowed_caller = nullptr;
    } else {
      allowed_caller = this->edges.begin()->second.get();
      allowed_caller->rebuild_agg();
    }
  }
}


Sketch* EulerTourTree::get_sketch(SplayTreeNode* caller) {
  return caller == allowed_caller ? sketch.get() : nullptr;
}

bool EulerTourTree::link(EulerTourTree& other) {
  Sptr this_sentinel = SplayTree::get_last(this->edges.begin()->second);
  Sptr other_sentinel = SplayTree::get_last(other.edges.begin()->second);

  // There should always be a sentinel
  assert(this_sentinel == this_sentinel->node->edges.at(nullptr));
  assert(other_sentinel == other_sentinel->node->edges.at(nullptr));

  // If the nodes are already part of the same tree, don't link
  if (this_sentinel == other_sentinel) {
    return false;
  }

  // linking BD, ABA with CDEDC
  // ABA split on B gives A BA
  // CDEDC removing sentinel gives CDED
  //                               ^ might be null
  // CDED split on D gives C DED
  // A, construct B, DED, C, construct D, BA
  // ^                    ^
  // '--------------------'--- might be null

  Sptr aux_this_right = this->edges.begin()->second;
  Sptr aux_this_left = SplayTree::split_left(aux_this_right);

  // Unlink and destory other_sentinel
  Sptr aux_other = SplayTree::split_left(other_sentinel);
  other_sentinel->node->delete_edge(nullptr);

  Sptr aux_other_left, aux_other_right;
  if (aux_other == nullptr) {
    aux_other_right = aux_other_left = nullptr;
  } else {
    aux_other_right = other.edges.begin()->second;
    aux_other_left = SplayTree::split_left(aux_other_right);
  }

  // reroot other tree
  // A, construct B, DED, C, construct D, BA
  // R  LR           L    R  LR           L
  // N                    N

  Sptr aux_edge_left = this->make_edge(&other);
  Sptr aux_edge_right = other.make_edge(this);

  // FIXME: what in gods name is this
  SplayTree::join(
      SplayTree::join(
        SplayTree::join(
          SplayTree::join(
            SplayTree::join(
              aux_this_left,
              aux_edge_left),
            aux_other_right),
          aux_other_left),
        aux_edge_right),
      aux_this_right);


  return true;
}

bool EulerTourTree::cut(EulerTourTree& other) {
  if (this->edges.find(&other) == this->edges.end()) {
    assert(other.edges.find(this) == other.edges.end());
    return false;
  }
  Sptr e1 = this->edges[&other];
  Sptr e2 = other.edges[this];

  Sptr frag1r = SplayTree::split_right(e1);
  bool order_is_e1e2 = SplayTree::get_last(e2) != e1;
  Sptr frag1l = SplayTree::split_left(e1);
  this->delete_edge(&other);
  Sptr frag2r = SplayTree::split_right(e2);
  Sptr frag2l = SplayTree::split_left(e2);
  other.delete_edge(this);

  if (order_is_e1e2) {
    // e1 is to the left of e2
    // e2 should be made into a sentinel
    Sptr sentinel = other.make_edge(nullptr);
    SplayTree::join(frag2l, sentinel);
    SplayTree::join(frag1l, frag2r);
  } else {
    // e2 is to the left of e1
    // e1 should be made into a sentinel
    Sptr sentinel = this->make_edge(nullptr);
    SplayTree::join(frag2r, sentinel);
    SplayTree::join(frag2l, frag1r);
  }

  return true;
}
