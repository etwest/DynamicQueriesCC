#pragma once
#include "ufo_tree/types.h"
#include "ufo_tree/util.h"
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

/* These constants determines the maximum size of array of nieghbors and
the vector of neighbors for each UFOCluster. Any additional neighbors will
be stored in the hash set for efficiency. Minimum value is 3 for queries
function correctly. */
#define UFO_ARRAY_MAX 3

// #define COLLECT_ROOT_CLUSTER_STATS
#ifdef COLLECT_ROOT_CLUSTER_STATS
    static std::map<int, int> root_clusters_histogram;
#endif


namespace ufo {

template<typename v_t, typename e_t>
class UFOCluster {
using Cluster = UFOCluster<v_t, e_t>;
using NeighborSet = absl::flat_hash_map<Cluster*,e_t>;
public:
    // Query fields, note that the [[no_unique_address]] fields must be declared first
    [[no_unique_address]] e_t edge_value1;
    [[no_unique_address]] e_t edge_value2;
    [[no_unique_address]] e_t edge_value3;
    [[no_unique_address]] v_t value;
    // Parent pointer
    Cluster* parent = nullptr;
    /* We tag the last neighbor pointer in the array with information about the degree of the cluster.
    If it is 1, 2, or 3, that is the degree of the cluster. If it is 4, then the cluster has degree 4
    or higher and the last neighbor pointer is actually a pointer to the NeighborsSet object containing
    the remaining neighbors of the cluster. */
    Cluster* neighbors[UFO_ARRAY_MAX];
    int degree = 0;
    int fanout = 0;
    // Constructors
    UFOCluster() : parent(), neighbors(), degree(), fanout(), edge_value1(), edge_value2(), edge_value3(), value() {};
    UFOCluster(v_t val) : parent(), neighbors(), degree(), fanout(), edge_value1(), edge_value2(), edge_value3(), value(val) {};
    // Helper functions
    Cluster* get_root();
    bool contracts();
    int get_degree();
    bool has_neighbor_set();
    NeighborSet* get_neighbor_set();
    bool parent_high_fanout();
    bool contains_neighbor(Cluster* c);
    void insert_neighbor(Cluster* c);
    void insert_neighbor_with_value(Cluster* c, e_t value);
    void remove_neighbor(Cluster* c);
    void set_edge_value(int index, e_t value);
    e_t get_edge_value(int index);
    size_t calculate_size();
};

template<typename v_t, typename e_t>
UFOCluster<v_t,e_t>* UFOCluster<v_t,e_t>::get_root() {
    Cluster* curr = this;
    while (curr->parent) curr = curr->parent;
    return curr;
}

template<typename v_t, typename e_t>
bool UFOCluster<v_t,e_t>::contracts() {
    assert(get_degree() <= UFO_ARRAY_MAX);
    for (auto neighborp : neighbors) {
        auto neighbor = UNTAG(neighborp);
        if (neighbor && neighbor->parent == parent) return true;
    }
    return false;
}

template<typename v_t, typename e_t>
int UFOCluster<v_t,e_t>::get_degree() {
    int tag = GET_TAG(neighbors[UFO_ARRAY_MAX-1]);
    if (tag <= 3) [[likely]] return tag;
    return 2 + get_neighbor_set()->size();
}

template<typename v_t, typename e_t>
bool UFOCluster<v_t,e_t>::has_neighbor_set() {
    int tag = GET_TAG(neighbors[UFO_ARRAY_MAX-1]);
    if (tag <= 3) [[likely]] return false;
    return true;
}

template<typename v_t, typename e_t>
absl::flat_hash_map<UFOCluster<v_t, e_t>*,e_t>* UFOCluster<v_t,e_t>::get_neighbor_set() {
    return (NeighborSet*) UNTAG(neighbors[UFO_ARRAY_MAX-1]);
}

template<typename v_t, typename e_t>
bool UFOCluster<v_t,e_t>::parent_high_fanout() {
    assert(parent);
    int parent_degree = parent->get_degree();
    if (get_degree() == 1) {
        auto neighbor = neighbors[0];
        if (neighbor->parent == parent)
        if (neighbor->get_degree() - parent_degree > 2) return true;
    } else {
        if (get_degree() - parent_degree > 2) return true;
    }
    return false;
}

template<typename v_t, typename e_t>
bool UFOCluster<v_t,e_t>::contains_neighbor(Cluster* c) {
    for (auto neighbor : neighbors) if (UNTAG(neighbor) == c) return true;
    if (has_neighbor_set() && get_neighbor_set()->find(c) != get_neighbor_set()->end()) return true;
    return false;
}

template<typename v_t, typename e_t>
void UFOCluster<v_t,e_t>::insert_neighbor(Cluster* c) {
    assert(!contains_neighbor(c));
    // degree++;
    for (int i = 0; i < UFO_ARRAY_MAX; ++i) {
        if (UNTAG(neighbors[i]) == nullptr) [[likely]] {
            int deg = GET_TAG(neighbors[UFO_ARRAY_MAX-1]);
            neighbors[i] = c;
            neighbors[UFO_ARRAY_MAX-1] = TAG(UNTAG(neighbors[UFO_ARRAY_MAX-1]), deg+1);
            return;
        }
    }
    if (!has_neighbor_set()) {
        auto neighbor_set = new NeighborSet();
        std::pair<Cluster*,e_t> insert_pair;
        insert_pair.first = UNTAG(neighbors[UFO_ARRAY_MAX-1]);
        neighbor_set->insert(insert_pair);
        neighbors[UFO_ARRAY_MAX-1] = TAG(neighbor_set, 4);
    }
    std::pair<Cluster*,e_t> insert_pair;
    insert_pair.first = c;
    get_neighbor_set()->insert(insert_pair);
}

template<typename v_t, typename e_t>
void UFOCluster<v_t,e_t>::insert_neighbor_with_value(Cluster* c, e_t value) {
    if constexpr (!std::is_same<e_t, empty_t>::value) {
        assert(!contains_neighbor(c));
        // degree++;
        for (int i = 0; i < UFO_ARRAY_MAX; ++i) {
            if (UNTAG(neighbors[i]) == nullptr) [[likely]] {
                int deg = GET_TAG(neighbors[UFO_ARRAY_MAX-1]);
                neighbors[i] = c;
                set_edge_value(i, value);
                neighbors[UFO_ARRAY_MAX-1] = TAG(UNTAG(neighbors[UFO_ARRAY_MAX-1]), deg+1);
                return;
            }
        }
        if (!has_neighbor_set()) {
            auto neighbor_set = new NeighborSet();
            neighbor_set->insert({UNTAG(neighbors[UFO_ARRAY_MAX-1]), get_edge_value(UFO_ARRAY_MAX-1)});
            neighbors[UFO_ARRAY_MAX-1] = TAG(neighbor_set, 4);
        }
        get_neighbor_set()->insert({c,value});
    }
}

template<typename v_t, typename e_t>
void UFOCluster<v_t,e_t>::remove_neighbor(Cluster* c) {
    assert(contains_neighbor(c));
    // degree--;
    for (int i = 0; i < UFO_ARRAY_MAX; ++i) {
        if (UNTAG(neighbors[i]) == c) {
            neighbors[i] = TAG(nullptr, GET_TAG(neighbors[i]));
            if (has_neighbor_set()) [[unlikely]] { // Put an element from the set into the array
                auto neighbor_set = get_neighbor_set();
                auto replacement = *neighbor_set->begin();
                neighbors[i] = replacement.first;
                if constexpr (!std::is_same<e_t,empty_t>::value)
                    set_edge_value(i, replacement.second);
                neighbor_set->erase(replacement.first);
                if (neighbor_set->size() == 1) {
                    auto temp = *neighbor_set->begin();
                    delete neighbor_set;
                    neighbors[UFO_ARRAY_MAX-1] = TAG(temp.first, 3);
                    if constexpr (!std::is_same<e_t,empty_t>::value)
                        set_edge_value(UFO_ARRAY_MAX-1, temp.second);
                }
            } else [[likely]] {
                for (int j = UFO_ARRAY_MAX-1; j > i; --j) {
                    if (UNTAG(neighbors[j])) [[unlikely]] {
                        neighbors[i] = UNTAG(neighbors[j]);
                        neighbors[j] = TAG(nullptr, GET_TAG(neighbors[j]));
                        if constexpr (!std::is_same<e_t,empty_t>::value)
                            set_edge_value(i, get_edge_value(j));
                        break;
                    }
                }
                neighbors[UFO_ARRAY_MAX-1] = TAG(UNTAG(neighbors[UFO_ARRAY_MAX-1]), GET_TAG(neighbors[UFO_ARRAY_MAX-1])-1);
            }
            return;
        }
    }
    auto neighbor_set = get_neighbor_set();
    neighbor_set->erase(c);
    if (neighbor_set->size() == 1) {
        auto temp = *neighbor_set->begin();
        delete neighbor_set;
        neighbors[UFO_ARRAY_MAX-1] = TAG(temp.first, 3);
        if constexpr (!std::is_same<e_t,empty_t>::value)
            set_edge_value(UFO_ARRAY_MAX-1, temp.second);
    }
}

template<typename v_t, typename e_t>
void UFOCluster<v_t,e_t>::set_edge_value(int index, e_t value) {
    e_t* address = &edge_value1 + index;
    *address = value;
}

template<typename v_t, typename e_t>
e_t UFOCluster<v_t,e_t>::get_edge_value(int index) {
    e_t* address = &edge_value1 + index;
    return *address;
}

template<typename v_t, typename e_t>
size_t UFOCluster<v_t,e_t>::calculate_size() {
    size_t memory = sizeof(UFOCluster<v_t, e_t>);
    if (has_neighbor_set()) memory += get_neighbor_set()->bucket_count() * sizeof(std::pair<Cluster*, e_t>);
    return memory;
}

}
