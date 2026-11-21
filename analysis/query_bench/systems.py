"""Load N benchmark CSVs declared in an experiment's ``systems.toml``.

Format of ``systems.toml`` (paths relative to the experiment directory)::

    title = "EXP-020"
    ref = "X-rb"                      # reference of the "against ref" figures and default pairs
    min_ms = 100                      # main subset: t of the subset reference >= min_ms
    skip = [9, 14]                    # query ids left out of the run (--skip of the bench)
    pairs = ["X-rb/C2ss", "C2ss/C3ss"]

    [subset]                          # optional; default: the ref system of this experiment
    csv = "../EXP-018-.../03-bench/bench-x.csv"
    label = "X of EXP-018/03"         # or: system = "<name>" for a system of this experiment

    [[system]]                        # one per system; the order fixes the color
    name = "X-rb"
    csv = "03-bench/bench-xrb.csv"
    label = "X-rb"
    color = "#2a78d6"                 # optional, default: palette slot of its position
    group = "base"                    # base: its timeouts define the base set; extra: only its pairs
    faded = false                     # optional, drawn lighter (a reference variant)
    commit = "eb22e75"                # optional, shown in index.html
    size = 20348652924                # optional, index bytes when the CSV header lacks them
"""

from __future__ import annotations

import tomllib
from dataclasses import dataclass, field
from pathlib import Path

import pandas as pd

TIMEOUT_NS = 590e9
PALETTE = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
SUBSET_COL = "t_subset"


@dataclass
class System:
    name: str
    csv: Path
    label: str
    color: str
    group: str = "base"
    faded: bool = False
    commit: str | None = None
    size: int | None = None


@dataclass
class Config:
    title: str
    systems: list[System]
    ref: str
    min_ms: float = 100.0
    skip: list[int] = field(default_factory=list)
    pairs: list[tuple[str, str]] = field(default_factory=list)
    subset_csv: Path | None = None
    subset_system: str | None = None
    subset_label: str | None = None

    def system(self, name: str) -> System:
        return next(s for s in self.systems if s.name == name)


def parse_pairs(spec: list[str] | str) -> list[tuple[str, str]]:
    items = spec.split(",") if isinstance(spec, str) else spec
    pairs = [tuple(x.strip() for x in p.split("/")) for p in items if p.strip()]
    bad = [p for p in pairs if len(p) != 2 or not all(p)]
    if bad:
        raise ValueError(f"pairs must look like REF/SYS: {['/'.join(p) for p in bad]}")
    return pairs


def validate(cfg: Config) -> None:
    names = {s.name for s in cfg.systems}
    unknown = [n for n in [cfg.ref, cfg.subset_system] + [x for p in cfg.pairs for x in p] if n and n not in names]
    if unknown:
        raise ValueError(f"unknown systems {sorted(set(unknown))}; declared: {sorted(names)}")


def load_config(path: str | Path, exp_dir: str | Path) -> Config:
    path, exp_dir = Path(path), Path(exp_dir)
    raw = tomllib.loads(path.read_text())
    systems = []
    for i, s in enumerate(raw["system"]):
        assert s.get("group", "base") in ("base", "extra"), s
        systems.append(System(
            name=s["name"], csv=exp_dir / s["csv"], label=s.get("label", s["name"]),
            color=s.get("color", PALETTE[i % len(PALETTE)]), group=s.get("group", "base"),
            faded=s.get("faded", False), commit=s.get("commit"), size=s.get("size"),
        ))
    names = [s.name for s in systems]
    assert len(set(names)) == len(names), f"duplicate system names in {path}"
    sub = raw.get("subset", {})
    cfg = Config(
        title=raw.get("title", exp_dir.name), systems=systems, ref=raw.get("ref", names[0]),
        min_ms=raw.get("min_ms", 100.0), skip=raw.get("skip", []), pairs=parse_pairs(raw.get("pairs", [])),
        subset_csv=exp_dir / sub["csv"] if "csv" in sub else None, subset_system=sub.get("system"),
        subset_label=sub.get("label"),
    )
    validate(cfg)
    return cfg


def read_bench(path: str | Path) -> tuple[pd.DataFrame, int | None]:
    """``query_id;n_results;time_ns`` rows indexed by query id, and the bytes of the "Index loaded" line."""
    path = Path(path)
    with path.open() as f:
        first = f.readline()
    size = int(first.split()[2]) if first.startswith("Index loaded:") else None
    b = (pd.read_csv(path, sep=";", names=["q", "n", "t"], on_bad_lines="skip", comment="I")
         .dropna().astype("int64").set_index("q"))
    return b, size


def load_types(path: str | Path) -> pd.Series:
    return pd.read_csv(path, sep=";", names=["q", "type"]).astype({"q": int}).set_index("q").type


@dataclass
class Loaded:
    """``wide`` has ``type`` and ``n_<sys>`` / ``t_<sys>`` per loaded system, one row per expected query."""

    wide: pd.DataFrame
    systems: list[str]
    sizes: dict[str, int | None]
    pending: list[str]

    def long(self) -> pd.DataFrame:
        """``query_id, type, system, n, t_ns``."""
        rows = []
        for s in self.systems:
            rows.append(pd.DataFrame({"query_id": self.wide.index, "type": self.wide.type.values, "system": s,
                                      "n": self.wide[f"n_{s}"].values, "t_ns": self.wide[f"t_{s}"].values}))
        return pd.concat(rows, ignore_index=True)


def load_systems(cfg: Config, types_path: str | Path) -> Loaded:
    """A system enters only when its CSV has every expected query (types.txt minus ``skip``)."""
    ty = load_types(types_path).drop(cfg.skip, errors="ignore")
    d = pd.DataFrame({"type": ty})
    loaded, sizes, pending = [], {}, []
    for s in cfg.systems:
        if not s.csv.exists():
            pending.append(f"{s.name}: pending (no file {s.csv.name})")
            continue
        b, size = read_bench(s.csv)
        missing = d.index.difference(b.index)
        if len(missing):
            pending.append(f"{s.name}: pending ({len(d) - len(missing)}/{len(d)} queries)")
            continue
        d[f"n_{s.name}"], d[f"t_{s.name}"] = b.n, b.t
        loaded.append(s.name)
        sizes[s.name] = s.size if s.size is not None else size
    if cfg.subset_csv is not None:
        b, _ = read_bench(cfg.subset_csv)
        d[SUBSET_COL] = b.t.reindex(d.index)
    elif (cfg.subset_system or cfg.ref) in loaded:
        d[SUBSET_COL] = d[f"t_{cfg.subset_system or cfg.ref}"]
    else:
        raise ValueError(f"subset reference {cfg.subset_system or cfg.ref} is pending: {pending}")
    return Loaded(d, loaded, sizes, pending)
