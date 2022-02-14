#include <functional>
#include <gtest/gtest.h>
#include "splay_tree.h"
#include "euler_tour_tree.h"

bool SplayTreeNode::isvalid() const {
  bool invalid = false;
  EXPECT_TRUE(this->get_cparent() == nullptr ||
      this->get_cparent()->left.get() == this ||
      this->get_cparent()->right.get() == this)
      << (invalid = true, "");
  if (invalid) return false;
  EXPECT_TRUE(this->left == nullptr || this->left->get_cparent().get() == this)
      << (invalid = true, "");
  if (invalid) return false;
  EXPECT_TRUE(this->right == nullptr || this->right->get_cparent().get() == this)
      << (invalid = true, "");
  if (invalid) return false;
  return true;
}

const SplayTreeNode* SplayTreeNode::next() const {
  const SplayTreeNode* ret;
  if (this->right == nullptr) {
    // Follow parent up until we are a left child
    ret = this;
    while (ret->get_cparent() != nullptr && ret->get_cparent()->right.get() == ret) {
      ret = ret->get_cparent().get();
    }
    ret = ret->get_cparent().get();
  } else {
    // Follow right child all the way to the left
    ret = this->right.get();
    while (ret->left != nullptr) {
      ret = ret->left.get();
    }
  }
  return ret;
}

std::ostream& operator<<(std::ostream& os, const SplayTreeNode& tree) {
  const SplayTreeNode* root = &tree;
  while (!root->parent.expired()) {
    root = root->parent.lock().get();
  }
  std::function<void(const SplayTreeNode* ptr, int depth)> inorder_dump =
      [&os, &tree, &inorder_dump](auto ptr, int depth) {
        if (ptr != nullptr) {
          inorder_dump(ptr->left.get(), depth + 1);
          os << (ptr == &tree ? '>' : ' ') << std::string(depth, ' ') << ptr
              << std::endl;
          inorder_dump(ptr->right.get(), depth + 1);
        }
      };
  inorder_dump(root, 0);
  return os;
}

TEST(SplayTreeSuite, links_and_cuts) {
  using Sptr = std::shared_ptr<SplayTreeNode>;
	
}

TEST(SplayTreeSuite, random_splays) {
  using Sptr = std::shared_ptr<SplayTreeNode>;

  long seed = rand();
  std::cout << "SEED: " << seed << std::endl;
  srand(seed);

  //number of nodes
  int n = 3;
  //number of random splays
  int rsplays = 1;
  // sketch variables
  vec_t len = 10;
  vec_t err = 10;
  
  Sketch::configure(len, err);

  size_t space = Sketch::sketchSizeof();
  // Let the last sketch be the aggregate
  void* sketch_space = malloc(space * (n+1));
  Sketch* agg = Sketch::makeSketch((char*)sketch_space + space * n, seed);

  std::vector<Sketch*> sketches;
  for (int i = 0; i < n; i++)
  {
    sketches.push_back(Sketch::makeSketch((char*)sketch_space + space*i, (long)seed));
    sketches[i]->update((vec_t)i);
    *agg += *sketches[i];
  }

  Sptr tree = std::make_shared<SplayTreeNode>(new EulerTourTree(sketches[0]));
  Sptr next = tree;
  for (int i = 1; i < n; i++)
  {
    next->link_right(std::make_shared<SplayTreeNode>(new EulerTourTree(sketches[i])));
    next = next->right;
  }

  auto root = tree;
  for (int i = 0; i < rsplays; i++)
  {
    root = root->splay_random_child();
  }
  // This validates both the parent-child pointer relations and the number of nodes in the tree
  long nnodes = root->count_children();
  if(nnodes != n)
    FAIL() << "Expected " << n << " nodes, found " << nnodes << std::endl;
  ASSERT_EQ(*agg, *root->sketch_agg);
  free(sketch_space);
}

