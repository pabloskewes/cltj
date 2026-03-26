"""Statistics derived from the per-node MPHF trace DataFrame."""

import numpy as np
import pandas as pd

# Log-scale size buckets for n_children grouping
SIZE_BINS = [0, 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000, float("inf")]
SIZE_LABELS = ["<10K", "10K–100K", "100K–1M", "1M–10M", "10M–100M", ">100M"]


def retry_distribution(df: pd.DataFrame) -> pd.DataFrame:
    """Distribution of retries_used across all nodes.

    Returns a DataFrame with columns:
      retries_used, n_nodes, pct, cumulative_pct
    """
    counts = df["retries_used"].value_counts().sort_index()
    total = len(df)
    cumsum = counts.cumsum()
    return pd.DataFrame(
        {
            "retries_used": counts.index,
            "n_nodes": counts.values,
            "pct": counts.values / total * 100,
            "cumulative_pct": cumsum.values / total * 100,
        }
    ).reset_index(drop=True)


def size_bucket_stats(df: pd.DataFrame) -> pd.DataFrame:
    """Aggregate stats per log-scale size bucket of n_children.

    Returns a DataFrame with one row per bucket and columns:
      size_bucket, n_nodes, n_success, success_rate,
      first_try_rate, mean_retries, max_retries, total_time_s
    """
    df = df.copy()
    df["size_bucket"] = pd.cut(
        df["n_children"],
        bins=SIZE_BINS,
        labels=SIZE_LABELS,
        right=False,
    )

    grp = df.groupby("size_bucket", observed=True)
    agg = grp.agg(
        n_nodes=("n_children", "count"),
        n_success=("success", "sum"),
        n_first_try=("retries_used", lambda x: (x == 0).sum()),
        mean_retries=("retries_used", "mean"),
        max_retries=("retries_used", "max"),
        total_time_s=("total_elapsed_ms", lambda x: x.sum() / 1_000),
    ).reset_index()

    agg["success_rate_pct"] = agg["n_success"] / agg["n_nodes"] * 100
    agg["first_try_rate_pct"] = agg["n_first_try"] / agg["n_nodes"] * 100
    return agg


def residual_stats(df: pd.DataFrame) -> pd.DataFrame:
    """Nodes that had at least one failed retry, sorted by n_children descending."""
    cols = [
        "trie_id",
        "node_pos",
        "n_children",
        "success",
        "retries_used",
        "min_residual",
        "max_residual",
        "best_peeled_frac",
        "total_elapsed_ms",
    ]
    return (
        df[df["retries_used"] > 0][cols]
        .sort_values("n_children", ascending=False)
        .reset_index(drop=True)
    )


TRIE_NAMES = ["SPO", "SOP", "POS", "PSO", "OSP", "OPS"]


def percentile_table(df: pd.DataFrame) -> pd.DataFrame:
    """Percentile table of n_children and total_elapsed_ms."""
    pcts = [50, 75, 90, 95, 99, 99.9, 100]
    rows = []
    for p in pcts:
        rows.append(
            {
                "percentile": f"p{p}",
                "n_children": np.percentile(df["n_children"], p),
                "elapsed_s": np.percentile(df["total_elapsed_ms"], p) / 1_000,
            }
        )
    return pd.DataFrame(rows)
