# CLTJ Analysis Tools

This directory contains Python tooling for analyzing various aspects of the CLTJ index, intersection complexity, and MPHF hashing strategies.

The tools are organized into self-contained modules by topic. Each module has its own `README.md` and a Jupyter Notebook (`*.ipynb`) as the primary entry point for interactive exploration.

## Modules

### Intersection Complexity
- [`alternation_complexity/`](alternation_complexity/): Prototyping and visualization of the alternation complexity ($\delta$) partition certificate algorithm.
- [`complexity_distribution_analysis/`](complexity_distribution_analysis/): Statistical analysis comparing empirical intersection cost vs theoretical complexity metrics (like $q \times \min$).

### MPHF Strategy Evaluation
- [`hashing/`](hashing/): Performance comparison of different MPHF families (BDZ, GlGh, PTHash) and storage policies (Quotienting) in isolation.

### Query benchmark (X-CLTJ vs H-CLTJ)

- [`query_bench/`](query_bench/): CLI `main.py` merges `bench-xcltj.csv` / `bench-hcltj.csv`, writes `comparison.csv`, summaries, `report.txt`, and figures under `figures/`: `time_scatter.png`, `median_by_type.png`, `mean_by_type.png` (mean times per query type; sensitive to tails/timeouts), `speedup_boxplot.png`, `winrate_by_type.png` (stacked bar of win-rate per type), `timeout_heatmap.png` (timeouts per engine/type), `summary_card.png` (dashboard with key metrics).

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