"""Tests of the N-systems overview.

The synthetic cases always run. The experiment cases load the CSVs of cltj-experiments (``CLTJ_EXPERIMENTS``,
default a sibling of the cltj checkouts) and check numbers already written down for EXP-017 and EXP-020; they
are skipped when the CSVs are not there.

Run from ``analysis/``: ``python -m unittest query_bench.test_query_bench``
"""

from __future__ import annotations

import os
import tempfile
import unittest
from pathlib import Path

from .filters import base_set, subset
from .pairs import pair_table
from .systems import load_config, load_systems

TYPES = ["TYPE1", "TYPE2", "TYPE3", "ALL"]
TYPES_TXT = Path(__file__).resolve().parents[2] / "Queries" / "types.txt"
EXPERIMENTS = Path(os.environ.get("CLTJ_EXPERIMENTS", Path(__file__).resolve().parents[4] / "cltj-experiments"))
EXP017 = EXPERIMENTS / "EXP-017-translation-schemes-a-vs-c"
EXP020 = EXPERIMENTS / "EXP-020-c3-and-upstream-baselines"


def run(exp: Path, pairs: list[tuple[str, str]]):
    cfg = load_config(exp / "analysis" / "overview" / "systems.toml", exp)
    ld = load_systems(cfg, TYPES_TXT)
    d = base_set(ld.wide, cfg, ld.systems)
    g = subset(d, cfg.min_ms * 1e6)
    r = pair_table({"main": g, "all": d}, pairs, TYPES).set_index(["subset", "pair", "type"])
    return ld, d, g, r


class Checks(unittest.TestCase):
    def check_rows(self, r, sub: str, rows: dict[str, list[tuple[float, ...]]], cols=("tot", "med", "win%")):
        for pair, cells in rows.items():
            for t, cell in zip(TYPES, cells):
                for col, want in zip(cols, cell):
                    got = r.loc[(sub, pair, t), col]
                    digits = 1 if col == "win%" else 3
                    self.assertEqual(round(got, digits), want, f"{sub} {pair} {t} {col}: {got}")


class Synthetic(Checks):
    def write(self, root: Path, name: str, rows: list[tuple[int, int, int]], header: str | None = "Index loaded: 1000 bytes."):
        lines = ([header] if header else []) + [f"{q};{n};{t}" for q, n, t in rows]
        (root / name).write_text("\n".join(lines) + "\n")

    def test_pending_extra_timeouts_and_external_subset(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "types.txt").write_text("0;TYPE1\n1;TYPE1\n2;TYPE2\n3;TYPE2\n4;TYPE3\n")
            ms = 10**6
            self.write(root, "a.csv", [(0, 1, 200 * ms), (1, 1, 50 * ms), (2, 1, 400 * ms), (4, 1, 300 * ms)])
            self.write(root, "b.csv", [(0, 1, 100 * ms), (1, 1, 50 * ms), (2, 1, 200 * ms), (4, 1, 100 * ms)])
            self.write(root, "e.csv", [(0, 1, 100 * ms), (1, 1, 25 * ms), (2, 1, 600 * 10**9), (4, 1, 150 * ms)],
                       header=None)
            self.write(root, "half.csv", [(0, 1, ms)])
            self.write(root, "old.csv", [(0, 1, 150 * ms), (1, 1, 99 * ms), (2, 1, 150 * ms), (4, 1, 150 * ms)])
            (root / "systems.toml").write_text("""
ref = "A"
skip = [3]
pairs = ["B/E"]
[subset]
csv = "old.csv"
[[system]]
name = "A"
csv = "a.csv"
[[system]]
name = "B"
csv = "b.csv"
[[system]]
name = "E"
csv = "e.csv"
group = "extra"
size = 7
[[system]]
name = "H"
csv = "half.csv"
[[system]]
name = "M"
csv = "missing.csv"
""")
            cfg = load_config(root / "systems.toml", root)
            ld = load_systems(cfg, root / "types.txt")
            self.assertEqual(ld.systems, ["A", "B", "E"])
            self.assertEqual(len(ld.pending), 2)
            self.assertEqual(ld.sizes, {"A": 1000, "B": 1000, "E": 7})
            self.assertEqual(list(ld.wide.index), [0, 1, 2, 4])
            self.assertEqual(len(ld.long()), 12)
            d = base_set(ld.wide, cfg, ld.systems)
            self.assertEqual(len(d), 4)
            g = subset(d, cfg.min_ms * 1e6)
            self.assertEqual(list(g.index), [0, 2, 4])
            r = pair_table({"main": g}, [("A", "B"), ("A", "E"), ("B", "E")], TYPES).set_index(["pair", "type"])
            self.assertEqual(r.loc[("A/B", "ALL"), "n"], 3)
            self.assertAlmostEqual(r.loc[("A/B", "ALL"), "tot"], 900 / 400)
            self.assertEqual(r.loc[("A/E", "ALL"), "n"], 2)
            self.assertAlmostEqual(r.loc[("A/E", "ALL"), "tot"], 500 / 250)
            self.assertAlmostEqual(r.loc[("B/E", "TYPE3"), "tot"], 100 / 150)
            self.assertEqual(r.loc[("B/E", "TYPE3"), "win%"], 0.0)


@unittest.skipUnless((EXP017 / "02-bench" / "bench-xcltj.csv").exists(), f"no EXP-017 CSVs under {EXPERIMENTS}")
class Exp017(Checks):
    """thesis-notes 0025/09-resultados-exp017.md."""

    @classmethod
    def setUpClass(cls):
        pairs = [("X", "H"), ("X", "A"), ("X", "C"), ("H", "C"), ("H", "A"), ("A", "C"), ("X", "A2"), ("X", "C2"),
                 ("H", "A2"), ("H", "C2"), ("A2", "C2"), ("A", "A2"), ("C", "C2")]
        cls.ld, cls.d, cls.g, cls.r = run(EXP017, pairs)

    def test_sets(self):
        self.assertEqual(self.ld.pending, [])
        self.assertEqual(len(self.d), 1283)
        self.assertEqual(self.g.type.value_counts().to_dict(), {"TYPE1": 108, "TYPE2": 282, "TYPE3": 97})

    def test_totals(self):
        want = {"X": [327.2, 1410.0, 1808.8, 3546.1], "H": [748.8, 2356.7, 1586.9, 4692.3],
                "A": [1240.4, 3349.3, 1945.4, 6535.1], "C": [762.4, 2411.5, 1619.2, 4793.1],
                "A2": [1237.9, 3377.2, 1908.1, 6523.2], "C2": [750.4, 2353.8, 1526.2, 4630.4]}
        for s, vals in want.items():
            for t, v in zip(TYPES, vals):
                g = self.d if t == "ALL" else self.d[self.d.type == t]
                self.assertEqual(round(g[f"t_{s}"].sum() / 1e9, 1), v, f"{s} {t}")

    def test_against_x(self):
        self.check_rows(self.r, "main", {
            "X/H": [(0.436, 0.413), (0.598, 0.642), (1.140, 1.025), (0.756, 0.641)],
            "X/A": [(0.263, 0.238), (0.420, 0.421), (0.930, 0.850), (0.543, 0.418)],
            "X/C": [(0.428, 0.403), (0.584, 0.640), (1.117, 0.995), (0.740, 0.635)],
        }, cols=("tot", "med"))

    def test_pairs_quantiles(self):
        cols = ("tot", "p10", "p25", "med", "p75", "p90", "win%")
        self.check_rows(self.r, "main", {
            "H/C": [(0.982, 0.969, 0.974, 0.979, 0.981, 0.985, 0.9), (0.977, 0.952, 0.982, 0.993, 1.002, 1.013, 29.1),
                    (0.980, 0.900, 0.955, 0.973, 0.996, 1.018, 21.6)],
            "H/A": [(0.604, 0.567, 0.572, 0.577, 0.582, 0.599, 0.0), (0.704, 0.617, 0.636, 0.661, 0.736, 0.830, 0.7),
                    (0.816, 0.655, 0.718, 0.791, 0.884, 0.936, 0.0)],
            "A/C": [(1.627, 1.641, 1.682, 1.695, 1.710, 1.729, 98.1), (1.389, 1.155, 1.347, 1.500, 1.562, 1.609, 95.7),
                    (1.202, 0.992, 1.053, 1.226, 1.389, 1.541, 85.6)],
        }, cols=cols)

    def test_a2_c2(self):
        rows = {
            "X/A2": [(0.263, 0.239), (0.417, 0.425), (0.948, 0.846), (0.544, 0.425, 9.9)],
            "X/C2": [(0.435, 0.409), (0.598, 0.661), (1.185, 1.051), (0.766, 0.658, 16.8)],
            "H/A2": [(0.605, 0.580), (0.698, 0.667), (0.832, 0.793), (0.719, 0.662, 3.7)],
            "H/C2": [(0.998, 0.992), (1.001, 1.023), (1.040, 1.026), (1.013, 1.015, 64.3)],
            "A2/C2": [(1.650, 1.708), (1.435, 1.552), (1.250, 1.278), (1.409, 1.563, 97.5)],
            "A/A2": [(1.002, 1.005), (0.992, 0.999), (1.020, 1.014), (1.002, 1.003, 60.4)],
            "C/C2": [(1.016, 1.013), (1.025, 1.031), (1.061, 1.050), (1.035, 1.024, 90.3)],
        }
        self.check_rows(self.r, "main", rows)


@unittest.skipUnless((EXP020 / "03-bench" / "bench-xrb.csv").exists(), f"no EXP-020 CSVs under {EXPERIMENTS}")
class Exp020(Checks):
    """thesis-notes 0025/24-resultados-exp020.md, as corrected after 25-."""

    @classmethod
    def setUpClass(cls):
        pairs = [("X-rb", "X-rb-2"), ("Xns", "X-rb"), ("Xns", "X-rb-2"), ("X-rb", "Xup"), ("C2ss", "C3ss"),
                 ("Xup", "C3ss"), ("Xup", "EF"), ("X-rb", "C3ss"), ("X-rb", "EF"), ("X-rb", "C2ss"), ("EF", "C3ss")]
        cls.ld, cls.d, cls.g, cls.r = run(EXP020, pairs)

    def test_sets_and_sizes(self):
        self.assertEqual(self.ld.pending, [])
        self.assertEqual(len(self.d), 1283)
        self.assertEqual(self.g.type.value_counts().to_dict(), {"TYPE1": 107, "TYPE2": 280, "TYPE3": 97})
        mb = {s: round(b / 1e6, 1) for s, b in self.ld.sizes.items()}
        self.assertEqual(mb, {"X-rb": 20348.7, "Xns": 20348.7, "X-rb-2": 20348.7, "C2ss": 17926.0,
                              "C3ss": 15227.3, "Xup": 17601.0, "EF": 14323.5})

    def test_main_subset(self):
        self.check_rows(self.r, "main", {
            "X-rb/X-rb-2": [(1.002, 1.000, 61.7), (0.999, 1.000, 43.2), (0.999, 0.999, 42.3), (1.000, 1.000, 47.1)],
            "Xns/X-rb": [(1.132, 1.111, 98.1), (1.089, 1.096, 99.3), (1.068, 1.059, 97.9), (1.082, 1.094, 98.8)],
            "Xns/X-rb-2": [(1.134, 1.108, 99.1), (1.088, 1.096, 98.2), (1.067, 1.058, 97.9), (1.082, 1.094, 98.3)],
            "X-rb/Xup": [(1.204, 1.245, 97.2), (1.162, 1.426, 94.3), (1.317, 1.183, 89.7), (1.239, 1.320, 94.0)],
            "C2ss/C3ss": [(1.040, 1.039, 83.2), (1.027, 1.019, 78.2), (1.059, 1.052, 89.7), (1.042, 1.027, 81.6)],
            "Xup/C3ss": [(0.839, 0.842, 0.0), (0.788, 0.668, 0.4), (0.971, 0.991, 49.5), (0.870, 0.785, 10.1)],
            "Xup/EF": [(0.974, 0.959, 0.0), (0.965, 0.950, 1.4), (1.054, 0.982, 46.4), (1.006, 0.954, 10.1)],
            "X-rb/C3ss": [(1.010, 1.034, 82.2), (0.916, 0.950, 30.4), (1.279, 1.178, 78.4), (1.077, 1.004, 51.4)],
            "X-rb/EF": [(1.172, 1.194, 97.2), (1.122, 1.337, 93.9), (1.388, 1.336, 89.7), (1.245, 1.282, 93.8)],
            "X-rb/C2ss": [(0.972, 0.988, 21.5), (0.891, 0.932, 17.1), (1.208, 1.127, 68.0), (1.034, 0.973, 28.3)],
            "EF/C3ss": [(0.862, 0.874, 6.5), (0.816, 0.720, 1.1), (0.922, 0.994, 44.3), (0.865, 0.819, 11.0)],
        })

    def test_all_queries(self):
        self.check_rows(self.r, "all", {
            "X-rb/X-rb-2": [(1.002, 1.001, 54.6), (0.999, 1.001, 53.4), (0.999, 1.000, 47.4), (1.000, 1.000, 53.0)],
            "C2ss/C3ss": [(1.040, 1.008, 60.4), (1.027, 1.026, 79.2), (1.059, 1.070, 86.8), (1.042, 1.026, 72.7)],
            "EF/C3ss": [(0.863, 0.953, 34.0), (0.816, 0.776, 14.7), (0.922, 0.942, 38.4), (0.865, 0.925, 26.0)],
        })

    def test_geo(self):
        self.assertEqual(round(self.r.loc[("main", "X-rb/Xup", "ALL"), "geo"], 3), 1.354)
        self.assertEqual(round(self.r.loc[("main", "C2ss/C3ss", "ALL"), "geo"], 3), 1.039)


if __name__ == "__main__":
    unittest.main()
