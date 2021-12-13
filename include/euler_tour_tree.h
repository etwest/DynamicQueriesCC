#include <Supernode.h>

struct NodeData {
    EulerTourTree *ett_ptrs[2]; // pointers to ETT aux tree (first and last)
    Supernode agg_sketch;       // aggregate sketch
}

class EulerTourTree {
    private:
        // Tree representation splay tree, bbst, or skip-list
        SplayTree aux_tree; // each node of tree points to a NodeData

    public:
        EulerTourTree(NodeData to_store);
        ~EulerTourTree();

        void link();
        void cut();

        void aggregate() { return aux_tree->root->agg_sketch; };
};
