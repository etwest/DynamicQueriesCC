#include <cassert>

#include <euler_tour_tree.h>


EulerTourTree::EulerTourTree(long seed, node_id_t vertex, uint32_t tier) :
    sketch((Sketch *) ::operator new(Sketch::sketchSizeof())), seed(seed), vertex(vertex), tier(tier) {
  // Initialize sentinel
  this->make_edge(nullptr);
  // Initialize sketch
  Sketch::makeSketch((char*)sketch, seed);
}

EulerTourTree::EulerTourTree(long seed) :
    sketch((Sketch *) ::operator new(Sketch::sketchSizeof())), seed(seed) {
  // Initialize sentinel
  this->make_edge(nullptr);
  // Initialize sketch
  Sketch::makeSketch((char*)sketch, seed);
}

EulerTourTree::EulerTourTree(Sketch* sketch, long seed) :
  sketch(sketch), seed(seed) {
  // Initialize sentinel
  this->make_edge(nullptr);
}

SkipListNode* EulerTourTree::make_edge(EulerTourTree* other) {
  assert(!other || this->tier == other->tier);
  //Constructing a new SkipListNode with pointer to this ETT object
  SkipListNode* node = SkipListNode::init_element(this);
  if (allowed_caller == nullptr) {
    allowed_caller = node;
    node->update_path_agg(this->sketch);
  }
  //Add the new SkipListNode to the edge list
  return this->edges.emplace(std::make_pair(other, node)).first->second;
  //Returns the new node pointer or the one that already existed if it did
}

void EulerTourTree::delete_edge(EulerTourTree* other) {
  assert(!other || this->tier == other->tier);
  bool deleting_allowed = this->edges[other] == allowed_caller;
  this->edges.erase(other);
  if (deleting_allowed) {
    if (this->edges.empty()) {
      allowed_caller = nullptr;
    } else {
      allowed_caller = this->edges.begin()->second;
      allowed_caller->update_path_agg(this->sketch);
    }
  }
}

void EulerTourTree::update_sketch(vec_t update_idx) {
  assert(allowed_caller);
  this->sketch->update(update_idx);
  this->allowed_caller->update_path_agg(update_idx);
}

//Get the aggregate sketch at the root of the ETT for this node
Sketch* EulerTourTree::get_aggregate() {
  assert(allowed_caller);
  return this->allowed_caller->get_list_aggregate();
}

uint32_t EulerTourTree::get_size() {
  return this->allowed_caller->get_list_size();
}

bool EulerTourTree::has_edge_to(EulerTourTree* other) {
  return !(this->edges.find(other) == this->edges.end());
}

std::set<EulerTourTree*> EulerTourTree::get_component() {
  return this->allowed_caller->get_component();
}

bool EulerTourTree::link(EulerTourTree& other) {
  assert(this->tier == other.tier);
  SkipListNode* this_sentinel = this->edges.begin()->second->get_last();
  SkipListNode* other_sentinel = other.edges.begin()->second->get_last();

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

  SkipListNode* aux_this_right = this->edges.begin()->second;
  SkipListNode* aux_this_left = SkipListNode::split_left(aux_this_right);

  // Unlink and destory other_sentinel
  SkipListNode* aux_other = SkipListNode::split_left(other_sentinel);
  other_sentinel->node->delete_edge(nullptr);

  SkipListNode* aux_other_left, *aux_other_right;
  if (aux_other == nullptr) {
    aux_other_right = aux_other_left = nullptr;
  } else {
    aux_other_right = other.edges.begin()->second;
    aux_other_left = SkipListNode::split_left(aux_other_right);
  }

  // reroot other tree
  // A, construct B, DED, C, construct D, BA
  // R  LR           L    R  LR           L
  // N                    N

  SkipListNode* aux_edge_left = this->make_edge(&other);
  SkipListNode* aux_edge_right = other.make_edge(this);

  SkipListNode::join(aux_this_left, aux_edge_left, aux_other_right,
      aux_other_left, aux_edge_right, aux_this_right);

  return true;
}

bool EulerTourTree::cut(EulerTourTree& other) {
  assert(this->tier == other.tier);
  if (this->edges.find(&other) == this->edges.end()) {
    assert(other.edges.find(this) == other.edges.end());
    return false;
  }
  SkipListNode* e1 = this->edges[&other];
  SkipListNode* e2 = other.edges[this];

  SkipListNode* frag1r = SkipListNode::split_right(e1);
  bool order_is_e1e2 = e2->get_last() != e1;
  SkipListNode* frag1l = SkipListNode::split_left(e1);
  this->delete_edge(&other);
  SkipListNode* frag2r = SkipListNode::split_right(e2);
  SkipListNode* frag2l = SkipListNode::split_left(e2);
  other.delete_edge(this);

  if (order_is_e1e2) {
    // e1 is to the left of e2
    // e2 should be made into a sentinel
    SkipListNode* sentinel = other.make_edge(nullptr);
    SkipListNode::join(frag2l, sentinel);
    SkipListNode::join(frag1l, frag2r);
  } else {
    // e2 is to the left of e1
    // e1 should be made into a sentinel
    SkipListNode* sentinel = this->make_edge(nullptr);
    SkipListNode::join(frag2r, sentinel);
    SkipListNode::join(frag2l, frag1r);
  }

  return true;
}
