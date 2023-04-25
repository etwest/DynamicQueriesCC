#include <cassert>


#include "splay_tree.h"
#include "euler_tour_tree.h"

SplayTreeNode::SplayTreeNode(EulerTourTree* node) :
  sketch_agg((Sketch*) ::operator new(Sketch::sketchSizeof())),
  node(node) {
  Sketch::makeSketch((char*)sketch_agg.get(), node ? node->get_seed() : 0);
}

SplayTreeNode::SplayTreeNode() : SplayTreeNode(nullptr) {}

SplayTreeNode::SplayTreeNode(EulerTourTree& node) : SplayTreeNode(&node) {}

void SplayTreeNode::rotate_up() {
  assert(!this->parent.expired());
  const Sptr& parent = this->get_parent();
  const Sptr& grandparent = parent->get_parent();
  assert(this->node->tier == parent->node->tier && (!grandparent || this->node->tier == grandparent->node->tier));

  if (parent->left.get() == this) {
    parent->link_left(this->right);
    this->link_right(parent);
  } else {
    parent->link_right(this->left);
    this->link_left(parent);
  }

  if (grandparent == nullptr) {
    this->parent = Wptr();
  } else if (grandparent->left == parent) {
    grandparent->link_left(shared_from_this());
  } else {
    grandparent->link_right(shared_from_this());
  }
}

void SplayTreeNode::splay() {
  while (!this->parent.expired()) {
    const Sptr& parent = this->get_parent();
    const Sptr& grandparent = parent->get_parent();
    assert(this->node->tier == parent->node->tier && (!grandparent || this->node->tier == grandparent->node->tier));
    if (grandparent == nullptr) {
      // zig
      this->rotate_up();
    } else if ((grandparent->left == parent) == (parent->left.get() == this)) {
      // zig-zig
      parent->rotate_up();
      this->rotate_up();
    } else {
      // zig-zag
      this->rotate_up();
      this->rotate_up();
    }
  }
  needs_rebuilding = true;
  rebuild_agg();
}

void SplayTreeNode::link_left(const Sptr& other) {
  assert(!other || this->node->tier == other->node->tier);
  this->left = other;
  if (other != nullptr) {
    other->parent = shared_from_this();

  }
  needs_rebuilding = true;
  rebuild_agg();
  size = 1;
  if (left) size += left->size;
  if (right) size += right->size;
}

void SplayTreeNode::link_right(const Sptr& other) {
  assert(!other || this->node->tier == other->node->tier);
  this->right = other;
  if (other != nullptr) {
    other->parent = shared_from_this();
  }
  needs_rebuilding = true;
  rebuild_agg();
  size = 1;
  if (left) size += left->size;
  if (right) size += right->size;
}

std::shared_ptr<SplayTreeNode> SplayTree::get_last(Sptr node) {
  node->splay();
  while (node->right != nullptr) {
    node = node->right;
  }
  return node;
}

std::shared_ptr<SplayTreeNode> SplayTree::split_left(const Sptr& node) {
  node->splay();
  Sptr ret = node->left;
  if (ret != nullptr) {
    ret->parent = Wptr();
  }
  node->link_left(nullptr);
  return ret;
}

std::shared_ptr<SplayTreeNode> SplayTree::split_right(const Sptr& node) {
  node->splay();
  Sptr ret = node->right;
  if (ret != nullptr) {
    ret->parent = Wptr();
  }
  node->link_right(nullptr);
  return ret;
}

std::shared_ptr<Sketch> SplayTreeNode::get_root_aggregate() {
  this->splay();
  this->rebuild_agg();
  return std::shared_ptr<Sketch>(this->sketch_agg);
}

uint32_t SplayTreeNode::get_root_size() {
  this->splay();
  return this->size;
}

void SplayTreeNode::inorder(SplayTreeNode* node, std::set<EulerTourTree*>& nodes) {
    if (node == nullptr) return;
    inorder(node->left.get(), nodes);
    nodes.insert(node->node);
    inorder(node->right.get(), nodes);

}

std::set<EulerTourTree*> SplayTreeNode::get_component(SplayTreeNode* node) {
    SplayTreeNode* curr = node;
    SplayTreeNode* root = curr;
    while (curr) {
        if (curr->get_parent() == nullptr) root = curr;
        curr = curr->get_parent().get();
    }
    std::set<EulerTourTree*> nodes;
    inorder(root, nodes);
    return nodes;
}

long SplayTreeNode::count_children()
{
	long count = 1;
	if (right)
		count += right->count_children();
	if (left)
		count += left->count_children();
	return count;
}

std::shared_ptr<SplayTreeNode> SplayTreeNode::splay_random_child()
{
  Sptr node = shared_from_this();
	while(true)
	{
		int which = rand() % 20;
		if (which == 0)
		{
			node->splay();
			return node;
		}
		which = rand() % 2;
		if (which == 0)
			if (node->left)
				node = node->left;
			else
			{
				node->splay();
				return node;
			}
		else
			if (node->right)
				node = node->right;
			else
			{
				node->splay();
				return node;
			}
	}
}

const std::shared_ptr<SplayTreeNode>& SplayTree::join(const Sptr& left, const Sptr& right) {
  assert(!left || !right || left->node->tier == right->node->tier);
  if (left == nullptr) {
    return right;
  }
  SplayTree::get_last(left)->link_right(right);
  return left;
}

void SplayTreeNode::rebuild_one()
{
  assert(needs_rebuilding == true);
  // If we have a sketch, then copy it over. otherwise, empty sketch
  
  Sketch* sketch = get_sketch();
  if (sketch)
    Sketch::makeSketch((char*)sketch_agg.get(), *sketch);
  else
    *sketch_agg += *sketch_agg;

  // Agg our left and right child, if we have them
  if (left)
    *sketch_agg += *left->sketch_agg;
  if (right)
    *sketch_agg += *right->sketch_agg;

  needs_rebuilding = false;
}

void SplayTreeNode::rebuild_agg()
{
  if (!needs_rebuilding)
    return;
  
  if (left)
    left->rebuild_agg();
  if (right)
    right->rebuild_agg();

  rebuild_one();
}

void SplayTreeNode::update_path_agg(vec_t update_idx) {
  this->sketch_agg.get()->update(update_idx);
  if (!this->parent.expired()) this->get_parent()->update_path_agg(update_idx);
}

Sketch* SplayTreeNode::get_sketch()
{
  if (!node)
    return nullptr;
  // we pass this to allow the node to differentiate the caller/owner
  return node->get_sketch(this);
}
