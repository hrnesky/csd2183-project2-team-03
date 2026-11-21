// Your minimum spanning forests. Every function below is a stub that compiles
// but does nothing useful yet; replace each one with your implementation.

#include <algorithm>
#include <queue>

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

SpanningForest prim(int n, const std::vector<Edge> & bridges) {
  // store each neighboring vertex and the index of the bridge connecting it
  std::vector<std::vector<std::pair<int, int>>> adj(n);

  // add every bridge in both directions because bridges are undirected
  for (int i = 0; i < static_cast<int>(bridges.size()); i++) {
    const Edge &e = bridges[i];
    adj[e.from].push_back({e.to, i});
    adj[e.to].push_back({e.from, i});
  }

  // start with empty forest
  SpanningForest forest;
  
  // store the cheapest known bridge that can connect
  std::vector<std::int64_t> key(n, kUnreachable);

  // store which bridge gives each island its current cheapest connection
  std::vector<int> parentEdge(n, -1);

  // track which islands have already joined the forest
  std::vector<bool> inTree(n, false);

  // start a new Prim search from each island not reached by an earlier search
  for (int start = 0; start < n; start++) {
    // skip islands that already belong to a region's spanning tree
    if (inTree[start]) continue;

    // use a min-priority queue of (bridge weight, island) entries
    std::priority_queue<std::pair<std::int64_t, int>, std::vector<std::pair<std::int64_t, int>>, std::greater<std::pair<std::int64_t, int>>> q;

    key[start] = 0;
    q.push({0, start});

    // keep adding cheapest available island to this region's tree
    while (!q.empty()) {
      std::int64_t const cost = q.top().first; // get the island with the smallest queued key
      int const u = q.top().second;
      q.pop();

      if (inTree[u] || cost != key[u]) continue; // skip this entry if the island is already added or this key is outdated

      inTree[u] = true;

      // add its selected bridge, except for the region's starting island
      if (parentEdge[u] != -1) {
        Edge const& e = bridges[parentEdge[u]];
        forest.edges.push_back(e);
        forest.total_length += e.weight;
      }

      // check every bridge leading out of this island
      for (std::pair<int, int> const& neighbour : adj[u]) {
        // read the neighbouring island and the bridge's index
        int const v = neighbour.first;
        int const edgeIndex = neighbour.second;

        // get this bridge's weight
        std::int64_t const weight = bridges[edgeIndex].weight;

        // update the neighbour if it is outside the tree and this bridge is cheaper
        if (!inTree[v] && weight < key[v]) {
          key[v] = weight;
          parentEdge[v] = edgeIndex; // record the cheaper key and the bridge that provides it

          // queue the neighbour with its updated key
          q.push({key[v], v});
        }
      }
    }
  }

  return forest; // return the minimum spanning forest and its total bridge length
}
