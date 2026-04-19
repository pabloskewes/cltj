import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


ALT_COL = "alternation_complexity"
MIN_COL = "q * min"
TEORICAL_COL = "δ * log2(n)"


def plot_scatter_alt_vs_min(
    df: pd.DataFrame,
    title: str = "Alternation Complexity vs Minimum Size",
    x_col: str = ALT_COL,
    y_col: str = MIN_COL,
) -> plt.Figure:
    """
    Plot a scatter plot of alternation complexity vs minimum size.
    """
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.scatter(df[x_col], df[y_col], alpha=0.5)
    ax.set_xlabel(x_col)
    ax.set_ylabel(y_col)
    ax.set_title(title)
    ax.grid(True, which="both", ls="--", alpha=0.7)

    ax.set_xscale("log")
    ax.set_yscale("log")

    lims = [
        np.min([df[x_col].min(), df[y_col].min()]),
        np.max([df[x_col].max(), df[y_col].max()]),
    ]
    ax.plot(lims, lims, "r--", label="x = y")
    ax.legend()

    return fig


def plot_boxplot_alt_vs_min(
    df: pd.DataFrame,
    title: str = "Alternation Complexity vs q * min",
    x_col: str = ALT_COL,
    y_col: str = MIN_COL,
) -> plt.Figure:
    """
    Plot a boxplot of alternation complexity vs minimum size.
    """
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.boxplot(
        [df[x_col], df[y_col]],
        tick_labels=[x_col, y_col],
        patch_artist=True,
    )
    ax.set_title(title)
    ax.set_yscale("log")
    ax.set_ylabel("Value (log scale)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.7)
    return fig
