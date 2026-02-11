#pragma once
#include "ufo_tree/types.h"
#include "ufo_tree/util.h"
#include "ufo_tree/ufo_cluster.h"
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>


namespace ufo {

template<typename v_t, typename e_t>
class UFOTree {
using Cluster = UFOCluster<v_t, e_t>;
public:
    // UFO tree interface
    UFOTree(
        vertex_t n, QueryType q = CONNECTIVITY,
        std::function<v_t(v_t, v_t)> f_v = [](v_t x, v_t y) -> v_t {return x;},
        std::function<e_t(e_t, e_t)> f_e = [](e_t x, e_t y) -> e_t {return x;});
    UFOTree(
        vertex_t n, QueryType q,
        std::function<v_t(v_t, v_t)> f_v, std::function<e_t(e_t, e_t)> f_e,
        v_t id_v, e_t id_e, v_t dval_v, e_t dval_e);
    UFOTree(int n, QueryType q, std::function<v_t(v_t, v_t)> f, v_t id, v_t d_val);
    ~UFOTree();
    void link(vertex_t u, vertex_t v);
    void link(vertex_t u, vertex_t v, e_t value);
    void cut(vertex_t u, vertex_t v);
    bool connected(vertex_t u, vertex_t v);
    e_t path_query(vertex_t u, vertex_t v);
    // Testing helpers
    size_t space();
    size_t count_nodes();
    size_t get_height();
    bool is_valid();
    void print_tree();
private:
    // Class data and parameters
    std::vector<Cluster> leaves;
    std::vector<std::vector<Cluster*>> root_clusters;
    int max_level;
    std::vector<std::pair<Cluster*,vertex_t>> lower_deg[2]; // lower_deg helps to identify clusters who became low degree during a deletion update
    QueryType query_type;
    std::function<v_t(v_t, v_t)> f_v;
    v_t identity_v;
    v_t default_v;
    std::function<e_t(e_t, e_t)> f_e;
    e_t identity_e;
    e_t default_e;
    // We preallocate UFO clusters and store unused clusters in free_clusters
    std::vector<Cluster*> free_clusters;
    Cluster* allocate_cluster();
    void free_cluster(Cluster* c);
    // Helper functions
    void remove_ancestors(Cluster* c, int start_level = 0);
    void recluster_tree();
    bool is_high_degree_or_high_fanout(Cluster* cluster, Cluster* child, int level);
    void disconnect_siblings(Cluster* c, int level);
    void insert_adjacency(Cluster* u, Cluster* v);
    void insert_adjacency(Cluster* u, Cluster* v, e_t value);
    void remove_adjacency(Cluster* u, Cluster* v);
};

template<typename v_t, typename e_t>
UFOTree<v_t, e_t>::UFOTree(vertex_t n, QueryType q,
        std::function<v_t(v_t, v_t)> f_v, std::function<e_t(e_t, e_t)> f_e)
    : query_type(q), f_v(f_v), f_e(f_e) {
    leaves.resize(n);
    root_clusters.resize(max_tree_height(n));
    for (int i = 0; i < n; ++i)
        free_clusters.push_back(new Cluster());
}

template<typename v_t, typename e_t>
UFOTree<v_t, e_t>::UFOTree(vertex_t n, QueryType q,
        std::function<v_t(v_t, v_t)> f_v, std::function<e_t(e_t, e_t)> f_e,
        v_t id_v, e_t id_e, v_t dval_v, e_t dval_e)
    : query_type(q), f_v(f_v), f_e(f_e), identity_v(id_v), identity_e(id_e),
     default_v(dval_v), default_e(dval_e) {
    leaves.resize(n, default_v);
    root_clusters.resize(max_tree_height(n));
    for (int i = 0; i < n; ++i)
        free_clusters.push_back(new Cluster());
}

template<typename v_t, typename e_t>
UFOTree<v_t, e_t>::UFOTree(int n, QueryType q,
        std::function<v_t(v_t, v_t)> f, v_t id, v_t d_val)
    : query_type(q), f_v(f), identity_v(id), default_v(d_val) {
    if constexpr (std::is_same<v_t,e_t>::value) {
        f_e = f;
        identity_e = id;
        default_e = d_val;
    }
    leaves.resize(n, default_v);
    root_clusters.resize(max_tree_height(n));
    for (int i = 0; i < n; ++i)
        free_clusters.push_back(new Cluster());
}

template<typename v_t, typename e_t>
UFOTree<v_t, e_t>::~UFOTree() {
    // Clear all memory
    std::unordered_set<Cluster*> clusters;
    for (auto leaf : leaves) {
        auto curr = leaf.parent;
        while (curr) {
            clusters.insert(curr);
            curr = curr->parent;
        }
    }
    for (auto cluster : clusters) delete cluster;
    for (auto cluster : free_clusters) delete cluster;
    #ifdef COLLECT_ROOT_CLUSTER_STATS
    std::cout << "Number of root clusters: Frequency" << std::endl;
        for (auto entry : root_clusters_histogram)
            std::cout << entry.first << "\t" << entry.second << std::endl;
    #endif
}

template<typename v_t, typename e_t>
UFOCluster<v_t, e_t>* UFOTree<v_t, e_t>::allocate_cluster() {
    if (!free_clusters.empty()) {
        auto c = free_clusters.back();
        free_clusters.pop_back();
        return c;
    }
    return new Cluster();
}

template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::free_cluster(UFOCluster<v_t, e_t>* c) {
    c->parent = nullptr;
    if (c->has_neighbor_set()) [[unlikely]] delete c->get_neighbor_set();
    for (int i = 0; i < UFO_ARRAY_MAX; ++i)
        c->neighbors[i] = nullptr;
    c->degree = 0;
    c->fanout = 0;
    free_clusters.push_back(c);
}

template<typename v_t, typename e_t>
size_t UFOTree<v_t, e_t>::space() {
    std::unordered_set<Cluster*> visited;
    size_t memory = sizeof(UFOTree<v_t, e_t>);
    for (auto cluster : leaves) {
        memory += cluster.calculate_size();
        auto parent = cluster.parent;
        while (parent != nullptr && visited.count(parent) == 0) {
            memory += parent->calculate_size();
            visited.insert(parent);
            parent = parent->parent;
        }
    }
    return memory;
}

template<typename v_t, typename e_t>
size_t UFOTree<v_t, e_t>::count_nodes() {
    std::unordered_set<Cluster*> visited;
    size_t node_count = 0;
    for(auto cluster : leaves){
        node_count += 1;
        auto parent = cluster.parent;
        while(parent != nullptr && visited.count(parent) == 0){
            node_count += 1;
            visited.insert(parent);
            parent = parent->parent;
        }
    }
    return node_count;
}

template<typename v_t, typename e_t>
size_t UFOTree<v_t, e_t>::get_height() {
    size_t max_height = 0;
    for (vertex_t v = 0; v < leaves.size(); ++v) {
        size_t height = 0;
        Cluster* curr = &leaves[v];
        while (curr) {
            height++;
            curr = curr->parent;
        }
        max_height = std::max(max_height, height);
    }
    return max_height;
}

/* Link vertex u and vertex v in the tree. Optionally include an
augmented value for the new edge (u,v). If no augmented value is
provided, the default value is 1. */
template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::link(vertex_t u, vertex_t v) {
    assert(u >= 0 && u < leaves.size() && v >= 0 && v < leaves.size());
    assert(u != v && !connected(u,v));
    max_level = 0;
    remove_ancestors(&leaves[u]);
    remove_ancestors(&leaves[v]);
    insert_adjacency(&leaves[u], &leaves[v]);
    recluster_tree();
}
template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::link(vertex_t u, vertex_t v, e_t value) {
    assert(u >= 0 && u < leaves.size() && v >= 0 && v < leaves.size());
    assert(u != v && !connected(u,v));
    max_level = 0;
    remove_ancestors(&leaves[u]);
    remove_ancestors(&leaves[v]);
    insert_adjacency(&leaves[u], &leaves[v], value);
    recluster_tree();
}

/* Cut vertex u and vertex v in the tree. */
template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::cut(vertex_t u, vertex_t v) {
    assert(u >= 0 && u < leaves.size() && v >= 0 && v < leaves.size());
    assert(leaves[u].contains_neighbor(&leaves[v]));
    max_level = 0;
    auto curr_u = &leaves[u];
    auto curr_v = &leaves[v];
    while (curr_u != curr_v) {
        lower_deg[0].push_back({curr_u, curr_u->get_degree()-1});
        lower_deg[1].push_back({curr_v, curr_v->get_degree()-1});
        curr_u->degree = curr_u->get_degree()-1;
        curr_v->degree = curr_v->get_degree()-1;
        curr_u = curr_u->parent;
        curr_v = curr_v->parent;
    }
    remove_ancestors(&leaves[u]);
    remove_ancestors(&leaves[v]);
    for (auto cluster: lower_deg[0]) cluster.first->degree = 0;
    for (auto cluster: lower_deg[1]) cluster.first->degree = 0;
    lower_deg[0].clear();
    lower_deg[1].clear();
    remove_adjacency(&leaves[u], &leaves[v]);
    recluster_tree();
}

/* Removes the ancestors of cluster c that are not high degree nor
high fan-out and add them to root_clusters. */
template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::remove_ancestors(Cluster* c, int start_level) {
    int level = start_level; // level is always the level of cluster prev, 0 being the leaves
    auto prev = c;
    auto curr = c->parent;
    bool del = false;
    while (curr) {
        // Different cases for if curr will or will not be deleted later
        if (!is_high_degree_or_high_fanout(curr, prev, level)) [[likely]] { // We will delete curr next round
            disconnect_siblings(prev, level);
            if (del) [[likely]] { // Possibly delete prev
                assert(prev->get_degree() <= UFO_ARRAY_MAX);
                for (auto neighborp : prev->neighbors) {
                    auto neighbor = UNTAG(neighborp);
                    if (neighbor) neighbor->remove_neighbor(prev); // Remove prev from adjacency
                }
                auto position = std::find(root_clusters[level].begin(), root_clusters[level].end(), prev);
                if (position != root_clusters[level].end()) root_clusters[level].erase(position);
                free_cluster(prev);
            } else [[unlikely]] {
                prev->parent = nullptr;
                curr->fanout--;
                root_clusters[level].push_back(prev);
            }
            del = true;
        } else [[unlikely]] { // We will not delete curr next round
            if (del) [[likely]] { // Possibly delete prev
                assert(prev->get_degree() <= UFO_ARRAY_MAX);
                for (auto neighborp : prev->neighbors) {
                    auto neighbor = UNTAG(neighborp);
                    if (neighbor) neighbor->remove_neighbor(prev); // Remove prev from adjacency
                }
                auto position = std::find(root_clusters[level].begin(), root_clusters[level].end(), prev);
                if (position != root_clusters[level].end()) root_clusters[level].erase(position);
                free_cluster(prev);
                curr->fanout--;
            } else [[unlikely]] if (prev->get_degree() <= 1) {
                prev->parent = nullptr;
                curr->fanout--;
                root_clusters[level].push_back(prev);
            }
            del = false;
        }
        // Update pointers
        prev = curr;
        curr = prev->parent;
        level++;
    }
    // DO LAST DELETIONS
    if (del) [[likely]] { // Possibly delete prev
        assert(prev->get_degree() <= UFO_ARRAY_MAX);
        for (auto neighborp : prev->neighbors) {
            auto neighbor = UNTAG(neighborp);
            if (neighbor) neighbor->remove_neighbor(prev); // Remove prev from adjacency
        }
        auto position = std::find(root_clusters[level].begin(), root_clusters[level].end(), prev);
        if (position != root_clusters[level].end()) root_clusters[level].erase(position);
        free_cluster(prev);
    } else [[unlikely]] root_clusters[level].push_back(prev);
    if (level > max_level) max_level = level;
}

template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::recluster_tree() {
    for (int level = 0; level <= max_level; level++) {
        if (root_clusters[level].empty()) [[unlikely]] continue;
        // Update root cluster stats if we are collecting them
        #ifdef COLLECT_ROOT_CLUSTER_STATS
            if (root_clusters_histogram.find(root_clusters[level].size()) == root_clusters_histogram.end())
                root_clusters_histogram[root_clusters[level].size()] = 1;
            else
                root_clusters_histogram[root_clusters[level].size()] += 1;
        #endif
        // Merge deg 3-5 root clusters with all of its deg 1 neighbors
        for (auto cluster : root_clusters[level]) {
            if (!cluster->parent && cluster->get_degree() > 2) [[unlikely]] {
                assert(cluster->get_degree() <= 5);
                auto parent = allocate_cluster();
                if constexpr (!std::is_same<e_t, empty_t>::value) {
                    parent->value = identity_v;
                }
                parent->fanout = 1;
                cluster->parent = parent;
                root_clusters[level+1].push_back(parent);
                assert(UFO_ARRAY_MAX >= 3);
                if (!cluster->has_neighbor_set()) [[likely]] {
                    for (int i = 0; i < UFO_ARRAY_MAX; ++i) {
                        auto neighbor = UNTAG(cluster->neighbors[i]);
                        if (neighbor->get_degree() == 1) [[unlikely]] {
                            auto curr = neighbor->parent;
                            int lev = level+1;
                            while (curr) {
                                auto temp = curr;
                                curr = curr->parent;
                                auto position = std::find(root_clusters[lev].begin(), root_clusters[lev].end(), temp);
                                if (position != root_clusters[lev].end()) root_clusters[lev].erase(position);
                                free_cluster(temp);
                                lev++;
                            }
                            neighbor->parent = cluster->parent;
                            parent->fanout++;
                        } else if (neighbor->parent) { // Populate new parent's neighbors
                            if constexpr (std::is_same<e_t, empty_t>::value) {
                                parent->insert_neighbor(neighbor->parent);
                                neighbor->parent->insert_neighbor(parent);
                            } else {
                                parent->insert_neighbor_with_value(neighbor->parent, cluster->get_edge_value(i));
                                neighbor->parent->insert_neighbor_with_value(parent, cluster->get_edge_value(i));
                            }
                        }
                    }
                } else [[unlikely]] {
                    for (int i = 0; i < UFO_ARRAY_MAX-1; ++i) {
                        auto neighbor = cluster->neighbors[i];
                        if (neighbor->get_degree() == 1) [[unlikely]] {
                            auto curr = neighbor->parent;
                            int lev = level+1;
                            while (curr) {
                                auto temp = curr;
                                curr = curr->parent;
                                auto position = std::find(root_clusters[lev].begin(), root_clusters[lev].end(), temp);
                                if (position != root_clusters[lev].end()) root_clusters[lev].erase(position);
                                free_cluster(temp);
                                lev++;
                            }
                            neighbor->parent = cluster->parent;
                            parent->fanout++;
                        } else if (neighbor->parent) { // Populate new parent's neighbors
                            if constexpr (std::is_same<e_t, empty_t>::value) {
                                parent->insert_neighbor(neighbor->parent);
                                neighbor->parent->insert_neighbor(parent);
                            } else {
                                parent->insert_neighbor_with_value(neighbor->parent, cluster->get_edge_value(i));
                                neighbor->parent->insert_neighbor_with_value(parent, cluster->get_edge_value(i));
                            }
                        }
                    }
                    for (auto neighbor_pair : *cluster->get_neighbor_set()) {
                        auto neighbor = neighbor_pair.first;
                        if (neighbor->get_degree() == 1) [[unlikely]] {
                            auto curr = neighbor->parent;
                            int lev = level+1;
                            while (curr) {
                                auto temp = curr;
                                curr = curr->parent;
                                auto position = std::find(root_clusters[lev].begin(), root_clusters[lev].end(), temp);
                                if (position != root_clusters[lev].end()) root_clusters[lev].erase(position);
                                free_cluster(temp);
                                lev++;
                            }
                            neighbor->parent = cluster->parent;
                            parent->fanout++;
                        } else if (neighbor->parent) { // Populate new parent's neighbors
                            if constexpr (std::is_same<e_t, empty_t>::value) {
                                parent->insert_neighbor(neighbor->parent);
                                neighbor->parent->insert_neighbor(parent);
                            } else {
                                parent->insert_neighbor_with_value(neighbor->parent, neighbor_pair.second);
                                neighbor->parent->insert_neighbor_with_value(parent, neighbor_pair.second);
                            }
                        }
                    }
                }
            }
        }
        // This loop handles all deg 2 and 1 root clusters
        for (auto cluster : root_clusters[level]) {
            // Combine deg 2 root clusters with deg 2 root clusters
            if (!cluster->parent && cluster->get_degree() == 2) [[unlikely]] {
                assert(UFO_ARRAY_MAX >= 2);
                for (int i = 0; i < 2; ++i) {
                    auto neighbor = cluster->neighbors[i];
                    if (!neighbor->parent && (neighbor->get_degree() == 2)) [[unlikely]] {
                        auto parent = allocate_cluster();
                        cluster->parent = parent;
                        neighbor->parent = parent;
                        parent->fanout = 2;
                        if constexpr (!std::is_same<e_t, empty_t>::value) { // Path query
                            parent->value = f_e(cluster->value, f_e(neighbor->value, cluster->get_edge_value(i)));
                        }
                        root_clusters[level+1].push_back(parent);
                        for (int i = 0; i < 2; ++i) { // Populate new parent's neighbors
                            if (cluster->neighbors[i]->parent && cluster->neighbors[i]->parent != parent) {
                                if constexpr (std::is_same<e_t, empty_t>::value) {
                                    parent->insert_neighbor(cluster->neighbors[i]->parent);
                                    cluster->neighbors[i]->parent->insert_neighbor(parent);
                                } else {
                                    parent->insert_neighbor_with_value(cluster->neighbors[i]->parent, cluster->get_edge_value(i));
                                    cluster->neighbors[i]->parent->insert_neighbor_with_value(parent, cluster->get_edge_value(i));
                                }
                            }
                            if (neighbor->neighbors[i]->parent && neighbor->neighbors[i]->parent != parent) {
                                if constexpr (std::is_same<e_t, empty_t>::value) {
                                    parent->insert_neighbor(neighbor->neighbors[i]->parent);
                                    neighbor->neighbors[i]->parent->insert_neighbor(parent);
                                } else {
                                    parent->insert_neighbor_with_value(neighbor->neighbors[i]->parent, neighbor->get_edge_value(i));
                                    neighbor->neighbors[i]->parent->insert_neighbor_with_value(parent, neighbor->get_edge_value(i));
                                }
                            }
                        }
                        break;
                    }
                }
                // Combine deg 2 root clusters with deg 1 or 2 non-root clusters
                if (!cluster->parent) [[unlikely]] {
                    assert(UFO_ARRAY_MAX >= 2);
                    for (int i = 0; i < 2; ++i) {
                        auto neighbor = cluster->neighbors[i];
                        if (neighbor->parent && (neighbor->get_degree() == 1 || neighbor->get_degree() == 2)) [[unlikely]] {
                            if (neighbor->contracts()) continue;
                            cluster->parent = neighbor->parent;
                            neighbor->parent->fanout++;
                            if constexpr (!std::is_same<e_t, empty_t>::value) { // Path query
                                cluster->parent->value = f_e(cluster->value, f_e(neighbor->value, cluster->get_edge_value(i)));
                            }
                            remove_ancestors(cluster->parent, level+1); // Recursive remove ancestor call
                            auto other_neighbor = cluster->neighbors[!i]; // Popoulate neighbors
                            // if (other_neighbor->parent && (long) other_neighbor->parent->parent != 1) {
                            if (other_neighbor->parent) {
                                if constexpr (std::is_same<e_t, empty_t>::value) {
                                    insert_adjacency(cluster->parent, other_neighbor->parent);
                                } else {
                                    insert_adjacency(cluster->parent, other_neighbor->parent, cluster->get_edge_value(!i));
                                }
                            }
                            break;
                        }
                    }
                }
            // Always combine deg 1 root clusters with its neighboring cluster
            } else if (!cluster->parent && cluster->get_degree() == 1) [[unlikely]] {
                auto neighbor = cluster->neighbors[0];
                if (neighbor->parent) {
                    if (neighbor->get_degree() == 2 && neighbor->contracts()) continue;
                    cluster->parent = neighbor->parent;
                    neighbor->parent->fanout++;
                    remove_ancestors(cluster->parent, level+1);
                } else {
                    auto parent = allocate_cluster();
                    cluster->parent = parent;
                    neighbor->parent = parent;
                    parent->fanout = 2;
                    if constexpr (!std::is_same<e_t, empty_t>::value) { // Path query
                        parent->value = identity_v;
                    }
                    for (int i = 0; i < 2; ++i) { // Populate new parent's neighbors
                        if (neighbor->neighbors[i] && neighbor->neighbors[i] != cluster && neighbor->neighbors[i]->parent) {
                            if constexpr (std::is_same<e_t, empty_t>::value) {
                                parent->insert_neighbor(neighbor->neighbors[i]->parent);
                                neighbor->neighbors[i]->parent->insert_neighbor(parent);
                            } else {
                                parent->insert_neighbor_with_value(neighbor->neighbors[i]->parent, neighbor->get_edge_value(i));
                                neighbor->neighbors[i]->parent->insert_neighbor_with_value(parent, neighbor->get_edge_value(i));
                            }
                        }
                    }
                    root_clusters[level+1].push_back(parent);
                }
            }
        }
        // Add remaining uncombined clusters to the next level
        for (auto cluster : root_clusters[level]) {
            if (!cluster->parent && cluster->get_degree() > 0) [[unlikely]] {
                auto parent = allocate_cluster();
                cluster->parent = parent;
                parent->fanout = 1;
                if constexpr (!std::is_same<v_t, empty_t>::value) { // Path query
                    parent->value = cluster->value;
                }
                for (int i = 0; i < 2; ++i) { // Populate new parent's neighbors
                    if (cluster->neighbors[i] && cluster->neighbors[i]->parent) {
                        if constexpr (std::is_same<e_t, empty_t>::value) {
                            parent->insert_neighbor(cluster->neighbors[i]->parent);
                            cluster->neighbors[i]->parent->insert_neighbor(parent);
                        } else {
                            parent->insert_neighbor_with_value(cluster->neighbors[i]->parent, cluster->get_edge_value(i));
                            cluster->neighbors[i]->parent->insert_neighbor_with_value(parent, cluster->get_edge_value(i));
                        }
                    }
                }
                root_clusters[level+1].push_back(parent);
            }
        }
        // Clear the contents of this level
        root_clusters[level].clear();
        if (level == max_level && !root_clusters[max_level+1].empty()) max_level++;
    }
}

template<typename v_t, typename e_t>
bool UFOTree<v_t, e_t>::is_high_degree_or_high_fanout(Cluster* cluster, Cluster* child, int level) {
    int cluster_degree = cluster->degree > 0 ? cluster->degree : cluster->get_degree();
    if (cluster_degree > 2) [[unlikely]] return true;
    if (!child->neighbors[1] && cluster->fanout > 2) [[unlikely]] return true;
    int child_degree = child->degree > 0 ? child->degree : child->get_degree();
    if (child_degree - cluster_degree > 2) [[unlikely]] return true;
    return false;
}

/* Helper function which takes a cluster c and the level of that cluster. The function
should find every cluster that shares a parent with c, disconnect it from their parent
and add it as a root cluster to be processed. */
template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::disconnect_siblings(Cluster* c, int level) {
    if (c->get_degree() == 1) {
        auto center = c->neighbors[0];
        if (center->parent && c->parent != center->parent) return;
        assert(center->get_degree() <= 5);
        if (!center->has_neighbor_set()) [[likely]] {
            for (auto neighborp : center->neighbors) {
                Cluster* neighbor = UNTAG(neighborp);
                if (neighbor && neighbor->parent == c->parent && neighbor != c) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
        } else [[unlikely]] {
            for (int i = 0; i < UFO_ARRAY_MAX-1; ++i) {
                Cluster* neighbor = center->neighbors[i];
                if (neighbor->parent == c->parent && neighbor != c) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
            for (auto neighbor_pair : *center->get_neighbor_set()) {
                Cluster* neighbor = neighbor_pair.first;
                if (neighbor && neighbor->parent == c->parent && neighbor != c) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
        }
        center->parent = nullptr;
        root_clusters[level].push_back(center);
    } else {
        assert(c->get_degree() <= 5);
        if (!c->has_neighbor_set()) [[likely]] {
            for (auto neighborp : c->neighbors) {
                Cluster* neighbor = UNTAG(neighborp);
                if (neighbor && neighbor->parent == c->parent) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
        } else [[unlikely]] {
            for (int i = 0; i < UFO_ARRAY_MAX-1; ++i) {
                Cluster* neighbor = c->neighbors[i];
                if (neighbor->parent == c->parent) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
            for (auto neighbor_pair : *c->get_neighbor_set()) {
                Cluster* neighbor = neighbor_pair.first;
                if (neighbor && neighbor->parent == c->parent) {
                    neighbor->parent = nullptr; // Set sibling parent pointer to null
                    root_clusters[level].push_back(neighbor); // Keep track of root clusters
                }
            }
        }
    }
}

template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::insert_adjacency(Cluster* u, Cluster* v) {
    auto curr_u = u;
    auto curr_v = v;
    while (curr_u && curr_v && curr_u != curr_v) {
        curr_u->insert_neighbor(curr_v);
        curr_v->insert_neighbor(curr_u);
        curr_u = curr_u->parent;
        curr_v = curr_v->parent;
    }
}

template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::insert_adjacency(Cluster* u, Cluster* v, e_t value) {
    auto curr_u = u;
    auto curr_v = v;
    while (curr_u && curr_v && curr_u != curr_v) {
        curr_u->insert_neighbor_with_value(curr_v, value);
        curr_v->insert_neighbor_with_value(curr_u, value);
        curr_u = curr_u->parent;
        curr_v = curr_v->parent;
    }
}

template<typename v_t, typename e_t>
void UFOTree<v_t, e_t>::remove_adjacency(Cluster* u, Cluster* v) {
    auto curr_u = u;
    auto curr_v = v;
    while (curr_u && curr_v && curr_u != curr_v) {
        curr_u->remove_neighbor(curr_v);
        curr_v->remove_neighbor(curr_u);
        // curr_u->degree = 0;
        // curr_v->degree = 0;
        curr_u = curr_u->parent;
        curr_v = curr_v->parent;
    }
}

/* Return true if and only if there is a path from vertex u to
vertex v in the tree. */
template<typename v_t, typename e_t>
bool UFOTree<v_t, e_t>::connected(vertex_t u, vertex_t v) {
    return leaves[u].get_root() == leaves[v].get_root();
}

template<typename v_t, typename e_t>
e_t UFOTree<v_t, e_t>::path_query(vertex_t u, vertex_t v) {
    assert(u < leaves.size() && u >= 0 && v < leaves.size() && v >= 0 && u != v && connected(u, v)); 

    e_t path_u1, path_u2, path_v1, path_v2;
    path_u1 = path_u2 = path_v1 = path_v2 = identity_e;
    Cluster *bdry_u1, *bdry_u2, *bdry_v1, *bdry_v2;
    bdry_u1 = bdry_u2 = bdry_v1 = bdry_v2 = nullptr;
    if (leaves[u].get_degree() == 2) {
        bdry_u1 = leaves[u].neighbors[0];
        bdry_u2 = leaves[u].neighbors[1];
    }
    if (leaves[v].get_degree() == 2) {
        bdry_v1 = leaves[v].neighbors[0];
        bdry_v2 = leaves[v].neighbors[1];
    }
    auto curr_u = &leaves[u];
    auto curr_v = &leaves[v];
    while (curr_u->parent != curr_v->parent) {
        // NOTE(ATHARVA): Make this all into one function.
        if (curr_u->get_degree() > 2) {
            if (curr_u->parent->get_degree() == 2) {
                // Superunary to Binary
                bdry_u1 = curr_u->parent->neighbors[0];
                bdry_u2 = curr_u->parent->neighbors[1];
                path_u2 = path_u1;
            }
        } else {
            for (int i = 0; i < 2; i++) {
                auto neighbor = curr_u->neighbors[i];
                if (neighbor && neighbor->parent == curr_u->parent) {
                    if (curr_u->get_degree() == 2) {
                        if (curr_u->parent->get_degree() == 2) {
                            // Binary to Binary
                            if (neighbor == bdry_u1) {
                                path_u1 = f_e(path_u1, f_e(curr_u->get_edge_value(i), neighbor->value));
                                bdry_u2 = bdry_u2->parent;
                                for (int i = 0; i < 2; i++)
                                    if (curr_u->parent->neighbors[i] && curr_u->parent->neighbors[i] != bdry_u2)
                                        bdry_u1 = curr_u->parent->neighbors[i];
                            } else {
                                path_u2 = f_e(path_u2, f_e(curr_u->get_edge_value(i), neighbor->value));
                                bdry_u1 = bdry_u1->parent;
                                for (int i = 0; i < 2; i++)
                                    if (curr_u->parent->neighbors[i] && curr_u->parent->neighbors[i] != bdry_u1)
                                        bdry_u2 = curr_u->parent->neighbors[i];
                            }
                        } else {
                            // Binary to Unary
                            path_u1 = (neighbor == bdry_u1) ? path_u2 : path_u1;
                        }
                    } else {
                        if (curr_u->parent->get_degree() == 2) {
                            // Unary to Binary
                            path_u1 = path_u2 = f_e(path_u1, curr_u->get_edge_value(i));
                            bdry_u1 = curr_u->parent->neighbors[0];
                            bdry_u2 = curr_u->parent->neighbors[1];
                        } else {
                            // Unary to Unary and Unary to Superunary
                            path_u1 = f_e(path_u1, f_e(curr_u->get_edge_value(i), neighbor->value));
                        }
                    }
                    break;
                }
            }
            if (!curr_u->contracts()) {
                if (bdry_u1) bdry_u1 = bdry_u1->parent;
                if (bdry_u2) bdry_u2 = bdry_u2->parent;
            }
        }
        curr_u = curr_u->parent;
        // Same thing for the side of curr_v
        if (curr_v->get_degree() > 2) {
            if (curr_v->parent->get_degree() == 2) {
                // Superunary to Superunary/Binary
                bdry_v1 = curr_v->parent->neighbors[0];
                bdry_v2 = curr_v->parent->neighbors[1];
                path_v2 = path_v1;
            }
        } else {
            for (int i = 0; i < 2; i++) {
                auto neighbor = curr_v->neighbors[i];
                if (neighbor && neighbor->parent == curr_v->parent) {
                    if (curr_v->get_degree() == 2) {
                        if (curr_v->parent->get_degree() == 2) {
                            // Binary to Binary
                            if (neighbor == bdry_v1) {
                                path_v1 = f_e(path_v1, f_e(curr_v->get_edge_value(i), neighbor->value));
                                bdry_v2 = bdry_v2->parent;
                                for (int i = 0; i < 2; i++)
                                    if (curr_v->parent->neighbors[i] && curr_v->parent->neighbors[i] != bdry_v2)
                                        bdry_v1 = curr_v->parent->neighbors[i];
                            } else {
                                path_v2 = f_e(path_v2, f_e(curr_v->get_edge_value(i), neighbor->value));
                                bdry_v1 = bdry_v1->parent;
                                for (int i = 0; i < 2; i++)
                                    if (curr_v->parent->neighbors[i] && curr_v->parent->neighbors[i] != bdry_v1)
                                        bdry_v2 = curr_v->parent->neighbors[i];
                            }
                        } else {
                            // Binary to Unary
                            path_v1 = (neighbor == bdry_v1) ? path_v2 : path_v1;
                        }
                    } else {
                        if (curr_v->parent->get_degree() == 2) {
                            // Unary to Binary
                            path_v1 = path_v2 = f_e(path_v1, curr_v->get_edge_value(i));
                            bdry_v1 = curr_v->parent->neighbors[0];
                            bdry_v2 = curr_v->parent->neighbors[1];
                        } else {
                            // Unary to Unary and Unary to Superunary
                            path_v1 = f_e(path_v1, f_e(curr_v->get_edge_value(i), neighbor->value));
                        }
                    }
                    break;
                }
            }
            if (!curr_v->contracts()) {
                if (bdry_v1) bdry_v1 = bdry_v1->parent;
                if (bdry_v2) bdry_v2 = bdry_v2->parent;
            }
        }
        curr_v = curr_v->parent;
    }
    // Get the correct path sides when the two vertices meet at the LCA
    e_t total = identity_e;
    if (curr_u->get_degree() == 2)
        total = f_e(total, (curr_v == bdry_u1) ? path_u1 : path_u2);
    else
        total = f_e(total, path_u1);
    if (curr_v->get_degree() == 2)
        total = f_e(total, (curr_u == bdry_v1) ? path_v1 : path_v2);
    else
        total = f_e(total, path_v1);
    // If the LCA contracts them in a star merge, take both edges to the center
    if (curr_u->get_degree() == 1 && curr_v->get_degree() == 1
    && curr_u->neighbors[0] != curr_v) [[unlikely]] {
        total = f_e(total, curr_u->get_edge_value(0));
        total = f_e(total, curr_v->get_edge_value(0));
    }
    // Add the value of the last edge (since they contract one must be deg <= 2)
    else [[likely]] {
        for (int i = 0; i < 2; i++) {
            if (curr_u->neighbors[i] == curr_v) {
                total = f_e(total, curr_u->get_edge_value(i));
                break;
            }
            if (curr_v->neighbors[i] == curr_u) {
                total = f_e(total, curr_v->get_edge_value(i));
                break;
            }
        }
    }
    return total;
}

}
