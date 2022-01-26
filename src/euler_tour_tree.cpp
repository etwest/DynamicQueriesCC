#include <cassert>

#include <euler_tour_tree.h>

EulerTourTree::EulerTourTree() : edges({{nullptr, SplayTreeNode(*this)}}) {
}

bool EulerTourTree::link(EulerTourTree& other) {
  SplayTreeNode* this_sentinel = SplayTree::get_last(&this->edges.begin()->second);
  SplayTreeNode* other_sentinel = SplayTree::get_last(&other.edges.begin()->second);

  // There should always be a sentinel
  assert(this_sentinel == &this_sentinel->node->edges.at(nullptr));
  assert(other_sentinel == &other_sentinel->node->edges.at(nullptr));

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

  SplayTreeNode* aux_this_right = &this->edges.begin()->second;
  SplayTreeNode* aux_this_left = SplayTree::split_left(aux_this_right);

  // Unlink and destory other_sentinel
  SplayTreeNode* aux_other = SplayTree::split_left(other_sentinel);
  other_sentinel->node->edges.erase(nullptr);

  SplayTreeNode* aux_other_left, *aux_other_right;
  if (aux_other == nullptr) {
    aux_other_right = aux_other_left = nullptr;
  } else {
    aux_other_right = &other.edges.begin()->second;
    aux_other_left = SplayTree::split_left(aux_other_right);
  }

  // reroot other tree
  // A, construct B, DED, C, construct D, BA
  // R  LR           L    R  LR           L
  // N                    N

  SplayTreeNode* aux_edge_left = &this->edges.emplace(
      std::make_pair(&other, SplayTreeNode(*this))).first->second;
  SplayTreeNode* aux_edge_right = &other.edges.emplace(
      std::make_pair(this, SplayTreeNode(other))).first->second;

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
  SplayTreeNode* e1 = &this->edges[&other];
  SplayTreeNode* e2 = &other.edges[this];

  SplayTreeNode* frag1r = SplayTree::split_right(e1);
  bool order_is_e1e2 = SplayTree::get_last(e2) != e1;
  SplayTreeNode* frag1l = SplayTree::split_left(e1);
  this->edges.erase(&other);
  SplayTreeNode* frag2r = SplayTree::split_right(e2);
  SplayTreeNode* frag2l = SplayTree::split_left(e2);
  other.edges.erase(this);

  if (order_is_e1e2) {
    // e1 is to the left of e2
    // e2 should be made into a sentinel
    SplayTreeNode* sentinel = &other.edges.emplace(
        std::make_pair(nullptr, SplayTreeNode(other))).first->second;
    SplayTree::join(frag2l, sentinel);
    SplayTree::join(frag1l, frag2r);
  } else {
    // e2 is to the left of e1
    // e1 should be made into a sentinel
    SplayTreeNode* sentinel = &this->edges.emplace(
        std::make_pair(nullptr, SplayTreeNode(*this))).first->second;
    SplayTree::join(frag2r, sentinel);
    SplayTree::join(frag2l, frag1r);
  }

  return true;
}
