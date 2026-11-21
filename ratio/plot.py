# Plot mean and minimum ratios from ratio/results.csv.  Dependencies: matplotlib, pandas.
# Run: python3 ratio/plot.py     -> ratio/ratio.png
import pandas as pd, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

df = pd.read_csv("ratio/results.csv")
fig, axes = plt.subplots(1, 2, figsize=(10, 4), sharey=True)
for ax, model in zip(axes, ["independent", "grows"]):
    sub = df[df.model == model]
    for rule, g in sub.groupby("rule"):
        line, = ax.plot(g.n, g.mean_ratio, "o-", label=f"{rule} (mean)")
        ax.plot(g.n, g.min_ratio, "s--", color=line.get_color(), alpha=.6, label=f"{rule} (min)")
    ax.set_xscale("log"); ax.set_xlabel("events per instance (n)")
    ax.set_title(f"reward model: {model}"); ax.grid(alpha=.3)
axes[0].set_ylabel("rule reward / best_events reward")
axes[1].legend(fontsize=7)
plt.tight_layout(); plt.savefig("ratio/ratio.png", dpi=150)
