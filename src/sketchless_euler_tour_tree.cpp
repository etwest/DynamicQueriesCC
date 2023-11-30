#include <cassert>

#include <sketchless_euler_tour_tree.h>


SketchlessEulerTourTree::SketchlessEulerTourTree(node_id_t num_nodes, uint32_t tier_num, int seed) {
  // Initialize all the ETT node
  ett_nodes.reserve(num_nodes);
  for (node_id_t i = 0; i < num_nodes; ++i) {
      ett_nodes.emplace_back(seed, i, tier_num);
  }
}

void SketchlessEulerTourTree::link(node_id_t u, node_id_t v) {
  ett_nodes[u].link(ett_nodes[v]);
}

void SketchlessEulerTourTree::cut(node_id_t u, node_id_t v) {
  ett_nodes[u].cut(ett_nodes[v]);
}

bool SketchlessEulerTourTree::has_edge(node_id_t u, node_id_t v) {
  return ett_nodes[u].has_edge_to(&ett_nodes[v]);
}

SketchlessSkipListNode* SketchlessEulerTourTree::get_root(node_id_t u) {
  return ett_nodes[u].get_root();
}

SketchlessEulerTourNode::SketchlessEulerTourNode(long seed, node_id_t vertex, uint32_t tier) : seed(seed), vertex(vertex), tier(tier) {
  // Initialize sentinel
  this->make_edge(nullptr);
}

SketchlessEulerTourNode::SketchlessEulerTourNode(long seed) : seed(seed) {
  // Initialize sentinel
  this->make_edge(nullptr);
}

SketchlessEulerTourNode::~SketchlessEulerTourNode(){
  
}

SketchlessSkipListNode* SketchlessEulerTourNode::make_edge(SketchlessEulerTourNode* other) {
  assert(!other || this->tier == other->tier);
  //Constructing a new SkipListNode with pointer to this ETT object
  SketchlessSkipListNode* node = SketchlessSkipListNode::init_element(this);
  if (allowed_caller == nullptr) {
    allowed_caller = node;
  }
  //Add the new SkipListNode to the edge list
  return this->edges.emplace(std::make_pair(other, node)).first->second;
  //Returns the new node pointer or the one that already existed if it did
}

void SketchlessEulerTourNode::delete_edge(SketchlessEulerTourNode* other) {
  assert(!other || this->tier == other->tier);
  SketchlessSkipListNode* node_to_delete = this->edges[other];
  this->edges.erase(other);
  if (node_to_delete == allowed_caller) {
    if (this->edges.empty()) {
      allowed_caller = nullptr;
    } else {
      allowed_caller = this->edges.begin()->second;
    }
  }
  node_to_delete->uninit_element(true);
}

SketchlessSkipListNode* SketchlessEulerTourNode::get_root() {
  return this->allowed_caller->get_root();
}

bool SketchlessEulerTourNode::has_edge_to(SketchlessEulerTourNode* other) {
  return !(this->edges.find(other) == this->edges.end());
}

std::set<SketchlessEulerTourNode*> SketchlessEulerTourNode::get_component() {
  return this->allowed_caller->get_component();
}

bool SketchlessEulerTourNode::link(SketchlessEulerTourNode& other) {
  assert(this->tier == other.tier);
  SketchlessSkipListNode* this_sentinel = this->edges.begin()->second->get_last();
  SketchlessSkipListNode* other_sentinel = other.edges.begin()->second->get_last();

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

  SketchlessSkipListNode* aux_this_right = this->edges.begin()->second;
  SketchlessSkipListNode* aux_this_left = SketchlessSkipListNode::split_left(aux_this_right);

  // Unlink and destory other_sentinel
  SketchlessSkipListNode* aux_other = SketchlessSkipListNode::split_left(other_sentinel);
  other_sentinel->node->delete_edge(nullptr);

  SketchlessSkipListNode* aux_other_left, *aux_other_right;
  if (aux_other == nullptr) {
    aux_other_right = aux_other_left = nullptr;
  } else {
    aux_other_right = other.edges.begin()->second;
    aux_other_left = SketchlessSkipListNode::split_left(aux_other_right);
  }

  // reroot other tree
  // A, construct B, DED, C, construct D, BA
  // R  LR           L    R  LR           L
  // N                    N

  SketchlessSkipListNode* aux_edge_left = this->make_edge(&other);
  SketchlessSkipListNode* aux_edge_right = other.make_edge(this);

  SketchlessSkipListNode::join(aux_this_left, aux_edge_left, aux_other_right,
      aux_other_left, aux_edge_right, aux_this_right);

  return true;
}

bool SketchlessEulerTourNode::cut(SketchlessEulerTourNode& other) {
  assert(this->tier == other.tier);
  if (this->edges.find(&other) == this->edges.end()) {
    assert(other.edges.find(this) == other.edges.end());
    return false;
  }
  SketchlessSkipListNode* e1 = this->edges[&other];
  SketchlessSkipListNode* e2 = other.edges[this];

  SketchlessSkipListNode* frag1r = SketchlessSkipListNode::split_right(e1);
  bool order_is_e1e2 = e2->get_last() != e1;
  SketchlessSkipListNode* frag1l = SketchlessSkipListNode::split_left(e1);
  this->delete_edge(&other);
  SketchlessSkipListNode* frag2r = SketchlessSkipListNode::split_right(e2);
  SketchlessSkipListNode* frag2l = SketchlessSkipListNode::split_left(e2);
  other.delete_edge(this);

  if (order_is_e1e2) {
    // e1 is to the left of e2
    // e2 should be made into a sentinel
    SketchlessSkipListNode* sentinel = other.make_edge(nullptr);
    SketchlessSkipListNode::join(frag2l, sentinel);
    SketchlessSkipListNode::join(frag1l, frag2r);
  } else {
    // e2 is to the left of e1
    // e1 should be made into a sentinel
    SketchlessSkipListNode* sentinel = this->make_edge(nullptr);
    SketchlessSkipListNode::join(frag2r, sentinel);
    SketchlessSkipListNode::join(frag2l, frag1r);
  }

  return true;
}
