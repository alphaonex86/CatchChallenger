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

run_test testingtools.py
run_test testingmap2png.py
run_test testingclient.py
run_test testingbots.py
run_test testingserver.py
run_test testinghttp.py
run_test testingmulti.py
run_test testingcompilationwindows.py
run_test testingcompilationmac.py
run_test testingcompilationandroid.py

if [ "$FAILED" = "1" ]; then
    echo -e "\n${RED}Some tests FAILED.${RESET}"
    exit 1
else
    echo -e "\n${GREEN}All tests PASSED.${RESET}"
fi
