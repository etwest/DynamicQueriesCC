#include "../include/link_cut_tree.h"

void LinkCutNode::set_parent(LinkCutNode* parent) { this->parent = parent; }
void LinkCutNode::set_dparent(LinkCutNode* dparent) { this->dparent = dparent; }
void LinkCutNode::set_left(LinkCutNode* left) { this->left = left; }
void LinkCutNode::set_right(LinkCutNode* right){ this->right = right; }
void LinkCutNode::set_edge_weight(uint32_t weight){ this->edge_weight = weight; }
void LinkCutNode::set_max(uint32_t weight){ this->max = weight; }
void LinkCutNode::set_reversed(bool reversed){ this->reversed = reversed; }

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
            curr->set_left(curr->right);
            curr->set_right(temp);
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
    //Perform rotations
    if (grandparent) {
        if (grandparent->left == parent) {
            grandparent->set_left(this);
        } else {
            grandparent->set_right(this);
        }
    }
    
    if (parent->left == this) {
        parent->set_left(this->right);
        this->set_right(parent);
    } else {
        parent->set_right(this->left);
        this->set_left(parent);
    }
    //Rebuild max aggregates
    parent->rebuild_max();
    this->rebuild_max();
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
