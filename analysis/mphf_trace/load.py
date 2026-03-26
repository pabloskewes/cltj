"""Parse mphf_trace.jsonl into a per-node DataFrame.

Input format
------------
The JSONL file contains three event types emitted by the MPHF builder:

  {"type":"node_start", "trie_id":0, "node_pos":0, "n_children":106829756, "threshold":4000}
  {"type":"try_result",  "retry":0, "m":133537427, "n":106829756,
                         "peeled":106829636, "peeled_frac":0.999999,
                         "success":false, "elapsed_ms":3177090}
  {"type":"node_end",   "success":false, "elapsed_ms":19956400}

Each node produces one node_start, zero or more try_result events, and one node_end.
A try_result is emitted for every attempt (successful or not); only failed ones are
kept here since the winning retry is implied by its absence.

Output columns
--------------
  trie_id, node_pos, n_children, threshold
  success          : bool   - did the node build successfully?
  retries_used     : int    - index of the winning retry (0 = first attempt succeeded),
                              or MAX_RETRIES if all retries failed
  min_residual     : int    - smallest (n_children - peeled) across all failed retries,
                              0 if first attempt succeeded
  max_residual     : int    - largest residual across failed retries, 0 if first succeeded
  best_peeled_frac : float  - best peeled_frac seen in any failed retry (1.0 if none)
  total_elapsed_ms : float  - wall time for the whole node (from node_start to node_end)

Derived filters (not stored, compute inline):
  first attempt succeeded : retries_used == 0
  had any failed retry    : retries_used > 0
"""

import json
from pathlib import Path

import pandas as pd


def load_trace(path: str | Path) -> pd.DataFrame:
    """Parse *path* (mphf_trace.jsonl) and return a per-node DataFrame."""
    path = Path(path)
    rows: list[dict] = []

    current: dict | None = None
    failed_retries: list[dict] = []

    with path.open() as fh:
        for raw in fh:
            raw = raw.strip()
            if not raw:
                continue
            ev = json.loads(raw)
            t = ev["type"]

            if t == "node_start":
                current = {
                    "trie_id": ev["trie_id"],
                    "node_pos": ev["node_pos"],
                    "n_children": ev["n_children"],
                    "threshold": ev["threshold"],
                }
                failed_retries = []

            elif t == "try_result" and current is not None:
                if not ev["success"]:
                    failed_retries.append(
                        {
                            "retry": ev["retry"],
                            "peeled": ev["peeled"],
                            "peeled_frac": ev["peeled_frac"],
                        }
                    )

            elif t == "node_end" and current is not None:
                n = current["n_children"]
                success: bool = ev["success"]

                if failed_retries:
                    residuals = [n - r["peeled"] for r in failed_retries]
                    min_res = min(residuals)
                    max_res = max(residuals)
                    best_frac = max(r["peeled_frac"] for r in failed_retries)
                    # winning retry index = last failed retry index + 1 (if success),
                    # or MAX_RETRIES (= len of failed list) if all failed
                    retries_used = (
                        failed_retries[-1]["retry"] + 1
                        if success
                        else len(failed_retries)
                    )
                else:
                    min_res = 0
                    max_res = 0
                    best_frac = 1.0
                    retries_used = 0

                rows.append(
                    {
                        **current,
                        "success": success,
                        "retries_used": retries_used,
                        "min_residual": min_res,
                        "max_residual": max_res,
                        "best_peeled_frac": best_frac,
                        "total_elapsed_ms": ev["elapsed_ms"],
                    }
                )
                current = None
                failed_retries = []

    return pd.DataFrame(rows)
