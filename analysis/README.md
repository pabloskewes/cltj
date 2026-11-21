# CLTJ Analysis Tools

This directory contains Python tooling for analyzing various aspects of the CLTJ index, intersection complexity, and MPHF hashing strategies.

The tools are organized into self-contained modules by topic. Each module has its own `README.md` and a Jupyter Notebook (`*.ipynb`) as the primary entry point for interactive exploration.

## Modules

### Intersection Complexity
- [`alternation_complexity/`](alternation_complexity/): Prototyping and visualization of the alternation complexity ($\delta$) partition certificate algorithm.
- [`complexity_distribution_analysis/`](complexity_distribution_analysis/): Statistical analysis comparing empirical intersection cost vs theoretical complexity metrics (like $q \times \min$).

### MPHF Strategy Evaluation
- [`hashing/`](hashing/): Performance comparison of different MPHF families (BDZ, GlGh, PTHash) and storage policies (Quotienting) in isolation.

### Query benchmark

- [`query_bench/`](query_bench/), N systems: `python -m query_bench <exp_dir> [--systems TOML] [--ref NAME] [--min-ms 100] [--pairs A/B,C/D]`, run from `analysis/`. The systems of an experiment are declared in a `systems.toml` (format in `query_bench/systems.py`; default `<exp_dir>/analysis/overview/systems.toml`). Shared query sets (base set without timeouts in the `base` systems, main subset by the time of a reference that may come from another experiment), paired per-query ratios `t_ref / t_sys` for the reference against every system plus the declared pairs, figures with one panel per query type, `pairs.csv` and a self-contained `index.html`. Tests: `python -m unittest query_bench.test_query_bench` (the experiment cases need the CSVs of `cltj-experiments`, found through `CLTJ_EXPERIMENTS`).
- [`query_bench/`](query_bench/), X-CLTJ vs H-CLTJ: CLI `main.py` merges `bench-xcltj.csv` / `bench-hcltj.csv`, writes `comparison.csv`, summaries, `report.txt`, and figures under `figures/`: `time_scatter.png`, `median_by_type.png`, `mean_by_type.png` (mean times per query type; sensitive to tails/timeouts), `speedup_boxplot.png`, `winrate_by_type.png` (stacked bar of win-rate per type), `timeout_heatmap.png` (timeouts per engine/type), `summary_card.png` (dashboard with key metrics).

## Setup
- [`children_histogram/`](children_histogram/): Analyzes trie node size distributions to determine optimal hashing thresholds.
- [`trie_viz/`](trie_viz/): Visualizes the metatrie structure (full vs partial tries) from node dumps.
- [`mphf_trace/`](mphf_trace/): Analyzes MPHF build traces to diagnose peeling failures, retry distributions, and residual 2-cores.

## Setup

The directory uses `uv` for dependency management.

```bash
cd analysis
uv sync
source .venv/bin/activate
```