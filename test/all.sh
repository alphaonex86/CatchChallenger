#!/bin/bash
# Run all testing*.py scripts sequentially.
# Remote compilations run in parallel with local builds inside each script.
# Exit on first script failure unless --continue is passed.

set -e

cd "$(dirname "$0")"

CONTINUE=0
if [ "$1" = "--continue" ]; then
    CONTINUE=1
    set +e
fi

GREEN='\033[92m'
RED='\033[91m'
CYAN='\033[96m'
RESET='\033[0m'

FAILED=0

run_test() {
    echo -e "\n${CYAN}========================================${RESET}"
    echo -e "${CYAN}  Running: $1${RESET}"
    echo -e "${CYAN}========================================${RESET}\n"
    if python3 "$1"; then
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
run_test testingclient.py
run_test testingserver.py
run_test testinghttp.py
run_test testingmulti.py

if [ "$FAILED" = "1" ]; then
    echo -e "\n${RED}Some tests FAILED.${RESET}"
    exit 1
else
    echo -e "\n${GREEN}All tests PASSED.${RESET}"
fi
