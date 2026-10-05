"""LTJ-26 (throwaway): X vs H-default vs H-variant, from two main.py comparison.csv outputs.

Usage:
    python compare3.py <hdef_comparison.csv> <hvar_comparison.csv> [--out report3.txt]

Both inputs must share the same X run (same time_ns_xcltj per query).
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


def _geomean(s: pd.Series) -> float:
    s = s[s > 0]
    return float(np.exp(np.log(s).mean())) if len(s) else float("nan")


def _block(g: pd.DataFrame) -> dict:
    return {
        "n": len(g),
        "X_total_s": g["time_ns_xcltj"].sum() / 1e9,
        "Hdef_total_s": g["time_ns_hdef"].sum() / 1e9,
        "Hvar_total_s": g["time_ns_hvar"].sum() / 1e9,
        "med_X/Hdef": (g["time_ns_xcltj"] / g["time_ns_hdef"]).median(),
        "med_X/Hvar": (g["time_ns_xcltj"] / g["time_ns_hvar"]).median(),
        "med_Hdef/Hvar": (g["time_ns_hdef"] / g["time_ns_hvar"]).median(),
        "geo_X/Hdef": _geomean(g["time_ns_xcltj"] / g["time_ns_hdef"]),
        "geo_X/Hvar": _geomean(g["time_ns_xcltj"] / g["time_ns_hvar"]),
        "pct_Hdef_wins": 100.0 * (g["time_ns_hdef"] < g["time_ns_xcltj"]).mean(),
        "pct_Hvar_wins": 100.0 * (g["time_ns_hvar"] < g["time_ns_xcltj"]).mean(),
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("hdef")
    ap.add_argument("hvar")
    ap.add_argument("--out")
    ap.add_argument("--min-x-ms", type=float, default=0.0, help="also report the subset with X time >= this")
    args = ap.parse_args()

    a = pd.read_csv(args.hdef)
    b = pd.read_csv(args.hvar)
    keep = ["query_id", "query_type", "n_results_xcltj", "time_ns_xcltj"]
    df = a[keep + ["n_results_hcltj", "time_ns_hcltj"]].rename(
        columns={"n_results_hcltj": "n_results_hdef", "time_ns_hcltj": "time_ns_hdef"}
    )
    df = df.merge(
        b[["query_id", "time_ns_xcltj", "n_results_hcltj", "time_ns_hcltj"]].rename(
            columns={"time_ns_xcltj": "x_check", "n_results_hcltj": "n_results_hvar", "time_ns_hcltj": "time_ns_hvar"}
        ),
        on="query_id",
    )
    assert (df["x_check"] == df["time_ns_xcltj"]).all(), "the two comparisons do not share the X run"
    df = df.drop(columns="x_check")

    lines = ["X vs H-default vs H-variant", "===========================", ""]
    cnt_ok = (df["n_results_xcltj"] == df["n_results_hdef"]) & (df["n_results_xcltj"] == df["n_results_hvar"])
    lines.append(f"queries: {len(df)}   identical counts in the three: {int(cnt_ok.sum())}")
    lines.append("")
    subsets = [("all", df)]
    if args.min_x_ms > 0:
        subsets.append((f"X >= {args.min_x_ms} ms", df[df["time_ns_xcltj"] >= args.min_x_ms * 1e6]))
    for name, sub in subsets:
        rows = [{"type": "ALL", **_block(sub)}]
        rows += [{"type": t, **_block(g)} for t, g in sub.groupby("query_type")]
        lines.append(f"[{name}]")
        lines.append(pd.DataFrame(rows).to_string(index=False, float_format=lambda v: f"{v:.3f}"))
        lines.append("")
    text = "\n".join(lines)
    print(text)
    if args.out:
        Path(args.out).write_text(text)
        df.to_csv(Path(args.out).with_suffix(".csv"), index=False)


if __name__ == "__main__":
    main()
