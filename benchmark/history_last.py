#!/usr/bin/env python3
"""history_last.py -- write one compact last.json per (benchmark, platform, node).

benchmark/history/ holds one JSON per run and stays gitignored: it is already
1300+ files / 25 MB and grows every run, most of it the same platform
description repeated. This distils each
history/<benchmark>/<platform>/<node>/ directory down to a single last.json --
the most recent run, minus the fields that are null, bulky, or run-local noise
-- and THAT file is the one tracked in git.

Why only the latest run: git itself is the time series. Every commit of a
last.json is one datapoint, so

    git log -p benchmark/history/<benchmark>/<platform>/<node>/last.json

replays the performance evolution of that node without carrying a thousand
intermediate run files in the tree. Across nodes, the current last.json set is
the hardware comparison.

Run standalone (`python3 history_last.py`) or let benchmark/all.sh call it at
the end of a suite run.
"""
import json
import os
import sys

HISTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "history")

# Dropped from the compact record:
#   kernel_config_gz -- base64 kernel config, can be megabytes, and the kernel
#                       string already identifies it
#   sensors / noise_services / loadavg_1min_at_start -- run-local environment
#                       noise, meaningless once the run is over
#   batch_id / started_utc -- run bookkeeping; ended_utc + commit locate the run
#   cpu_flags        -- very long and superseded by simd_tier for comparison
# Files this script itself writes into a node dir -- they are NOT runs and must
# never be mistaken for one. "platform.json" sorts AFTER any 2026-... stamp, so
# missing it here silently made the generator read its own output as the newest
# run and emit an empty last.json.
GENERATED = frozenset(("last.json", "platform.json"))

DROP_KEYS = frozenset((
    "kernel_config_gz", "sensors", "noise_services", "loadavg_1min_at_start",
    "batch_id", "started_utc", "cpu_flags",
))


SIG_DIGITS = 4

# Run-specific keys: everything else describes the MACHINE and goes to
# platform.json, which is rewritten only when the machine description actually
# changes. Splitting them keeps a rerun from rewriting a kilobyte of unchanged
# cpu_model / kernel / net_card text on every node, every run.
RUN_KEYS = frozenset((
    "benchmark", "node", "commit", "commit_short", "ended_utc", "decision",
    "comment", "harness_version", "compile_flags", "simd_tier", "results",
))


def round_sig(value, digits=SIG_DIGITS):
    """Round a measurement to `digits` significant figures.

    A raw score like 5646546513248 carries ~9 digits of run-to-run noise that
    no one reads and that guarantees a fresh git blob every run. 4 significant
    figures still resolves ~0.02%, far finer than the noise floor of any of
    these benchmarks, and makes an unchanged machine produce a byte-identical
    file. Booleans are left alone (bool is an int subclass), as are integers
    that are already short enough to be exact."""
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return value
    if value == 0 or value != value or value in (float("inf"), float("-inf")):
        return value
    import math
    exp = math.floor(math.log10(abs(value)))
    quant = digits - 1 - exp
    rounded = round(value, quant)
    if quant <= 0:
        rounded = int(rounded)
    elif float(rounded).is_integer() and isinstance(value, int):
        rounded = int(rounded)
    return rounded


def strip_metrics(node):
    """Recursively drop the raw per-run `samples` arrays from metric blocks.

    A metric block is {value, median, stddev, unit, better, samples}. `samples`
    is the raw measurement list -- it is what makes a botactions record 231 KB
    on its own, and it says nothing about evolution or hardware that median +
    stddev do not already say. Everything else is kept verbatim."""
    if isinstance(node, dict):
        # A metric block is {value, median, stddev, unit, better, samples}.
        # `value` and `median` are the same number in the overwhelming majority
        # of blocks, and a stddev that rounds to 0 says only "no spread" --
        # both are pure repetition in a file that is 72% key names already.
        is_metric = "median" in node or "value" in node
        drop_value = (is_metric and node.get("value") is not None
                      and node.get("value") == node.get("median"))
        out = {}
        for key, value in node.items():
            if key == "samples":
                continue
            if key == "value" and drop_value:
                continue
            if key == "stddev" and is_metric and round_sig(value) in (0, 0.0):
                continue
            if value is None or value == {} or value == []:
                continue
            out[key] = strip_metrics(value)
        return out
    if isinstance(node, list):
        return [strip_metrics(v) for v in node]
    return round_sig(node)


def compact(record):
    """Strip the record to what is needed for performance evolution + hardware
    comparison: every non-empty identity/hardware/build field, plus results
    without their raw sample arrays."""
    out = {}
    for key, value in record.items():
        if key in DROP_KEYS:
            continue
        # null / empty dict / empty list carry nothing; most of the ~60 keys of
        # a run record are null on any given node.
        if value is None or value == {} or value == [] or value == "":
            continue
        out[key] = strip_metrics(value)
    return out


def newest_run(node_dir):
    """Newest run JSON in a node dir. Filenames are ISO-ish
    (2026-07-25T17-48-23Z.json) so lexicographic order IS chronological;
    ended_utc is used when present so a hand-copied file still sorts right."""
    best = None
    best_key = None
    for name in os.listdir(node_dir):
        if not name.endswith(".json") or name in GENERATED:
            continue
        path = os.path.join(node_dir, name)
        try:
            with open(path) as handle:
                record = json.load(handle)
        except (OSError, ValueError):
            continue
        key = record.get("ended_utc") or name
        if best_key is None or key > best_key:
            best_key = key
            best = record
    return best


def node_dirs():
    """history/<benchmark>/<platform>/<node>/ -- exactly 3 levels down. Anything
    else under history/ (e.g. kernel-configs/) is not a node dir and is skipped."""
    found = []
    if not os.path.isdir(HISTORY):
        return found
    for benchmark in sorted(os.listdir(HISTORY)):
        bdir = os.path.join(HISTORY, benchmark)
        if not os.path.isdir(bdir):
            continue
        for platform in sorted(os.listdir(bdir)):
            pdir = os.path.join(bdir, platform)
            if not os.path.isdir(pdir):
                continue
            for node in sorted(os.listdir(pdir)):
                ndir = os.path.join(pdir, node)
                if os.path.isdir(ndir):
                    found.append(ndir)
    return found


def write_if_changed(path, payload):
    """Write JSON only when the bytes differ. sort_keys makes an unchanged run
    byte-identical, so git records nothing. Returns True when written."""
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    try:
        with open(path) as handle:
            if handle.read() == text:
                return False, len(text)
    except (OSError, ValueError):
        pass
    with open(path, "w") as handle:
        handle.write(text)
    return True, len(text)


def main():
    dirs = node_dirs()
    wrote_run = 0
    wrote_platform = 0
    total = 0
    for ndir in dirs:
        record = newest_run(ndir)
        if record is None:
            continue
        small = compact(record)
        run = {k: v for k, v in small.items() if k in RUN_KEYS}
        platform = {k: v for k, v in small.items() if k not in RUN_KEYS}
        changed, size = write_if_changed(os.path.join(ndir, "last.json"), run)
        wrote_run += 1 if changed else 0
        total += size
        if platform:
            changed, size = write_if_changed(
                os.path.join(ndir, "platform.json"), platform)
            wrote_platform += 1 if changed else 0
            total += size
    print(f"[history-last] {len(dirs)} node dirs, {wrote_run} last.json + "
          f"{wrote_platform} platform.json updated, {total} bytes total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
