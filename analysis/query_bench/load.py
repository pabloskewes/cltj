"""Load query-benchmark CSVs produced by bench-query-hcltj / bench-query-xcltj."""

from pathlib import Path

import pandas as pd


def load_bench_csv(path: str | Path) -> pd.DataFrame:
    """Load a single benchmark CSV.

    The file starts with an informational line ("Index loaded: X bytes.")
    followed by semicolon-separated rows: ``query_id;n_results;time_ns``.

    Returns a DataFrame with columns:
      - ``query_id``  (int)
      - ``n_results`` (int)
      - ``time_ns``   (int)
      - ``time_us``   (float, convenience)
      - ``time_ms``   (float, convenience)
    """
    path = Path(path)
    df = pd.read_csv(
        path,
        sep=";",
        comment="#",
        names=["query_id", "n_results", "time_ns"],
        # The header line ("Index loaded: …") cannot be parsed as ints;
        # on_bad_lines='skip' drops it silently.
        on_bad_lines="skip",
    )
    df = df.dropna().astype({"query_id": int, "n_results": int, "time_ns": int})
    df["time_us"] = df["time_ns"] / 1_000
    df["time_ms"] = df["time_ns"] / 1_000_000
    return df.reset_index(drop=True)


def load_types(path: str | Path) -> pd.DataFrame:
    """Load a ``types.txt`` file mapping query ids to TYPE1/TYPE2/TYPE3.

    Returns a DataFrame with columns ``query_id`` (int) and ``query_type`` (str).
    """
    path = Path(path)
    df = pd.read_csv(path, sep=";", names=["query_id", "query_type"])
    return df.astype({"query_id": int})
