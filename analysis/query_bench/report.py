"""Self-contained ``index.html``: systems, convention, query sets, tables and embedded figures."""

from __future__ import annotations

import base64
import html
import os
from pathlib import Path

import pandas as pd

from .pairs import CONVENTION
from .systems import Config

CSS = """body{font:14px/1.5 system-ui,sans-serif;color:#0b0b0b;background:#fcfcfb;max-width:1500px;margin:2em auto;padding:0 1em}
h1{font-size:20px} h2{font-size:16px;margin-top:2em} p,li{color:#52514e} table{border-collapse:collapse;margin:1em 0}
td,th{border-bottom:1px solid #e4e3df;padding:4px 12px;text-align:right;font-variant-numeric:tabular-nums}
td.l,th.l{text-align:left} figure{margin:2em 0} img{max-width:100%}
figcaption{font-size:12px;color:#7a7974} tr.faded td{color:#9a9994} .sw{display:inline-block;width:10px;height:10px;margin-right:6px}"""


def _table(head: list[str], rows: list[tuple[list[str], str]], left: int = 1) -> str:
    al = lambda i: " class=l" if i < left else ""
    th = "".join(f"<th{al(i)}>{html.escape(h)}</th>" for i, h in enumerate(head))
    body = "".join(f"<tr{cls}>" + "".join(f"<td{al(i)}>{c}</td>" for i, c in enumerate(cells)) + "</tr>"
                   for cells, cls in rows)
    return f"<table><tr>{th}</tr>{body}</table>"


def _counts(g: pd.DataFrame, types: list[str]) -> str:
    per = ", ".join(f"{t} {int((g.type == t).sum())}" for t in types if t != "ALL")
    return f"n = {len(g)} ({per})"


def systems_table(cfg: Config, exp_dir: Path, loaded: list[str], sizes: dict[str, int | None]) -> str:
    rows = []
    for s in cfg.systems:
        size = sizes.get(s.name)
        cells = [f'<span class=sw style="background:{s.color}"></span>{html.escape(s.name)}', html.escape(s.label),
                 s.group, html.escape(s.commit or ""), html.escape(os.path.relpath(s.csv, exp_dir)),
                 f"{size / 1e9:.2f}" if size else "", "loaded" if s.name in loaded else "pending"]
        rows.append((cells, " class=faded" if s.faded else ""))
    return _table(["system", "label", "group", "commit", "csv", "GB", "status"], rows, left=5)


def pair_summary(r: pd.DataFrame, subset: str, types: list[str], faded: set[str]) -> str:
    rs = r[r.subset == subset]
    rows = []
    for pair, g in rs.groupby("pair", sort=False):
        g = g.set_index("type")
        cells = [html.escape(pair)] + [f"{g.tot[t]:.3f} / {g.med[t]:.3f} / {g['win%'][t]:.1f}" for t in types]
        cells.append(" / ".join(str(int(g.n[t])) for t in types))
        rows.append((cells, " class=faded" if faded & {g.ref.iloc[0], g.sys.iloc[0]} else ""))
    return _table(["ref/sys"] + [t if t != "ALL" else "all" for t in types] + ["n per type"], rows)


def write(out: Path, cfg: Config, exp_dir: Path, loaded: list[str], sizes: dict[str, int | None], pending: list[str],
          d: pd.DataFrame, g: pd.DataFrame, r: pd.DataFrame, figs: list[str], types: list[str],
          base: list[str], sub: str, timeout_ns: float) -> Path:
    faded = {s.name for s in cfg.systems if s.faded}
    extra = [s.name for s in cfg.systems if s.group == "extra" and s.name in loaded]
    pend = "".join(f"<li>{html.escape(p)}</li>" for p in pending) or "<li>none</li>"
    imgs = "".join(
        f'<figure><img alt="{n}" src="data:image/png;base64,{base64.b64encode((out / n).read_bytes()).decode()}">'
        f"<figcaption>{n}</figcaption></figure>" for n in figs)
    extra_note = (f" Extra systems ({', '.join(extra)}) do not define it: their own timeouts drop a query only from "
                  "their pairs." if extra else "")
    skip = f" minus the {len(cfg.skip)} skipped ids {cfg.skip}" if cfg.skip else ""
    page = f"""<!doctype html><html lang="en"><head><meta charset="utf-8">
<title>{html.escape(cfg.title)} query benchmark</title><style>{CSS}</style></head><body>
<h1>{html.escape(cfg.title)}: query benchmark of {len(loaded)} systems</h1>
{systems_table(cfg, exp_dir, loaded, sizes)}
<p><b>Convention:</b> {html.escape(CONVENTION)}. Cells: tot / median / % of queries where sys wins.</p>
<p><b>Base set:</b> queries of types.txt{skip}, without timeout (t &gt;= {timeout_ns / 1e9:g} s) in {html.escape(', '.join(base))},
{_counts(d, types)}.{html.escape(extra_note)}</p>
<p><b>Main subset:</b> {html.escape(sub)}, {_counts(g, types)}.</p>
<p>Pending:</p><ul>{pend}</ul>
<h2>Pairs, {html.escape(sub)}</h2>{pair_summary(r, "main", types, faded)}
<h2>Pairs, base set</h2>{pair_summary(r, "all", types, faded)}
{imgs}</body></html>
"""
    path = out / "index.html"
    path.write_text(page)
    return path
