#!/bin/bash
# Run all testing*.py scripts sequentially.
# Remote compilations run in parallel with local builds inside each script.
# Exit on first script failure unless --continue is passed.
#
# Diagnostic-tool runs:
#   --sanitize asan|lsan|msan      forwarded to each testing*.py
#   --valgrind memcheck|helgrind|drd  forwarded to each testing*.py
#                                  (per-test timeouts are scaled 10x there)
# --sanitize and --valgrind are mutually exclusive.
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

set -e

cd "$(dirname "$0")"

# Resolve the operator's machine-local paths from
# ~/.config/catchchallenger-testing/config.json (test_config.py is the
# single source of truth — same module the python scripts use). Done
# once at startup; the python helper also writes a dummy config when
# the file is missing so a fresh checkout has something to read.
TEST_CONFIG_OUT="$(python3 -c "import test_config as c; print(c.TMPFS_ROOT); print(c.TMPFS_BUILD_ROOT); print(c.LOCAL_CACHE_ROOT); print(c.CCACHE_ROOT); print(c.PYCACHE_DIR)")"
TMPFS_ROOT="$(echo "$TEST_CONFIG_OUT"        | sed -n '1p')"
TMPFS_BUILD_ROOT="$(echo "$TEST_CONFIG_OUT"  | sed -n '2p')"
LOCAL_CACHE_ROOT="$(echo "$TEST_CONFIG_OUT"  | sed -n '3p')"
CCACHE_ROOT="$(echo "$TEST_CONFIG_OUT"       | sed -n '4p')"
PYCACHE_DIR="$(echo "$TEST_CONFIG_OUT"       | sed -n '5p')"

# to start with fresh folder to be sure test all case.
# EXCEPT: cc-datapack/ (datapack stage cache, see stage_datapacks.py)
# and ccache/ (compiler cache); both are persistent benefits across
# runs and rebuilding them every time would defeat the caching.
find "$TMPFS_ROOT/" -mindepth 1 \
     -not -path "$LOCAL_CACHE_ROOT*" \
     -not -path "$CCACHE_ROOT*" \
     -type f -delete 2>/dev/null
find "$TMPFS_ROOT/" -mindepth 1 -maxdepth 1 \
     -not -name "$(basename "$LOCAL_CACHE_ROOT")" \
     -not -name "$(basename "$CCACHE_ROOT")" \
     -exec rm -rf {} + 2>/dev/null

# Keep the source tree clean: route .pyc bytecode the testing*.py scripts
# (and their imports) would normally drop into ./__pycache__/ to tmpfs.
# Mirrors what build_paths.py does for cmake / qmake artefacts.
# When running an individual testing*.py outside this wrapper, export both
# vars yourself or pass `python3 -B` to opt out of .pyc writes entirely.
export PYTHONPYCACHEPREFIX="$PYCACHE_DIR"
mkdir -p "$PYTHONPYCACHEPREFIX"

# ── ccache configuration (local builds only) ────────────────────────────────
# CMakeLists.txt auto-picks ccache as the compiler launcher when ccache is
# installed; here we only choose where the cache lives. Live on tmpfs by
# explicit operator choice — the speed of RAM-backed cache lookups outweighs
# losing the cache on reboot, since a single full warm run repopulates it.
# Fall back to ccache's default ($HOME/.ccache) when the configured tmpfs
# mount isn't present, e.g. when running all.sh inside a container or on
# a fresh box.
#
# CCACHE_DIR is intentionally NOT propagated to remote build nodes —
# remote_build.py runs cmake over SSH and the SSH session inherits no
# parent env, so each remote node uses its own $HOME/.ccache. That's the
# right call: the operator's tmpfs root doesn't exist on remote nodes,
# and making the local cache addressable remotely would defeat ccache's
# per-host hashing of compiler ABI / system headers.
if [ -d "$TMPFS_ROOT" ]; then
    export CCACHE_DIR="$CCACHE_ROOT"
    mkdir -p "$CCACHE_DIR"
fi

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
# rate to ~40%. tmpfs at /mnt/data/perso/tmpfs is 63G with ~35G free, so
# 25G gives the cache enough room for the whole matrix without crowding
# the build dirs themselves. Persisted to ccache.conf inside CCACHE_DIR
# so it only takes effect once per cache slot.
if command -v ccache >/dev/null 2>&1; then
    ccache --max-size=25G >/dev/null 2>&1 || true
fi

CONTINUE=0
DIAG_ARGS=()
NODE_LABELS=()

while [ $# -gt 0 ]; do
    case "$1" in
        --continue)
            CONTINUE=1
            set +e
            shift
            ;;
        --sanitize|--valgrind)
            if [ -z "$2" ]; then
                echo "$1 requires a value (asan|lsan|msan or memcheck|helgrind|drd)" >&2
                exit 2
            fi
            DIAG_ARGS+=("$1" "$2")
            shift 2
            ;;
        --node)
            if [ -z "$2" ]; then
                echo "$1 requires a node label (e.g. mips-lxc, x86-lxc, pentium-m, atom-n455)" >&2
                exit 2
            fi
            NODE_LABELS+=("$2")
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

if [ ${#NODE_LABELS[@]} -gt 0 ]; then
    # join with commas; consumed by remote_build.py at module load time.
    JOINED="$(IFS=,; echo "${NODE_LABELS[*]}")"
    export CC_NODE_FILTER="$JOINED"
fi

# argparse-side check still runs in each python script; this is just a
# friendly preflight for the orchestrator.
SAW_S=0
SAW_V=0
for a in "${DIAG_ARGS[@]}"; do
    [ "$a" = "--sanitize" ] && SAW_S=1
    [ "$a" = "--valgrind" ] && SAW_V=1
done
if [ "$SAW_S" = "1" ] && [ "$SAW_V" = "1" ]; then
    echo "error: --sanitize and --valgrind are mutually exclusive" >&2
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

FAILED=0

run_test() {
    echo -e "\n${CYAN}========================================${RESET}"
    if [ -n "${CC_NODE_FILTER}" ]; then
        echo -e "${CYAN}  Running: $1 ${DIAG_ARGS[*]} (node filter: ${CC_NODE_FILTER})${RESET}"
    else
        echo -e "${CYAN}  Running: $1 ${DIAG_ARGS[*]}${RESET}"
    fi
    echo -e "${CYAN}========================================${RESET}\n"
    if python3 "$1" "${DIAG_ARGS[@]}"; then
        echo -e "\n${GREEN}[OK] $1${RESET}\n"
    else
        echo -e "\n${RED}[FAILED] $1${RESET}\n"
        FAILED=1
        if [ "$CONTINUE" = "0" ]; then
            echo -e "${RED}Stopping. Use --continue to run all scripts regardless.${RESET}"
            exit 1
        fi
    fi
}

run_test testingcmake.py
run_test testingtools.py
run_test testingmap2png.py
run_test testingmap4client.py
run_test testingqtserver.py
run_test testingclient.py
run_test testingbots.py
run_test testingserver.py
run_test testinghttp.py
run_test testingmulti.py
run_test testingcompilationwindows.py
# testingcompilationmac.py disabled: host VM /Users/user/Desktop/... has
# cmake missing from the non-login ssh PATH, so every cmake-configure step
# returns rc=127. Re-enable once the VM environment is fixed.
#run_test testingcompilationmac.py
run_test testingcompilationandroid.py

if [ "$FAILED" = "1" ]; then
    echo -e "\n${RED}Some tests FAILED.${RESET}"
    exit 1
else
    echo -e "\n${GREEN}All tests PASSED.${RESET}"
fi
