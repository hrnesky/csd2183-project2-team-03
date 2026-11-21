# Chat summary: Kruskal edge order bias
# Link: https://chat.deepseek.com/share/3wouv065sdw0lo7jci

# you asked

message time: 2026-11-21 18:36:49

std::vector<Edge> make_sparse(int n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> node_dist(0, n-1);
    std::uniform_int_distribution<std::int64_t>len_dist(1, 1000000);

    std::size_t target = static_cast<std::size_t>(4) * n;
    std::unordered_set<std::uint64_t> seen;
    seen.reserve(target*2);

    std::vector<Edge> bridges;
    bridges.reserve(target);

    while(bridges.size() < target) {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) continue;
        if (u > v) std::swap(u, v);

        std::uint64_t key = (static_cast<std::uint64_t>(u) << 32) | static_cast<std::uint64_t>(v);
        if (seen.insert(key).second) {
            bridges.push_back(Edge{u,v,len_dist(rng)});
        }
    }
    return bridges;
}
// 2. Dense random graph (~n^2 / 4 bridges)[cite: 10]
std::vector<Edge> make_dense(int n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::bernoulli_distribution keep(0.5);
    std::uniform_int_distribution<std::int64_t> len_dist(1, 1'000'000);

    std::vector<Edge> bridges;
    bridges.reserve(static_cast<std::size_t>(n) * n / 4);

    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (keep(rng)) {
                bridges.push_back(Edge{u, v, len_dist(rng)});
            }
        }
    }
    return bridges;
}

here is my make_sparse and make_dense implementation. for sparse graphs (~4n bridges) and dense graphs (~(n^2)/4 bridges), i used std::mt19937 with a fixed seed. could generating edges in sorted (u, v) order in make_dense give Kruskal an unfair advantage by pre-sorting the edge list and should I shuffle the edges first?

---

# deepseek response

# Short answer: yes, shuffle. It's a real (if usually small) bias, and it's cheap to fix.

## Why the sorted order matters

Kruskal's algorithm iterates edges in input order and does `find(u) != find(v)` before a union. The *number of union operations* is invariant (exactly `n-1` for a connected component), but the *number of find calls* and, more importantly, the **union-by-rank/path-compression behavior and the cache locality of parent-array accesses** depend on input order.

Sorted `(u, v)` input is not a random ordering of edges. Vertices are processed roughly in numeric order, so early edges tend to connect low-index vertices and later edges touch high-index vertices. That means:

- **Parent-array access locality is unusual** — you get long runs of edges whose endpoints are "near" each other in a way real random graphs don't exhibit.
- **The number of rejected edges per union** deviates from the random-order expectation. In random edge order, once a component of size `k` exists, the probability that the next edge is internal is `C(k,2)/C(n,2)`. With sorted order you systematically pick up intra-low-index edges first, which changes when components merge and hence the find/union trace.
- **Timing can be measurably different** on cache-sensitive architectures, not just in theory.

So if you're benchmarking Kruskal and comparing against another algorithm (Prim, Borůvka, Kruskal with sorting included, etc.), the sorted input can flatter Kruskal specifically, because the sorted-and-just-shuffled-by-generator edge list is *already* the order Kruskal wants — you skipped the `std::sort` cost that a fair comparison would charge it. That's the bigger issue than cache locality: **you're feeding Kruskal an edge list that's already (nearly) in the order it would have produced after sorting**, so any comparison that includes sorting is unfair to the competitors.

## How your two generators differ

- `make_sparse` draws `u, v` independently and inserts into an `unordered_set`, so the output order is essentially a random permutation of the chosen edges. Kruskal gets a random edge order — this is the correct baseline.
- `make_dense` emits edges in lexicographic `(u, v)` order. Kruskal gets a pre-sorted list. That's the asymmetry.

## Fix

Shuffle once at the end of `make_dense` (and arguably in `make_sparse` too, for consistency and to remove any residual iteration-order dependence):

```cpp
std::shuffle(bridges.begin(), bridges.end(), rng);
```

Two notes:

1. **Shuffle *after* generating all edges**, and note that `len_dist` has already been consumed in generation order — if you care about reproducing a specific graph from a seed, shuffling changes the RNG stream, so document it. If you want the *graph* (edge set + weights) to be independent of shuffle, generate weights first, then shuffle only the ordering — but in practice shuffling with the same `rng` after generation is fine and reproducible.
2. If you want a **fair head-to-head** with sort-included Kruskal, keep a variant that returns unshuffled (pre-sorted) edges and let Kruskal sort them, and a variant that returns pre-shuffled edges for the "given in random order" case. Benchmark both and state which you're reporting. Many textbook benchmarks feed Kruskal a shuffled list precisely because "the graph is given as an unordered edge set."

## Also worth fixing regardless of shuffle

- `make_dense`'s `reserve(n*n/4)` is a tight estimate (binomial mean `0.5 * n(n-1)/2 ≈ n²/4`), so you'll get ~1 reallocation occasionally; harmless.
- `node_dist` is `uniform_int_distribution<int>(0, n-1)` — fine, but for `n` near `INT_MAX` the `u > v` swap plus `<<32` key still works since you cast to `uint64_t` first. Good.
- `make_sparse`'s rejection loop will spin badly if `target` approaches `n(n-1)/2`; for `4n` it's fine, but guard against `n < 3` (target `4n` with `n=2` only has 1 possible edge → infinite loop). Worth an `assert` or clamp.

---
Powered by [AI Exporter](https://saveai.net)