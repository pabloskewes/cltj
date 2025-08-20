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
):
    """
    Plot a scatter plot of alternation complexity vs minimum size.
    """
    plt.figure(figsize=(8, 6))
    plt.scatter(df[x_col], df[y_col], alpha=0.5)
    plt.xlabel(x_col)
    plt.ylabel(y_col)
    plt.title(title)
    plt.grid(True, which="both", ls="--", alpha=0.7)

    plt.xscale("log")
    plt.yscale("log")

    # add x=y reference line
    lims = [
        np.min([df[x_col].min(), df[y_col].min()]),
        np.max([df[x_col].max(), df[y_col].max()]),
    ]
    plt.plot(lims, lims, "r--", label="x = y")
    plt.legend()

    plt.show()


def plot_boxplot_alt_vs_min(
    df: pd.DataFrame,
    title: str = "Alternation Complexity vs q * min",
    x_col: str = ALT_COL,
    y_col: str = MIN_COL,
):
    """
    Plot a boxplot of alternation complexity vs minimum size.
    """
    plt.figure(figsize=(8, 6))
    plt.boxplot(
        [df[x_col], df[y_col]],
        tick_labels=[x_col, y_col],
        patch_artist=True,
    )
    plt.title(title)
    plt.yscale("log")
    plt.ylabel("Value (log scale)")
    plt.grid(True, axis="y", linestyle="--", alpha=0.7)
    plt.show()
