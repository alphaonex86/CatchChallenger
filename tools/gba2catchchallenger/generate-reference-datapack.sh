#!/bin/sh
# Generate the operator's reference datapack region set from the English Gen3
# ROMs. The dataset is LOCAL only: its location comes from $DP, never from git.
#
# gba2catchchallenger auto-detects each ROM's role from its header (base vs
# sibling-sub vs standalone hack), so this driver only has to run the ROMs in the
# right ORDER: a sub overlay is diffed against the already-generated main, so
# every main must run before its subs. The labels below are CONVERTER labels —
# they name the ROM input and its output folder, not any content of ours.
#
# Override BIN/DP/GBA via the environment.
set -e
BIN="${BIN:-/tmp/gba2cc-build/gba2catchchallenger}"
DP="${DP:?set DP to the reference datapack root (kept out of git)}"
GBA="${GBA:-/mnt/data/read/games/gba/gba/en}"

# Run the first ROM matching a glob (so minor filename variations still resolve).
run() {
    pattern="$1"
    # find -name keeps the (space-containing) glob as a single argument.
    rom=$(find "$GBA" -maxdepth 1 -type f -name "$pattern" 2>/dev/null | sort | head -n1)
    if [ -z "$rom" ]; then
        echo "### SKIP (not found): $pattern"
        return 0
    fi
    echo "### $rom"
    "$BIN" --datapack "$DP" --gba "$rom"
    echo
}

# Mains first, then their subs.
run "Pokemon - FireRed*.gba"
run "Pokemon - Leaf Green*.gba"
run "Pokemon - Ruby*.gba"
run "Pokemon - Sapphire*.gba"
run "Pokemon - Emerald*.gba"
# Standalone hacks (own main, gMapGroups located by scan).
run "Pokemon Glazed*.gba"
run "HnS*.gba"

echo "All done."
