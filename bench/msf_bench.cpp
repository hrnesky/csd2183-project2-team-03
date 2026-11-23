#include "nanobench.h"
#include "maintenance.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void check_same_weight(int n, const std::vector<Edge> &bridges) {
    const SpanningForest k = kruskal(n, bridges);
    const SpanningForest p = prim(n, bridges);
    if (k.total_length != p.total_length) {
        std::cerr << "Kruskal/Prim weight mismatch for n=" << n << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

// 1. Sparse random graph (~4n bridges)
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
// 3. 2D Grid graph (rows x cols = n)[cite: 10]
std::vector<Edge> make_grid(int rows, int cols, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::int64_t> len_dist(1, 1'000'000);

    std::vector<Edge> bridges;
    bridges.reserve(static_cast<std::size_t>(2) * rows * cols);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int u = r * cols + c;
            if (c + 1 < cols) {
                bridges.push_back(Edge{u, u + 1, len_dist(rng)});
            }
            if (r + 1 < rows) {
                bridges.push_back(Edge{u, u + cols, len_dist(rng)});
            }
        }
    }
    return bridges;
}

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("MSF: Kruskal vs Prim")
         .unit("graph")
         .epochs(7)
         .minEpochIterations(5);

    // Sparse graphs: 4 sizes including n = 2^18[cite: 10]
    for (int n : {1 << 12, 1 << 14, 1 << 16, 1 << 18}) {
        auto bridges = make_sparse(n, 42);
        check_same_weight(n, bridges);
        bench.run("sparse_kruskal_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(kruskal(n, bridges));
        });
        bench.run("sparse_prim_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(prim(n, bridges));
        });
    }

    // Dense graphs: 4 sizes including n = 2^12[cite: 10]
    for (int n : {1 << 9, 1 << 10, 1 << 11, 1 << 12}) {
        auto bridges = make_dense(n, 42);
        check_same_weight(n, bridges);
        bench.run("dense_kruskal_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(kruskal(n, bridges));
        });
        bench.run("dense_prim_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(prim(n, bridges));
        });
    }

    // Grid graphs: 4 sizes including n = 2^18 (512 x 512)[cite: 10]
    for (int side : {64, 128, 256, 512}) {
        int n = side * side;
        auto bridges = make_grid(side, side, 42);
        check_same_weight(n, bridges);
        bench.run("grid_kruskal_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(kruskal(n, bridges));
        });
        bench.run("grid_prim_n=" + std::to_string(n), [&] {
            ankerl::nanobench::doNotOptimizeAway(prim(n, bridges));
        });
    }
    
    std::ofstream csv("bench/results/msf_results.csv");
    csv << "benchmark,median_seconds,median_abs_percent_error\n";
    for (const auto &result : bench.results()) {
        csv << '"' << result.config().mBenchmarkName << "\","
            << result.median(ankerl::nanobench::Result::Measure::elapsed) << ','
            << result.medianAbsolutePercentError(
                   ankerl::nanobench::Result::Measure::elapsed)
            << '\n';
    }

    return 0;
}
