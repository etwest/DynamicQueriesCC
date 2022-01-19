#include <algorithm>
#include <functional>
#include <iostream>

#include <gtest/gtest.h>

#include <euler_tour_tree.h>

bool isvalid(const EulerTourTree& ett) {
  for (const auto& [k, v] : ett.edges) {
    if (!(v.node == &ett))
      return false;
    if (!(v.parent == nullptr || v.parent->left == &v || v.parent->right == &v))
      return false;
    if (!(v.left == nullptr || v.left->parent == &v))
      return false;
    if (!(v.right == nullptr || v.right->parent == &v))
      return false;
    if (k == nullptr) {
      if(!(v.right == nullptr))
        return false;
      const SplayTree* vnext = &v;
      while (vnext->parent != nullptr) {
        if (!(vnext->parent->right == vnext))
          return false;
        vnext = vnext->parent;
      }
    } else {
      const SplayTree* vnext;
      if (v.right == nullptr) {
        vnext = &v;
        if (!(vnext->parent != nullptr))
          return false;
        while (vnext->parent->right == vnext) {
          vnext = vnext->parent;
          if (!(vnext->parent != nullptr))
            return false;
        }
        vnext = vnext->parent;
      } else {
        vnext = v.right;
        while (vnext->left != nullptr) {
          vnext = vnext->left;
        }
      }
      if (!(vnext->node == k))
        return false;
    }
  }
  return true;
}

void dump(const EulerTourTree& ett) {
  std::cout << "EulerTourTree " << &ett << std::endl;
  for (const auto& [k, v] : ett.edges) {
    std::cout << "to EulerTourTree " << k << " is " << &v << std::endl;
    if (v.parent == nullptr) {
      std::function<void(const SplayTree* ptr, int depth)> inorder_dump =
        [&](auto ptr, int depth) {
          if (ptr != nullptr) {
            inorder_dump(ptr->left, depth + 1);
            std::cout << std::string(depth, ' ') << ptr << std::endl;
            inorder_dump(ptr->right, depth + 1);
          }
        };
      inorder_dump(&v, 0);
    }
  }
  std::cout << std::endl;
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
    if (!std::all_of(nodes.begin(), nodes.end(), [](auto& node){return isvalid(node);})) {
      std::cout << "Stress test validation failed, final state:" << std::endl;
      for (const auto& node : nodes) {
        dump(node);
      }
      ASSERT_TRUE(false);
    }
  }
}
