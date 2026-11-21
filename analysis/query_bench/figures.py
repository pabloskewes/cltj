"""Figures for N systems, one panel per query type."""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter

from .filters import by_type, done
from .pairs import ratio
from .systems import System

INK, MUTED, GRID = "#0b0b0b", "#52514e", "#e4e3df"
STYLE = {
    "figure.dpi": 110, "savefig.dpi": 150, "font.size": 10, "axes.edgecolor": MUTED,
    "axes.labelcolor": INK, "xtick.color": MUTED, "ytick.color": MUTED, "axes.grid": True,
    "grid.color": GRID, "grid.linewidth": 0.6, "axes.axisbelow": True,
    "axes.spines.top": False, "axes.spines.right": False, "legend.frameon": False,
}


def _save(fig, out: Path, name: str) -> str:
    fig.savefig(out / name, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    return name


def _log_y(ax) -> None:
    ax.set_yscale("log")
    ax.yaxis.set_major_locator(LogLocator(subs=(1, 1.25, 1.5, 2, 2.5, 3, 4, 5, 6, 8)))
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    ax.yaxis.set_minor_formatter(NullFormatter())


def _stat(ax, text: str) -> None:
    ax.text(0.98, 0.97, text, transform=ax.transAxes, ha="right", va="top", fontsize=8, color=INK,
            bbox={"facecolor": "white", "edgecolor": GRID, "boxstyle": "round,pad=0.3", "alpha": 0.9}, zorder=6)


def _ticks(ax, ss: list[System]) -> None:
    ax.set_xticks(range(len(ss)), [s.name for s in ss], rotation=45 if len(ss) > 5 else 0,
                  ha="right" if len(ss) > 5 else "center")
    for lab, s in zip(ax.get_xticklabels(), ss):
        lab.set_color(MUTED if s.faded else INK)


def _tot(g: pd.DataFrame, ref: str, sys: str) -> float:
    g = done(g, ref, sys)
    return g[f"t_{ref}"].sum() / g[f"t_{sys}"].sum()


def ratio_vs_ref(g: pd.DataFrame, ref: System, ss: list[System], types: list[str], sub: str, out: Path) -> str:
    sys = [s for s in ss if s.name != ref.name]
    fig, axes = plt.subplots(1, len(types), figsize=(4 * len(types), 4.8), sharey=True, squeeze=False)
    rng = np.random.default_rng(0)
    for ax, (t, gt) in zip(axes[0], by_type(g, types)):
        data = [ratio(gt, ref.name, s.name) for s in sys]
        bp = ax.boxplot(data, positions=range(len(sys)), widths=0.55, whis=(10, 90), showfliers=False,
                        patch_artist=True, medianprops={"color": INK, "lw": 1.5})
        for patch, s in zip(bp["boxes"], sys):
            patch.set(facecolor=s.color, alpha=0.25 if s.faded else 0.45, edgecolor=s.color,
                      linestyle="--" if s.faded else "-")
        for i, (r, s) in enumerate(zip(data, sys)):
            ax.scatter(i + rng.uniform(-0.2, 0.2, len(r)), r, s=4, color=s.color, alpha=0.3 if s.faded else 0.45, lw=0)
            tot = _tot(gt, ref.name, s.name)
            ax.scatter(i, tot, marker="D", s=40, color="white", edgecolor=INK, zorder=5)
            ax.annotate(f"{tot:.2f}", (i + 0.3, tot), fontsize=8, color=INK, va="center")
        ax.axhline(1, color=INK, lw=1, ls="--")
        _log_y(ax)
        _ticks(ax, sys)
        ax.set_title(f"{t}  (n={len(gt)})", fontsize=10)
    axes[0][0].set_ylabel(f"t_{ref.name} / t_sys per query  (> 1 = faster than {ref.name})")
    fig.suptitle(f"Paired ratio against {ref.label}, {sub}. Box p25-p75, whiskers p10-p90, line = median, "
                 f"diamond = total {ref.name} / total sys", y=1.02, fontsize=10)
    return _save(fig, out, "01-ratio-vs-ref-by-type.png")


def totals(d: pd.DataFrame, ss: list[System], types: list[str], out: Path) -> str:
    g = done(d, *[s.name for s in ss])
    fig, axes = plt.subplots(1, len(types), figsize=(4 * len(types), 4.4), squeeze=False)
    for ax, (t, gt) in zip(axes[0], by_type(g, types)):
        vals = [gt[f"t_{s.name}"].sum() / 1e9 for s in ss]
        bars = ax.bar(range(len(ss)), vals, color=[s.color for s in ss], width=0.75, lw=0)
        for b, s in zip(bars, ss):
            if s.faded:
                b.set(hatch="///", edgecolor="white")
        ax.bar_label(bars, [f"{v:,.0f}" for v in vals], fontsize=8, color=INK, padding=2)
        _ticks(ax, ss)
        ax.set_title(f"{t}  (n={len(gt)})", fontsize=10)
        ax.grid(axis="x", visible=False)
        ax.margins(y=0.12)
    axes[0][0].set_ylabel("total time (s)")
    fig.suptitle("Total time per type, base set minus the timeouts of any shown system (each panel has its own scale)",
                 y=1.02, fontsize=10)
    return _save(fig, out, "02-totals-by-type.png")


def cdf(d: pd.DataFrame, ss: list[System], types: list[str], out: Path) -> str:
    types = [t for t in types if t != "ALL"]
    g = done(d, *[s.name for s in ss])
    fig, axes = plt.subplots(1, len(types), figsize=(5 * len(types), 4.4), sharey=True, squeeze=False)
    for ax, (t, gt) in zip(axes[0], by_type(g, types)):
        for s in ss:
            v = np.sort(gt[f"t_{s.name}"].values / 1e6)
            ax.step(v, np.arange(1, len(v) + 1) / len(v), where="post", color=s.color,
                    lw=1.2 if s.faded else 1.8, ls="--" if s.faded else "-", label=s.label)
        ax.set_xscale("log")
        ax.set_xlabel("time per query (ms, log)")
        ax.set_title(f"{t}  (n={len(gt)})", fontsize=10)
    axes[0][0].set_ylabel("fraction of queries")
    handles, labels = axes[0][0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=min(len(ss), 7), bbox_to_anchor=(0.5, -0.02))
    fig.suptitle("Cumulative distribution of times per query (unpaired; further left = faster)", y=1.02, fontsize=10)
    return _save(fig, out, "03-cdf-times-by-type.png")


def _pair_rows(g: pd.DataFrame, pairs: list[tuple[System, System]], types: list[str], sub: str, name: str,
               title: str, out: Path) -> str:
    types = [t for t in types if t != "ALL"]
    fig, axes = plt.subplots(len(pairs), len(types), figsize=(5 * len(types), 2.9 * len(pairs)), sharex=True,
                             sharey=True, squeeze=False)
    for i, (row, (ref, sys)) in enumerate(zip(axes, pairs)):
        for ax, (t, gt) in zip(row, by_type(g, types)):
            gt = done(gt, ref.name, sys.name)
            r = gt[f"t_{ref.name}"] / gt[f"t_{sys.name}"]
            ax.scatter(gt[f"t_{ref.name}"] / 1e9, r, s=9, color=sys.color, alpha=0.6, lw=0)
            ax.axhline(1, color=INK, lw=1, ls="--")
            ax.set_xscale("log")
            _log_y(ax)
            tot = gt[f"t_{ref.name}"].sum() / gt[f"t_{sys.name}"].sum()
            _stat(ax, f"{ref.name}/{sys.name} · n {len(r)} · med {r.median():.3f} · tot {tot:.3f}"
                      f" · {sys.name} wins {100 * (r > 1).mean():.0f}%")
            if i == 0:
                ax.set_title(t, fontsize=10)
        row[0].set_ylabel(f"t_{ref.name} / t_{sys.name}")
    for ax in axes[-1]:
        ax.set_xlabel("t of the pair's reference (s, log)")
    fig.suptitle(f"{title} ({sub}; > 1 = the second is faster)", y=1.0, fontsize=10)
    fig.tight_layout()
    return _save(fig, out, name)


def ratio_vs_ref_per_query(g: pd.DataFrame, ref: System, ss: list[System], types: list[str], sub: str,
                           out: Path) -> str:
    pairs = [(ref, s) for s in ss if s.name != ref.name]
    return _pair_rows(g, pairs, types, sub, "04-ratio-vs-ref-per-query.png",
                      f"Per query: ratio against {ref.label} by the time of {ref.name}", out)


def size_vs_time(g: pd.DataFrame, ref: System, ss: list[System], sizes: dict[str, int | None], types: list[str],
                 sub: str, out: Path) -> str | None:
    ss = [s for s in ss if sizes.get(s.name)]
    if not ss:
        return None
    fig, axes = plt.subplots(1, len(types), figsize=(3.75 * len(types), 4.2), sharex=True, squeeze=False)
    for ax, (t, gt) in zip(axes[0], by_type(g, types)):
        for s in ss:
            x, y = sizes[s.name] / 1e9, _tot(gt, ref.name, s.name)
            if s.faded:
                ax.scatter(x, y, s=150, facecolor="none", edgecolor=s.color, lw=1.8, zorder=5)
            else:
                ax.scatter(x, y, s=70, color=s.color, edgecolor="white", lw=2, zorder=4)
            ax.annotate(s.name, (x, y), xytext=(8, -12 if s.faded else 4), textcoords="offset points", fontsize=8,
                        color=MUTED if s.faded else INK)
        ax.axhline(1, color=INK, lw=1, ls="--")
        ax.set_xlabel("index size (GB)")
        ax.set_title(t, fontsize=10)
        ax.margins(x=0.15, y=0.2)
    axes[0][0].set_ylabel(f"total {ref.name} / total sys")
    fig.suptitle(f"Space against time, {sub}: further left = smaller index, higher = faster", y=1.02, fontsize=10)
    return _save(fig, out, "05-size-vs-time.png")


def pairs_per_query(g: pd.DataFrame, pairs: list[tuple[System, System]], types: list[str], sub: str,
                    out: Path) -> str | None:
    if not pairs:
        return None
    return _pair_rows(g, pairs, types, sub, "06-pairs-per-query.png", "Pairs, per query", out)


def draw_all(d: pd.DataFrame, g: pd.DataFrame, ref: System, ss: list[System], pairs: list[tuple[System, System]],
             sizes: dict[str, int | None], types: list[str], sub: str, out: Path) -> list[str]:
    with plt.rc_context(STYLE):
        names = [
            ratio_vs_ref(g, ref, ss, types, sub, out),
            totals(d, ss, types, out),
            cdf(d, ss, types, out),
            ratio_vs_ref_per_query(g, ref, ss, types, sub, out),
            size_vs_time(g, ref, ss, sizes, types, sub, out),
            pairs_per_query(g, [p for p in pairs if p[0].name != ref.name], types, sub, out),
        ]
    return [n for n in names if n]
