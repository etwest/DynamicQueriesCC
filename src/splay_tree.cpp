#include <cassert>

#include "splay_tree.h"
#include "euler_tour_tree.h"

SplayTreeNode::SplayTreeNode(EulerTourTree* node) :
  sketch_agg((Sketch*) ::operator new(Sketch::sketchSizeof())),
  node(node) {
  Sketch::makeSketch((char*)sketch_agg.get(), 0);
}

SplayTreeNode::SplayTreeNode() : SplayTreeNode(nullptr) {
}

SplayTreeNode::SplayTreeNode(EulerTourTree& node) : SplayTreeNode(&node) {
}

//TODO: call rebuild_agg in a way that doesn't waste too much time
void SplayTreeNode::rotate_up() {
  assert(!this->parent.expired());
  const Sptr& parent = this->get_parent();
  const Sptr& grandparent = parent->get_parent();

  if (grandparent == nullptr) {
    this->parent = Wptr();
  } else if (grandparent->left == parent) {
    grandparent->link_left(shared_from_this());
  } else {
    grandparent->link_right(shared_from_this());
  }

  if (parent->left.get() == this) {
    parent->link_left(this->right);
    this->link_right(parent);
  } else {
    parent->link_right(this->left);
    this->link_left(parent);
  }
}

void SplayTreeNode::splay() {
  while (!this->parent.expired()) {
    const Sptr& parent = this->get_parent();
    const Sptr& grandparent = parent->get_parent();
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
}

void SplayTreeNode::link_left(const Sptr& other) {
  this->left = other;
  if (other != nullptr) {
    other->parent = shared_from_this();
  }
}

void SplayTreeNode::link_right(const Sptr& other) {
  this->right = other;
  if (other != nullptr) {
    other->parent = shared_from_this();
  }
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
  node->left = nullptr;
  return ret;
}

std::shared_ptr<SplayTreeNode> SplayTree::split_right(const Sptr& node) {
  node->splay();
  Sptr ret = node->right;
  if (ret != nullptr) {
    ret->parent = Wptr();
  }
  node->right = nullptr;
  return ret;
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
  if (left == nullptr) {
    return right;
  }
  SplayTree::get_last(left)->link_right(right);
  return left;
}

void SplayTreeNode::rebuild_agg()
{
  // If we have a sketch, then copy it over. otherwise, empty sketch
  Sketch* sketch = get_sketch();
  if (sketch)
    Sketch::makeSketch((char*)sketch_agg.get(), *sketch);
  else
    Sketch::makeSketch((char*)sketch_agg.get(), 0);

  // Agg our left and right child, if we have them
  if (left)
    *sketch_agg += *left->sketch_agg;
  if (right)
    *sketch_agg += *right->sketch_agg;

  //TODO: some sort of lazy aggregation here, instead?
  if (!parent.expired())
  {
    get_parent()->rebuild_agg();
  }
}

Sketch* SplayTreeNode::get_sketch()
{
  if (!node)
    return nullptr;
  // we pass this to allow the node to differentiate the caller/owner
  return node->get_sketch(this);
}
