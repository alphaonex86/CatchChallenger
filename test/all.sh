#!/bin/bash
# Run all testing*.py scripts sequentially. ALWAYS walks the full list
# even when a script fails — the exit code reflects whether any test
# failed, but the run still produces a complete result table.
# Remote compilations run in parallel with local builds inside each
# script.
#
# Resume mode:
#   --onlyfailed                   re-run ONLY testing*.py scripts that
#                                  had a non-empty entry in failed.json
#                                  from the previous run. Each
#                                  testing*.py also reads failed.json
#                                  internally and re-runs only its
#                                  previously-failed cases. Skips the
#                                  failed.json wipe and the time.json /
#                                  monitor.json truncation so the
#                                  cumulative log keeps growing.
#
# Diagnostic-tool runs:
#   --sanitize asan|lsan|msan      forwarded to each testing*.py
#   --valgrind memcheck|helgrind|drd  forwarded to each testing*.py
#                                  (per-test timeouts are scaled 10x there)
#   --profile [callgrind|massif|perf]  run every test under a profiler
#                                  (default callgrind). Each launched
#                                  binary is wrapped; callgrind.out.* /
#                                  massif.out.* / perf.data.* land under
#                                  /mnt/data/perso/tmpfs/profile/ and are
#                                  preserved across runs. Local only;
#                                  per-test timeouts scaled (callgrind 30x,
#                                  massif 20x, perf 2x).
# --sanitize, --valgrind and --profile are mutually exclusive.
#
# Remote-node filter:
#   --node <label>                 only run remote tests on the named node;
#                                  may be passed multiple times to allow
#                                  several labels.  Bypasses the per-node
#                                  `enabled` flag in remote_nodes.json so
#                                  you can target a disabled node for
#                                  debugging without editing the file.
#                                  Implemented as the CC_NODE_FILTER env
#                                  var, exported below.

# Walk the full test list regardless of individual failures — `set -e`
# would otherwise abort at the first FAIL and hide the rest of the
# matrix from the operator. Each `run_test` invocation still records
# a non-zero rc per-script; FAILED=1 is set and propagated at the end.
set +e

usage() {
    cat <<'EOF'
Usage: all.sh [options]

Run every testing*.py sequentially (the full matrix), walking the whole
list even when a script fails. Exit code is non-zero if any test failed.

Options:
  -h, --help            Show this help and exit (no tmpfs cleanup is done).

  --onlyfailed          Re-run ONLY the testing*.py scripts (and, inside
                        each, only the cases) that failed in the previous
                        run, per failed.json. Preserves failed.json and
                        appends to time.json / monitor.json.

  --sanitize <asan|lsan|msan>        Forward a sanitizer to each testing*.py.
  --valgrind <memcheck|helgrind|drd> Forward valgrind (per-test timeouts x10).
  --profile [callgrind|massif|perf]  Run every test under a profiler
                                     (default callgrind); artefacts under
                                     /mnt/data/perso/tmpfs/profile/.
                        --sanitize / --valgrind / --profile are mutually
                        exclusive.

  --node <label>        Only run remote tests on the named node; repeatable.
                        Bypasses the per-node `enabled` flag in
                        remote_nodes.json (CC_NODE_FILTER).
EOF
}

# Honour --help / -h BEFORE the destructive tmpfs cleanup below, so asking
# for usage never wipes the previous run's artefacts.
for _a in "$@"; do
    case "$_a" in
        -h|--help) usage; exit 0 ;;
    esac
done

# Pre-run cleanup: drop the previous run's transient build trees + cluster
# state to keep the tmpfs from saturating, BUT preserve the JSON
# bookkeeping (all.log, failed.json[.lock], time.json, monitor.json,
# testing-individual-time.json) and the previous run's shipping
# artifacts (catchchallenger-*.exe / .msi / .dmg / .apk / .aab) so the
# tmpfs root always shows the operator at least the last run's outcome.
# (Neither ccache NOR the datapack cache live here any more — both are on
# persistent disk, see CCACHE_ROOT / LOCAL_CACHE_ROOT; a stale tmpfs
# cc-datapack from the old layout is now intentionally swept.) The
# per-script atexit hook in
# test/cleanup_helpers.py also tears down on successful exit, but doing
# the sweep here too lets the operator skip a stale post-mortem run
# without re-running the whole matrix.
find /mnt/data/perso/tmpfs/ -mindepth 1 -maxdepth 1 \
     ! -name 'all.log' \
     ! -name 'failed.json' \
     ! -name 'failed.json.lock' \
     ! -name 'time.json' \
     ! -name 'monitor.json' \
     ! -name 'testing-individual-time.json' \
     ! -name 'testing-time-donut.svg' \
     ! -name 'catchchallenger-*.exe' \
     ! -name 'catchchallenger-*.msi' \
     ! -name 'catchchallenger-*.dmg' \
     ! -name 'catchchallenger-*.apk' \
     ! -name 'catchchallenger-*.aab' \
     -exec rm -rf {} + 2>/dev/null || true

cd "$(dirname "$0")"

# Resolve the operator's machine-local paths from
# ~/.config/catchchallenger-testing/config.json (test_config.py is the
# single source of truth — same module the python scripts use). Done
# once at startup; the python helper also writes a dummy config when
# the file is missing so a fresh checkout has something to read.
TEST_CONFIG_OUT="$(python3 -c "import test_config as c; print(c.TMPFS_ROOT); print(c.TMPFS_BUILD_ROOT); print(c.LOCAL_CACHE_ROOT); print(c.CCACHE_ROOT); print(c.PYCACHE_DIR); print(c.FAILED_JSON); print(c.TIME_JSON); print(c.MONITOR_JSON)")"
TMPFS_ROOT="$(echo "$TEST_CONFIG_OUT"        | sed -n '1p')"
TMPFS_BUILD_ROOT="$(echo "$TEST_CONFIG_OUT"  | sed -n '2p')"
LOCAL_CACHE_ROOT="$(echo "$TEST_CONFIG_OUT"  | sed -n '3p')"
CCACHE_ROOT="$(echo "$TEST_CONFIG_OUT"       | sed -n '4p')"
PYCACHE_DIR="$(echo "$TEST_CONFIG_OUT"       | sed -n '5p')"
FAILED_JSON="$(echo "$TEST_CONFIG_OUT"       | sed -n '6p')"
TIME_JSON="$(echo "$TEST_CONFIG_OUT"         | sed -n '7p')"
MONITOR_JSON="$(echo "$TEST_CONFIG_OUT"      | sed -n '8p')"

# Parse args early.
DIAG_ARGS=()
NODE_LABELS=()
ONLY_FAILED=0
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    a="${ARGS[$i]}"
    case "$a" in
        -h|--help)
            usage; exit 0
            ;;
        --onlyfailed)
            # Re-run only the testing*.py scripts that had a non-empty
            # failed.json entry from the previous run. Each testing*.py
            # also reads failed.json internally and only re-runs its
            # previously-failed cases (driven by failed_cases.py). Lets
            # the operator iterate on a single failing test without
            # paying the full matrix's runtime.
            ONLY_FAILED=1
            i=$((i+1))
            ;;
        --sanitize|--valgrind)
            n="${ARGS[$((i+1))]}"
            if [ -z "$n" ]; then
                echo "$a requires a value (asan|lsan|msan or memcheck|helgrind|drd)" >&2
                exit 2
            fi
            DIAG_ARGS+=("$a" "$n")
            i=$((i+2))
            ;;
        --profile)
            # Optional value (callgrind|massif|perf); defaults to callgrind.
            # Drops profile artefacts under /mnt/data/perso/tmpfs/profile/.
            n="${ARGS[$((i+1))]}"
            case "$n" in
                callgrind|massif|perf)
                    DIAG_ARGS+=("--profile" "$n"); i=$((i+2)) ;;
                *)
                    DIAG_ARGS+=("--profile" "callgrind"); i=$((i+1)) ;;
            esac
            ;;
        --node)
            n="${ARGS[$((i+1))]}"
            if [ -z "$n" ]; then
                echo "$a requires a node label (e.g. mips-lxc, x86-lxc, pentium-m, atom-n455)" >&2
                exit 2
            fi
            NODE_LABELS+=("$n")
            i=$((i+2))
            ;;
        *)
            echo "unknown argument: $a" >&2
            exit 2
            ;;
    esac
done

if [ "$ONLY_FAILED" = "1" ] && [ ! -f "$FAILED_JSON" ]; then
    echo "[all.sh] --onlyfailed: $FAILED_JSON missing — nothing to re-run; exit 0"
    exit 0
fi

# Pre-run cleanup: drop transient build artefacts but keep cc-datapack/
# (expensive to rebuild, nothing run-specific in it). ccache now lives on
# persistent disk (CCACHE_ROOT), outside this tmpfs, so it survives anyway.
# time.json + monitor.json are preserved across the wipe; truncated
# explicitly below unless --onlyfailed (where we append).
# Under --onlyfailed we also preserve failed.json — testing*.py reads
# it to know which cases to re-run.
EXTRA_KEEP=()
if [ "$ONLY_FAILED" = "1" ]; then
    EXTRA_KEEP+=(-not -path "$FAILED_JSON")
fi
find "$TMPFS_ROOT/" -mindepth 1 \
     -not -path "$LOCAL_CACHE_ROOT*" \
     -not -path "$CCACHE_ROOT*" \
     -not -path "$TIME_JSON" \
     -not -path "$MONITOR_JSON" \
     -not -path "$TMPFS_ROOT/profile*" \
     "${EXTRA_KEEP[@]}" \
     -type f -delete 2>/dev/null
EXTRA_KEEP_NAME=()
if [ "$ONLY_FAILED" = "1" ]; then
    EXTRA_KEEP_NAME+=(-not -name "$(basename "$FAILED_JSON")")
fi
# Keep profile/ across runs too: --profile artefacts (callgrind.out.* /
# massif.out.* / perf.data.*) accumulate there with unique names, so the
# operator can still inspect a prior profile run after a later plain run.
find "$TMPFS_ROOT/" -mindepth 1 -maxdepth 1 \
     -not -name "$(basename "$LOCAL_CACHE_ROOT")" \
     -not -name "$(basename "$CCACHE_ROOT")" \
     -not -name "$(basename "$TIME_JSON")" \
     -not -name "$(basename "$MONITOR_JSON")" \
     -not -name "profile" \
     "${EXTRA_KEEP_NAME[@]}" \
     -exec rm -rf {} + 2>/dev/null

if [ "$ONLY_FAILED" = "0" ]; then
    # Full run: wipe failed.json so every testing*.py executes its
    # full case matrix from scratch — no per-script resume.
    rm -f "$FAILED_JSON"
    : > "$TIME_JSON"
    : > "$MONITOR_JSON"
    echo "[all.sh] full run from scratch: wiped $FAILED_JSON, truncated $TIME_JSON $MONITOR_JSON; no test will be skipped"
else
    echo "[all.sh] --onlyfailed: preserving $FAILED_JSON, appending to $TIME_JSON $MONITOR_JSON; only previously-failed testing*.py scripts (and their previously-failed cases) will run"
fi

# Pre-seed empty placeholder JSON files so the operator always sees
# *something* at the tmpfs root, even when a testing*.py segfaults
# before reaching its summary() / save_failed_cases() call. testing*.py
# writes overwrite these placeholders as soon as a real result lands.
mkdir -p "$(dirname "$FAILED_JSON")"
[ -f "$FAILED_JSON" ] || echo '{}' > "$FAILED_JSON"
TESTING_TIMING_JSON="$TMPFS_ROOT/testing-individual-time.json"
[ -f "$TESTING_TIMING_JSON" ] || echo '{}' > "$TESTING_TIMING_JSON"

# Mirror everything we print (and every child process's stdout/stderr)
# to /mnt/data/perso/tmpfs/all.log so the full run is reproducible from
# disk after-the-fact — useful when the terminal scroll-back has been
# eaten by a 2 h run or the operator was AFK when something failed.
# MUST come AFTER the tmpfs wipe above, otherwise the find -delete
# would erase the freshly-opened all.log mid-run. The tee runs in a
# process-substitution so the script's own stdout/stderr still go to
# the terminal in real time.
ALL_LOG=/mnt/data/perso/tmpfs/all.log
mkdir -p "$(dirname "$ALL_LOG")"
: > "$ALL_LOG"
exec > >(tee -a "$ALL_LOG") 2>&1
echo "[all.sh] capturing console output to $ALL_LOG"

# Keep the source tree clean: route .pyc bytecode the testing*.py scripts
# (and their imports) would normally drop into ./__pycache__/ to tmpfs.
# Mirrors what build_paths.py does for cmake / qmake artefacts.
# When running an individual testing*.py outside this wrapper, export both
# vars yourself or pass `python3 -B` to opt out of .pyc writes entirely.
export PYTHONPYCACHEPREFIX="$PYCACHE_DIR"
mkdir -p "$PYTHONPYCACHEPREFIX"

# ── ccache configuration (local builds only) ────────────────────────────────
# CMakeLists.txt auto-picks ccache as the compiler launcher when ccache is
# installed; here we only choose where the cache lives. CCACHE_ROOT lives on
# PERSISTENT disk (machine-local path from config.json, see test_config.py),
# NOT under tmpfs: a multi-gigabyte cache on a RAM-backed mount wastes RAM
# and is lost on every reboot, while the cache is cheap to keep around and
# expensive to repopulate. We just mkdir -p it; if its parent dir doesn't
# exist (fresh box / container) the mkdir fails silently and ccache falls
# back to its own default ($HOME/.ccache).
#
# CCACHE_DIR is intentionally NOT propagated to remote build nodes —
# remote_build.py runs cmake over SSH and the SSH session inherits no
# parent env, so each remote node uses its own $HOME/.ccache. That's the
# right call: the operator's cache dir doesn't exist on remote nodes,
# and making the local cache addressable remotely would defeat ccache's
# per-host hashing of compiler ABI / system headers.
export CCACHE_DIR="$CCACHE_ROOT"
mkdir -p "$CCACHE_DIR" 2>/dev/null || true

# ccache cross-build-dir hit rate fixes (mirror what remote_build.py
# already exports for SSH compiles). Without these, every per-config
# build_dir under <tmpfs_build_root>/.../testing-{cpu,gl}-c++NN-{gcc,clang}-{default,hardened}/
# generates a fresh hash and the warm cache misses constantly:
#
#   CCACHE_BASEDIR=<repo root> — rewrites absolute paths inside the
#   repo to repo-relative before hashing, so the same source file
#   compiled from multiple build dirs hashes identically.
#
#   CCACHE_NOHASHDIR=1 — drop the compile cwd / build-dir suffix from
#   the hash key. Without it cmake's per-target subdirectory leaks in.
#
#   CCACHE_SLOPPINESS=… — tolerate __TIME__/__DATE__ macros and trust
#   file content over inode-stat changes (rsync's `-a` recreates
#   inodes even when content is identical, which under default
#   sloppiness invalidates every entry).
#
# Applied locally only; remote nodes get the same env via their own
# ssh-prefix in remote_build._build_pro_remote.
export CCACHE_BASEDIR="$(realpath "$(dirname "$0")/..")"
export CCACHE_NOHASHDIR=1
export CCACHE_SLOPPINESS="time_macros,include_file_mtime,include_file_ctime,file_stat_matches"
# 5G turned out to be too small: with c++11+c++23 × gcc+clang × multiple
# flag-set variants per target plus the remote build dirs, the LOCAL cache
# was hitting 100% utilisation after the first full sweep and ccache was
# evicting entries (`Cleanups: 7947` after one full run), tanking the hit
# rate to ~40%. CCACHE_ROOT now lives on persistent disk (not tmpfs), so
# 25G gives the cache room for the whole matrix without any RAM pressure.
# Persisted to ccache.conf inside CCACHE_DIR so it only takes effect once
# per cache slot.
if command -v ccache >/dev/null 2>&1; then
    ccache --max-size=25G >/dev/null 2>&1 || true
fi

if [ ${#NODE_LABELS[@]} -gt 0 ]; then
    # join with commas; consumed by remote_build.py at module load time.
    JOINED="$(IFS=,; echo "${NODE_LABELS[*]}")"
    export CC_NODE_FILTER="$JOINED"
fi

# argparse-side check still runs in each python script; this is just a
# friendly preflight for the orchestrator.
SAW_S=0
SAW_V=0
SAW_P=0
for a in "${DIAG_ARGS[@]}"; do
    [ "$a" = "--sanitize" ] && SAW_S=1
    [ "$a" = "--valgrind" ] && SAW_V=1
    [ "$a" = "--profile" ] && SAW_P=1
done
if [ $((SAW_S + SAW_V + SAW_P)) -gt 1 ]; then
    echo "error: --sanitize / --valgrind / --profile are mutually exclusive" >&2
    exit 2
fi

GREEN='\033[92m'
RED='\033[91m'
CYAN='\033[96m'
RESET='\033[0m'

# ── stage every datapack to the local tmpfs cache + each enabled
# execution_node's persistent cache, in parallel. See stage_datapacks.py
# / datapack_stage.py / CLAUDE.md "Datapack staging cache". Aborts the
# whole run on any rsync failure — running tests against a stale or
# half-staged cache would just produce confusing FAILs downstream.
echo -e "\n${CYAN}========================================${RESET}"
echo -e "${CYAN}  Pre-stage datapacks${RESET}"
echo -e "${CYAN}========================================${RESET}\n"
if ! python3 stage_datapacks.py; then
    echo -e "${RED}[ABORT] datapack staging failed; not running tests${RESET}"
    exit 1
fi

# Datapack-unchanged invariant. The datapack is staged ONCE above and is
# READ-ONLY for the rest of the run — every test only symlinks the cache,
# never rewrites it. Snapshot a cheap signature (mtime of the cache parent
# folder + each slot dir): a full delete/re-upload bumps the PARENT dir
# mtime, an in-place edit bumps a SLOT dir mtime. run_test re-checks after
# every script and reports if anything mutated the cache. (stage_datapacks
# already aborted the run if the cache resolved onto tmpfs.)
_datapack_sig() {
    stat -c '%n %Y' "$LOCAL_CACHE_ROOT" "$LOCAL_CACHE_ROOT"/*/ 2>/dev/null | sort
}
DATAPACK_SIG_BASELINE="$(_datapack_sig)"
echo -e "${CYAN}  datapack cache: $LOCAL_CACHE_ROOT (read-only baseline captured)${RESET}"

FAILED=0

# ── start the system+process sampler in the background ──────────────────
# monitor.py samples /proc + cc-related pids every 60s into
# <tmpfs>/monitor.json. Pair with time.json post-mortem to find idle
# host stretches that warrant adding parallelism (CPU < 30% during a
# slow phase) vs. single-core hot loops (one core pinned). EXIT trap
# kills the sampler whether the run succeeds, fails, or is Ctrl-C'd
# so we don't leak python3 monitor.py processes across runs.
python3 monitor.py --out "$MONITOR_JSON" --interval 60 &
MONITOR_PID=$!
_cleanup_monitor() {
    [ -n "$MONITOR_PID" ] && kill "$MONITOR_PID" 2>/dev/null
    wait "$MONITOR_PID" 2>/dev/null
    true
}
# EXIT cleans up on any termination. INT/TERM must ALSO abort the whole run:
# a bare signal trap returns control to the script, so Ctrl-C would only kill
# the current `timeout python3 ...` (foreground child) and then bash would fall
# through to the NEXT run_test — exactly the "Ctrl-C ignored, keeps going"
# behaviour. Re-exit 130 (128+SIGINT) so the run stops; `trap - EXIT` avoids a
# redundant second cleanup from the EXIT trap.
trap _cleanup_monitor EXIT
trap '_cleanup_monitor; trap - EXIT; echo -e "\n${RED}[all.sh] interrupted by signal — aborting run${RESET}"; exit 130' INT TERM
echo "[all.sh] monitor.py started pid=$MONITOR_PID writing $MONITOR_JSON"

## Per-script wall clock cap. Each testing*.py gets its own ceiling,
## sized roughly to "twice the longest healthy observed run" so a
## genuine bug surfaces as a [TIMEOUT] instead of soaking the build.
## `timeout` enforces it from the outside: on expiry, SIGTERM after
## the configured limit then SIGKILL 30s later. exit code 124
## distinguishes "killed by timeout" from regular non-zero exits so
## we can flag it specifically (and skip the per-test timing log
## entry — a timed-out run has no meaningful "duration").
PER_TEST_KILL_AFTER=30s

# Per-script timeout table (script → "Xm" duration). Anything not
# listed gets the DEFAULT_PER_TEST_TIMEOUT below. Keep this in sync
# with the operator-supplied table; bumping a number should be a
# deliberate one-line change, not a sneaky drift.
declare -A PER_TEST_TIMEOUT_MAP=(
    [testingbots.py]=15m
    [testingbroadcast.py]=15m
    [testingbyIA.py]=30m
    [testingclient.py]=40m
    [testingcluster.py]=10m
    [testingclustersecurity.py]=90m
    [testingcmake.py]=30m
    [testingcompilationandroid.py]=25m
    [testingcompilationESP32.py]=45m
    [testingcompilationmac.py]=15m
    [testingcompilationmsdos.py]=20m
    [testingcompilationwindows.py]=15m
    [testingfight.py]=15m
    [testinggateway.py]=15m
    [testinghttp.py]=15m
    [testingmap2png.py]=15m
    [testingmap4client.py]=30m
    [testingmapmanagement.py]=10m
    [testingpathfinding.py]=10m
    [testingmulti.py]=30m
    [testingprotocolstate.py]=90m
    [testingqtserver.py]=15m
    [testingremote.py]=45m
    [testingserver.py]=30m
    [testingstats.py]=10m
    [testingtools.py]=15m
    [testingwebsocket.py]=30m
)
DEFAULT_PER_TEST_TIMEOUT=30m

# Per-script wall-time log. testing*.py that complete (PASS or FAIL
# with rc!=124/137) get an entry. Timed-out runs are deliberately
# skipped — their "duration" would just be the cap, not a measure of
# the test's natural runtime, and feeding that back as historical
# data would slowly inflate every cap that touched a flaky script.
TIMING_LOG=/mnt/data/perso/tmpfs/testing-individual-time.json
# Truncate at the start of every fresh run so the file holds exactly
# the timings for THIS invocation — one all.sh = one timing log.
rm -f "$TIMING_LOG"

_log_test_timing() {
    # $1=script, $2=duration_s (int seconds), $3=rc
    python3 - "$TIMING_LOG" "$1" "$2" "$3" <<'PY'
import json, os, sys
path, script, dur_s, rc = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
entries = []
if os.path.exists(path):
    try:
        with open(path) as f:
            entries = json.load(f)
    except Exception:
        entries = []
entries.append({"script": script, "duration_s": dur_s, "rc": rc})
tmp = path + ".tmp"
with open(tmp, "w") as f:
    json.dump(entries, f, indent=2)
os.replace(tmp, path)
PY
}

run_test() {
    local script="$1"
    local cap="${PER_TEST_TIMEOUT_MAP[$script]:-$DEFAULT_PER_TEST_TIMEOUT}"
    # --onlyfailed gate: skip scripts whose failed.json entry is
    # absent or empty (== last run passed every case). Python parse
    # because bash + JSON is fragile.
    if [ "$ONLY_FAILED" = "1" ]; then
        local has_failed
        has_failed=$(python3 -c "
import json, sys
try:
    with open('$FAILED_JSON') as f: j = json.load(f)
except Exception: print('0'); sys.exit(0)
e = j.get('$script')
print('1' if isinstance(e, dict) and len(e) > 0 else '0')
")
        if [ "$has_failed" != "1" ]; then
            echo -e "${CYAN}[all.sh --onlyfailed] skipping $script (no failed case in $FAILED_JSON)${RESET}"
            return
        fi
    fi
    echo -e "\n${CYAN}========================================${RESET}"
    if [ -n "${CC_NODE_FILTER}" ]; then
        echo -e "${CYAN}  Running: $script ${DIAG_ARGS[*]} (node filter: ${CC_NODE_FILTER}, cap ${cap})${RESET}"
    else
        echo -e "${CYAN}  Running: $script ${DIAG_ARGS[*]} (cap ${cap})${RESET}"
    fi
    echo -e "${CYAN}========================================${RESET}\n"
    local t0=$(date +%s)
    timeout --kill-after="${PER_TEST_KILL_AFTER}" "${cap}" \
        python3 "$script" "${DIAG_ARGS[@]}"
    rc=$?
    local elapsed=$(( $(date +%s) - t0 ))
    # Datapack must be unchanged between tests — no in-place edit, no full
    # delete/re-upload. Compare the cache signature against the post-stage
    # baseline; report + fail if a test mutated the read-only datapack.
    local cur_sig
    cur_sig="$(_datapack_sig)"
    if [ "$cur_sig" != "$DATAPACK_SIG_BASELINE" ]; then
        echo -e "\n${RED}[DATAPACK-CHANGED] $script mutated the staged datapack cache ($LOCAL_CACHE_ROOT) — it must stay read-only/unchanged between tests (no delete/upload, no in-place edit):${RESET}"
        diff <(printf '%s\n' "$DATAPACK_SIG_BASELINE") <(printf '%s\n' "$cur_sig") | head -20
        FAILED=1
        # re-baseline so the same drift isn't re-reported for every later script
        DATAPACK_SIG_BASELINE="$cur_sig"
    fi
    if [ "$rc" = "0" ]; then
        echo -e "\n${GREEN}[OK] $script  (${elapsed}s)${RESET}\n"
        _log_test_timing "$script" "$elapsed" "$rc"
    elif [ "$rc" = "124" ] || [ "$rc" = "137" ]; then
        # Don't log a timing entry — see TIMING_LOG comment above.
        echo -e "\n${RED}[TIMEOUT] $script — exceeded ${cap}${RESET}\n"
        FAILED=1
    else
        echo -e "\n${RED}[FAILED] $script (rc=${rc}, ${elapsed}s)${RESET}\n"
        _log_test_timing "$script" "$elapsed" "$rc"
        FAILED=1
    fi
}

run_test testingcmake.py
run_test testingtools.py
run_test testingmap2png.py
run_test testingmap4client.py
run_test testingqtserver.py
run_test testingfight.py
run_test testingmapmanagement.py
run_test testingpathfinding.py
run_test testingprotocolstate.py
run_test testingclient.py
run_test testingbots.py
run_test testingserver.py
run_test testingbroadcast.py
run_test testinghttp.py
run_test testingwebsocket.py
run_test testingstats.py
run_test testinggateway.py
run_test testingmulti.py
run_test testingbyIA.py
run_test testingcluster.py
run_test testingclustersecurity.py
run_test testingremote.py
run_test testingcompilationwindows.py
run_test testingcompilationmac.py
run_test testingcompilationandroid.py
run_test testingcompilationmsdos.py
run_test testingcompilationESP32.py

# Publish freshly-built installers to the web VPS files dir and bump
# updater.txt. publish_binaries.sh aborts (and leaves updater.txt
# untouched) if any artifact is missing or <10 MiB, so we never
# advertise a version whose downloads would 404. Skipped under
# --onlyfailed since the testing*.py scripts that produce the
# installers haven't necessarily run.
# Soak guard: publish AT MOST ONCE per all.sh invocation. In --maxtime
# loop mode the matrix re-runs every pass, but the artifacts are the same
# commit — re-uploading to the production VPS + re-bumping updater.txt on
# every green pass would just spam an outward-facing service.
if [ "$FAILED" = "0" ] && [ "$ONLY_FAILED" = "0" ] && [ "${PUBLISHED:-0}" = "0" ]; then
    PUBLISHED=1
    echo -e "\n${CYAN}========================================${RESET}"
    echo -e "${CYAN}  Publish: windows + mac + android → web VPS${RESET}"
    echo -e "${CYAN}========================================${RESET}\n"
    if ./publish_binaries.sh windows mac android; then
        echo -e "\n${GREEN}[OK] publish_binaries.sh${RESET}\n"
    else
        echo -e "\n${RED}[FAILED] publish_binaries.sh${RESET}\n"
        FAILED=1
    fi
fi

# Donut chart of per-script wall time: produces
# <tmpfs_root>/testing-time-donut.svg from testing-individual-time.json.
# Only meaningful after a full run (--onlyfailed reuses an old timing
# JSON that doesn't reflect the current run, so skip there).
if [ "$ONLY_FAILED" = "0" ] && [ -f "$TESTING_TIMING_JSON" ]; then
    echo -e "\n${CYAN}[all.sh] rendering wall-time donut chart${RESET}"
    python3 testing_time_chart.py || true
fi

if [ "$FAILED" = "1" ]; then
    echo -e "\n${RED}Some tests FAILED.${RESET}"
    # On any failure, leave the tmpfs untouched so the operator can
    # inspect every build dir / log of every failing test. The
    # successful tests have already torn their dirs down via the
    # per-script atexit hook in cleanup_helpers.py.
    exit 1
else
    # All tests passed → final sweep so the tmpfs root holds only:
    #   - shipping artifacts (catchchallenger-*.exe/.msi/.apk/.aab/.dmg)
    #   - bookkeeping JSON (all.log, failed.json[.lock], time.json,
    #     monitor.json, testing-individual-time.json)
    #   - cc-datapack (staged datapack cache, expensive to rebuild)
    # (ccache is kept warm too but lives on persistent disk, not here.)
    # Anything else (transient build trees, datapack staging, cluster
    # state, nginx config) is removed.
    if [ -n "$TMPFS_ROOT" ] && [ -d "$TMPFS_ROOT" ]; then
        echo -e "\n${CYAN}[all.sh] sweep tmpfs root → keep artifacts + JSON + cc-datapack${RESET}"
        find "$TMPFS_ROOT/" -mindepth 1 -maxdepth 1 \
             ! -name 'cc-datapack' \
             ! -name 'all.log' \
             ! -name 'failed.json' \
             ! -name 'failed.json.lock' \
             ! -name 'time.json' \
             ! -name 'monitor.json' \
             ! -name 'testing-individual-time.json' \
             ! -name 'testing-time-donut.svg' \
             ! -name 'catchchallenger*.exe' \
             ! -name 'catchchallenger*.msi' \
             ! -name 'catchchallenger*.dmg' \
             ! -name 'catchchallenger*.apk' \
             ! -name 'catchchallenger*.aab' \
             -exec rm -rf {} + 2>/dev/null
    fi
    echo -e "\n${GREEN}All tests PASSED.${RESET}"
    exit 0
fi
