#include <gtest/gtest.h>
#include "splay_tree.h"
#include "sketch.h"


TEST(SplayTreeSuite, random_splays) {
  SplayTree tree;
  //sketch variables
  vec_t len = 1000;
  vec_t err = 100;
  //number of nodes
  int n = 500;
  //number of random splays
  int rsplays = 100000;
  //configure the sketch globally
  Sketch::configure(len, err);
  void* sketch_space = alloca(Sketch::sketchSizeof() * n);

  std::vector<Sketch*> sketches;

  for (int i = 0; i < n; i++)
  {
    sketches.push_back(Sketch::makeSketch((char*)sketch_space + Sketch::sketchSizeof()*i, (long)i));
    tree.insert(sketches[i]);
  }

  for (int i = 0; i < rsplays; i++)
  {
    tree.splay_random();
  }
  // This validates both the parent-child pointer relations and the number of nodes in the tree
  long nnodes = tree.validate();
  if(nnodes != n)
    FAIL() << "Expected " << n << " nodes, found " << nnodes << std::endl;
}
