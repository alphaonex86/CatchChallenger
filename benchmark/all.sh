#!/bin/bash
# all.sh — run every benchmark, optionally looping for a time budget.
#
# Usage:
#   ./all.sh                       Run all benchmarks once (records history).
#   ./all.sh <seconds>             Loop until <seconds> of wall-clock time have
#                                  passed (legacy positional form). Checks only
#                                  after each full loop (never aborts mid-
#                                  benchmark). Exit code = loops completed.
#   ./all.sh --maxtime <minutes>   Same soak loop, budget expressed in MINUTES.
#   ./all.sh --maxtime=<minutes>   Equivalent. At least one full loop always
#                                  runs (budget checked AFTER each loop), so
#                                  --maxtime=0 means "exactly one loop".
#   ./all.sh --profile [tool]      Profile every benchmark across local + all
#                                  exec nodes (callgrind+perf by default, or
#                                  the named tool). Runs ONCE, writes NO
#                                  history / champion / candidate / chart — a
#                                  profiling build (callgrind ~30x) perturbs
#                                  timing and must never become the regression
#                                  datum. The benchmarks themselves skip
#                                  recording under --profile; all.sh just
#                                  forwards the flag and never loops in this
#                                  mode.
#   ./all.sh --node <label|arch>   Restrict the run to specific execution
#                                  node(s) by execution_nodes label or arch
#                                  (repeatable; 'local' = host baseline only).
#                                  Combines with any other flag. Default: every
#                                  benchmark-enabled node.
#   ./all.sh --help                Show this help and exit.
#
# Environment:
#   COMMENT="..."  Free-text tag forwarded as --comment to every benchmark and
#                  stored verbatim in this run's candidate/champion/history
#                  JSON (e.g. COMMENT="Invert loop" ./all.sh). Empty = untagged.
#
# Examples:
#   COMMENT="Invert loop" ./all.sh --node armv6   one comment, armv6 nodes only
#   ./all.sh --maxtime=120 --node local           2h soak on the host baseline

usage() {
    sed -n '2,39p' "$0" | sed 's/^# \{0,1\}//'
}

set -e

cd "$(dirname "$0")"
BENCHMARKS=(
    ./benchmarkmapmanager.py
    # Same production min_network(), multi-map load model: the population is
    # spread over outdoor/city/indoor maps and the players walk against a
    # collision grid, so it measures the per-map constant and the crowded-map
    # diff that the single-map benchmark above cannot separate.
    ./benchmarkmapmanager2.py
    ./benchmarkserversave.py
    ./benchmarkbotactions.py
    ./benchmarkclientlatency.py
    # The epoll-vs-io_uring A/B. Fixed-TIME like the rest: each cell is a
    # --seconds saturating window, and every node measures at once (the
    # client is node-local, so hosts share nothing), so the fleet costs
    # ~2 x --reps windows of wall time, not one per node. Only its builds
    # are capped, and only per compile node.
    ./benchmarkepolliouring.py
)

COMMENT="${COMMENT:-}"

# Pre-scan for --node LABEL|ARCH (repeatable; --node=X form too): pull them
# out of the positional args and forward verbatim to every benchmark*.py so
# the run is restricted to those execution nodes. 'local' = host baseline
# only. Everything else stays in "$@" for the maxtime/profile parsing below.
NODE_ARGS=()
REST=()
while [ $# -gt 0 ]; do
    case "$1" in
        --node)
            if [ -z "${2:-}" ]; then
                echo "--node requires a value (execution_nodes label or arch)" >&2
                exit 2
            fi
            NODE_ARGS+=("--node" "$2"); shift 2 ;;
        --node=*)
            NODE_ARGS+=("--node" "${1#*=}"); shift ;;
        *)
            REST+=("$1"); shift ;;
    esac
done
set -- "${REST[@]}"

case "${1:-}" in
    -h|--help) usage; exit 0 ;;
esac

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
        "$b" "${PROFILE_ARGS[@]}" "${NODE_ARGS[@]}"
    done
    exit 0
fi

# Wall-time budget in seconds. Two forms:
#   --maxtime <minutes> / --maxtime=<minutes>  → minutes, converted to seconds
#   <seconds>                                  → legacy positional, seconds
BUDGET_S=0
case "${1:-}" in
    --maxtime=*)
        MAXTIME_MIN="${1#*=}"
        ;;
    --maxtime)
        MAXTIME_MIN="${2:-}"
        if [ -z "$MAXTIME_MIN" ]; then
            echo "--maxtime requires a value in minutes (e.g. --maxtime=120)" >&2
            exit 2
        fi
        ;;
    *)
        BUDGET_S="${1:-0}"
        ;;
esac
if [ -n "${MAXTIME_MIN:-}" ]; then
    if ! [[ "$MAXTIME_MIN" =~ ^[0-9]+$ ]]; then
        echo "--maxtime must be a non-negative integer number of minutes; got: '$MAXTIME_MIN'" >&2
        exit 2
    fi
    BUDGET_S=$((MAXTIME_MIN * 60))
fi

run_loop() {
    for b in "${BENCHMARKS[@]}"; do
        if [ -n "$COMMENT" ]; then
            "$b" --comment="$COMMENT" "${NODE_ARGS[@]}"
        else
            "$b" "${NODE_ARGS[@]}"
        fi
    done
    python3 chart_generator.py "${NODE_ARGS[@]}"
    # Distil the per-run history into the tracked series.json + platform.json
    # per node. Cheap, and keeps the git-tracked performance timeline from
    # going stale behind the untracked per-run JSONs.
    python3 history_series.py
    # Keep only the newest candidate chart per folder. They accumulate at
    # ~10 MB/run and are reconstructible from the candidate/history JSONs that
    # stay on disk; without this results/ reached 700 MB, 652 MB of it obsolete
    # SVG. Never touches champion*.svg or any .json.
    python3 prune_charts.py --apply
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
