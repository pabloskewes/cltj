"""Query sets shared by every table and figure."""

from __future__ import annotations

import numpy as np
import pandas as pd

from .systems import SUBSET_COL, TIMEOUT_NS, Config


def done(d: pd.DataFrame, *systems: str) -> pd.DataFrame:
    """Rows where none of ``systems`` timed out."""
    return d[np.logical_and.reduce([d[f"t_{s}"] < TIMEOUT_NS for s in systems])] if systems else d


def base_set(d: pd.DataFrame, cfg: Config, loaded: list[str]) -> pd.DataFrame:
    """Rows without timeout in any loaded ``base`` system; ``extra`` systems only leave their own pairs."""
    return done(d, *[s.name for s in cfg.systems if s.group == "base" and s.name in loaded])


def subset(d: pd.DataFrame, min_ns: float) -> pd.DataFrame:
    """Rows whose subset reference took at least ``min_ns``."""
    return d[d[SUBSET_COL] >= min_ns]


def by_type(d: pd.DataFrame, types: list[str]):
    for t in types:
        yield t, (d if t == "ALL" else d[d.type == t])
