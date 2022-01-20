#include "../include/splay_tree.h"
#include <assert.h>
#include <iostream>

// See https://en.wikipedia.org/wiki/Splay_tree#Operations
// For images on how the zig, zigzig, and zigzag steps work.

//TODO: Add validation functions 

void SplayTreeNode::splay()
{
  if (parent.expired())
    return;
  //Are we left of the parent?
  bool leftp = (this == get_parent()->left.get());
  
  //Check if we have a grandparent
  if (get_parent()->parent.expired())
    return zig(leftp);
  Sptr gp = get_parent()->get_parent();

  //Is our parent left of the grandparent?
  bool leftgp = (get_parent() == gp->left);

  //Same direction-- zig zig
  if (leftgp == leftp)
    return zigzig(leftgp);
  else
    return zigzag(leftgp);
}

// This is a single rotation to move the node to the root
// Does not touch the old grandparent node, but does point to them 
void SplayTreeNode::zig(bool leftm)
{
  std::cout << "zig" << std::endl;
  auto oldparent = std::move(get_parent());
  auto newparent = std::move(oldparent->parent);
  if (leftm)
  {
    // We are left of the parent
    auto thisnode = std::move(get_parent()->left);

    // Replace us with our right
    // This is the 'zig' step
    oldparent->left = std::move(right);
    if (oldparent->left)
      oldparent->left->parent = oldparent;

    // Our right is now our old parent
    right = oldparent;
    right->parent = thisnode;

    // Set our parent pointer properly.
    parent = newparent;
  }
  else
  {
    // We are right of the parent
    auto thisnode = std::move(get_parent()->right);

    // Replace us with our left
    // This is the 'zig' step
    oldparent->right = std::move(left);
    if (oldparent->right)
      oldparent->right->parent = oldparent;

    // Our right is now our old parent
    left = oldparent;
    left->parent = thisnode;

    // Set our parent pointer properly.
    parent = newparent;
  }

  // Rebuild sketches in order
  oldparent->rebuild_agg();
  rebuild_agg();

  return; // No more splaying
}

//Left defines GP direction. Whether or not its mirrored
void SplayTreeNode::zigzig(bool leftm)
{
  std::cout << "zigzig" << std::endl;
  auto oldparent = std::move(get_parent());
  auto oldgp = std::move(oldparent->get_parent());

  bool hasggp = !oldgp->parent.expired();
  auto oldggp = hasggp ? oldgp->get_parent() : Sptr(nullptr);
  bool gpleft = hasggp? (oldgp->get_parent()->left == oldgp) : false;

  Sptr thisnode;
  if (leftm)
  {
    thisnode = std::move(get_parent()->left);
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
    thisnode = std::move(get_parent()->right);
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
  if (hasggp)
  {
    if (gpleft)
      oldggp->left = thisnode;
    else
      oldggp->right = thisnode;
  }
  // Rebuild sketches in order
  oldgp->rebuild_agg();
  oldparent->rebuild_agg();
  rebuild_agg();

  return splay(); // Probably more splaying
}

//Left defines GP direction. Whether or not its mirrored
void SplayTreeNode::zigzag(bool leftm)
{
  std::cout << "zigzag" << std::endl;
  auto oldparent = std::move(get_parent());
  auto oldgp = std::move(oldparent->get_parent());

  bool hasggp = !oldgp->parent.expired();
  auto oldggp = hasggp ? oldgp->get_parent() : Sptr(nullptr);
  bool gpleft = hasggp? (oldgp->get_parent()->left == oldgp) : false;

  Sptr thisnode;
  if (leftm)
  {
    thisnode = std::move(get_parent()->left);
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
    thisnode = std::move(get_parent()->left);
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
  if (hasggp)
  {
    if (gpleft)
      oldggp->left = thisnode;
    else
      oldggp->right = thisnode;
  }
  // Rebuild sketches in order
  oldgp->rebuild_agg();
  oldparent->rebuild_agg();
  rebuild_agg();

  return splay(); // Probably more splaying
}

void SplayTreeNode::rebuild_agg()
{
  /*
  bool hasleft = left && left->sketch;
  bool hasright = right && right->sketch;
  // Case 1: No children. Our aggregate is just ourself
  if (!hasleft && !hasright)
  {
    has_agg = false;
  }
  // Case 2: Both children have information.
  else if (hasleft && hasright)
  {
    sketch_agg = left->sketch_agg;
    sketch_agg.merge(right->sketch_agg);
    has_agg = true;
  }
  // Case 3: Only left
  else if (hasleft)
  {
    sketch_agg = left->sketch_agg;
    has_agg = true;
  }
  // Case 4: Only right
  else
  {
    sketch_agg = right->sketch_agg;
    has_agg = true;
  }
  // We've added our children's information. Now we can add our own.
  if (sketch_ptr)
    if (has_agg)
      sketch_agg.merge(*sketch_ptr);
    else
      sketch_agg = *sketch_ptr;
      */
}

void SplayTree::join(SplayTree* other)
{
  assert(other);

  validate();
  if (!head)
  {
    head = other->head;
    return;
  }
  splay_largest();

  head->right = std::move(other->head);
  head->right->parent = head;
  validate();
}

SplayTree* SplayTree::split()
{
  assert(head);
  assert(head->right);

  validate();
  SplayTree* toret = new SplayTree(std::move(head->right));
  head->right = Sptr(nullptr);
  toret->head->parent = Sptr(nullptr);
  validate();

  return toret;
}

void SplayTree::insert(Sketch* sketch)
{
  SplayTree right(sketch);
  validate();
  join(&right);
  validate();
}

void SplayTree::remove(Sketch* sketch)
{
  validate();
  return;
}

void SplayTree::splay(Sptr node)
{
  assert(node);

  std::cout << "Splaying" << std::endl;
  validate();

  node->splay();
  head = node;
  
  validate();
}

long SplayTree::validateNode(Sptr node)
{
  long nodecount = 1;
  if (!node)
    return 0;
  if (node->left)
  {
    check_child(node, node->left);
    nodecount += validateNode(node->left);
  }
  if (node->right)
  {
    check_child(node, node->right);
    nodecount += validateNode(node->right);
  }
  return nodecount;
}

void SplayTree::check_child(Sptr parent, Sptr child)
{
  assert(child->get_parent() == parent);
}

//asserts if a child doesn't point to its parent
long SplayTree::validate()
{
  if (!head)
    return 0;
  assert(head->parent.expired());
  return validateNode(head);
}

void SplayTree::splay_random()
{
  Sptr node = head;
  while(true)
  {
    int which = rand() % 2;
    if (which == 0)
      if (node->left)
        node = node->left;
      else
      {
        splay(node);
        return;
      }
    else
      if (node->right)
        node = node->right;
      else
      {
        splay(node);
        return;
      }
  }
}

