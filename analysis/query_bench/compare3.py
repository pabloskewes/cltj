"""LTJ-26 (throwaway): X vs several H runs, from main.py comparison.csv outputs that share the X run.

Usage:
    python compare3.py hdef=<comparison.csv> hvar=<comparison.csv> [...] [--min-x-ms 100] [--out report.txt]

The first H run is the reference for the H/H ratios.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


def _geomean(s: pd.Series) -> float:
    s = s[s > 0]
    return float(np.exp(np.log(s).mean())) if len(s) else float("nan")


def _block(g: pd.DataFrame, labels: list[str]) -> dict:
    ref = labels[0]
    row = {"n": len(g), "X_s": g["t_x"].sum() / 1e9}
    for lab in labels:
        row[f"{lab}_s"] = g[f"t_{lab}"].sum() / 1e9
    for lab in labels:
        r = g["t_x"] / g[f"t_{lab}"]
        row[f"med X/{lab}"] = r.median()
        row[f"geo X/{lab}"] = _geomean(r)
        row[f"win% {lab}"] = 100.0 * (r > 1).mean()
    for lab in labels[1:]:
        row[f"med {ref}/{lab}"] = (g[f"t_{ref}"] / g[f"t_{lab}"]).median()
    return row


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("runs", nargs="+", help="label=path/to/comparison.csv")
    ap.add_argument("--out")
    ap.add_argument("--min-x-ms", type=float, default=0.0, help="also report the subset with X time >= this")
    ap.add_argument("--exclude", default="", help="comma-separated query ids to drop (e.g. timeouts)")
    args = ap.parse_args()

    labels, df = [], None
    for spec in args.runs:
        lab, path = spec.split("=", 1)
        c = pd.read_csv(path)
        part = c[["query_id", "query_type", "n_results_xcltj", "time_ns_xcltj", "n_results_hcltj", "time_ns_hcltj"]]
        part = part.rename(
            columns={"time_ns_xcltj": "t_x", "n_results_xcltj": "n_x", "time_ns_hcltj": f"t_{lab}", "n_results_hcltj": f"n_{lab}"}
        )
        if df is None:
            df = part
        else:
            m = df.merge(part[["query_id", "t_x", f"t_{lab}", f"n_{lab}"]], on="query_id", suffixes=("", "_chk"))
            assert (m["t_x"] == m["t_x_chk"]).all(), f"{lab}: does not share the X run"
            df = m.drop(columns="t_x_chk")
        labels.append(lab)
    if args.exclude:
        df = df[~df["query_id"].isin([int(q) for q in args.exclude.split(",")])]

    lines = ["X vs " + ", ".join(labels), "=" * 40, ""]
    same = np.logical_and.reduce([df["n_x"] == df[f"n_{lab}"] for lab in labels])
    lines.append(f"queries: {len(df)}   identical counts in all runs: {int(same.sum())}")
    lines.append("")
    subsets = [("all", df)]
    if args.min_x_ms > 0:
        subsets.append((f"X >= {args.min_x_ms} ms", df[df["t_x"] >= args.min_x_ms * 1e6]))
    for name, sub in subsets:
        rows = [{"type": "ALL", **_block(sub, labels)}]
        rows += [{"type": t, **_block(g, labels)} for t, g in sub.groupby("query_type")]
        lines.append(f"[{name}]")
        lines.append(pd.DataFrame(rows).set_index("type").T.to_string(float_format=lambda v: f"{v:.3f}"))
        lines.append("")
    text = "\n".join(lines)
    print(text)
    if args.out:
        Path(args.out).write_text(text)
        df.to_csv(Path(args.out).with_suffix(".csv"), index=False)


if __name__ == "__main__":
    main()
