"""CLI: filter an original query file by correctness mismatch query ids.

Usage:
    python filter_queries.py <mismatches_csv> <queries_path> [--outdir DIR]
    python filter_queries.py <mismatches_csv> <queries_path> \
        --output-queries filtered.txt --mapping query_id_map.csv
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def load_query_ids(mismatches_csv: str | Path) -> list[int]:
    """Load unique ``query_id`` values, preserving their CSV order."""
    query_ids: list[int] = []
    seen: set[int] = set()

    with Path(mismatches_csv).open(newline="") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames is None or "query_id" not in reader.fieldnames:
            raise ValueError(f"{mismatches_csv} must contain a query_id column")

        for row_number, row in enumerate(reader, start=2):
            raw_query_id = row.get("query_id", "")
            try:
                query_id = int(raw_query_id)
            except ValueError as exc:
                raise ValueError(
                    f"invalid query_id at {mismatches_csv}:{row_number}: {raw_query_id!r}"
                ) from exc

            if query_id not in seen:
                seen.add(query_id)
                query_ids.append(query_id)

    return query_ids


def load_queries(queries_path: str | Path) -> list[str]:
    """Load original query lines without trailing newlines."""
    return Path(queries_path).read_text().splitlines()


def write_filtered_queries(
    query_ids: list[int],
    queries: list[str],
    output_queries: str | Path,
    mapping_path: str | Path,
) -> None:
    """Write the filtered query file and original-to-filtered id mapping."""
    output_queries = Path(output_queries)
    mapping_path = Path(mapping_path)
    output_queries.parent.mkdir(parents=True, exist_ok=True)
    mapping_path.parent.mkdir(parents=True, exist_ok=True)

    missing = [query_id for query_id in query_ids if query_id < 0 or query_id >= len(queries)]
    if missing:
        sample = ", ".join(str(query_id) for query_id in missing[:10])
        raise ValueError(
            f"{len(missing)} query_id values are outside the query file range "
            f"0..{len(queries) - 1}: {sample}"
        )

    with output_queries.open("w", newline="") as f:
        for query_id in query_ids:
            f.write(queries[query_id])
            f.write("\n")

    with mapping_path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["original_qid", "filtered_qid"])
        for filtered_qid, original_qid in enumerate(query_ids):
            writer.writerow([original_qid, filtered_qid])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "mismatches_csv",
        help="CSV with a query_id column, e.g. correctness/mismatches_non_timeout.csv",
    )
    parser.add_argument(
        "queries_path",
        help="Original query file whose 0-based line numbers match query_id values",
    )
    parser.add_argument(
        "--outdir",
        help=(
            "Directory for outputs (default: <mismatches_csv parent>/filtered_queries). "
            "Ignored when both explicit output paths are provided."
        ),
    )
    parser.add_argument(
        "--output-queries",
        help="Path for the filtered query file (default: <outdir>/queries.txt)",
    )
    parser.add_argument(
        "--mapping",
        help="Path for the original-to-filtered qid map (default: <outdir>/query_id_map.csv)",
    )
    args = parser.parse_args()

    mismatches_csv = Path(args.mismatches_csv).resolve()
    queries_path = Path(args.queries_path).resolve()
    outdir = (
        Path(args.outdir).resolve()
        if args.outdir
        else mismatches_csv.parent / "filtered_queries"
    )
    output_queries = (
        Path(args.output_queries).resolve()
        if args.output_queries
        else outdir / "queries.txt"
    )
    mapping_path = (
        Path(args.mapping).resolve()
        if args.mapping
        else outdir / "query_id_map.csv"
    )

    query_ids = load_query_ids(mismatches_csv)
    queries = load_queries(queries_path)
    write_filtered_queries(query_ids, queries, output_queries, mapping_path)

    print(f"Loaded {len(query_ids)} unique query ids from {mismatches_csv}")
    print(f"Loaded {len(queries)} original queries from {queries_path}")
    print(f"Saved {output_queries}")
    print(f"Saved {mapping_path}")


if __name__ == "__main__":
    main()
