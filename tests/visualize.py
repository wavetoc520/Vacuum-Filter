# Matheo Kesar (again, mostly claude generated)
# Plots incremental benchmark results from the vacuum filter CSV output

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

# ── Config ────────────────────────────────────────────────────────────────────
CSV_OURS     = "incremental.csv"
CSV_OFFICIAL = "official_plots/incremental_official.csv"
OUT_DIR      = "plots"
os.makedirs(OUT_DIR, exist_ok=True)

# Color palette — one color per k value
K_COLORS = {10: "#e63946", 20: "#f4a261", 50: "#2a9d8f",
            100: "#457b9d", 200: "#9b5de5", 0: "#adb5bd"}

# data_type drives line style (solid vs dashed within each implementation)
TYPE_STYLE = {"ecoli": "-", "artificial": "--"}
TYPE_LABEL = {"ecoli": "E. coli", "artificial": "Artificial"}

# Implementation drives solid vs dotted
IMPL_STYLE = {"ours": "-", "official": ":"}
IMPL_WIDTH = {"ours": 1.8, "official": 1.4}

# ── Load & tag data ───────────────────────────────────────────────────────────
def load(path, impl):
    d = pd.read_csv(path)
    d.columns = d.columns.str.strip()
    d["impl"] = impl
    return d

df = pd.concat([load(CSV_OURS, "ours"), load(CSV_OFFICIAL, "official")],
               ignore_index=True)

# Smooth noisy throughput columns with a rolling window
WINDOW = 20
for col in ["insert_throughput_mops", "lookup_throughput_mops"]:
    df[col] = df.groupby(["impl", "data_type", "k"])[col].transform(
        lambda x: x.rolling(window=WINDOW, center=True, min_periods=1).mean()
    )

data_types = df["data_type"].unique()
k_values   = sorted(df["k"].unique())
impls      = ["ours", "official"]

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

def iter_groups():
    """Yield (impl, dtype, k, subset) for every non-empty combination."""
    for impl in impls:
        for dtype in data_types:
            for k in k_values:
                sub = df[(df["impl"] == impl) & (df["data_type"] == dtype) & (df["k"] == k)]
                if not sub.empty:
                    yield impl, dtype, k, sub

def line_style(impl, dtype):
    """Combine impl dotting with data_type dashing."""
    if impl == "official":
        return (0, (1, 1)) if dtype == "artificial" else ":"   # dotted for official
    else:
        return TYPE_STYLE.get(dtype, "-")                       # solid / dashed for ours

def make_label(impl, dtype, k):
    impl_tag = "Official" if impl == "official" else "Ours"
    k_tag    = f", k={k}" if k != 0 else ""
    return f"{impl_tag} — {TYPE_LABEL.get(dtype, dtype)}{k_tag}"

# ── Plot 1: Insertion throughput vs load factor ───────────────────────────────
fig, ax = base_fig(
    "Insertion Throughput vs Load Factor",
    "Load Factor", "Throughput (MOPS)"
)
for impl, dtype, k, sub in iter_groups():
    ax.plot(sub["load_factor"], sub["insert_throughput_mops"],
            color=K_COLORS.get(k, "#888"),
            linestyle=line_style(impl, dtype),
            linewidth=IMPL_WIDTH[impl],
            label=make_label(impl, dtype, k),
            alpha=0.85)
add_legend(ax)
save(fig, "insert_throughput_vs_lf.png")

# ── Plot 2: False positive rate vs load factor ────────────────────────────────
fig, ax = base_fig(
    "False Positive Rate vs Load Factor",
    "Load Factor", "False Positive Rate"
)
for impl, dtype, k, sub in iter_groups():
    ax.plot(sub["load_factor"], sub["false_positive_rate"],
            color=K_COLORS.get(k, "#888"),
            linestyle=line_style(impl, dtype),
            linewidth=IMPL_WIDTH[impl],
            label=make_label(impl, dtype, k),
            alpha=0.85)
ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{x*100:.4f}%"))
ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=8))
add_legend(ax)
save(fig, "fpr_vs_lf.png")

# ── Plot 3: Overview side-by-side for a single k ─────────────────────────────
K_FOCUS = 50 if 50 in k_values else k_values[0]

fig, axes = plt.subplots(1, 2, figsize=(13, 4.5))
fig.suptitle(f"Vacuum Filter Benchmarks — k={K_FOCUS}", fontsize=14, fontweight="bold")

metrics = [
    ("insert_throughput_mops", "Insert Throughput (MOPS)"),
    ("false_positive_rate",    "False Positive Rate"),
]

DT_COLOR = {"ecoli": "#2a9d8f", "artificial": "#e63946"}

for ax, (col, ylabel) in zip(axes, metrics):
    for impl in impls:
        for dtype in data_types:
            sub = df[(df["impl"] == impl) & (df["data_type"] == dtype) & (df["k"] == K_FOCUS)]
            if sub.empty:
                continue
            impl_tag = "Official" if impl == "official" else "Ours"
            ax.plot(sub["load_factor"], sub[col],
                    linestyle=line_style(impl, dtype),
                    linewidth=IMPL_WIDTH[impl],
                    label=f"{impl_tag} — {TYPE_LABEL.get(dtype, dtype)}",
                    color=DT_COLOR.get(dtype, "#888"))
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

# ── Plot 4: Failed inserts vs load factor (if any failures) ──────────────────
if df["failed_inserts"].sum() > 0:
    fig, ax = base_fig(
        "Cumulative Failed Inserts vs Load Factor",
        "Load Factor", "Failed Inserts"
    )
    for impl, dtype, k, sub in iter_groups():
        if k < 20 or sub["failed_inserts"].sum() == 0:
            continue
        ax.plot(sub["load_factor"], sub["failed_inserts"].cumsum(),
                color=K_COLORS.get(k, "#888"),
                linestyle=line_style(impl, dtype),
                linewidth=IMPL_WIDTH[impl],
                label=make_label(impl, dtype, k))
    add_legend(ax)
    save(fig, "failed_inserts_vs_lf.png")
else:
    print("No failed inserts found — skipping that plot.")

print(f"\nAll plots saved to: {OUT_DIR}/")