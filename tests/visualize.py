# Matheo Kesar (again, mostly claude generated)
# Plots incremental benchmark results from the vacuum filter CSV output

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import sys
import os

# ── Config ────────────────────────────────────────────────────────────────────
CSV_PATH  = "incremental.csv"
OUT_DIR   = "plots"
os.makedirs(OUT_DIR, exist_ok=True)

# Color palette — one color per k value, distinct style per data type
K_COLORS  = {10: "#e63946", 20: "#f4a261", 50: "#2a9d8f",
             100: "#457b9d", 200: "#9b5de5", 0: "#adb5bd"}
TYPE_STYLE = {"ecoli": "-", "artificial": "--"}
TYPE_LABEL = {"ecoli": "E. coli", "artificial": "Artificial"}
 
# ── Load data ─────────────────────────────────────────────────────────────────
df = pd.read_csv(CSV_PATH)
df.columns = df.columns.str.strip()

# Smooth noisy throughput columns with a rolling window
WINDOW = 20  # increase if still spiky
for col in ["insert_throughput_mops", "lookup_throughput_mops"]:
    df[col] = df.groupby(["data_type", "k"])[col].transform(
        lambda x: x.rolling(window=WINDOW, center=True, min_periods=1).mean()
    )
 
data_types = df["data_type"].unique()
k_values   = sorted(df["k"].unique())
 
# ── Helpers ───────────────────────────────────────────────────────────────────
def save(fig, name):
    path = os.path.join(OUT_DIR, name)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    print(f"Saved: {path}")
    plt.close(fig)
 
def base_fig(title, xlabel, ylabel):
    fig, ax = plt.subplots(figsize=(10, 5))
    ax.set_title(title, fontsize=14, fontweight="bold", pad=12)
    ax.set_xlabel(xlabel, fontsize=11)
    ax.set_ylabel(ylabel, fontsize=11)
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.spines[["top", "right"]].set_visible(False)
    return fig, ax
 
def add_legend(ax):
    ax.legend(fontsize=8, framealpha=0.85, loc="best")
 
# ── Plot 1: Insertion throughput vs load factor ───────────────────────────────
fig, ax = base_fig(
    "Insertion Throughput vs Load Factor",
    "Load Factor", "Throughput (MOPS)"
)
for dtype in data_types:
    for k in k_values:
        sub = df[(df["data_type"] == dtype) & (df["k"] == k)]
        if sub.empty:
            continue
        label = f"{TYPE_LABEL[dtype]}, k={k}" if k != 0 else TYPE_LABEL[dtype]
        ax.plot(sub["load_factor"], sub["insert_throughput_mops"],
                color=K_COLORS.get(k, "#888"),
                linestyle=TYPE_STYLE.get(dtype, "-"),
                linewidth=1.6, label=label, alpha=0.85)
add_legend(ax)
save(fig, "insert_throughput_vs_lf.png")
 
# ── Plot 2: False positive rate vs load factor ────────────────────────────────
fig, ax = base_fig(
    "False Positive Rate vs Load Factor",
    "Load Factor", "False Positive Rate"
)
for dtype in data_types:
    for k in k_values:
        sub = df[(df["data_type"] == dtype) & (df["k"] == k)]
        if sub.empty:
            continue
        label = f"{TYPE_LABEL[dtype]}, k={k}" if k != 0 else TYPE_LABEL[dtype]
        ax.plot(sub["load_factor"], sub["false_positive_rate"],
                color=K_COLORS.get(k, "#888"),
                linestyle=TYPE_STYLE.get(dtype, "-"),
                linewidth=1.6, label=label, alpha=0.85)
ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{x*100:.4f}%"))
ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=8))
add_legend(ax)
save(fig, "fpr_vs_lf.png")
 
# ── Plot 4: All three side by side for a single k (default k=50) ──────────────
K_FOCUS = 50 if 50 in k_values else k_values[0]
 
fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))
fig.suptitle(f"Vacuum Filter Benchmarks — k={K_FOCUS}", fontsize=14, fontweight="bold")
 
metrics = [
    ("insert_throughput_mops", "Insert Throughput (MOPS)"),
    ("false_positive_rate",    "False Positive Rate"),
]
 
for ax, (col, ylabel) in zip(axes, metrics):
    for dtype in data_types:
        sub = df[(df["data_type"] == dtype) & (df["k"] == K_FOCUS)]
        if sub.empty:
            continue
        ax.plot(sub["load_factor"], sub[col],
                linestyle=TYPE_STYLE.get(dtype, "-"),
                linewidth=1.8, label=TYPE_LABEL[dtype],
                color="#2a9d8f" if dtype == "ecoli" else "#e63946")
    ax.set_xlabel("Load Factor", fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.4)
    ax.spines[["top", "right"]].set_visible(False)
    if col == "false_positive_rate":
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{x*100:.4f}%"))
        ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=8))
    ax.legend(fontsize=8)
 
plt.tight_layout()
save(fig, f"overview_k{K_FOCUS}.png")
 
# ── Plot 5: Failed inserts vs load factor (if any failures) ───────────────────
if df["failed_inserts"].sum() > 0:
    fig, ax = base_fig(
        "Cumulative Failed Inserts vs Load Factor",
        "Load Factor", "Failed Inserts"
    )
    for dtype in data_types:
        for k in k_values:
            if k < 20: continue # sve manje od 20 samo dominira grafom
            sub = df[(df["data_type"] == dtype) & (df["k"] == k)]
            if sub.empty or sub["failed_inserts"].sum() == 0:
                continue
            label = f"{TYPE_LABEL[dtype]}, k={k}" if k != 0 else TYPE_LABEL[dtype]
            ax.plot(sub["load_factor"], sub["failed_inserts"].cumsum(),
                    color=K_COLORS.get(k, "#888"),
                    linestyle=TYPE_STYLE.get(dtype, "-"),
                    linewidth=1.6, label=label)
    add_legend(ax)
    save(fig, "failed_inserts_vs_lf.png")
else:
    print("No failed inserts found — skipping that plot.")
 
print(f"\nAll plots saved to: {OUT_DIR}/")
