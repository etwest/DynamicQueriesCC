#include "../include/link_cut_tree.h"
#include <cassert>

void LinkCutNode::set_parent(LinkCutNode* parent) { this->parent = parent; }
void LinkCutNode::set_dparent(LinkCutNode* dparent) { this->dparent = dparent; }
void LinkCutNode::set_edge_weight_up(uint32_t weight){ this->edge_weight_up = weight; }
void LinkCutNode::set_edge_weight_down(uint32_t weight){ this->edge_weight_down = weight; }
void LinkCutNode::set_max(uint32_t weight){ this->max = weight; }
void LinkCutNode::set_use_edge_up(bool use_edge_up){ this->use_edge_up = use_edge_up; }
void LinkCutNode::set_use_edge_down(bool use_edge_down){ this->use_edge_down = use_edge_down; }
void LinkCutNode::set_reversed(bool reversed){ this->reversed = reversed; }
void LinkCutNode::reverse() { this->reversed = !this->reversed; }

void LinkCutNode::link_left(LinkCutNode* other) {
  this->left = other;
  if (other != nullptr) {
    other->parent = this;
    this->head = other->get_head();
  }
  else {
    this->head = this;
  }
  this->rebuild_max();
}

void LinkCutNode::link_right(LinkCutNode* other) {
  this->right = other;
  if (other != nullptr) {
    other->parent = this;
    this->tail = other->get_tail();
  }
  else {
    this->tail = this;
  }
  this->rebuild_max();
}

LinkCutNode* LinkCutNode::get_left() { return this->left; }
LinkCutNode* LinkCutNode::get_right() { return this->right; }
LinkCutNode* LinkCutNode::get_parent() { return this->parent; }
LinkCutNode* LinkCutNode::get_dparent() { return this->dparent; }
LinkCutNode* LinkCutNode::get_head() { return this->reversed ? this->tail : this->head; }
LinkCutNode* LinkCutNode::get_tail() { return this->reversed ? this->head : this->tail; }

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

// rewrite this to use edge__weight_up and edge_weight_down
void LinkCutNode::rebuild_max() {
    uint32_t edges[] = {0,0,0,0,0,0};

    if (this->use_edge_up) { edges[0] = this->edge_weight_up; }
    if (this->use_edge_down) { edges[1] = this->edge_weight_down; }
    if (this->left && this->left->use_edge_up) { edges[2] = this->left->edge_weight_up; }
    if (this->left && this->left->use_edge_down) { edges[3] = this->left->edge_weight_down; }
    if (this->right && this->right->use_edge_up) { edges[4] = this->right->edge_weight_up; }
    if (this->right && this->right->use_edge_down) { edges[5] = this->right->edge_weight_down; }

    this->set_max(*std::max_element(edges, edges+6));
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

    if (grandparent != nullptr) {
        if (grandparent->left == parent) {
            grandparent->link_left(this);
        } else {
            grandparent->link_right(this);
        }
    } else {
        this->set_parent(nullptr);
    }
}

LinkCutNode* LinkCutNode::splay() {
    this->correct_reversals();

    while (this->parent != nullptr) {
        LinkCutNode* parent = this->parent;
        LinkCutNode* grandparent = parent->parent;
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
    assert(this->get_parent() == nullptr);
    return this;
}

LinkCutNode* LinkCutTree::join(LinkCutNode* v, LinkCutNode* w) {
    LinkCutNode* tail = v->get_tail();
    LinkCutNode* head = w->get_head();
    tail->set_use_edge_down(true);
    head->set_use_edge_up(true);
    tail->splay();
    head->splay();
    tail->link_right(head);
    return tail;
}

std::pair<LinkCutNode*, LinkCutNode*> LinkCutTree::split(LinkCutNode* v) {
    v->set_use_edge_down(false);
    v->splay();
    LinkCutNode* w = v->get_right();
    if (w != nullptr) {
        v->link_right(nullptr);
        w->set_parent(nullptr);
        w = w->get_head();
        w->set_use_edge_up(false);
        w->splay();
    }
    std::pair<LinkCutNode*, LinkCutNode*> paths = {v, w};
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

    LinkCutNode* p = v->splay();
    while(p->get_head()->get_dparent() != nullptr) {
        p = LinkCutTree::splice(p);
    }
    return p;
}

void LinkCutTree::evert(LinkCutNode* v) {
    LinkCutNode* p = LinkCutTree::expose(v);
    p->reverse();
}
