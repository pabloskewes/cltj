from __future__ import annotations

import pandas as pd


def summarize_by_strategy(df: pd.DataFrame) -> pd.DataFrame:
    """
    Simple aggregate stats per strategy over all n, all in pandas.

    Returns a DataFrame with one row per strategy and columns:
    - mean_build_time_us
    - mean_query_time_ns_per_key
    - mean_bits_per_key
    - mean_m_over_n
    """
    ok = df[df["build_success"] == 1]
    grouped = ok.groupby("strategy").agg(
        mean_build_time_us=("build_time_us", "mean"),
        mean_query_time_ns_per_key=("query_time_ns_per_key", "mean"),
        mean_bits_per_key=("bits_per_key", "mean"),
        mean_m_over_n=("m_over_n", "mean"),
    )
    return grouped.reset_index()


def group_by_strategy(df: pd.DataFrame) -> dict[str, pd.DataFrame]:
    """Return a dict[strategy -> DataFrame] purely for convenience."""
    return {name: g.copy() for name, g in df.groupby("strategy")}
