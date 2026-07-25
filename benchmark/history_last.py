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
DROP_KEYS = frozenset((
    "kernel_config_gz", "sensors", "noise_services", "loadavg_1min_at_start",
    "batch_id", "started_utc", "cpu_flags",
))


def strip_metrics(node):
    """Recursively drop the raw per-run `samples` arrays from metric blocks.

    A metric block is {value, median, stddev, unit, better, samples}. `samples`
    is the raw measurement list -- it is what makes a botactions record 231 KB
    on its own, and it says nothing about evolution or hardware that median +
    stddev do not already say. Everything else is kept verbatim."""
    if isinstance(node, dict):
        out = {}
        for key, value in node.items():
            if key == "samples":
                continue
            if value is None or value == {} or value == []:
                continue
            out[key] = strip_metrics(value)
        return out
    if isinstance(node, list):
        return [strip_metrics(v) for v in node]
    return node


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
        if not name.endswith(".json") or name == "last.json":
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


def main():
    written = 0
    total = 0
    for ndir in node_dirs():
        record = newest_run(ndir)
        if record is None:
            continue
        small = compact(record)
        out = os.path.join(ndir, "last.json")
        # sort_keys so an unchanged run produces a byte-identical file and git
        # records no churn.
        text = json.dumps(small, indent=2, sort_keys=True) + "\n"
        previous = None
        if os.path.isfile(out):
            try:
                with open(out) as handle:
                    previous = handle.read()
            except OSError:
                previous = None
        if previous != text:
            with open(out, "w") as handle:
                handle.write(text)
            written += 1
        total += len(text)
    print(f"[history-last] {len(node_dirs())} node dirs, {written} last.json "
          f"updated, {total} bytes total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
