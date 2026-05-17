"""Pairwise comparison helpers for query-benchmark tables."""

from pathlib import Path

import pandas as pd

from load import load_bench_csv, load_types


def load_comparison(
    xcltj_path: str | Path,
    hcltj_path: str | Path,
    types_path: str | Path | None = None,
) -> pd.DataFrame:
    """Load both benchmark CSVs and merge them into a single comparison table.

    Returns a DataFrame with columns:
      - ``query_id``
      - ``n_results_xcltj`` / ``n_results_hcltj``
      - ``time_ns_xcltj`` / ``time_ns_hcltj``
      - ``time_us_xcltj`` / ``time_us_hcltj``
      - ``time_ms_xcltj`` / ``time_ms_hcltj``
      - ``query_type`` (if *types_path* is provided, else ``"UNKNOWN"``)
    """
    xcltj = load_bench_csv(xcltj_path).rename(
        columns={
            "n_results": "n_results_xcltj",
            "time_ns": "time_ns_xcltj",
            "time_us": "time_us_xcltj",
            "time_ms": "time_ms_xcltj",
        }
    )
    hcltj = load_bench_csv(hcltj_path).rename(
        columns={
            "n_results": "n_results_hcltj",
            "time_ns": "time_ns_hcltj",
            "time_us": "time_us_hcltj",
            "time_ms": "time_ms_hcltj",
        }
    )

    df = xcltj.merge(
        hcltj,
        on="query_id",
        how="inner",
    )

    if types_path is not None:
        types = load_types(types_path)
        df = df.merge(types, on="query_id", how="left")
        df["query_type"] = df["query_type"].fillna("UNKNOWN")
    else:
        df["query_type"] = "UNKNOWN"

    return df.sort_values("query_id").reset_index(drop=True)
