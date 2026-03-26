"""Load children-histogram CSVs produced by stats-hcltj."""

import re
from pathlib import Path

import pandas as pd

TRIE_NAMES = ["SPO", "SOP", "POS", "PSO", "OSP", "OPS"]
CSV_PATTERN = re.compile(r"trie_(\d)_([A-Z]{3})\.csv")


def load_histograms(outdir: str | Path) -> dict[str, pd.DataFrame]:
    """Load per-trie histogram CSVs from *outdir*.

    Returns a dict keyed by trie name (e.g. "SPO") with DataFrames
    containing columns ``children`` and ``nodes``.
    """
    outdir = Path(outdir)
    result: dict[str, pd.DataFrame] = {}
    for path in sorted(outdir.glob("trie_*_*.csv")):
        m = CSV_PATTERN.match(path.name)
        if m is None:
            continue
        name = m.group(2)
        df = pd.read_csv(path)
        result[name] = df
    return result
