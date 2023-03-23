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
        other->set_parent(this);
    }
    this->rebuild_max();
    assert(this->get_left() == nullptr || this->get_left()->get_parent() == this);
}

void LinkCutNode::link_right(LinkCutNode* other) {
    this->right = other;
    if (other != nullptr) {
        other->set_parent(this);
    }
    this->rebuild_max();
    assert(this->get_right() == nullptr || this->get_right()->get_parent() == this);
}

LinkCutNode* LinkCutNode::get_left() { return this->left; }
LinkCutNode* LinkCutNode::get_right() { return this->right; }
LinkCutNode* LinkCutNode::get_parent() { return this->parent; }
LinkCutNode* LinkCutNode::get_dparent() { return this->dparent; }
LinkCutNode* LinkCutNode::get_head() { return this->head; }
LinkCutNode* LinkCutNode::get_tail() { return this->tail; }
bool LinkCutNode::get_reversed() { return this->reversed; }

LinkCutNode* LinkCutNode::recompute_head() {
    LinkCutNode* curr = this;
    LinkCutNode* next;
    bool reversal_state = false;
    while (curr != nullptr) {
        reversal_state = reversal_state != curr->reversed;
        if((next = reversal_state ? curr->right : curr->left) == nullptr) { this->head = curr; }
        curr = next;
    }
    return this->head;
}

LinkCutNode* LinkCutNode::recompute_tail() {
    LinkCutNode* curr = this;
    LinkCutNode* next;
    bool reversal_state = false;
    while (curr != nullptr) {
        reversal_state = reversal_state != curr->reversed;
        if((next = reversal_state ? curr->left : curr->right) == nullptr) { this->tail = curr; }
        curr = next;
    }
    return this->tail;
}

void inorder(LinkCutNode* node, std::vector<LinkCutNode*>& nodes, bool reversal_state) {
    if (node != nullptr) {
        reversal_state = reversal_state != node->get_reversed();
        if (!reversal_state) {
            inorder(node->get_left(), nodes, reversal_state);
            nodes.push_back(node);
            inorder(node->get_right(), nodes, reversal_state);
        } else {
            inorder(node->get_right(), nodes, reversal_state);
            nodes.push_back(node);
            inorder(node->get_left(), nodes, reversal_state);
        }
    }
}

std::vector<LinkCutNode*> get_inorder(LinkCutNode* node) {
    LinkCutNode* curr = node;
    LinkCutNode* root;
    while (curr) {
        if (curr->get_parent() == nullptr) { root = curr; }
        curr = curr->get_parent();
    }
    std::vector<LinkCutNode*> nodes;
    inorder(root, nodes, false);
    return nodes;
}

void LinkCutNode::correct_reversals() {
    std::vector<LinkCutNode*> inorder = get_inorder(this);
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
        bool next_reversal_state = reversal_state != curr->reversed;
        if (reversal_state) {
            LinkCutNode* temp = curr->left;
            curr->left = curr->right;
            curr->right = temp;
            if (curr->left && curr->left != prev) {
                curr->left->set_reversed(!curr->left->reversed);
            }
            if (curr->right && curr->right != prev) {
                curr->right->set_reversed(!curr->right->reversed);
            }
        }
        curr->set_reversed(false);
        reversal_state = next_reversal_state;
        prev = curr;
        curr = curr->parent;
    }
    assert(inorder == get_inorder(this));
}

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
    this->recompute_head();
    this->recompute_tail();
    assert(this->get_parent() == nullptr);
    return this;
}

LinkCutTree::LinkCutTree(node_id_t num_nodes) {
    this->nodes.reserve(num_nodes);
    for (uint32_t i = 0; i < num_nodes; i++) {
        this->nodes.emplace_back();
    }
}

LinkCutNode* LinkCutTree::join(LinkCutNode* v, LinkCutNode* w) {
    assert(v != nullptr && w != nullptr && v->get_parent() == nullptr && w->get_parent() == nullptr);
    LinkCutNode* tail = v->get_tail();
    LinkCutNode* head = w->get_head();
    tail->set_use_edge_down(true);
    head->set_use_edge_up(true);
    tail->splay();
    head->splay(); // To recompute the aggregate
    assert(tail->get_right() == nullptr);
    tail->link_right(head);
    tail->recompute_head();
    tail->recompute_tail();
    return tail;
}

std::pair<LinkCutNode*, LinkCutNode*> LinkCutTree::split(LinkCutNode* v) {
    assert(v != nullptr);
    v->set_use_edge_down(false);
    v->splay();
    LinkCutNode* w = v->get_right();
    if (w != nullptr) {
        v->link_right(nullptr);
        w->set_parent(nullptr);
        w = w->recompute_head();
        w->set_use_edge_up(false);
        w->splay(); // Recompute the aggregate
        w->recompute_head();
        w->recompute_tail();
    }
    v->recompute_head();
    v->recompute_tail();
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

    LinkCutNode* p = paths.first;
    while(p->get_head()->get_dparent() != nullptr) {
        p = LinkCutTree::splice(p);
    }
    return p;
}

LinkCutNode* LinkCutTree::evert(LinkCutNode* v) {
    LinkCutNode* p = LinkCutTree::expose(v);
    p->reverse();
    return p;
}

void LinkCutTree::link(node_id_t v, node_id_t w, uint32_t weight) {
    assert(find_root(v) != find_root(w));
    LinkCutNode* v_node = &this->nodes[v];
    LinkCutNode* w_node = &this->nodes[w];
    v_node->set_edge_weight_down(weight);
    w_node->set_edge_weight_up(weight);
    LinkCutTree::join(LinkCutTree::expose(v_node), LinkCutTree::evert(w_node));
}

void LinkCutTree::cut(node_id_t v, node_id_t w) {
    assert(find_root(v) == find_root(w));
    LinkCutNode* v_node = &this->nodes[v];
    LinkCutNode* w_node = &this->nodes[w];
    v_node->set_edge_weight_down(0);
    w_node->set_edge_weight_up(0);
    LinkCutTree::evert(v_node);
    LinkCutTree::expose(v_node);
    w_node->set_dparent(nullptr);
}

void* LinkCutTree::find_root(node_id_t v) {
    return expose(&this->nodes[v])->get_head();
}
