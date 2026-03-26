"""Load trie-dump CSVs produced by dump-trie-hcltj."""

import re
from pathlib import Path

import pandas as pd

FULL_TRIES = {"SPO", "POS", "OSP"}
CSV_PATTERN = re.compile(r"trie_(\d)_([A-Z]{3})_nodes\.csv")


def load_trie_dumps(
    outdir: str | Path,
    full_only: bool = True,
) -> dict[str, pd.DataFrame]:
    """Load per-trie node CSVs from *outdir*.

    Returns a dict keyed by trie name (e.g. "SPO") with DataFrames
    containing columns: node, parent, depth, key, n_children.

    If *full_only* is True (default), only loads full tries (SPO, POS, OSP)
    since partial tries lack first-level keys.
    """
    outdir = Path(outdir)
    result: dict[str, pd.DataFrame] = {}
    for path in sorted(outdir.glob("trie_*_*_nodes.csv")):
        m = CSV_PATTERN.match(path.name)
        if m is None:
            continue
        name = m.group(2)
        if full_only and name not in FULL_TRIES:
            continue
        df = pd.read_csv(path)
        result[name] = df
    return result
