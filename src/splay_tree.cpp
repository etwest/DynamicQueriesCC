#include <cassert>

#include <splay_tree.h>

SplayTree::SplayTree(EulerTourTree& node) :
  left(nullptr), right(nullptr), parent(nullptr), node(&node) {
}

void SplayTree::rotate_up() {
  assert(this->parent != nullptr);
  SplayTree* parent = this->parent;
  SplayTree* grandparent = parent->parent;

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

void SplayTree::splay() {
  while (this->parent != nullptr) {
    SplayTree* parent = this->parent;
    SplayTree* grandparent = parent->parent;
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

void SplayTree::link_left(SplayTree* other) {
  this->left = other;
  if (other != nullptr) {
    other->parent = this;
  }
}

void SplayTree::link_right(SplayTree* other) {
  this->right = other;
  if (other != nullptr) {
    other->parent = this;
  }
}

SplayTree* SplayTree::traverse_right() {
  this->splay();
  SplayTree* ret = this;
  while (ret->right != nullptr) {
    ret = ret->right;
  }
  return ret;
}

SplayTree* SplayTree::split_left() {
  this->splay();
  SplayTree* ret = this->left;
  if (ret != nullptr) {
    ret->parent = nullptr;
  }
  this->left = nullptr;
  return ret;
}

SplayTree* SplayTree::split_right() {
  this->splay();
  SplayTree* ret = this->right;
  if (ret != nullptr) {
    ret->parent = nullptr;
  }
  this->right = nullptr;
  return ret;
}
