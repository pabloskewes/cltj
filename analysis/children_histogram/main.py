"""CLI: analyse children-histogram CSVs from stats-hcltj."""

import argparse
from pathlib import Path

from load import load_histograms
from plot import plot_coverage, plot_distribution
from stats import coverage_by_threshold, summary

DEFAULT_THRESHOLDS = [10, 50, 100, 500, 1000, 5000, 10000]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outdir", help="Directory with trie_*_*.csv files")
    parser.add_argument(
        "--figdir",
        help="Directory to save figures (default: outdir/figures)",
    )
    parser.add_argument(
        "--thresholds",
        nargs="+",
        type=int,
        default=DEFAULT_THRESHOLDS,
        help="Threshold values to evaluate",
    )
    args = parser.parse_args()

    outdir = Path(args.outdir)
    figdir = Path(args.figdir) if args.figdir else outdir / "figures"
    figdir.mkdir(parents=True, exist_ok=True)

    histograms = load_histograms(outdir)
    if not histograms:
        print(f"No histogram CSVs found in {outdir}")
        return

    print(f"Loaded {len(histograms)} tries: {', '.join(histograms.keys())}\n")

    for name, hist in histograms.items():
        s = summary(hist)
        print(
            f"{name}: {s['total_nodes']} nodes, "
            f"{s['total_children']} children, "
            f"max={s['max_children']}"
        )
    print()

    coverage_tables = {}
    for name, hist in histograms.items():
        cov = coverage_by_threshold(hist, args.thresholds)
        coverage_tables[name] = cov
        print(f"--- {name} ---")
        print(cov.to_string())
        print()

    plot_distribution(histograms, savepath=figdir / "distribution.png")
    print(f"Saved {figdir / 'distribution.png'}")

    plot_coverage(coverage_tables, savepath=figdir / "coverage.png")
    print(f"Saved {figdir / 'coverage.png'}")


if __name__ == "__main__":
    main()
