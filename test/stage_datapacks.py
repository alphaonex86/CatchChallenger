#!/usr/bin/env python3
"""
stage_datapacks.py — pre-stage every test datapack to the local tmpfs
cache and (in parallel) to each enabled execution_node's persistent
cache. Invoked once by test/all.sh at the very top, before any
testing*.py runs.

Tests (testingserver / testingbots / testingmulti / testinghttp /
testingmap4client / testingremote, …) then create symlinks pointing
at <staged_local>(src) instead of copying the whole tree, which turns
each per-test datapack-setup from a 5-15s shutil.copytree into an
~instant ln -s.

Idempotent: repeat invocations re-rsync the source over the cache (the
inner `--delete` keeps the cache a true mirror). Failures abort with
exit code 1 so all.sh stops before running broken tests.
"""

import sys
sys.dont_write_bytecode = True

import json
import os
import time

import build_paths

build_paths.ensure_root()

import datapack_stage
from remote_build import all_enabled_exec_nodes


def main():
    cfg_path = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
    with open(cfg_path, "r") as f:
        cfg = json.load(f)
    srcs = list(cfg["paths"]["datapacks"])
    if not srcs:
        print("[INFO] no datapacks configured; skipping stage step")
        return 0
    nodes = all_enabled_exec_nodes()
    print(f"[INFO] staging {len(srcs)} datapack(s) to local tmpfs "
          f"({datapack_stage.LOCAL_CACHE_ROOT}) and {len(nodes)} exec "
          f"node(s) in parallel")
    t0 = time.monotonic()
    ok, errors = datapack_stage.stage_all(srcs, nodes,
                                          log_info=lambda m: print(f"[INFO] {m}"))
    elapsed = time.monotonic() - t0
    if ok:
        print(f"[OK]   datapack staging done in {elapsed:.1f}s")
        return 0
    print(f"[FAIL] datapack staging failed in {elapsed:.1f}s "
          f"({len(errors)} error(s)):")
    for e in errors:
        print(f"  | {e}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
