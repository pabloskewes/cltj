"""Timeout-aware correctness analysis for query-benchmark comparisons."""

import pandas as pd


def add_correctness_enrichment(
    df: pd.DataFrame,
    timeout_ns: int = 600_000_000_000,
) -> pd.DataFrame:
    """Add timeout and count-difference diagnostics to a comparison table."""
    enriched = df.copy()

    enriched["count_diff"] = (
        enriched["n_results_hcltj"] - enriched["n_results_xcltj"]
    )
    enriched["count_diff_abs"] = enriched["count_diff"].abs()
    enriched["count_match"] = enriched["count_diff"] == 0

    rel_pct = pd.Series(0.0, index=enriched.index, dtype=float)
    nonzero = enriched["n_results_xcltj"] != 0
    rel_pct.loc[nonzero] = (
        100.0
        * enriched.loc[nonzero, "count_diff_abs"]
        / enriched.loc[nonzero, "n_results_xcltj"]
    )
    zero_den_mismatch = (~nonzero) & (enriched["count_diff_abs"] > 0)
    rel_pct.loc[zero_den_mismatch] = float("inf")
    enriched["count_diff_rel_pct"] = rel_pct

    enriched["timeout_xcltj"] = enriched["time_ns_xcltj"] >= timeout_ns
    enriched["timeout_hcltj"] = enriched["time_ns_hcltj"] >= timeout_ns
    enriched["timeout_any"] = enriched["timeout_xcltj"] | enriched["timeout_hcltj"]
    enriched["timeout_both"] = (
        enriched["timeout_xcltj"] & enriched["timeout_hcltj"]
    )

    enriched["mismatch_class"] = "exact_match"
    enriched.loc[
        (~enriched["count_match"]) & (~enriched["timeout_any"]), "mismatch_class"
    ] = "real_mismatch"
    enriched.loc[
        (~enriched["count_match"]) & enriched["timeout_both"], "mismatch_class"
    ] = "timeout_both_mismatch"
    enriched.loc[
        (~enriched["count_match"])
        & enriched["timeout_any"]
        & (~enriched["timeout_both"]),
        "mismatch_class",
    ] = "timeout_one_side_mismatch"
    return enriched


def summary_overall(df: pd.DataFrame) -> pd.DataFrame:
    """Return a one-row timeout-aware correctness summary."""
    return pd.DataFrame(
        [
            {
                "n_queries": len(df),
                "n_count_matches": int(df["count_match"].sum()),
                "n_count_mismatches": int((~df["count_match"]).sum()),
                "n_real_mismatches": int(
                    (df["mismatch_class"] == "real_mismatch").sum()
                ),
                "n_timeout_both_mismatches": int(
                    (df["mismatch_class"] == "timeout_both_mismatch").sum()
                ),
                "n_timeout_one_side_mismatches": int(
                    (df["mismatch_class"] == "timeout_one_side_mismatch").sum()
                ),
                "n_timeout_xcltj": int(df["timeout_xcltj"].sum()),
                "n_timeout_hcltj": int(df["timeout_hcltj"].sum()),
                "max_count_diff_abs": int(df["count_diff_abs"].max()),
                "median_count_diff_abs": float(df["count_diff_abs"].median()),
                "max_count_diff_rel_pct": float(df["count_diff_rel_pct"].max()),
                "median_count_diff_rel_pct": float(
                    df["count_diff_rel_pct"].median()
                ),
            }
        ]
    )


def summary_by_type(df: pd.DataFrame) -> pd.DataFrame:
    """Return timeout-aware correctness summary by query type."""
    rows = []
    for query_type, group in df.groupby("query_type"):
        rows.append(
            {
                "query_type": query_type,
                "n_queries": len(group),
                "n_count_matches": int(group["count_match"].sum()),
                "n_count_mismatches": int((~group["count_match"]).sum()),
                "n_real_mismatches": int(
                    (group["mismatch_class"] == "real_mismatch").sum()
                ),
                "n_timeout_both_mismatches": int(
                    (group["mismatch_class"] == "timeout_both_mismatch").sum()
                ),
                "n_timeout_one_side_mismatches": int(
                    (group["mismatch_class"] == "timeout_one_side_mismatch").sum()
                ),
                "n_timeout_xcltj": int(group["timeout_xcltj"].sum()),
                "n_timeout_hcltj": int(group["timeout_hcltj"].sum()),
            }
        )
    return pd.DataFrame(rows).set_index("query_type")


def mismatches_non_timeout(df: pd.DataFrame) -> pd.DataFrame:
    """Return mismatches not explained by timeout."""
    cols = [
        "query_id",
        "query_type",
        "n_results_xcltj",
        "n_results_hcltj",
        "count_diff",
        "count_diff_abs",
        "count_diff_rel_pct",
        "time_ns_xcltj",
        "time_ns_hcltj",
        "mismatch_class",
    ]
    return (
        df[df["mismatch_class"] == "real_mismatch"][cols]
        .sort_values(["count_diff_abs", "query_id"], ascending=[False, True])
        .reset_index(drop=True)
    )


def mismatches_timeout_only(df: pd.DataFrame) -> pd.DataFrame:
    """Return mismatches where at least one engine timed out."""
    cols = [
        "query_id",
        "query_type",
        "n_results_xcltj",
        "n_results_hcltj",
        "count_diff",
        "count_diff_abs",
        "count_diff_rel_pct",
        "time_ns_xcltj",
        "time_ns_hcltj",
        "timeout_xcltj",
        "timeout_hcltj",
        "mismatch_class",
    ]
    return (
        df[
            df["mismatch_class"].isin(
                ["timeout_both_mismatch", "timeout_one_side_mismatch"]
            )
        ][cols]
        .sort_values(["count_diff_abs", "query_id"], ascending=[False, True])
        .reset_index(drop=True)
    )
