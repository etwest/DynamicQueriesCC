#include <cassert>

#include <euler_tour_tree.h>

EulerTourTree::EulerTourTree() : edges({{nullptr, SplayTree(*this)}}) {
}

bool EulerTourTree::link(EulerTourTree& other) {
  SplayTree* this_sentinel = this->edges.begin()->second.traverse_right();
  SplayTree* other_sentinel = other.edges.begin()->second.traverse_right();

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

  SplayTree* aux_this_right = &this->edges.begin()->second;
  SplayTree* aux_this_left = aux_this_right->split_left();

  // Unlink and destory other_sentinel
  SplayTree* aux_other = other_sentinel->split_left();
  other_sentinel->node->edges.erase(nullptr);

  if (aux_other == nullptr) {
    // other tree was only a sentinel
    // A, construct B, construct D, BA
    SplayTree* aux_edge_left = &this->edges.emplace(std::make_pair(&other, SplayTree(*this))).first->second;
    SplayTree* aux_edge_right = &other.edges.emplace(std::make_pair(this, SplayTree(other))).first->second;
    aux_edge_left->link_left(aux_this_left);
    aux_edge_left->link_right(aux_edge_right);
    aux_edge_right->link_right(aux_this_right);
  } else {
    // reroot other tree
    // A, construct B, DED, C, construct D, BA
    // R  LR           L    R  LR           L
    // N                    N
    SplayTree* aux_other_right = &other.edges.begin()->second;
    SplayTree* aux_other_left = aux_other_right->split_left();

    SplayTree* aux_edge_left = &this->edges.emplace(std::make_pair(&other, SplayTree(*this))).first->second;
    SplayTree* aux_edge_right = &other.edges.emplace(std::make_pair(this, SplayTree(other))).first->second;

    aux_edge_left->link_left(aux_this_left);
    aux_other_right->link_left(aux_edge_left);
    aux_edge_right->link_left(aux_other_left);
    aux_other_right->traverse_right()->link_right(aux_edge_right);
    aux_edge_right->link_right(aux_this_right);
  }

  return true;
}

bool EulerTourTree::cut(EulerTourTree& other) {
  if (this->edges.find(&other) == this->edges.end()) {
    assert(other.edges.find(this) == other.edges.end());
    return false;
  }
  SplayTree* e1 = &this->edges[&other];
  SplayTree* e2 = &other.edges[this];

  SplayTree* frag1r = e1->split_right();
  bool order_is_e1e2 = e2->traverse_right() != e1;
  SplayTree* frag1l = e1->split_left();
  this->edges.erase(&other);
  SplayTree* frag2r = e2->split_right();
  SplayTree* frag2l = e2->split_left();
  other.edges.erase(this);

  if (order_is_e1e2) {
    // e1 is to the left of e2
    // e2 should be made into a sentinel
    SplayTree* sentinel = &other.edges.emplace(std::make_pair(nullptr, SplayTree(other))).first->second;
    sentinel->link_left(frag2l);
    if (frag1l != nullptr) {
      frag1l->traverse_right()->link_right(frag2r);
    }
  } else {
    // e2 is to the left of e1
    // e1 should be made into a sentinel
    SplayTree* sentinel = &this->edges.emplace(std::make_pair(nullptr, SplayTree(*this))).first->second;
    sentinel->link_left(frag2r);
    if (frag2l != nullptr) {
      frag2l->traverse_right()->link_right(frag1r);
    }
  }

  return true;
}
