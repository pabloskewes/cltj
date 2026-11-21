"""Paired per-query ratios: ``ref/sys`` = t_ref / t_sys, > 1 means sys is faster."""

from __future__ import annotations

import numpy as np
import pandas as pd

from .filters import by_type, done

CONVENTION = "ratio = t_ref / t_sys per query (> 1 = sys faster); tot = total ref / total sys"


def ratio(d: pd.DataFrame, ref: str, sys: str) -> pd.Series:
    g = done(d, ref, sys)
    return g[f"t_{ref}"] / g[f"t_{sys}"]


def ratio_stats(num: pd.Series, den: pd.Series) -> dict:
    r = num / den
    return {
        "n": len(r), "ref_s": num.sum() / 1e9, "sys_s": den.sum() / 1e9, "tot": num.sum() / den.sum(),
        "geo": float(np.exp(np.log(r).mean())), "p10": r.quantile(.1), "p25": r.quantile(.25),
        "med": r.median(), "p75": r.quantile(.75), "p90": r.quantile(.9), "win%": 100.0 * (r > 1).mean(),
    }


def pair_table(subsets: dict[str, pd.DataFrame], pairs: list[tuple[str, str]], types: list[str]) -> pd.DataFrame:
    """One row per (subset, pair, type); each pair drops only the timeouts of its two systems."""
    rows = []
    for name, d in subsets.items():
        for ref, sys in pairs:
            for t, g in by_type(done(d, ref, sys), types):
                rows.append({"subset": name, "pair": f"{ref}/{sys}", "ref": ref, "sys": sys, "type": t,
                             **ratio_stats(g[f"t_{ref}"], g[f"t_{sys}"])})
    return pd.DataFrame(rows)
