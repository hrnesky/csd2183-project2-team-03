// Your minimum spanning forests. Every function below is a stub that compiles
// but does nothing useful yet; replace each one with your implementation.

#include <algorithm>

#include "maintenance.h"
#include "disjoint_set.h"

SpanningForest kruskal(int n, const std::vector<Edge> &bridges) {
  std::vector<Edge> sorted = bridges;
  std::stable_sort(sorted.begin(), sorted.end(), [](const Edge &a, const Edge &b) { return a.weight < b.weight; });

  SpanningForest forest;
  forest.total_length = 0;
  DisjointSet sets(n);
  for (const Edge &e : sorted) {
    if (sets.unite(e.from, e.to)) {
      forest.edges.push_back(e);
      forest.total_length += e.weight;
      if (static_cast<int>(forest.edges.size()) == n - 1) break; 
    }
  }
  return forest;
}

SpanningForest prim(int /*n*/, const std::vector<Edge> & /*bridges*/) {
  return {};  // TODO
}
