"""Visualisations for MPHF build-trace analysis."""

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

BLUE = "#2196F3"
GREEN = "#4CAF50"
ORANGE = "#FF9800"
RED = "#F44336"
GREY = "#9E9E9E"


def _save(fig: plt.Figure, path: Path | str | None) -> None:
    if path:
        fig.savefig(path, dpi=150, bbox_inches="tight")


# ---------------------------------------------------------------------------
# 1. Scatter: n_children (log x) vs retries_used (jittered y)
# ---------------------------------------------------------------------------


def plot_retries_vs_size(
    df: pd.DataFrame, savepath: str | Path | None = None
) -> plt.Figure:
    """Scatter: node size vs retries needed. The main "are large nodes harder?" plot.

    - Blue dots  : succeeded on first attempt (retries_used == 0)
    - Green dots : succeeded after ≥1 retry
    - Red X      : all retries exhausted (failed node)
    Y-axis is jittered to show density at each retry level.
    """
    rng = np.random.default_rng(42)

    def jitter(s: pd.Series, scale: float = 0.18) -> np.ndarray:
        return s.values + rng.uniform(-scale, scale, size=len(s))

    ok0 = df[df["retries_used"] == 0]
    ok1 = df[df["success"] & (df["retries_used"] > 0)]
    fail = df[~df["success"]]

    fig, ax = plt.subplots(figsize=(11, 5))

    ax.scatter(
        ok0["n_children"],
        jitter(ok0["retries_used"]),
        alpha=0.25,
        s=7,
        color=BLUE,
        label=f"success, retry 0 (n={len(ok0):,})",
        rasterized=True,
    )
    ax.scatter(
        ok1["n_children"],
        jitter(ok1["retries_used"]),
        alpha=0.85,
        s=25,
        color=GREEN,
        label=f"success, retry ≥1 (n={len(ok1):,})",
        zorder=3,
    )
    ax.scatter(
        fail["n_children"],
        jitter(fail["retries_used"]),
        alpha=1.0,
        s=120,
        color=RED,
        marker="X",
        label=f"FAILED (n={len(fail):,})",
        zorder=4,
    )

    # Annotate failures
    for _, row in fail.iterrows():
        ax.annotate(
            f"  n={row['n_children']:.2e}",
            xy=(row["n_children"], row["retries_used"]),
            fontsize=7,
            color=RED,
            va="center",
        )

    ax.set_xscale("log")
    ax.set_xlabel("n_children (log scale)", fontsize=11)
    ax.set_ylabel("retries used", fontsize=11)
    max_r = int(df["retries_used"].max())
    ax.set_yticks(range(max_r + 1))
    ax.set_title("Retries used vs node size", fontsize=13)
    ax.legend(fontsize=9, loc="upper left")
    ax.grid(True, alpha=0.25, axis="y")

    fig.tight_layout()
    _save(fig, savepath)
    return fig


# ---------------------------------------------------------------------------
# 2. Bar chart: retry count distribution (log y)
# ---------------------------------------------------------------------------


def plot_retry_histogram(
    df: pd.DataFrame, savepath: str | Path | None = None
) -> plt.Figure:
    """Bar chart: how many nodes needed 0, 1, 2, … retries.

    Y-axis is log-scaled to make rare high-retry events visible alongside
    the dominant zero-retry majority.
    """
    counts = df["retries_used"].value_counts().sort_index()

    # Colour failed nodes differently
    failed_retry = (
        df[~df["success"]]["retries_used"].iloc[0] if (~df["success"]).any() else -1
    )
    colors = [RED if r == failed_retry else BLUE for r in counts.index]

    fig, ax = plt.subplots(figsize=(9, 4))
    bars = ax.bar(
        counts.index,
        counts.values,
        color=colors,
        edgecolor="white",
        linewidth=0.6,
    )
    ax.set_yscale("log")
    ax.set_xlabel("retries used  (0 = first attempt succeeded)", fontsize=11)
    ax.set_ylabel("number of nodes (log scale)", fontsize=11)
    ax.set_title("Distribution of retries needed per node", fontsize=13)
    ax.set_xticks(counts.index)

    for bar, val in zip(bars, counts.values):
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            val * 1.15,
            f"{val:,}",
            ha="center",
            va="bottom",
            fontsize=8,
        )

    # Legend patch for failed bar
    if failed_retry >= 0:
        from matplotlib.patches import Patch

        ax.legend(
            handles=[
                Patch(facecolor=BLUE, label="succeeded"),
                Patch(facecolor=RED, label="failed (all retries exhausted)"),
            ],
            fontsize=9,
        )

    fig.tight_layout()
    _save(fig, savepath)
    return fig


# ---------------------------------------------------------------------------
# 3. Scatter: n_children vs min_residual (only nodes with ≥1 failed retry)
# ---------------------------------------------------------------------------


def plot_residual_vs_size(
    df: pd.DataFrame, savepath: str | Path | None = None
) -> plt.Figure | None:
    """Scatter: node size vs smallest residual (2-core) seen across failed retries.

    Only includes nodes with at least one failed retry. This directly shows
    whether the fallback strategy (handle tiny residuals inline) is viable.
    """
    partial = df[df["retries_used"] > 0].copy()
    if partial.empty:
        return None

    ok = partial[partial["success"]]
    fail = partial[~partial["success"]]

    fig, ax = plt.subplots(figsize=(9, 5))

    ax.scatter(
        ok["n_children"],
        ok["min_residual"],
        alpha=0.8,
        s=40,
        color=GREEN,
        label=f"eventually succeeded (n={len(ok):,})",
        zorder=3,
    )
    ax.scatter(
        fail["n_children"],
        fail["min_residual"],
        alpha=1.0,
        s=120,
        color=RED,
        marker="X",
        label=f"FAILED, all retries exhausted (n={len(fail):,})",
        zorder=4,
    )

    # Annotate each point with its n and residual
    for _, row in partial.iterrows():
        ax.annotate(
            f"  n={row['n_children']:.1e}\n  res={row['min_residual']}",
            xy=(row["n_children"], row["min_residual"]),
            fontsize=6.5,
            color=RED if not row["success"] else GREEN,
            va="bottom",
        )

    ax.set_xscale("log")
    if partial["min_residual"].min() > 0:
        ax.set_yscale("log")
    ax.set_xlabel("n_children (log scale)", fontsize=11)
    ax.set_ylabel("min residual edges (log scale)", fontsize=11)
    ax.set_title(
        "Smallest 2-core residual vs node size\n"
        "(only nodes with ≥1 failed retry shown)",
        fontsize=12,
    )
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.25)

    fig.tight_layout()
    _save(fig, savepath)
    return fig


# ---------------------------------------------------------------------------
# 4. Scatter: n_children vs total elapsed time (bubble = retries)
# ---------------------------------------------------------------------------


def plot_time_vs_size(
    df: pd.DataFrame, savepath: str | Path | None = None
) -> plt.Figure:
    """Scatter: node size vs total build time (seconds). Color = retries_used.

    Shows where wall-clock time actually goes. Useful to quantify the
    cost of the current retry policy vs the alternative strategies.
    """
    fig, ax = plt.subplots(figsize=(11, 5))

    sc = ax.scatter(
        df["n_children"],
        df["total_elapsed_ms"] / 1_000,
        c=df["retries_used"],
        cmap="YlOrRd",
        s=np.clip(df["retries_used"] * 12 + 6, 6, 90),
        alpha=0.55,
        rasterized=True,
    )
    plt.colorbar(sc, ax=ax, label="retries used", shrink=0.8)

    # Annotate failed nodes
    for _, row in df[~df["success"]].iterrows():
        ax.annotate(
            f"FAILED\nn={row['n_children']:.2e}\n{row['total_elapsed_ms']/3_600_000:.1f} h",
            xy=(row["n_children"], row["total_elapsed_ms"] / 1_000),
            xytext=(-60, 20),
            textcoords="offset points",
            fontsize=8,
            color=RED,
            arrowprops=dict(arrowstyle="->", color=RED, lw=1),
        )

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("n_children (log scale)", fontsize=11)
    ax.set_ylabel("total elapsed time (seconds, log scale)", fontsize=11)
    ax.set_title("Build time vs node size  (color = retries used)", fontsize=13)
    ax.grid(True, alpha=0.2)

    fig.tight_layout()
    _save(fig, savepath)
    return fig


# ---------------------------------------------------------------------------
# 5. CDF of n_children
# ---------------------------------------------------------------------------


def plot_size_cdf(
    df: pd.DataFrame,
    threshold: int | None = None,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Empirical CDF of node sizes (n_children).

    Helps answer: "how many large nodes are there?" and "what fraction of
    nodes are concentrated in the tail where peeling is hard?"
    """
    sizes = np.sort(df["n_children"].values)
    cdf = np.arange(1, len(sizes) + 1) / len(sizes)

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(sizes, cdf, color=BLUE, linewidth=1.8)
    ax.fill_between(sizes, cdf, alpha=0.08, color=BLUE)

    milestones = [1e4, 1e5, 1e6, 1e7, 1e8]
    for m in milestones:
        frac = float((sizes <= m).mean())
        if 0.01 < frac < 0.999:
            ax.axvline(m, color=GREY, linestyle="--", linewidth=0.8, alpha=0.7)
            ax.text(
                m * 1.08,
                0.07,
                f"{frac * 100:.1f}%\n≤{m:.0e}",
                fontsize=7,
                color=GREY,
                va="bottom",
            )

    thresh = (
        threshold or int(df["threshold"].iloc[0]) if "threshold" in df.columns else None
    )
    title = "CDF of node sizes (n_children)"
    if thresh:
        title += f"  [threshold = {thresh:,}]"
    ax.set_xscale("log")
    ax.set_xlabel("n_children (log scale)", fontsize=11)
    ax.set_ylabel("cumulative fraction of nodes", fontsize=11)
    ax.set_title(title, fontsize=13)
    ax.set_ylim(0, 1.05)
    ax.grid(True, alpha=0.25)

    fig.tight_layout()
    _save(fig, savepath)
    return fig


# ---------------------------------------------------------------------------
# 6. Time breakdown: where does the wall clock go?
# ---------------------------------------------------------------------------


def plot_time_breakdown(
    df: pd.DataFrame, savepath: str | Path | None = None
) -> plt.Figure:
    """Horizontal bar chart: top-N nodes by elapsed time.

    Makes it immediately clear that a handful of large nodes dominate
    total build time, justifying the fallback strategy for just those.
    """
    top = df.nlargest(20, "total_elapsed_ms").copy()
    top["label"] = top.apply(
        lambda r: f"t{int(r['trie_id'])} n={r['n_children']:.2e}"
        + (" ✗" if not r["success"] else ""),
        axis=1,
    )
    top["elapsed_s"] = top["total_elapsed_ms"] / 1_000
    top = top.sort_values("elapsed_s")

    colors = [
        RED if not s else (ORANGE if r > 0 else BLUE)
        for s, r in zip(top["success"], top["retries_used"])
    ]

    fig, ax = plt.subplots(figsize=(10, 6))
    bars = ax.barh(top["label"], top["elapsed_s"], color=colors)

    ax.set_xlabel("total elapsed time (seconds)", fontsize=11)
    ax.set_title("Top-20 nodes by build time", fontsize=13)
    ax.set_xscale("log")
    ax.grid(True, alpha=0.2, axis="x")

    from matplotlib.patches import Patch

    ax.legend(
        handles=[
            Patch(facecolor=BLUE, label="success, retry 0"),
            Patch(facecolor=ORANGE, label="success, retry ≥1"),
            Patch(facecolor=RED, label="FAILED"),
        ],
        fontsize=9,
        loc="lower right",
    )

    fig.tight_layout()
    _save(fig, savepath)
    return fig
