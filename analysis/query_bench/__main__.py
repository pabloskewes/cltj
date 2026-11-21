"""Overview of N query-benchmark systems declared in a ``systems.toml``.

Usage (from ``analysis/``):
    python -m query_bench <exp_dir> [--systems TOML] [--ref NAME] [--min-ms 100] [--pairs A/B,C/D]
                          [--types PATH] [--out DIR]

Defaults: ``--systems <exp_dir>/analysis/overview/systems.toml``, ``--out`` the folder of that file,
``--types`` ``Queries/types.txt`` of this repo. ``--ref``, ``--min-ms`` and ``--pairs`` override the toml.
Writes the PNGs, ``pairs.csv`` and a self-contained ``index.html``.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from . import figures, report
from .filters import base_set, subset
from .pairs import CONVENTION, pair_table
from .systems import load_config, load_systems, parse_pairs

TYPES = ["TYPE1", "TYPE2", "TYPE3", "ALL"]
DEFAULT_TYPES = Path(__file__).resolve().parents[2] / "Queries" / "types.txt"


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exp_dir")
    ap.add_argument("--systems", help="systems.toml (default: <exp_dir>/analysis/overview/systems.toml)")
    ap.add_argument("--ref", help="reference system of the 'against ref' pairs and figures")
    ap.add_argument("--min-ms", type=float, help="main subset: t of the subset reference >= this")
    ap.add_argument("--pairs", help="comma-separated ref/sys pairs, e.g. H/C,A/C")
    ap.add_argument("--types", default=str(DEFAULT_TYPES))
    ap.add_argument("--out")
    a = ap.parse_args()

    exp = Path(a.exp_dir).resolve()
    toml = Path(a.systems).resolve() if a.systems else exp / "analysis" / "overview" / "systems.toml"
    out = Path(a.out).resolve() if a.out else toml.parent
    out.mkdir(parents=True, exist_ok=True)
    cfg = load_config(toml, exp)
    if a.ref:
        cfg.ref = a.ref
    if a.min_ms is not None:
        cfg.min_ms = a.min_ms
    if a.pairs:
        cfg.pairs = parse_pairs(a.pairs)

    ld = load_systems(cfg, a.types)
    assert cfg.ref in ld.systems, f"reference {cfg.ref} is not loaded: {ld.pending}"
    d = base_set(ld.wide, cfg, ld.systems)
    g = subset(d, cfg.min_ms * 1e6)
    sub_ref = cfg.subset_label or cfg.subset_system or cfg.ref
    sub = f"{sub_ref} >= {cfg.min_ms:g} ms"

    vs_ref = [(cfg.ref, s) for s in ld.systems if s != cfg.ref]
    skipped = [p for p in cfg.pairs if not set(p) <= set(ld.systems)]
    pairs = vs_ref + [p for p in cfg.pairs if p not in vs_ref and p not in skipped]
    pending = ld.pending + [f"pair {a}/{b}: skipped, a system is pending" for a, b in skipped]
    r = pair_table({"main": g, "all": d}, pairs, TYPES)
    r.to_csv(out / "pairs.csv", index=False, float_format="%.6g")

    base = [s.name for s in cfg.systems if s.group == "base" and s.name in ld.systems]
    ss = [cfg.system(s) for s in ld.systems]
    figs = figures.draw_all(d, g, cfg.system(cfg.ref), ss, [(cfg.system(x), cfg.system(y)) for x, y in pairs],
                            ld.sizes, TYPES, sub, out)
    page = report.write(out, cfg, exp, ld.systems, ld.sizes, pending, d, g, r, figs, TYPES, base, sub)

    print(f"systems: {ld.systems}; pending: {pending or 'none'}")
    print(f"base set n={len(d)}; {sub} n={len(g)}; {CONVENTION}")
    for f in figs + ["pairs.csv", page.name]:
        print(out / f)


if __name__ == "__main__":
    main()
