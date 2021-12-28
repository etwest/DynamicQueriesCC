#include <splay_tree.h>
#include <assert.h>

// See https://en.wikipedia.org/wiki/Splay_tree#Operations
// For images on how the zig, zigzig, and zigzag steps work.

//TODO: Add validation functions 

void SplayTreeNode::splay()
{
  if (!parent)
    return;
  //Are we left of the parent?
  bool leftp = (this == parent->left);
  
  Sptr gp = parent->parent;
  if (!gp)
    return zig(leftp);

  //Is our parent left of the grandparent?
  bool leftgp = (parent == gp->left);

  //Same direction-- zig zig
  if (leftgp == leftp)
    return zigzig(leftgp);
  else
    return zigzag(leftgp);
}

// This is a single rotation to move the node to the root
// Does not touch the old grandparent node, but does point to them 
void SplayTreeNode::zig(bool left)
{
  auto oldparent = std::move(parent);
  auto newparent = std::move(oldparent->parent);
  if (left)
  {
    // We are left of the parent
    auto thisnode = std:move(parent->left);

    // Replace us with our right
    // This is the 'zig' step
    oldparent->left = std::move(right);
    if (oldparent->left)
      oldparent->left->parent = oldparent;

    // Our right is now our old parent
    right = oldparent;
    right->parent = thisnode;

    // Set our parent pointer properly.
    parent = newparant;
  }
  else
  {
    // We are right of the parent
    auto thisnode = std:move(parent->right);

    // Replace us with our left
    // This is the 'zig' step
    oldparent->right = std::move(left);
    if (oldparent->right)
      oldparent->right->parent = oldparent;

    // Our right is now our old parent
    left = oldparent;
    left->parent = thisnode;

    // Set our parent pointer properly.
    parent = newparant;
  }

  // Rebuild sketches in order
  oldparent->rebuild_agg();
  rebuild_agg();

  return; // No more splaying
}

//Left defines GP direction. Whether or not its mirrored
void SplayTreeNode::zigzig(bool left)
{
  auto oldparent = std::move(parent);
  auto oldgp = std::move(oldparent->parent);
  auto oldggp = std::move(oldgp->parent);
  bool gpleft = (oldggp->left == oldgp);
  if (left)
  {
    auto thisnode = std:move(parent->left);
    // 'A' and 'D' do not need fixing. 
    
    //Fix 'C'
    oldgp->left = oldparent->right;
    if (oldgp->left)
      oldgp->left->parent = oldgp;

    //Fix 'B'
    oldparent->left = right;
    if (oldparent->left)
      oldparent->left->parent = oldparent;

    oldparent->right = oldgp;
    oldgp->parent = oldparent;

    right = oldparent;
    oldparent->parent = thisnode;

  }
  else
  {
    auto thisnode = std:move(parent->right);
    // 'A' and 'D' do not need fixing. 
    
    //Fix 'C'
    oldgp->right = oldparent->left;
    if (oldgp->right)
      oldgp->right->parent = oldgp;

    //Fix 'B'
    oldparent->right = left;
    if (oldparent->right)
      oldparent->right->parent = oldparent;

    oldparent->left = oldgp;
    oldgp->parent = oldparent;

    left = oldparent;
    oldparent->parent = thisnode;
  }
  // Now, we fix the old ggp
  parent = oldggp;
  if (parent)
    if (gpleft)
      oldggp->left = thisnode;
    else
      oldggp->right = thisnode;

  // Rebuild sketches in order
  oldgp->rebuild_agg();
  oldparent->rebuild_agg();
  rebuild_agg();

  return splay(); // Probably more splaying
}

//Left defines GP direction. Whether or not its mirrored
void SplayTreeNode::zigzag(bool left)
{
  auto oldparent = std::move(parent);
  auto oldgp = std::move(oldparent->parent);
  auto oldggp = std::move(oldgp->parent);
  bool gpleft = (oldggp->left == oldgp);
  if (left)
  {
    auto thisnode = std:move(parent->left);
    // 'A' and 'D' do not need fixing. 
    
    //Fix 'C'
    oldgp->left = right;
    if (oldgp->left)
      oldgp->left->parent = oldgp;

    //Fix 'B'
    oldparent->right = left;
    if (oldparent->right)
      oldparent->right->parent = oldparent;

    left = oldparent;
    oldparent->parent = thisnode;

    right = oldgp;
    oldgp->parent = thisnode;
  }
  else
  {
    auto thisnode = std:move(parent->left);
    // 'A' and 'D' do not need fixing. 
    
    //Fix 'C'
    oldgp->right = left;
    if (oldgp->right)
      oldgp->right->parent = oldgp;

    //Fix 'B'
    oldparent->left = right;
    if (oldparent->left)
      oldparent->left->parent = oldparent;

    right = oldparent;
    oldparent->parent = thisnode;

    left = oldgp;
    oldgp->parent = thisnode;
  }
  // Now, we fix the old ggp
  parent = oldggp;
  if (parent)
    if (gpleft)
      oldggp->left = thisnode;
    else
      oldggp->right = thisnode;

  // Rebuild sketches in order
  oldgp->rebuild_agg();
  oldparent->rebuild_agg();
  rebuild_agg();

  return splay(); // Probably more splaying
}

void SplayTreeNode::rebuild_agg()
{
  //TODO: rebuilding doesn't work if there's no children?
  if (!left && !right)
    return;
  else if (left && right)
  {
    sketch_agg = left;
    sketch_agg.merge(right->sketch_agg);
  }
  else if (left)
      sketch_agg = left;
  else
      sketch_agg = right;
}

void SplayTree::splay(Sptr node)
{
  assert(node);

  node->splay();
  head = node;
}



