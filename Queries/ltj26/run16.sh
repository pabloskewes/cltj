#!/usr/bin/env bash
# EXP-016 batch (LTJ-26, throwaway). Runs only after an explicit ok.
# A (flock -s): build H variant -> verify -> size breakdowns, in parallel with the X and H-default counter runs.
# B (flock -x): paired timings X, H-default, H-variant, one at a time, EXP-013/02 arguments.
# C (flock -s): counter run on H-variant.
set -u
W=~/tesis/worktrees/ltj-26-diagnose-and-remove-h-cltjs-per-element-query-overhead
OUT=~/tesis/data/experiments/EXP-016-unhashed-full-trie-roots
IDX=~/tesis/data/experiments/EXP-013-recover-hashed-labels-from-mphf/02-rebuild-and-benchmark-deduped
DAT=~/tesis/cltj/wikidata/zenodo/80-id/wikidata-enum-80.dat
XI=$IDX/wikidata-enum-80.dat.xcltj
HI=$IDX/wikidata-enum-80.dat.hcltj
VI=$OUT/wikidata-enum-80.dat.noroot.hcltj
Q=$W/Queries/Queries-bgps-limit1000.txt
LOCK=~/tesis/data/.measure.lock
PROG=$OUT/progress.log
ARGS="0 normal 600 --count-only"

stage() { echo "[$(date -Is)] $*" >> "$PROG"; }
mkdir -p $OUT/counts
cd $W
stage "START commit=$(git rev-parse --short HEAD)"

cnt() {  # name bin index
  stage "COUNT $1 start"
  flock -s $LOCK ./build-count/bench/$2 $3 $Q $ARGS > $OUT/counts/$1.csv 2> $OUT/counts/$1.err
  stage "COUNT $1 done rc=$? lines=$(wc -l < $OUT/counts/$1.csv)"
}

# ---- A ----
(
  stage "BUILD_VAR start"
  flock -s $LOCK ./build/bench/build-hcltj $DAT -t 4000 --no-root-hash -o $VI > $OUT/build-hcltj-noroot.log 2>&1
  stage "BUILD_VAR done rc=$?"
  stage "VERIFY_VAR start"
  flock -s $LOCK ./build/analysis/verify-mphf-hcltj $VI > $OUT/verify-noroot.log 2>&1
  stage "VERIFY_VAR done rc=$?"
  flock -s $LOCK ./build/analysis/size-breakdown-hcltj $HI $OUT/size-hdef.json > $OUT/size-hdef.txt 2>&1
  flock -s $LOCK ./build/analysis/size-breakdown-hcltj $VI $OUT/size-hvar.json > $OUT/size-hvar.txt 2>&1
  stage "SIZE done"
) &
cnt x    bench-query-xcltj $XI &
cnt hdef bench-query-hcltj $HI &
wait
stage "PHASE_A done"

# ---- B ----
tm() {  # name bin index
  stage "BENCH $1 start"
  flock -x $LOCK ./build/bench/$2 $3 $Q $ARGS > $OUT/bench-$1.csv 2> $OUT/bench-$1.err
  stage "BENCH $1 done rc=$? lines=$(wc -l < $OUT/bench-$1.csv)"
}
tm xcltj      bench-query-xcltj $XI
tm hcltj      bench-query-hcltj $HI
tm hcltj-var  bench-query-hcltj $VI
stage "PHASE_B done"

# ---- C ----
cnt hvar bench-query-hcltj $VI
stage "ALL_DONE"
