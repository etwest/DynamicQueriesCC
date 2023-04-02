#include <algorithm>
#include <cstdint>
#include <unordered_set>

#include <gtest/gtest.h>

#include <euler_tour_tree.h>

bool EulerTourTree::isvalid() const {
  bool invalid = false;
  // validate allowed_caller is null iff edges is empty
  EXPECT_EQ(allowed_caller == nullptr, this->edges.empty()) << (invalid = true, "");
  if (invalid) return false;
  // make sure allowed_caller corresponds to one of the nodes
  bool allowed_valid = false;
  // validate each edge
  for (const auto& [k, v] : this->edges) {
    // validate node's back ptr
    EXPECT_EQ(v->node, this) << (invalid = true, "");
    if (invalid) return false;
    // validate node itself
    EXPECT_TRUE(v->isvalid()) << (invalid = true, "");
    if (invalid) return false;
    if (k == nullptr) {
      // node is sentinel, expect next to be null
      EXPECT_EQ(v->next(), nullptr) << (invalid = true, "");
      if (invalid) return false;
    } else {
      // node is not sentinel, expect next to be valid
      EXPECT_EQ(v->next()->node, k) << (invalid = true, "");
      if (invalid) return false;
    }
    // check allowed_caller
    if (v.get() == allowed_caller) {
      // make sure there's only one allowed
      EXPECT_FALSE(allowed_valid) << (invalid = true, "");
      if (invalid) return false;
      allowed_valid = true;
    }
  }
  // check that there was one allowed_caller
  EXPECT_TRUE(allowed_valid) << (invalid = true, "");
  if (invalid) return false;

  return true;
}

std::ostream& operator<<(std::ostream& os, const EulerTourTree& ett) {
  os << "EulerTourTree " << &ett << std::endl;
  for (const auto& [k, v] : ett.edges) {
    os << "to EulerTourTree " << k << " is " << &v << std::endl;
    os << v << std::endl;
  }
  os << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
    const std::vector<EulerTourTree>& nodes) {
  for (const auto& node : nodes) {
    os << node;
  }
  return os;
}

TEST(EulerTourTreeSuite, stress_test) {

	// sketch variables
  vec_t len = 1000;
  vec_t err = 100;
  //configure the sketch globally
  Sketch::configure(len, err);

  int nodecount = 1000;

  int n = 100000;

  int seed = time(NULL);
  std::vector<EulerTourTree> nodes;
  nodes.reserve(nodecount);

  for (int i = 0; i < nodecount; i++)
  {
    nodes.emplace_back(seed);
  }

  std::cout << "Seeding stress test with " << seed << std::endl;
  srand(seed);
  for (int i = 0; i < n; i++) {
    int a = rand() % nodecount, b = rand() % nodecount;
    if (rand() % 100 < 15) {
      nodes[a].link(nodes[b]);
    } else {
      nodes[a].cut(nodes[b]);
    }
    if (i % n/100 == 0)
    {
      ASSERT_TRUE(std::all_of(nodes.begin(), nodes.end(),
            [](auto& node){return node.isvalid();}))
          << "Stress test validation failed, final state:"
          << std::endl
          << nodes;
    }
  }
}

TEST(EulerTourTreeSuite, random_links_and_cuts) {

  // sketch variables
  vec_t len = 1000;
  vec_t err = 100;
  //configure the sketch globally
  Sketch::configure(len, err);
  size_t space = Sketch::sketchSizeof();
  int nodecount = 1000;
  int n = 500;
  int seed = time(NULL);
  std::vector<EulerTourTree> nodes;
  nodes.reserve(nodecount);
  for (int i = 0; i < nodecount; i++)
  {
    //nodes.emplace_back(sketches[i], seed);
    nodes.emplace_back(seed);
    nodes[i].update_sketch((vec_t)i);
  }

  std::cout << "Seeding random links and cuts test with " << seed << std::endl;
  srand(seed);
  // Do random links and cuts
  for (int i = 0; i < n; i++) {
    int a = rand() % nodecount, b = rand() % nodecount;
    if (rand() % 100 < 10) {
      nodes[a].link(nodes[b]);
    } else {
      nodes[a].cut(nodes[b]);
    }
    ASSERT_TRUE(std::all_of(nodes.begin(), nodes.end(),
          [](auto& node){return node.isvalid();}))
        << "Stress test validation failed, final state:"
        << std::endl
        << nodes;
  }

  std::unordered_set<SplayTreeNode*> sentinels;
  for (int i = 0; i < nodecount; i++)
  {
    SplayTreeNode *sentinel = SplayTree::get_last(nodes[i].edges.begin()->second).get();
    sentinels.insert(sentinel);
  }
  void *cc_sketch_space = malloc(space * sentinels.size());

  // Walk up from an occurrence of each node to the root of its auxiliary tre
  std::unordered_map<SplayTreeNode*, Sketch*> aggs;
  std::unordered_map<SplayTreeNode*, uint32_t> sizes;
  for (int i = 0; i < nodecount; i++)
  {
    SplayTreeNode *sentinel = SplayTree::get_last(nodes[i].edges.begin()->second).get();
    sentinel->splay();
    SplayTreeNode *aux_root = sentinel;
    ASSERT_FALSE(aux_root->needs_rebuilding)
      << "Found node " << i << " in incomplete state!"
      << std::endl
      << nodes;
    if (aggs.find(sentinel) == aggs.end())
    {
      char *location = (char*)cc_sketch_space + space*aggs.size();
      aggs.insert({sentinel, Sketch::makeSketch(location, seed)});
      *aggs[sentinel] += *aux_root->sketch_agg;
      sizes[sentinel] = aux_root->size;
    }
  }

  void *naive_cc_sketch_space = malloc(space * sentinels.size());
  std::unordered_map<SplayTreeNode*, Sketch*> naive_aggs;
  std::unordered_map<SplayTreeNode*, uint32_t> naive_sizes;
  // Naively compute aggregates for each connected component
  for (int i = 0; i < nodecount; i++)
  {
    SplayTreeNode *sentinel = SplayTree::get_last(nodes[i].edges.begin()->second).get();
    if (naive_aggs.find(sentinel) != naive_aggs.end())
    {
      *naive_aggs[sentinel] += *nodes[i].sketch;
      naive_sizes[sentinel] += 1;
    }
    else
    {
      char *location = (char*)naive_cc_sketch_space + space*naive_aggs.size();
      naive_aggs.insert({sentinel, Sketch::makeSketch(location, seed)});
      *naive_aggs[sentinel] += *nodes[i].sketch;
      naive_sizes[sentinel] = 1;
    }
  }
  for (auto agg : aggs)
  {
    ASSERT_EQ(*(agg.second), *(naive_aggs[agg.first])) 
      << *agg.second << "\n\n\n" << *naive_aggs[agg.first] << std::endl;
  }
  free(cc_sketch_space);
  for (auto size: sizes) {
    // Euler tour has length 2n-1
    EXPECT_EQ(size.second, 2*naive_sizes[size.first]-1);
  }
}

TEST(EulerTourTreeSuite, get_aggregate) {
  // Sketch variables
  vec_t len = 10;
  vec_t err = 10;
  // Configure the sketch globally
  Sketch::configure(len, err);

  int seed = time(NULL);

  // Keep a manual aggregate of all the sketches
  Sketch* true_aggregate = (Sketch *) ::operator new(Sketch::sketchSizeof());
  Sketch::makeSketch(true_aggregate, seed);

  int nodecount = 1000;
  std::vector<EulerTourTree> nodes;
  nodes.reserve(nodecount);

  // Add value to each sketch, update the manual aggregate
  for (int i = 0; i < nodecount; i++)
  {
    nodes.emplace_back(seed);
    nodes[i].update_sketch((vec_t)i);
    true_aggregate->update((vec_t)i);
  }

  // Link all the ETT nodes
  for (int i = 0; i < nodecount-1; i++) {
    nodes[i].link(nodes[i+1]);
  }

  // Check that the ETT aggregate is properly maintained and gotten
  std::shared_ptr<Sketch> aggregate = nodes[0].get_aggregate();
  ASSERT_TRUE(*aggregate == *true_aggregate);
}
