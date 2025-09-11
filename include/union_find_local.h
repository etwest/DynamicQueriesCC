#pragma once

// basically unmodified from parlay's union_find.h
// in its examples (as of commit e1b1f17)

#include <parlay/primitives.h>

// The following supports both "link" (a directed union) and "find".
// They are safe to run concurrently as long as there is no cycle among
// concurrent links.   This can be achieved, for example by only linking
// a vertex with lower id into one with higher degree.
// See:  "Internally deterministic parallel algorithms can be fast"
// Blelloch, Fineman, Gibbons, and Shun
// for a discussion of link/find.
template <class vertex>
struct union_find_local {
    parlay::sequence<std::atomic<vertex>> parents;

    bool is_root(vertex u) {
        return parents[u] < 0;
    }

    // initialize n elements all as roots
    union_find_local(size_t n) : parents(parlay::tabulate<std::atomic<vertex>>(n, [](long) { return -1; })) {}

    vertex find(vertex i) {
        if (is_root(i)) return i;
        vertex p = parents[i];
        if (is_root(p)) return p;

        // find root, shortcutting along the way
        do {
            vertex gp = parents[p];
            parents[i] = gp;
            i = p;
            p = gp;
        } while (!is_root(p));
        return p;
    }

    // Version of union that is safe for parallelism
    // when no cycles are created (e.g. only link from larger
    // to smaller vertex).
    // Does not use ranks.
    void link(vertex u, vertex v) {
        // ONLY MODIFICATION
        // we're going to enforce minimum as root
        if (u < v) {
            std::swap(u, v);
        }
        // ensure that u > v
        // make v the parent of u
        parents[u] = v;
    }

    void reset() {
        // make everything a root again
        parlay::parallel_for(0, parents.size(), [&](size_t i) {
            parents[i] = -1;
        });
    }
};

template struct union_find_local<int32_t>;