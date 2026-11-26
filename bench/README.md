# Benchmark Documentation

## Environment
*   **Operating System:** Ubuntu 26.04.1 LTS
*   **Processor:** AMD Ryzen 5 5600 6-Core Processor
*   **Compiler:** g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0

## Dependencies
*   C++23 compiler (g++ or clang++)
*   `nanobench` (included in the repository)
*   Python 3 (`pandas` and `matplotlib` required for analysis scripts)

## Reproducing the Results
All experiments were conducted on the same machine. To reproduce:
1. Run `make bench` to compile the benchmark executables.
2. Run `python .\bench\analysis\plot_disjoint.py .\build\disjoint_results.csv` and `.\build\msf_bench.exe` to generate the `.csv` files.
3. Run `python bench/analysis/plot_disjoint.py` to generate the plots and relative tables.

## Conclusions
Our results empirically correspond well with the theoretical complexities of time. The speedup ratios indicate that the selection of algorithms should be based on the nature of the input – Kruskal’s algorithm for disjoint sets is faster on sparse graphs, while Prim's algorithm using heaps is more efficient on dense graphs. Likewise, the results from the disjoint sets experiment indicate that both the union-by-rank and the path-compression operations are necessary for obtaining the near-constant amortized O(α(n)) performance.