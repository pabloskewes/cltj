#!/usr/bin/env bash
# LTJ-26 diagnostic batch (EXP-014). Throwaway.
# Phase A: callgrind (flock -s). Phase B: timings (flock -x), one run alone at a time.
set -u
W=~/tesis/worktrees/ltj-26-diagnose-and-remove-h-cltjs-per-element-query-overhead
OUT=~/tesis/data/experiments/EXP-014-diagnose-h-cltj-per-element-overhead
IDX=~/tesis/data/experiments/EXP-013-recover-hashed-labels-from-mphf/02-rebuild-and-benchmark-deduped
XI=$IDX/wikidata-enum-80.dat.xcltj
HI=$IDX/wikidata-enum-80.dat.hcltj
LOCK=~/tesis/data/.measure.lock
QD=$W/Queries/ltj26
PROG=$OUT/progress.log
VG=~/valgrind/bin/valgrind
VGOPTS="--tool=callgrind --instr-atstart=no --cache-sim=yes --branch-sim=yes \
  --I1=32768,8,64 --D1=32768,8,64 --LL=33554432,16,64"

stage() { echo "[$(date -Is)] $*" >> "$PROG"; }
mkdir -p $OUT/callgrind $OUT/timing
cd $W

# ---- Phase A: callgrind, build -O2 -g -DNDEBUG (build-prof) ----
cg() {  # name bin index queries [ENV=VAL...]
  local name=$1 bin=$2 idx=$3 q=$4; shift 4
  stage "CG $name start"
  env "$@" flock -s $LOCK $VG $VGOPTS --callgrind-out-file=$OUT/callgrind/$name.out \
    ./build-prof/bench/$bin $idx $QD/$q.txt 0 normal 600 --count-only \
    > $OUT/callgrind/$name.csv 2> $OUT/callgrind/$name.valgrind.log
  stage "CG $name done rc=$?"
}
cg x-def   bench-query-xcltj $XI prof     &
cg h-def   bench-query-hcltj $HI prof     &
cg x-rev   bench-query-xcltj $XI prof-rev LTJ26_LONELY_REVERSE=1 &
cg h-rev   bench-query-hcltj $HI prof-rev LTJ26_LONELY_REVERSE=1 &
wait
cg h-spo   bench-query-hcltj $HI prof-1   LTJ26_ENUM=spo &
cg h-key   bench-query-hcltj $HI prof-1   LTJ26_ENUM=key &
cg h-short bench-query-hcltj $HI prof-1   LTJ26_SHORTCUT=1 &
wait
for f in $OUT/callgrind/*.out.*; do
  ~/valgrind/bin/callgrind_annotate --inclusive=yes $f > $f.incl.txt 2>&1
  ~/valgrind/bin/callgrind_annotate $f > $f.self.txt 2>&1
done
stage "PHASE_A done"

# ---- Phase B: timings, Release build (build), exclusive lock ----
tm() {  # name bin index queries [ENV=VAL...]
  local name=$1 bin=$2 idx=$3 q=$4; shift 4
  stage "TM $name start"
  env "$@" flock -x $LOCK ./build/bench/$bin $idx $QD/$q.txt 0 normal 600 --count-only \
    > $OUT/timing/$name.csv 2> $OUT/timing/$name.err
  stage "TM $name done rc=$? lines=$(wc -l < $OUT/timing/$name.csv)"
}
tm x-def   bench-query-xcltj $XI vPv-all
tm h-def   bench-query-hcltj $HI vPv-all
tm x-rev   bench-query-xcltj $XI vPv-all LTJ26_LONELY_REVERSE=1
tm h-rev   bench-query-hcltj $HI vPv-all LTJ26_LONELY_REVERSE=1
tm h-key   bench-query-hcltj $HI vPv-sub LTJ26_ENUM=key
tm h-spo   bench-query-hcltj $HI vPv-sub LTJ26_ENUM=spo
tm h-short bench-query-hcltj $HI vPv-sub LTJ26_SHORTCUT=1
stage "ALL_DONE"
