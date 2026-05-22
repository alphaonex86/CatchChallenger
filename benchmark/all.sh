#!/bin/bash
# all.sh — run every benchmark, optionally looping for a time budget.
#
# Usage:
#   ./all.sh                  Run all benchmarks once (records history).
#   ./all.sh <seconds>        Loop until <seconds> of wall-clock time have
#                             passed. Checks only after each full loop (never
#                             aborts mid-benchmark). Exit code is the number
#                             of loops completed (0 = none).
#   ./all.sh --profile [tool] Profile every benchmark across local + all
#                             exec nodes (callgrind+perf by default, or the
#                             named tool). Runs ONCE, writes NO history /
#                             champion / candidate / chart — a profiling
#                             build (callgrind ~30x) perturbs timing and must
#                             never become the regression datum. The
#                             benchmarks themselves skip recording under
#                             --profile; all.sh just forwards the flag and
#                             never loops in this mode.

set -e

cd "$(dirname "$0")"
BENCHMARKS=(
    ./benchmarkmapmanager.py
    ./benchmarkserversave.py
    ./benchmarkbotactions.py
)

COMMENT="${COMMENT:-}"

# ---- profile mode -------------------------------------------------------
# Enter it when the first arg is --profile (optionally followed by a tool
# name, or given as --profile=tool). Forward the exact flag to each
# benchmark; do NOT loop and do NOT regenerate charts — nothing is recorded.
if [ "${1:-}" = "--profile" ] || [ "${1:-}" = "--profile=callgrind" ] \
   || [ "${1:-}" = "--profile=perf" ] || [ "${1:-}" = "--profile=massif" ] \
   || [ "${1:-}" = "--profile=all" ]; then
    PROFILE_ARGS=("$1")
    # Allow "./all.sh --profile callgrind" (tool as a separate token).
    case "$1" in
        --profile)
            case "${2:-}" in
                callgrind|perf|massif|all) PROFILE_ARGS+=("$2") ;;
            esac
            ;;
    esac
    for b in "${BENCHMARKS[@]}"; do
        "$b" "${PROFILE_ARGS[@]}"
    done
    exit 0
fi

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
