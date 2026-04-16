"""Aggregation and statistics for query-benchmark comparisons."""

import pandas as pd


def add_speedup(df: pd.DataFrame) -> pd.DataFrame:
    """Add a ``speedup`` column: xcltj_time / hcltj_time.

    Values > 1 mean H-CLTJ is faster; < 1 mean X-CLTJ is faster.
    Rows where hcltj time is 0 are dropped to avoid division by zero.
    """
    df = df[df["time_ns_hcltj"] > 0].copy()
    df["speedup"] = df["time_ns_xcltj"] / df["time_ns_hcltj"]
    return df


def summary_overall(df: pd.DataFrame) -> pd.DataFrame:
    """Return a one-row summary DataFrame with overall median/mean stats."""
    return pd.DataFrame(
        [
            {
                "n_queries": len(df),
                "median_time_us_xcltj": df["time_us_xcltj"].median(),
                "median_time_us_hcltj": df["time_us_hcltj"].median(),
                "mean_time_us_xcltj": df["time_us_xcltj"].mean(),
                "mean_time_us_hcltj": df["time_us_hcltj"].mean(),
                "median_speedup": df["speedup"].median()
                if "speedup" in df.columns
                else None,
                "mean_speedup": df["speedup"].mean()
                if "speedup" in df.columns
                else None,
                "pct_hcltj_faster": (
                    100.0 * (df["speedup"] > 1).sum() / len(df)
                    if "speedup" in df.columns
                    else None
                ),
            }
        ]
    )


def summary_by_type(df: pd.DataFrame) -> pd.DataFrame:
    """Return per-type summary stats.

    Expects ``df`` to have ``query_type`` and ``speedup`` columns
    (call :func:`add_speedup` first).
    """
    rows = []
    for qtype, g in df.groupby("query_type"):
        rows.append(
            {
                "query_type": qtype,
                "n_queries": len(g),
                "median_time_us_xcltj": g["time_us_xcltj"].median(),
                "median_time_us_hcltj": g["time_us_hcltj"].median(),
                "mean_time_us_xcltj": g["time_us_xcltj"].mean(),
                "mean_time_us_hcltj": g["time_us_hcltj"].mean(),
                "median_speedup": g["speedup"].median()
                if "speedup" in g.columns
                else None,
                "pct_hcltj_faster": (
                    100.0 * (g["speedup"] > 1).sum() / len(g)
                    if "speedup" in g.columns
                    else None
                ),
            }
        )
    return pd.DataFrame(rows).set_index("query_type")
