#include <cassert>

#include <splay_tree.h>

SplayTreeNode::SplayTreeNode(EulerTourTree& node) :
  left(nullptr), right(nullptr), parent(nullptr), node(&node) {
}

void SplayTreeNode::rotate_up() {
  assert(this->parent != nullptr);
  SplayTreeNode* parent = this->parent;
  SplayTreeNode* grandparent = parent->parent;

  if (grandparent == nullptr) {
    this->parent = nullptr;
  } else if (grandparent->left == parent) {
    grandparent->link_left(this);
  } else {
    grandparent->link_right(this);
  }

  if (parent->left == this) {
    parent->link_left(this->right);
    this->link_right(parent);
  } else {
    parent->link_right(this->left);
    this->link_left(parent);
  }
}

void SplayTreeNode::splay() {
  while (this->parent != nullptr) {
    SplayTreeNode* parent = this->parent;
    SplayTreeNode* grandparent = parent->parent;
    if (grandparent == nullptr) {
      // zig
      this->rotate_up();
    } else if ((grandparent->left == parent) == (parent->left == this)) {
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

void SplayTreeNode::link_left(SplayTreeNode* other) {
  this->left = other;
  if (other != nullptr) {
    other->parent = this;
  }
}

void SplayTreeNode::link_right(SplayTreeNode* other) {
  this->right = other;
  if (other != nullptr) {
    other->parent = this;
  }
}

SplayTreeNode* SplayTree::get_last(SplayTreeNode* node) {
  node->splay();
  while (node->right != nullptr) {
    node = node->right;
  }
  return node;
}

SplayTreeNode* SplayTree::split_left(SplayTreeNode* node) {
  node->splay();
  SplayTreeNode* ret = node->left;
  if (ret != nullptr) {
    ret->parent = nullptr;
  }
  node->left = nullptr;
  return ret;
}

SplayTreeNode* SplayTree::split_right(SplayTreeNode* node) {
  node->splay();
  SplayTreeNode* ret = node->right;
  if (ret != nullptr) {
    ret->parent = nullptr;
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

SplayTreeNode* SplayTreeNode::splay_random_child()
{
  SplayTreeNode* node = this;
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


SplayTreeNode* SplayTree::join(SplayTreeNode* left, SplayTreeNode* right) {
  if (left == nullptr) {
    return right;
  }
  SplayTree::get_last(left)->link_right(right);
  return left;
}
