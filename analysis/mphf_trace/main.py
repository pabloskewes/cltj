"""CLI: analyse MPHF build trace produced by build-hcltj.

Usage:
    python main.py <mphf_trace.jsonl> [--figdir DIR]

Outputs:
  - Text tables: retry distribution, per-size stats, residuals, percentiles
  - Figures (saved to figdir):
      retries_vs_size.png   scatter: node size vs retries used
      retry_histogram.png   bar chart: retry count distribution
      residual_vs_size.png  scatter: node size vs min 2-core residual
      time_vs_size.png      scatter: node size vs build time
      size_cdf.png          CDF of node sizes
      time_breakdown.png    top-20 nodes by elapsed time
"""

import argparse
from pathlib import Path

from load import load_trace
from plot import (
    plot_residual_vs_size,
    plot_retries_vs_size,
    plot_retry_histogram,
    plot_size_cdf,
    plot_time_breakdown,
    plot_time_vs_size,
)
from stats import (
    percentile_table,
    residual_stats,
    retry_distribution,
    size_bucket_stats,
)
from tabulate import tabulate

FMT = "simple"
FLOAT_FMT = ".2f"


def _section(title: str) -> None:
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("trace", help="Path to mphf_trace.jsonl")
    parser.add_argument(
        "--figdir",
        help="Directory to save figures (default: <trace_dir>/figures/)",
    )
    parser.add_argument(
        "--no-figures",
        action="store_true",
        help="Skip figure generation (text stats only)",
    )
    args = parser.parse_args()

    trace_path = Path(args.trace)
    figdir = Path(args.figdir) if args.figdir else trace_path.parent / "figures"

    print(f"Loading {trace_path} …")
    df = load_trace(trace_path)
    print(f"Parsed {len(df):,} nodes.\n")

    # ------------------------------------------------------------------
    # Retry distribution
    # ------------------------------------------------------------------
    _section("Retry distribution (all nodes)")
    rd = retry_distribution(df)
    print(
        tabulate(rd, headers="keys", tablefmt=FMT, floatfmt=FLOAT_FMT, showindex=False)
    )

    n_zero = (
        int(rd.loc[rd["retries_used"] == 0, "n_nodes"].sum())
        if 0 in rd["retries_used"].values
        else 0
    )
    print(
        f"\n  → {n_zero:,} nodes ({n_zero/len(df)*100:.2f}%) built on the first attempt."
    )
    print(f"  → {(~df['success']).sum()} nodes failed all retries.")

    # ------------------------------------------------------------------
    # Per-size-bucket stats
    # ------------------------------------------------------------------
    _section("Stats by node size bucket")
    sb = size_bucket_stats(df)
    print(
        tabulate(sb, headers="keys", tablefmt=FMT, floatfmt=FLOAT_FMT, showindex=False)
    )

    # ------------------------------------------------------------------
    # Percentile table
    # ------------------------------------------------------------------
    _section("Node size & time percentiles")
    pt = percentile_table(df)
    print(
        tabulate(pt, headers="keys", tablefmt=FMT, floatfmt=FLOAT_FMT, showindex=False)
    )

    # ------------------------------------------------------------------
    # Residual analysis
    # ------------------------------------------------------------------
    partial = residual_stats(df)
    _section(f"Nodes with ≥1 failed retry  (n={len(partial)})")
    if partial.empty:
        print("  None.")
    else:
        display_cols = [
            "trie_id",
            "n_children",
            "success",
            "retries_used",
            "min_residual",
            "max_residual",
            "best_peeled_frac",
            "total_elapsed_ms",
        ]
        print(
            tabulate(
                partial[display_cols].head(30),
                headers=display_cols,
                tablefmt=FMT,
                floatfmt=".6f",
                showindex=False,
            )
        )
        # Fallback feasibility summary
        print()
        max_res = partial["min_residual"].max()
        print(f"  Max min-residual across all nodes: {max_res}")
        print(
            f"  → A fallback capped at residual ≤ {max_res} edges would cover all cases."
        )
        bytes_for_max = max_res * 4
        print(
            f"  → Sorted-array fallback for {max_res} uint32_t keys: {bytes_for_max:,} bytes"
        )

    # ------------------------------------------------------------------
    # Figures
    # ------------------------------------------------------------------
    if args.no_figures:
        return

    figdir.mkdir(parents=True, exist_ok=True)
    print()
    _section(f"Figures → {figdir}/")

    specs = [
        ("retries_vs_size.png", lambda p: plot_retries_vs_size(df, p)),
        ("retry_histogram.png", lambda p: plot_retry_histogram(df, p)),
        ("residual_vs_size.png", lambda p: plot_residual_vs_size(df, p)),
        ("time_vs_size.png", lambda p: plot_time_vs_size(df, p)),
        ("size_cdf.png", lambda p: plot_size_cdf(df, savepath=p)),
        ("time_breakdown.png", lambda p: plot_time_breakdown(df, p)),
    ]
    for fname, fn in specs:
        path = figdir / fname
        result = fn(path)
        if result is not None:
            print(f"  ✓ {fname}")
        else:
            print(f"  – {fname} (skipped: no data)")


if __name__ == "__main__":
    main()
