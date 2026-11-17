// Your breadth-first and depth-first searches. Every function below is a stub
// that compiles but does nothing useful yet; replace each one with your
// implementation.

#include "map_audit.h"
#include <queue>
#include <algorithm>

namespace {
  std::vector<std::vector<int>> make_adjacency(int n,
                                              const std::vector<Edge>& bridges) {
    std::vector<std::vector<int>> adj(n);
    for (const Edge& e : bridges) {
      adj[e.from].push_back(e.to);
      adj[e.to].push_back(e.from);
    }
    return adj;
  }
}  // namespace


std::vector<int> region_labels(int n, const std::vector<Edge>& bridges) {
  auto adj = make_adjacency(n, bridges);
  std::vector<int> label(n, -1);
  for (int start = 0; start < n; ++start) {
    if (label[start] != -1) continue;
    // BFS from `start`, collecting the component into `members`.
    std::vector<int> members;
    std::queue<int> q;
    label[start] = start;   // temporary marker
    q.push(start);
    while (!q.empty()) {
      int u = q.front(); q.pop();
      members.push_back(u);
      for (int v : adj[u]) {
        if (label[v] == -1) { label[v] = start; q.push(v); }
      }
    }
    int mn = *std::min_element(members.begin(), members.end());
    for (int u : members) label[u] = mn;   // normalize to region minimum
  }
  return label;
}

int fewest_hops(int n, const std::vector<Edge>& bridges, int s, int t) {
  if (s == t) return 0;
  auto adj = make_adjacency(n, bridges);
  std::vector<int> dist(n, -1);
  std::queue<int> q;
  dist[s] = 0; q.push(s);
  while (!q.empty()) {
    int u = q.front(); q.pop();
    for (int v : adj[u]) {
      if (dist[v] == -1) {
        dist[v] = dist[u] + 1;
        if (v == t) return dist[v];   // early exit
        q.push(v);
      }
    }
  }
  return -1;
}