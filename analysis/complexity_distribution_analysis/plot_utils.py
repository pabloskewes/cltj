import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def plot_scatter_alt_vs_min(
    df: pd.DataFrame, title: str = "Alternation Complexity vs Minimum Size"
):
    """
    Plot a scatter plot of alternation complexity vs minimum size.
    """
    plt.figure(figsize=(8, 6))
    plt.scatter(df["alternation_complexity"], df["min_size"], alpha=0.5)
    plt.xlabel("Alternation Complexity")
    plt.ylabel("Minimum Size")
    plt.title(title)
    plt.grid(True, which="both", ls="--", alpha=0.7)

    plt.xscale("log")
    plt.yscale("log")

    # add x=y reference line
    lims = [
        np.min([df["alternation_complexity"].min(), df["min_size"].min()]),
        np.max([df["alternation_complexity"].max(), df["min_size"].max()]),
    ]
    plt.plot(lims, lims, "r--", label="x = y")
    plt.legend()

    plt.show()


def plot_boxplot_alt_vs_min(
    df: pd.DataFrame, title: str = "Alternation Complexity vs Minimum Size"
):
    """
    Plot a boxplot of alternation complexity vs minimum size.
    """
    plt.figure(figsize=(8, 6))
    plt.boxplot(
        [df["alternation_complexity"], df["min_size"]],
        tick_labels=["Alternation Complexity", "Minimum Size"],
        patch_artist=True,
    )
    plt.title(title)
    plt.yscale("log")
    plt.ylabel("Value (log scale)")
    plt.grid(True, axis="y", linestyle="--", alpha=0.7)
    plt.show()
