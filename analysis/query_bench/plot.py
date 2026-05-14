"""Plotting functions for query-benchmark comparisons (X-CLTJ vs H-CLTJ)."""

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

_PALETTE = {
    "TYPE1": "#4c72b0",
    "TYPE2": "#dd8452",
    "TYPE3": "#55a868",
    "UNKNOWN": "#8172b2",
}


def plot_time_scatter(
    df: pd.DataFrame,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Scatter plot: X-CLTJ time vs H-CLTJ time (µs), colored by query type.

    Points above the diagonal → H-CLTJ is slower.
    Points below the diagonal → H-CLTJ is faster.
    """
    fig, ax = plt.subplots(figsize=(7, 6))

    for qtype, g in df.groupby("query_type"):
        ax.scatter(
            g["time_us_xcltj"],
            g["time_us_hcltj"],
            label=qtype,
            alpha=0.55,
            s=18,
            color=_PALETTE.get(qtype),
        )

    lim_max = max(df["time_us_xcltj"].max(), df["time_us_hcltj"].max()) * 1.05
    lim_min = max(min(df["time_us_xcltj"].min(), df["time_us_hcltj"].min()) * 0.9, 1e-1)
    ax.plot([lim_min, lim_max], [lim_min, lim_max], "k--", lw=1, label="equal")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("X-CLTJ time (µs)")
    ax.set_ylabel("H-CLTJ time (µs)")
    ax.set_title("Query time: X-CLTJ vs H-CLTJ")
    ax.legend(title="Query type", markerscale=1.5)
    ax.grid(True, which="both", ls=":", alpha=0.4)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_speedup_cdf(
    df: pd.DataFrame,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """CDF of speedup (xcltj_time / hcltj_time) overall and per query type.

    Speedup > 1 → H-CLTJ faster; < 1 → X-CLTJ faster.
    """
    fig, ax = plt.subplots(figsize=(8, 5))

    # Overall
    s = df["speedup"].sort_values()
    ax.plot(s.values, np.linspace(0, 1, len(s)), color="black", lw=2, label="ALL")

    for qtype, g in df.groupby("query_type"):
        s = g["speedup"].sort_values()
        ax.plot(
            s.values,
            np.linspace(0, 1, len(s)),
            color=_PALETTE.get(qtype),
            label=qtype,
        )

    ax.axvline(1.0, color="red", ls="--", lw=1, label="equal (speedup=1)")
    ax.set_xscale("log")
    ax.set_xlabel("Speedup (X-CLTJ / H-CLTJ)")
    ax.set_ylabel("Cumulative fraction of queries")
    ax.set_title("CDF of speedup by query type")
    ax.legend()
    ax.grid(True, which="both", ls=":", alpha=0.4)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_median_by_type(
    summary: pd.DataFrame,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Grouped bar chart: median query time (µs) per type for X-CLTJ and H-CLTJ."""
    types = summary.index.tolist()
    x = np.arange(len(types))
    width = 0.35

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.bar(
        x - width / 2,
        summary["median_time_us_xcltj"],
        width,
        label="X-CLTJ",
        color="#4c72b0",
    )
    ax.bar(
        x + width / 2,
        summary["median_time_us_hcltj"],
        width,
        label="H-CLTJ",
        color="#dd8452",
    )

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(types)
    ax.set_xlabel("Query type")
    ax.set_ylabel("Median query time (µs, log scale)")
    ax.set_title("Median query time by type: X-CLTJ vs H-CLTJ")
    ax.legend(loc="upper left")
    ax.grid(True, axis="y", ls=":", alpha=0.4)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_mean_by_type(
    summary: pd.DataFrame,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Grouped bar chart: mean query time (µs) per type for X-CLTJ and H-CLTJ.

    Means are sensitive to heavy tails (e.g. timeout caps, ultra-dense queries).
    """
    types = summary.index.tolist()
    x = np.arange(len(types))
    width = 0.35

    fig, ax = plt.subplots(figsize=(7, 5))
    ax.bar(
        x - width / 2,
        summary["mean_time_us_xcltj"],
        width,
        label="X-CLTJ",
        color="#4c72b0",
    )
    ax.bar(
        x + width / 2,
        summary["mean_time_us_hcltj"],
        width,
        label="H-CLTJ",
        color="#dd8452",
    )

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(types)
    ax.set_xlabel("Query type")
    ax.set_ylabel("Mean query time (µs, log scale)")
    ax.set_title("Mean query time by type: X-CLTJ vs H-CLTJ")
    ax.legend(loc="upper left")
    ax.grid(True, axis="y", ls=":", alpha=0.4)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_winrate_by_type(
    df: pd.DataFrame,
    tie_band: float = 0.05,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Horizontal stacked bar chart of win-rate (%) per query type and overall.

    ``tie_band`` defines the symmetric tolerance around speedup=1 that counts as
    a tie: speedup in [1-tie_band, 1+tie_band].
    """
    groups = sorted(df["query_type"].unique()) + ["ALL"]
    pct_x, pct_tie, pct_h = [], [], []

    for label in groups:
        sub = df if label == "ALL" else df[df["query_type"] == label]
        n = len(sub)
        h_wins = (sub["speedup"] > 1 + tie_band).sum()
        x_wins = (sub["speedup"] < 1 - tie_band).sum()
        ties = n - h_wins - x_wins
        pct_x.append(100.0 * x_wins / n)
        pct_tie.append(100.0 * ties / n)
        pct_h.append(100.0 * h_wins / n)

    fig, ax = plt.subplots(figsize=(8, 3.5))
    y = np.arange(len(groups))

    bars_x = ax.barh(y, pct_x, color="#4c72b0", label="X-CLTJ faster")
    bars_tie = ax.barh(y, pct_tie, left=pct_x, color="#cccccc", label="Tie")
    left_h = [a + b for a, b in zip(pct_x, pct_tie)]
    bars_h = ax.barh(y, pct_h, left=left_h, color="#dd8452", label="H-CLTJ faster")

    for bars in (bars_x, bars_tie, bars_h):
        for bar in bars:
            w = bar.get_width()
            if w > 6:
                ax.text(
                    bar.get_x() + w / 2,
                    bar.get_y() + bar.get_height() / 2,
                    f"{w:.0f}%",
                    ha="center",
                    va="center",
                    fontsize=9,
                    fontweight="bold",
                )

    ax.set_yticks(y)
    ax.set_yticklabels(groups)
    ax.set_xlabel("Percentage of queries")
    ax.set_title("Win-rate: X-CLTJ vs H-CLTJ by query type")
    ax.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.15),
        ncol=3,
        fontsize=9,
        frameon=False,
    )
    ax.set_xlim(0, 100)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_timeout_heatmap(
    df: pd.DataFrame,
    timeout_ns: int = 600_000_000_000,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Annotated heatmap of timeout counts per query type and engine."""
    types = sorted(df["query_type"].unique())
    labels = types + ["TOTAL"]

    counts = np.zeros((len(labels), 2), dtype=int)
    for i, qtype in enumerate(types):
        sub = df[df["query_type"] == qtype]
        counts[i, 0] = (sub["time_ns_xcltj"] >= timeout_ns).sum()
        counts[i, 1] = (sub["time_ns_hcltj"] >= timeout_ns).sum()
    counts[-1, 0] = (df["time_ns_xcltj"] >= timeout_ns).sum()
    counts[-1, 1] = (df["time_ns_hcltj"] >= timeout_ns).sum()

    fig, ax = plt.subplots(figsize=(4, 3))
    sns.heatmap(
        counts,
        annot=True,
        fmt="d",
        cmap="YlOrRd",
        xticklabels=["X-CLTJ", "H-CLTJ"],
        yticklabels=labels,
        linewidths=0.5,
        cbar_kws={"label": "# timeouts"},
        ax=ax,
    )
    ax.set_title(f"Timeouts (>= {timeout_ns / 1e9:.0f}s) by type")

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_summary_card(
    df: pd.DataFrame,
    overall: pd.DataFrame,
    by_type: pd.DataFrame,
    timeout_ns: int = 600_000_000_000,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Dashboard-style summary card with key benchmark metrics."""
    row = overall.iloc[0]
    n = int(row["n_queries"])
    to_x = int((df["time_ns_xcltj"] >= timeout_ns).sum())
    to_h = int((df["time_ns_hcltj"] >= timeout_ns).sum())
    pct_h = row["pct_hcltj_faster"]
    med_sp = row["median_speedup"]
    med_x = row["median_time_us_xcltj"]
    med_h = row["median_time_us_hcltj"]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.axis("off")

    def _fmt_us(val: float) -> str:
        if val >= 1_000_000:
            return f"{val / 1_000_000:.2f} s"
        if val >= 1_000:
            return f"{val / 1_000:.2f} ms"
        return f"{val:.1f} µs"

    title = "Benchmark Summary — X-CLTJ vs H-CLTJ"
    ax.text(0.5, 0.95, title, transform=ax.transAxes, fontsize=14,
            fontweight="bold", ha="center", va="top")

    lines = [
        f"Queries: {n}",
        f"Timeouts (>= {timeout_ns / 1e9:.0f}s):  X = {to_x},  H = {to_h}",
        "",
        f"Median time:  X = {_fmt_us(med_x)},  H = {_fmt_us(med_h)}",
        f"Median speedup (X/H): {med_sp:.3f}",
        f"Global win-rate:  X = {100 - pct_h:.1f}%,  H = {pct_h:.1f}%",
    ]
    ax.text(0.05, 0.78, "\n".join(lines), transform=ax.transAxes,
            fontsize=11, va="top", family="monospace")

    header = f"{'Type':<8} {'n':>5}  {'Med X':>10}  {'Med H':>10}  {'H wins':>7}"
    table_lines = [header, "-" * len(header)]
    for qtype in sorted(by_type.index):
        r = by_type.loc[qtype]
        table_lines.append(
            f"{qtype:<8} {int(r['n_queries']):>5}  "
            f"{_fmt_us(r['median_time_us_xcltj']):>10}  "
            f"{_fmt_us(r['median_time_us_hcltj']):>10}  "
            f"{r['pct_hcltj_faster']:>6.1f}%"
        )
    ax.text(0.05, 0.35, "\n".join(table_lines), transform=ax.transAxes,
            fontsize=10, va="top", family="monospace")

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_speedup_boxplot(
    df: pd.DataFrame,
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Box plot of speedup distribution per query type."""
    fig, ax = plt.subplots(figsize=(7, 5))

    order = sorted(df["query_type"].unique())
    palette = {k: v for k, v in _PALETTE.items() if k in order}

    sns.boxplot(
        data=df,
        x="query_type",
        y="speedup",
        order=order,
        palette=palette,
        ax=ax,
        flierprops={"marker": ".", "markersize": 3, "alpha": 0.4},
    )

    ax.axhline(1.0, color="red", ls="--", lw=1, label="equal (speedup=1)")
    ax.set_yscale("log")
    ax.set_xlabel("Query type")
    ax.set_ylabel("Speedup (X-CLTJ / H-CLTJ, log scale)")
    ax.set_title("Speedup distribution by query type")
    ax.legend()
    ax.grid(True, axis="y", ls=":", alpha=0.4)

    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig
