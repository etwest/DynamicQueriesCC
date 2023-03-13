#include "../include/link_cut_tree.h"

void LinkCutNode::set_parent(LinkCutNode* parent) { this->parent = parent; }
void LinkCutNode::set_dparent(LinkCutNode* dparent) { this->dparent = dparent; }
void LinkCutNode::set_edge_weight(uint32_t weight){ this->edge_weight = weight; }
void LinkCutNode::set_max(uint32_t weight){ this->max = weight; }
void LinkCutNode::set_reversed(bool reversed){ this->reversed = reversed; }

void LinkCutNode::link_left(LinkCutNode* other) {
  this->left = other;
  if (other != nullptr) {
    other->parent = this;
    this->tail = other->tail;
  }
  else {
    this->tail = this;
  }
  this->rebuild_max();
}

void LinkCutNode::link_right(LinkCutNode* other) {
  this->right = other;
  if (other != nullptr) {
    other->parent = this;
    this->head = other->head;
  }
  else {
    this->head = this;
  }
  this->rebuild_max();
}

LinkCutNode* LinkCutNode::get_left() { return this->left; }
LinkCutNode* LinkCutNode::get_right() { return this->right; }
LinkCutNode* LinkCutNode::get_parent() { return this->parent; }
LinkCutNode* LinkCutNode::get_dparent() { return this->dparent; }
LinkCutNode* LinkCutNode::get_head() { return this->head; }
LinkCutNode* LinkCutNode::get_tail() { return this->tail; }

void LinkCutNode::correct_reversals() {
    //Get the XOR of all reversed booleans from this node to root
    bool reversal_state = 0;
    LinkCutNode* curr = this;
    while (curr) {
        reversal_state = reversal_state != curr->reversed;
        curr = curr->parent;
    }
    //Travel back up performing the reversals where needed
    curr = this;
    LinkCutNode* prev = nullptr;
    while (curr) {
        if (reversal_state) {
            LinkCutNode* temp = curr->left;
            curr->left = curr->right;
            curr->right = temp;
            if (curr->left && curr->left != prev) {
                curr->left->set_reversed(curr->left->reversed != 1);
            }
            if (curr->right && curr->right != prev) {
                curr->right->set_reversed(curr->left->reversed != 1);
            }
            curr->set_reversed(0);
        }
        curr = curr->parent;
        if (curr) {reversal_state = reversal_state != curr->reversed;}
    }
}

void LinkCutNode::rebuild_max() {
    if (this->left && this->right) {
        this->set_max(std::max(this->edge_weight, std::max(this->left->max, this->right->max)));
    } else if (this->left) {
        this->set_max(std::max(this->edge_weight, this->left->max));
    } else if (this->right) {
        this->set_max(std::max(this->edge_weight, this->right->max));
    } else {
        this->set_max(this->edge_weight);
    }
}

void LinkCutNode::rotate_up() {
    LinkCutNode* parent = this->parent;
    LinkCutNode* grandparent = parent->parent;

    if (parent->left == this) {
        parent->link_left(this->right);
        this->link_right(parent);
    } else {
        parent->link_right(this->left);
        this->link_left(parent);
    }

    if (grandparent) {
        if (grandparent->left == parent) {
            grandparent->link_left(this);
        } else {
            grandparent->link_right(this);
        }
    }
}

void LinkCutNode::splay() {
    this->correct_reversals();

    while (this->parent) {
        LinkCutNode* parent = this->parent;
        LinkCutNode* grandparent = parent->parent;
        if (!grandparent) {
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

LinkCutNode* LinkCutTree::join(LinkCutNode* v, LinkCutNode* w) {
    LinkCutNode* tail = v->get_tail();
    tail->splay();
    tail->link_right(w);
    return tail;
}

std::pair<LinkCutNode*, LinkCutNode*> LinkCutTree::split(LinkCutNode* v) {
    v->splay();
    if (v->get_right() != nullptr) {
        v->get_right()->set_parent(nullptr);
    }
    std::pair<LinkCutNode*, LinkCutNode*> paths = {v, v->get_right()};
    v->link_right(nullptr);
    return paths;
}

LinkCutNode* LinkCutTree::splice(LinkCutNode* p) {
    LinkCutNode* v = p->get_head()->get_dparent();
    std::pair<LinkCutNode*, LinkCutNode*> paths = LinkCutTree::split(v);
    if (paths.second != nullptr) {
        paths.second->get_head()->set_dparent(v);
    }
    p->get_head()->set_dparent(nullptr);
    return LinkCutTree::join(paths.first, p);
}

LinkCutNode* LinkCutTree::expose(LinkCutNode* v) {
    std::pair<LinkCutNode*, LinkCutNode*> paths = LinkCutTree::split(v);
    if (paths.second != nullptr) {
        paths.second->get_head()->set_dparent(v);
    }

    v->splay();
    LinkCutNode* p = v;
    while(p->get_head()->get_dparent() != nullptr) {
        p = LinkCutTree::splice(p);
    }
    return p;
}
