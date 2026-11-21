// Your shortest-path algorithms. Every function below is a stub that compiles
// but does nothing useful yet; replace each one with your implementation.

#include "routes.h"

#include <algorithm>   // std::reverse (Bellman-Ford)
#include <functional>  // std::greater (Dijkstra)
#include <queue>       // std::priority_queue
#include <utility>     // std::pair
#include <vector>      // std::vector

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

// bellman-ford: Like dijkstra, but supports NEGATIVE route costs.
// reports a reachable NEGATIVE CYCLE (an unlimited-stamina exploit).
BellmanFordResult bellman_ford(int n, const std::vector<Edge>& edges,
                              int source) {
  BellmanFordResult result;

  // these are references to the arrays stored in the result we will return
  auto& dist = result.paths.dist;
  auto& parent = result.paths.parent;

  // initially only the starting island is reachable, at cost zero
  dist.assign(n, kUnreachable);
  parent.assign(n, -1);
  dist[source] = 0;

  // one ROUND means checking every directed sea route once
  // normally, shortest paths settle within n-1 rounds (n = island count)
  // We allow an nth round to detect a reachable negative cycle
  int updated_vertex = -1;
  for (int round = 0; round < n; ++round) {
    updated_vertex = -1;  // no improvement seen in THIS round yet

    for (const Edge& e : edges) {
      // dont calculate a route from an island we cannot reach
      // this also avoids adding a weight to the kUnreachable sentinel
      if (dist[e.from] == kUnreachable) continue;

      // new possible cost cheapest cost to the routes start + route cost
      const std::int64_t candidate = dist[e.from] + e.weight;

      if (candidate < dist[e.to]) {
        // RELAXATION: We found a cheaper way to reach e.to
        dist[e.to] = candidate;
        parent[e.to] = e.from;   // save how we got to e.to
        updated_vertex = e.to;   // remember that something changed
      }
    }

    // The answers are already stable if no changes for the round
    if (updated_vertex == -1) break;
  }

  // if the last round made no change we found no reachable negative cycle
  // return the normal shortest path distances and parents
  if (updated_vertex == -1) return result;

  // we finished the nth round WITH an improvement, a reachable negative
  // cycle exists. But the last updated island might lie AFTER that cycle
  // Following n parent links guarantees that we land inside the cycle
  int on_cycle = updated_vertex;
  for (int i = 0; i < n; ++i) {
    on_cycle = parent[on_cycle];
  }

  // follow parent links until we return to the same island
  // parents point backwards along routes, so this collects a reversed cycle
  int v = on_cycle;
  do {
    result.negative_cycle.push_back(v);
    v = parent[v];
  } while (v != on_cycle);

  // reverse to list islands in the actual direction of sea travel
  std::reverse(result.negative_cycle.begin(), result.negative_cycle.end());
  return result;
}
