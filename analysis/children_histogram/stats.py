"""Compute threshold-coverage statistics from children histograms."""

import pandas as pd


def coverage_by_threshold(
    hist: pd.DataFrame,
    thresholds: list[int],
) -> pd.DataFrame:
    """For each threshold, compute how many nodes/children would be hashed.

    *hist* has columns ``children`` and ``nodes`` (output of stats-hcltj).
    Returns a DataFrame indexed by threshold with columns:
      - hashed_nodes, total_nodes, pct_nodes
      - hashed_children, total_children, pct_children
    """
    total_nodes = int(hist["nodes"].sum())
    total_children = int((hist["children"] * hist["nodes"]).sum())

    rows = []
    for t in thresholds:
        mask = hist["children"] >= t
        h_nodes = int(hist.loc[mask, "nodes"].sum())
        h_children = int((hist.loc[mask, "children"] * hist.loc[mask, "nodes"]).sum())
        rows.append(
            {
                "threshold": t,
                "hashed_nodes": h_nodes,
                "total_nodes": total_nodes,
                "pct_nodes": 100.0 * h_nodes / total_nodes if total_nodes else 0,
                "hashed_children": h_children,
                "total_children": total_children,
                "pct_children": (
                    100.0 * h_children / total_children if total_children else 0
                ),
            }
        )
    return pd.DataFrame(rows).set_index("threshold")


def summary(hist: pd.DataFrame) -> dict:
    """Basic summary stats for a single trie histogram."""
    total_nodes = int(hist["nodes"].sum())
    total_children = int((hist["children"] * hist["nodes"]).sum())
    max_children = int(hist["children"].max())
    return {
        "total_nodes": total_nodes,
        "total_children": total_children,
        "max_children": max_children,
    }
