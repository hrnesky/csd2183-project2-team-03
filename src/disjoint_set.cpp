// Your disjoint-set forest. Every member function below is a stub that
// compiles but does nothing useful yet; replace each one with your
// implementation.

#include "disjoint_set.h"
#include <numeric>   // std::iota

DisjointSet::DisjointSet(int n) : parent_(n), rank_(n, 0), count_(n) {
  std::iota(parent_.begin(), parent_.end(), 0);  // parent_[i] = i
}

int DisjointSet::find(int x) {
  // Pass 1: walk to the root.
  int root = x;
  while (parent_[root] != root) root = parent_[root];
  // Pass 2: point every node on the path straight at the root.
  while (parent_[x] != root) {
    int next = parent_[x];
    parent_[x] = root;
    x = next;
  }
  return root;
}

bool DisjointSet::unite(int x, int y) {
  int rx = find(x), ry = find(y);
  if (rx == ry) return false;               // already together: no change
  if (rank_[rx] < rank_[ry]) std::swap(rx, ry);  // rx is the higher-rank root
  parent_[ry] = rx;                         // lower under higher
  if (rank_[rx] == rank_[ry]) ++rank_[rx];  // tie: rank grows
  --count_;
  return true;
}

int DisjointSet::count() const { return count_; }