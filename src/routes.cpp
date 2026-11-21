// Your shortest-path algorithms. Every function below is a stub that compiles
// but does nothing useful yet; replace each one with your implementation.

#include "routes.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

// dijkstr find the cheapest path from source to every island
ShortestPaths dijkstra(int n, const std::vector<Edge>& edges, int source) {
  ShortestPaths result;

  // start by assuming no island is reachable
  // -1 since we do not yet know the previous island on its path
  result.dist.assign(n, kUnreachable);
  result.parent.assign(n, -1);
  result.dist[source] = 0;  // costs nothing to stay at the start

  // build a list of outgoing routes for every island
  // example: adj[0] = {{1, 5}, {2, 4}} means 0->1 costs 5, 0->2 costs 4
  std::vector<std::vector<std::pair<int, std::int64_t>>> adj(n);
  for (const Edge& e : edges) {
    adj[e.from].push_back({e.to, e.weight});
  }

  // a min-heap gives us the island with the lowest known cost first
  // each entry stores {cost_so_far, island_number}
  using Item = std::pair<std::int64_t, int>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;
  heap.push({0, source});

  while (!heap.empty()) {
    auto [distance, u] = heap.top();  // take the cheapest queued possibility
    heap.pop();

    // there may be an older, more expensive entry for this same island
    // ignore, have cheaper way there
    if (distance != result.dist[u]) continue;

    // try every route leaving island u.
    for (auto [v, weight] : adj[u]) {
      const std::int64_t candidate = distance + weight;

      // relaxation | if going through u is cheaper, remember that route
      if (candidate < result.dist[v]) {
        result.dist[v] = candidate;  // new cheapest total cost to v
        result.parent[v] = u;        // we reached v from 
        heap.push({candidate, v});  // explore this better route later
      }
    }
  }

  return result;  // return all distances and parents, not printed output
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
