"""CLI: analyse X-CLTJ vs H-CLTJ query-benchmark CSVs.

Usage:
    python main.py <results_dir> [--outdir DIR] [--types PATH]

Inputs inside ``results_dir``:
  - ``bench-xcltj.csv``
  - ``bench-hcltj.csv``

Outputs inside ``outdir`` (default: ``<results_dir>/analysis/query_bench``):
  - ``comparison.csv``         merged per-query table with speedup
  - ``summary_overall.csv``    one-row aggregate summary
  - ``summary_by_type.csv``    per-type aggregate summary
  - ``report.txt``             plain-text report with the same stats
  - ``figures/time_scatter.png``
  - ``figures/median_by_type.png``
  - ``figures/mean_by_type.png``
  - ``figures/speedup_boxplot.png``
  - ``figures/winrate_by_type.png``
  - ``figures/timeout_heatmap.png``
  - ``figures/summary_card.png``
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path

from load import load_comparison
from plot import (
    plot_mean_by_type,
    plot_median_by_type,
    plot_speedup_boxplot,
    plot_summary_card,
    plot_time_scatter,
    plot_timeout_heatmap,
    plot_winrate_by_type,
)
from stats import add_speedup, summary_by_type, summary_overall


def _default_types_path() -> Path:
    return Path(__file__).resolve().parents[2] / "Queries" / "types.txt"


def _write_report(
    report_path: Path,
    results_dir: Path,
    types_path: Path,
    comparison_rows: int,
    overall,
    by_type,
) -> None:
    lines = [
        "Query benchmark analysis",
        "========================",
        "",
        f"results_dir: {results_dir}",
        f"types_path:  {types_path}",
        f"n_queries:   {comparison_rows}",
        "",
        "Overall summary",
        "---------------",
        overall.to_string(index=False),
        "",
        "Summary by type",
        "---------------",
        by_type.reset_index().to_string(index=False),
        "",
    ]
    report_path.write_text("\n".join(lines))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "results_dir",
        help="Directory containing bench-xcltj.csv and bench-hcltj.csv",
    )
    parser.add_argument(
        "--outdir",
        help="Directory to save outputs (default: <results_dir>/analysis/query_bench)",
    )
    parser.add_argument(
        "--types",
        help="Path to types.txt (default: repo-root/Queries/types.txt)",
    )
    parser.add_argument(
        "--no-figures",
        action="store_true",
        help="Skip figure generation and only write CSV/text outputs",
    )
    args = parser.parse_args()

    results_dir = Path(args.results_dir).resolve()
    outdir = (
        Path(args.outdir).resolve()
        if args.outdir
        else results_dir / "analysis" / "query_bench"
    )
    figdir = outdir / "figures"
    types_path = Path(args.types).resolve() if args.types else _default_types_path()

    xcltj_path = results_dir / "bench-xcltj.csv"
    hcltj_path = results_dir / "bench-hcltj.csv"

    outdir.mkdir(parents=True, exist_ok=True)
    if not args.no_figures:
        figdir.mkdir(parents=True, exist_ok=True)
        os.environ.setdefault("MPLCONFIGDIR", str(outdir / ".mplconfig"))

    print(f"Loading benchmark CSVs from {results_dir} ...")
    df = load_comparison(xcltj_path, hcltj_path, types_path)
    df = add_speedup(df)
    overall = summary_overall(df)
    by_type = summary_by_type(df)

    comparison_path = outdir / "comparison.csv"
    overall_path = outdir / "summary_overall.csv"
    by_type_path = outdir / "summary_by_type.csv"
    report_path = outdir / "report.txt"

    df.to_csv(comparison_path, index=False)
    overall.to_csv(overall_path, index=False)
    by_type.to_csv(by_type_path)
    _write_report(report_path, results_dir, types_path, len(df), overall, by_type)

    print(f"Saved {comparison_path}")
    print(f"Saved {overall_path}")
    print(f"Saved {by_type_path}")
    print(f"Saved {report_path}")

    if args.no_figures:
        return

    specs = [
        ("time_scatter.png", lambda p: plot_time_scatter(df, savepath=p)),
        ("median_by_type.png", lambda p: plot_median_by_type(by_type, savepath=p)),
        ("mean_by_type.png", lambda p: plot_mean_by_type(by_type, savepath=p)),
        ("speedup_boxplot.png", lambda p: plot_speedup_boxplot(df, savepath=p)),
        ("winrate_by_type.png", lambda p: plot_winrate_by_type(df, savepath=p)),
        ("timeout_heatmap.png", lambda p: plot_timeout_heatmap(df, savepath=p)),
        ("summary_card.png", lambda p: plot_summary_card(df, overall, by_type, savepath=p)),
    ]
    for fname, fn in specs:
        path = figdir / fname
        fn(path)
        print(f"Saved {path}")


if __name__ == "__main__":
    main()
