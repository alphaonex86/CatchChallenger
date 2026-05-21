#!/bin/bash
# all.sh — run every benchmark, optionally looping for a time budget.
#
# Usage:
#   ./all.sh              Run all benchmarks once.
#   ./all.sh <seconds>    Loop until <seconds> of wall-clock time have
#                         passed. Checks only after each full loop (never
#                         aborts mid-benchmark). Exit code is the number
#                         of loops completed (0 = none).

set -e

cd "$(dirname "$0")"
BENCHMARKS=(
    ./benchmarkmapmanager.py
    ./benchmarkserversave.py
    ./benchmarkbotactions.py
)

COMMENT="${COMMENT:-}"
BUDGET_S="${1:-0}"

run_loop() {
    for b in "${BENCHMARKS[@]}"; do
        if [ -n "$COMMENT" ]; then
            "$b" --comment="$COMMENT"
        else
            "$b"
        fi
    done
    python3 chart_generator.py
}

if [ "$BUDGET_S" -le 0 ] 2>/dev/null; then
    # Single pass
    run_loop
    exit 0
fi

# Looping mode: run full loops until budget_s seconds have elapsed.
# Check only at loop boundaries, never mid-benchmark.
t_start=$(date +%s)
loops=0
while true; do
    run_loop
    loops=$((loops + 1))
    t_now=$(date +%s)
    elapsed=$((t_now - t_start))
    if [ "$elapsed" -ge "$BUDGET_S" ]; then
        break
    fi
done
exit $loops
