'''
Plots and tabulates the disjoint-set experiment (bench/disjoint_variants.cpp).

Purpose:
  1. Reads the CSV written by the benchmark (graph, n, m, variant, seconds,
     avg_links), one row per graph family, size and disjoint-set variant.
  2. Plots the median time per run in milliseconds against n, one panel per
     graph family, for the four variants (rank+compress, rank only,
     compress only, neither), on log-log axes.
  3. Plots the average number of parent links that find follows against n,
     in the same layout.
  4. Prints a table of times relative to rank+compress (1.0) and of average
     links, and saves it as a CSV for the report.

To run the script (from repo root):
python bench/analysis/plot_disjoint.py build/disjoint_results.csv
'''

import os
import sys

import matplotlib.pyplot as plt
import pandas as pd

csv_path = sys.argv[1] if len(sys.argv) > 1 else "build/disjoint_results.csv"
out_dir = "bench/results"
os.makedirs(out_dir, exist_ok=True)

df = pd.read_csv(csv_path)
df["ms"] = df["seconds"] * 1e3  # one unit for the whole report

# 1. Warn about duplicate rows (e.g. several runs appended to one file).
dups = df.duplicated(["graph", "n", "variant"], keep=False)
if dups.any():
    print("WARNING: duplicate (graph, n, variant) rows; values will be averaged.")

order = ["rank+compress", "rank only", "compress only", "neither"]
graphs = list(df["graph"].unique())

# 2. One figure per measurement, one panel per graph family.
for value, ylabel, fname in [
    ("ms", "median time per run (ms)", "disjoint_time.png"),
    ("avg_links", "average parent links per find", "disjoint_links.png"),
]:
    fig, axes = plt.subplots(1, len(graphs), figsize=(4 * len(graphs), 3.5), squeeze=False)
    styles = {"rank+compress": ("o", "-", 8), "rank only": ("s", "--", 6), "compress only": ("^", "-.", 5), "neither": ("x", ":", 6)}
    for ax, g in zip(axes[0], graphs):
        sub = df[df.graph == g]
        for v in order:
            s = sub[sub.variant == v].groupby("n", as_index=False)[value].mean()
            if not s.empty:
              marker, ls, ms = styles[v]
              ax.plot(s["n"], s[value], marker=marker, linestyle=ls, markersize=ms,
        label=v, clip_on=False)
        ax.set_xscale("log", base=2)
        if value == "avg_links":
          ax.set_yscale("symlog", linthresh=0.5)
          ax.margins(y=0.15)
          ax.set_ylim(bottom=0)
        else:
          ax.set_yscale("log")
        ax.set_title(g)
        ax.set_xlabel("n")
    axes[0][0].set_ylabel(ylabel)
    fig.tight_layout()
    handles, labels = axes[0][0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=4, fontsize=8)
    fig.tight_layout(rect=[0, 0, 1, 0.92])
    fig.savefig(os.path.join(out_dir, fname), dpi=150)
    plt.close(fig)

# 3. Relative table (rank+compress = 1.0) and average links, for the report.
base = (df[df.variant == "rank+compress"]
        .groupby(["graph", "n"])["seconds"].mean())
df["relative"] = df.apply(lambda r: r.seconds / base[(r.graph, r.n)], axis=1)
table = df.pivot_table(index=["graph", "n"], columns="variant",
                       values=["relative", "avg_links"]).round(2)
print(table.to_string())
table.to_csv(os.path.join(out_dir, "disjoint_table.csv"))