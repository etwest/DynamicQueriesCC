#include <splay_tree.h>

SplayTree::SplayTree(EulerTourTree& node) :
  left(nullptr), right(nullptr), parent(nullptr), node(&node) {
}

void SplayTree::splay() {
  while (this->parent != nullptr) {
    SplayTree& other = *this->parent;
    if (other.parent == nullptr) {
      this->parent = nullptr;
    } else {
      SplayTree& grandparent = *other.parent;
      if (grandparent.left == &other) {
        grandparent.link_left(this);
      } else {
        grandparent.link_right(this);
      }
    }
    if (other.left == this) {
      other.link_left(this->right);
      this->link_right(&other);
    } else {
      other.link_right(this->left);
      this->link_left(&other);
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
