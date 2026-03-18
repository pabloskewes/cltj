"""Plotting functions for children-histogram analysis."""

from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def plot_distribution(
    histograms: dict[str, pd.DataFrame],
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Log-log scatter: children count vs number of nodes, one subplot per trie."""
    n = len(histograms)
    cols = min(n, 3)
    rows = (n + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(5 * cols, 4 * rows), squeeze=False)

    for ax, (name, hist) in zip(axes.flat, histograms.items()):
        ax.scatter(hist["children"], hist["nodes"], s=12, alpha=0.7)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("children per node")
        ax.set_ylabel("number of nodes")
        ax.set_title(name)
        ax.grid(True, which="both", ls=":", alpha=0.4)

    for ax in axes.flat[n:]:
        ax.set_visible(False)

    fig.suptitle("Children-per-node distribution (log-log)", y=1.02)
    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig


def plot_coverage(
    coverage_tables: dict[str, pd.DataFrame],
    savepath: str | Path | None = None,
) -> plt.Figure:
    """Line plot: threshold vs % children covered, one line per trie."""
    fig, ax = plt.subplots(figsize=(8, 5))

    for name, cov in coverage_tables.items():
        ax.plot(cov.index, cov["pct_children"], marker="o", markersize=4, label=name)

    ax.set_xscale("log")
    ax.set_xlabel("threshold")
    ax.set_ylabel("% children covered by hashing")
    ax.set_title("Hashing coverage vs threshold")
    ax.legend()
    ax.grid(True, which="both", ls=":", alpha=0.4)
    fig.tight_layout()
    if savepath:
        fig.savefig(savepath, dpi=150, bbox_inches="tight")
    return fig
