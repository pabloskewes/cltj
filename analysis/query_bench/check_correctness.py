"""CLI: check timeout-aware count correctness for X-CLTJ vs H-CLTJ.

Usage:
    python check_correctness.py <results_dir> [--outdir DIR] [--types PATH]
    python check_correctness.py <results_dir> --xcltj-csv X.csv --hcltj-csv H.csv
"""

from __future__ import annotations

import argparse
from pathlib import Path

from compare import load_comparison
from correctness import (
    add_correctness_enrichment,
    mismatches_non_timeout,
    mismatches_timeout_only,
    summary_by_type,
    summary_overall,
)


def _default_types_path() -> Path:
    return Path(__file__).resolve().parents[2] / "Queries" / "types.txt"


def _write_report(
    report_path: Path,
    results_dir: Path,
    xcltj_path: Path,
    hcltj_path: Path,
    types_path: Path,
    timeout_seconds: float,
    overall,
    by_type,
    real_mismatches,
    timeout_only_mismatches,
) -> None:
    row = overall.iloc[0]
    lines = [
        "Query benchmark correctness report",
        "==================================",
        "",
        f"results_dir:      {results_dir}",
        f"xcltj_csv:        {xcltj_path}",
        f"hcltj_csv:        {hcltj_path}",
        f"types_path:       {types_path}",
        f"timeout_seconds:  {timeout_seconds}",
        "",
        "Overall summary",
        "---------------",
        overall.to_string(index=False),
        "",
        "Summary by type",
        "---------------",
        by_type.reset_index().to_string(index=False),
        "",
        "Interpretation",
        "--------------",
        f"count matches:             {int(row['n_count_matches'])}",
        f"real mismatches:           {int(row['n_real_mismatches'])}",
        f"timeout-both mismatches:   {int(row['n_timeout_both_mismatches'])}",
        f"timeout-one-side mismatch: {int(row['n_timeout_one_side_mismatches'])}",
        "",
        "Non-timeout mismatches",
        "----------------------",
        real_mismatches.to_string(index=False) if len(real_mismatches) else "(none)",
        "",
        "Timeout-involved mismatches",
        "---------------------------",
        timeout_only_mismatches.to_string(index=False)
        if len(timeout_only_mismatches)
        else "(none)",
        "",
    ]
    report_path.write_text("\n".join(lines))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "results_dir",
        help="Directory containing benchmark results",
    )
    parser.add_argument(
        "--outdir",
        help="Directory to save outputs (default: <results_dir>/analysis/query_bench/correctness)",
    )
    parser.add_argument(
        "--types",
        help="Path to types.txt (default: repo-root/Queries/types.txt)",
    )
    parser.add_argument(
        "--xcltj-csv",
        help="Path to X-CLTJ benchmark CSV (default: <results_dir>/bench-xcltj.csv)",
    )
    parser.add_argument(
        "--hcltj-csv",
        help="Path to H-CLTJ benchmark CSV (default: <results_dir>/bench-hcltj.csv)",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=600.0,
        help="Threshold used to classify timeout-affected rows (default: 600)",
    )
    args = parser.parse_args()

    results_dir = Path(args.results_dir).resolve()
    outdir = (
        Path(args.outdir).resolve()
        if args.outdir
        else results_dir / "analysis" / "query_bench" / "correctness"
    )
    types_path = Path(args.types).resolve() if args.types else _default_types_path()
    xcltj_path = (
        Path(args.xcltj_csv).resolve()
        if args.xcltj_csv
        else results_dir / "bench-xcltj.csv"
    )
    hcltj_path = (
        Path(args.hcltj_csv).resolve()
        if args.hcltj_csv
        else results_dir / "bench-hcltj.csv"
    )
    timeout_ns = int(args.timeout_seconds * 1_000_000_000)

    outdir.mkdir(parents=True, exist_ok=True)

    print(f"Loading benchmark CSVs from {results_dir} ...")
    df = load_comparison(xcltj_path, hcltj_path, types_path)
    df = add_correctness_enrichment(df, timeout_ns=timeout_ns)
    overall = summary_overall(df)
    by_type = summary_by_type(df)
    real_mismatches = mismatches_non_timeout(df)
    timeout_only = mismatches_timeout_only(df)

    comparison_path = outdir / "comparison.csv"
    overall_path = outdir / "summary_overall.csv"
    by_type_path = outdir / "summary_by_type.csv"
    real_mismatches_path = outdir / "mismatches_non_timeout.csv"
    timeout_only_path = outdir / "mismatches_timeout_only.csv"
    report_path = outdir / "report.txt"

    df.to_csv(comparison_path, index=False)
    overall.to_csv(overall_path, index=False)
    by_type.to_csv(by_type_path)
    real_mismatches.to_csv(real_mismatches_path, index=False)
    timeout_only.to_csv(timeout_only_path, index=False)
    _write_report(
        report_path,
        results_dir,
        xcltj_path,
        hcltj_path,
        types_path,
        args.timeout_seconds,
        overall,
        by_type,
        real_mismatches,
        timeout_only,
    )

    print(f"Saved {comparison_path}")
    print(f"Saved {overall_path}")
    print(f"Saved {by_type_path}")
    print(f"Saved {real_mismatches_path}")
    print(f"Saved {timeout_only_path}")
    print(f"Saved {report_path}")


if __name__ == "__main__":
    main()
