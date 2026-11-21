// Kruskal's algorithm with four disjoint-set variants. Build with `make bench`.
//
// This file does the following:
//   1. Builds graph families (sparse, grid, dense, star, binomial+deep).
//   2. Sorts each edge list once, so the sort is not timed.
//   3. Times the union-find loop of Kruskal for each variant with nanobench.
//   4. Counts the average parent links per find in a separate, untimed run.
//   5. Appends every result to disjoint_results.csv in the working directory.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "graph.h"
#include "nanobench.h"

namespace {

// ---------------------------------------------------------------------------
// Disjoint-set variants
// ---------------------------------------------------------------------------

// A copy of DisjointSet with the two optimizations as template flags:
//   Rank     = union by rank (otherwise naive linking: root of a below b)
//   Compress = path compression
// `links` and `finds` are counters for the average-links measurement.
template <bool Rank, bool Compress>
class DS {
 public:
  explicit DS(int n) : p_(n), r_(n, 0) { std::iota(p_.begin(), p_.end(), 0); }

  int find(int x) {
    ++finds;
    // 1. Walk up to the root, counting every parent link followed.
    int root = x;
    while (p_[root] != root) {
      root = p_[root];
      ++links;
    }
    // 2. Optionally point every node on the path directly at the root.
    if constexpr (Compress) {
      while (p_[x] != root) {
        const int next = p_[x];
        p_[x] = root;
        x = next;
      }
    }
    return root;
  }

  bool unite(int a, int b) {
    // 1. Find both roots; do nothing if they are already the same set.
    a = find(a);
    b = find(b);
    if (a == b) return false;
    // 2. Link one root below the other, by rank or naively.
    if constexpr (Rank) {
      if (r_[a] < r_[b]) std::swap(a, b);
      p_[b] = a;
      if (r_[a] == r_[b]) ++r_[a];
    } else {
      p_[a] = b;
    }
    return true;
  }

  std::uint64_t links = 0, finds = 0;

 private:
  std::vector<int> p_, r_;
};

// Kruskal's loop over edges that are already sorted by weight.
// Returns the total weight; optionally reports the average links per find.
template <bool Rank, bool Compress>
std::int64_t kruskal_variant(int n, const std::vector<Edge> &sorted,
                             double *avg_links = nullptr) {
  DS<Rank, Compress> ds(n);
  std::int64_t total = 0;
  for (const Edge &e : sorted) {
    if (ds.unite(e.from, e.to)) total += e.weight;
  }
  if (avg_links) *avg_links = double(ds.links) / double(ds.finds);
  return total;
}

void sort_edges(std::vector<Edge> &edges) {
  std::stable_sort(edges.begin(), edges.end(),
                   [](const Edge &a, const Edge &b) { return a.weight < b.weight; });
}

// ---------------------------------------------------------------------------
// Graph generators
// ---------------------------------------------------------------------------

// Connected random graph: a random spanning tree plus random extra edges
// until there are m edges. Used for both sparse and dense graphs.
std::vector<Edge> random_connected(int n, std::size_t m, std::mt19937_64 &rng) {
  std::uniform_int_distribution<std::int64_t> len(1, 1'000'000);
  std::uniform_int_distribution<int> island(0, n - 1);
  std::vector<Edge> g;
  // 1. Spanning tree: each island v joins a random earlier island.
  for (int v = 1; v < n; ++v) {
    std::uniform_int_distribution<int> earlier(0, v - 1);
    g.push_back({earlier(rng), v, len(rng)});
  }
  // 2. Extra edges between random pairs of islands.
  while (g.size() < m) g.push_back({island(rng), island(rng), len(rng)});
  return g;
}

// About 4n bridges.
std::vector<Edge> random_sparse(int n, std::mt19937_64 &rng) {
  return random_connected(n, std::size_t{4} * n, rng);
}

// About n^2 / 4 bridges.
std::vector<Edge> random_dense(int n, std::mt19937_64 &rng) {
  return random_connected(n, std::size_t(n) * n / 4, rng);
}

// side x side grid (n = side * side); each island is linked to its right and
// lower neighbor, which covers all four directions once per edge.
std::vector<Edge> grid(int side, std::mt19937_64 &rng) {
  std::uniform_int_distribution<std::int64_t> len(1, 1'000'000);
  std::vector<Edge> g;
  for (int r = 0; r < side; ++r) {
    for (int c = 0; c < side; ++c) {
      const int v = r * side + c;
      if (c + 1 < side) g.push_back({v, v + 1, len(rng)});
      if (r + 1 < side) g.push_back({v, v + side, len(rng)});
    }
  }
  return g;
}

// Chain-union family, defeats "neither":
//   1. Edges (0, i) with weight i, so they are processed in order.
//   2. Naive linking builds the chain 0 -> 1 -> 2 -> ...
//   3. Every find(0) then walks the whole chain: Theta(n) per find.
std::vector<Edge> star(int n) {
  std::vector<Edge> g;
  for (int i = 1; i < n; ++i) g.push_back({0, i, i});
  return g;
}

// Defeats "rank only" (n must be a power of two):
//   1. Light edges (s, s + half) with weight = level build a binomial tree,
//      in which island i has depth popcount(i) and island 0 is the root.
//   2. Heavy edges (n - 1, v) then call find on the deepest island again
//      and again.
//   3. Without compression each such find costs log2(n) links; with
//      compression only the first one does.
// With naive linking the root ends up at island n - 1, so this family does
// not hurt "neither": it depends on rank's tie-break.
std::vector<Edge> binomial_then_deep(int n) {
  std::vector<Edge> g;
  int level = 0;
  for (int half = 1; half < n; half *= 2, ++level) {
    for (int s = 0; s < n; s += 2 * half) g.push_back({s, s + half, level});
  }
  for (int v = 0; v < n - 1; ++v) g.push_back({n - 1, v, 1'000'000 + v});
  return g;
}

// ---------------------------------------------------------------------------
// Driver
// ---------------------------------------------------------------------------

// Appends one CSV row per timed variant. A skipped variant gives no row.
void append_csv(const std::string &graph, int n, std::size_t m,
                const ankerl::nanobench::Bench &b, const double links[4]) {
  using ankerl::nanobench::Result;
  static const char *names[4] = {"rank+compress", "rank only", "compress only",
                                 "neither"};
  std::ofstream out("disjoint_results.csv", std::ios::app);
  for (std::size_t i = 0; i < b.results().size() && i < 4; ++i) {
    out << graph << ',' << n << ',' << m << ',' << names[i] << ','
        << b.results()[i].median(Result::Measure::elapsed) << ',' << links[i]
        << '\n';
  }
}

// Benchmarks one graph. `with_neither` is false for sizes where the
// naive variant is too slow (its time grows quadratically).
void run(const std::string &graph, int n, std::vector<Edge> edges,
         bool with_neither = true) {
  // 1. Sort once, outside the timed code.
  sort_edges(edges);

  // 2. Configure nanobench: median of 7 epochs of at least 50 ms each.
  const std::string tag = graph + ", n = " + std::to_string(n) +
                          ", m = " + std::to_string(edges.size());
  ankerl::nanobench::Bench b;
  b.title(tag)
      .unit("run")
      .relative(true)
      .epochs(7)
      .minEpochIterations(1)
      .minEpochTime(std::chrono::milliseconds(50));

  // 3. Correctness check: every variant must give the same total length.
  const std::int64_t expected = kruskal_variant<true, true>(n, edges);
  if (kruskal_variant<true, false>(n, edges) != expected ||
      kruskal_variant<false, true>(n, edges) != expected ||
      (with_neither && kruskal_variant<false, false>(n, edges) != expected)) {
    std::abort();
  }

  // 4. Timed runs; the first one is the baseline for "relative".
  b.run("rank+compress", [&] {
    ankerl::nanobench::doNotOptimizeAway(kruskal_variant<true, true>(n, edges));
  });
  b.run("rank only", [&] {
    ankerl::nanobench::doNotOptimizeAway(kruskal_variant<true, false>(n, edges));
  });
  b.run("compress only", [&] {
    ankerl::nanobench::doNotOptimizeAway(kruskal_variant<false, true>(n, edges));
  });
  if (with_neither) {
    b.run("neither", [&] {
      ankerl::nanobench::doNotOptimizeAway(
          kruskal_variant<false, false>(n, edges));
    });
  }

  // 5. Untimed run that counts the average links per find.
  double links[4] = {0, 0, 0, 0};
  kruskal_variant<true, true>(n, edges, &links[0]);
  kruskal_variant<true, false>(n, edges, &links[1]);
  kruskal_variant<false, true>(n, edges, &links[2]);
  if (with_neither) kruskal_variant<false, false>(n, edges, &links[3]);

  // 6. Save the results.
  append_csv(graph, n, edges.size(), b, links);
}

}  // namespace

int main() {
  // 1. Start a fresh CSV with its header (truncates any old file).
  std::ofstream("disjoint_results.csv") << "graph,n,m,variant,seconds,avg_links\n";

  std::mt19937_64 rng(2183);

  // 2. Sparse, grid, and binomial+deep at n = 2^10 ... 2^18.
  //    "neither" is skipped above 2^16 because it is Theta(n) per find.
  for (const int k : {10, 12, 14, 16, 18}) {
    const int n = 1 << k;
    const int side = 1 << (k / 2);
    const bool with_neither = k <= 16;
    run("sparse", n, random_sparse(n, rng), with_neither);
    run("grid", side * side, grid(side, rng), with_neither);
    run("binomial+deep", n, binomial_then_deep(n), with_neither);
  }

  // 3. Dense graphs at n = 2^9 ... 2^12.
  for (const int k : {9, 10, 11, 12}) {
    run("dense", 1 << k, random_dense(1 << k, rng));
  }

  // 4. Star (chain-union) graphs at n = 2^11 ... 2^15; "neither" is Theta(n^2).
  for (const int k : {11, 12, 13, 14, 15}) {
    run("star", 1 << k, star(1 << k));
  }
}