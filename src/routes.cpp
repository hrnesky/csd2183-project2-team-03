// Your shortest-path algorithms. Every function below is a stub that compiles
// but does nothing useful yet; replace each one with your implementation.

#include "routes.h"
#include <queue>

ShortestPaths dijkstra(int n, const std::vector<Edge> & edges,
                       int source) {
  // store each outgoing edge as (destination, weight)
  std::vector<std::vector<std::pair<int, int64_t>>> adj(n);

  // build adj list, routes are direct and add each edge once
  for (Edge const& e : edges) {
    adj[e.from].push_back({e.to, e.weight});
  }

  // initialize all distances to inf and all parents to NIL
  ShortestPaths paths;
  paths.dist.assign(n, kUnreachable);
  paths.parent.assign(n, -1);

  // set the source distance to zero
  paths.dist[source] = 0;

  // store candidate entries (distance, vertex) with the smallest distance first
  std::priority_queue<std::pair<std::int64_t, int>, std::vector<std::pair<std::int64_t, int>>, std::greater<std::pair<std::int64_t, int>>> q;

  // add the source to the min-priority queue
  q.push({0, source});

  // process vertices in order of their current smallest known distance
  while (!q.empty()) {
    std::int64_t const distance = q.top().first; // get and remove the closest queued vertex
    int const u = q.top().second;
    q.pop();

    // ignore this entry if a shorter path to u was found after it was queued
    if (distance != paths.dist[u]) continue;

    // check each directed edge leaving u
    for (std::pair<int, int64_t> const& edge : adj[u]) {
      // read the edge's dest and weight
      int const v = edge.first;
      std::int64_t const weight = edge.second;

      // compute the distance to v by flwing this edge from you
      std::int64_t const newDistance = distance + weight;

      // relax the edge if it give v a shorter path
      if (newDistance < paths.dist[v]) {
        paths.dist[v] = newDistance; // record the improved distance to v
        paths.parent[v] = u; // record u as the predecessor of v on the shortest path
        q.push({newDistance, v}); // queue v so its outging edges can be processed at its new distance
      }
    }
  }
  return paths; // return distance and predecessors for every vertex
}

BellmanFordResult bellman_ford(int /*n*/, const std::vector<Edge> & /*edges*/,
                               int /*source*/) {
  return {};  // TODO
}
