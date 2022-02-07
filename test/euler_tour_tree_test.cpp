#include <algorithm>

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
  std::vector<EulerTourTree> nodes(10);

  int seed = time(NULL);
  std::cout << "Seeding stress test with " << seed << std::endl;
  srand(seed);
  for (int i = 0; i < 10000; i++) {
    int a = rand() % 10, b = rand() % 10;
    if (rand() % 100 < 15) {
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
}
