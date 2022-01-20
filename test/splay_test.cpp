#include <gtest/gtest.h>
#include "splay_tree.h"
#include "sketch.h"

TEST(SplayTreeSuite, basic_test) {
  SplayTree tree;
  vec_t len = 100;
  vec_t err = 10;
  int n = 10;
  Sketch::configure(len, err);

  void* sketch_space = alloca(Sketch::sketchSizeof() * n);

  std::vector<Sketch*> sketches;

  for (int i = 0; i < 10; i++)
  {
    sketches.push_back(Sketch::makeSketch((char*)sketch_space + Sketch::sketchSizeof()*i, (long)i));
    tree.insert(sketches[i]);
  }
  for (int i = 0; i < 10; i++)
  {
    tree.remove(sketches[i]);
  }
}
