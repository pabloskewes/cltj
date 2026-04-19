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
    ax.legend()
    ax.grid(True, axis="y", ls=":", alpha=0.4)

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
