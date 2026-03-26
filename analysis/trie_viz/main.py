"""CLI: visualise trie structure from dump-trie-hcltj CSVs."""

import argparse
from pathlib import Path

from load import load_trie_dumps
from plot import plot_all_tries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outdir", help="Directory with trie_*_*_nodes.csv files")
    parser.add_argument(
        "--figdir",
        help="Directory to save figures (default: outdir/figures)",
    )
    parser.add_argument(
        "--all-tries",
        action="store_true",
        help="Include partial tries (default: full tries only)",
    )
    args = parser.parse_args()

    outdir = Path(args.outdir)
    figdir = Path(args.figdir) if args.figdir else outdir / "figures"

    trie_dumps = load_trie_dumps(outdir, full_only=not args.all_tries)
    if not trie_dumps:
        print(f"No trie-dump CSVs found in {outdir}")
        return

    print(f"Loaded {len(trie_dumps)} tries: {', '.join(trie_dumps.keys())}")
    for name, df in trie_dumps.items():
        print(f"  {name}: {len(df)} nodes, max_depth={df['depth'].max()}")

    plot_all_tries(trie_dumps, savedir=figdir)
    print(f"\nFigures saved to {figdir}/")


if __name__ == "__main__":
    main()
