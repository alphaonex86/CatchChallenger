#!/usr/bin/env python3
"""
stage_datapacks.py — pre-stage every test datapack to the local
persistent DISK cache (paths.datapack_cache_root, NOT tmpfs — the
datapack is read-only for the whole run, so RAM-backing it only wastes
RAM; the run aborts if the cache resolves onto tmpfs/ramfs) and (in
parallel) to each enabled execution_node's persistent cache. Invoked
once by test/all.sh at the very top, before any testing*.py runs.

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
import shlex
import subprocess
import threading
import time

import build_paths

build_paths.ensure_root()

import datapack_stage
from remote_build import all_enabled_exec_nodes


def _ensure_remote_cache_off_ram(node, results, lock):
    """Make one exec node's datapack_cache live on persistent DISK, PERMANENTLY.

    If the cache is a dedicated tmpfs/ramfs mount: comment its /etc/fstab line
    (so it never re-mounts on reboot) and umount it, then recreate the dir on
    the underlying disk with the parent's ownership. Idempotent — a no-op once
    the node is already on disk (just one findmnt). Uses root ssh (same key as
    the node's `user`, per operator). Best-effort: records the outcome, never
    raises. This is the lifecycle "umount step" — the one-time conversion is
    already done; this keeps the fleet self-healing against a tmpfs regression."""
    host = node.get("host")
    cache = node.get("datapack_cache")
    label = node.get("label", host)
    if not host or not cache:
        return
    port = str(node.get("port", 22))
    cq = shlex.quote(cache)
    # Mirror the proven one-shot conversion: only touches a dedicated
    # `tmpfs <cache> tmpfs ...` line, leaving everything else untouched.
    remote = (
        f"c={cq}; "
        f'fs=$(findmnt -n -o FSTYPE --target "$c" 2>/dev/null || echo none); '
        f'if [ "$fs" = tmpfs ] || [ "$fs" = ramfs ]; then '
        f"  cp -n /etc/fstab /etc/fstab.bak-cc-datapack 2>/dev/null || true; "
        f"  sed -i 's|^tmpfs[[:space:]]\\{{1,\\}}'\"$c\"'[[:space:]]\\{{1,\\}}tmpfs|#cc-datapack-on-disk# &|' /etc/fstab; "
        f'  mountpoint -q "$c" && umount "$c" || true; '
        f'  mkdir -p "$c"; chown --reference="$(dirname "$c")" "$c" 2>/dev/null || true; '
        f'  echo CONVERTED; '
        f"fi; "
        f'findmnt -n -o FSTYPE --target "$c" 2>/dev/null || echo unknown'
    )
    argv = ["ssh", "-n", "-o", "ConnectTimeout=8", "-o", "BatchMode=yes",
            "-o", "StrictHostKeyChecking=no", "-p", port,
            f"root@{host}", remote]
    try:
        p = subprocess.run(argv, timeout=45, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
        out = p.stdout.decode(errors="replace").strip().splitlines()
        converted = any(ln.strip() == "CONVERTED" for ln in out)
        fstype = out[-1].strip() if out else ""
    except (subprocess.TimeoutExpired, OSError):
        converted = False
        fstype = "UNREACHABLE"
    with lock:
        results.append((label, cache, fstype, converted))


def ensure_remote_caches_off_ram(nodes):
    """Run _ensure_remote_cache_off_ram across all nodes in parallel; print a
    one-line report and a warning for any node left on RAM/unreachable.

    Returns the number of nodes that were CONVERTED this run (tmpfs→disk). An
    umount empties that node's cache, so the caller must force a full re-stage
    (the staging checksum-skip only checks LOCAL presence)."""
    if not nodes:
        return 0
    results = []
    lock = threading.Lock()
    threads = []
    for n in nodes:
        t = threading.Thread(target=_ensure_remote_cache_off_ram,
                             args=(n, results, lock), daemon=True)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
    bad = [(l, c, f) for (l, c, f, _cv) in results if f in ("tmpfs", "ramfs")]
    unreach = [(l, c, f) for (l, c, f, _cv) in results if f in ("UNREACHABLE", "unknown", "")]
    converted = [(l, c) for (l, c, _f, cv) in results if cv]
    print(f"[INFO] remote datapack caches: {len(results)} checked, "
          f"{len(converted)} just converted tmpfs→disk, {len(bad)} still on "
          f"RAM, {len(unreach)} unreachable/unknown")
    for (l, c) in converted:
        print(f"  [INFO] {l}: {c} converted tmpfs→disk (umounted + fstab fixed)")
    for (l, c, f) in bad:
        print(f"  [WARN] {l}: {c} is STILL on {f} (fstab/umount self-heal "
              f"failed — check root ssh / fstab line)")
    for (l, c, f) in unreach:
        print(f"  [WARN] {l}: {c} fstype {f or 'empty'} (node unreachable?)")
    return len(converted)


def main():
    cfg_path = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
    with open(cfg_path, "r") as f:
        cfg = json.load(f)
    srcs = list(cfg["paths"]["datapacks"])
    if not srcs:
        print("[INFO] no datapacks configured; skipping stage step")
        return 0
    # The datapack is READ-ONLY for the whole run and must NOT live on RAM
    # (tmpfs/ramfs): the cache is staged once here and only symlinked into
    # per-test build dirs afterwards, never rewritten, so a RAM-backed cache
    # buys nothing and just burns memory. Abort and report rather than
    # silently wasting RAM (point paths.datapack_cache_root at disk).
    cache = datapack_stage.LOCAL_CACHE_ROOT
    if datapack_stage.is_under_ram_fs(cache):
        print(f"[ABORT] datapack cache {cache} is on "
              f"{datapack_stage.fstype_of(cache)} (RAM). The datapack must "
              f"live on PERSISTENT DISK — set paths.datapack_cache_root in "
              f"~/.config/catchchallenger-testing/config.json to a disk path. "
              f"Refusing to stage the datapack onto tmpfs.")
        return 1
    nodes = all_enabled_exec_nodes()
    # Lifecycle off-RAM step: make sure every exec node's datapack_cache lives
    # on persistent DISK (umount + fstab) before we stage onto it. Idempotent;
    # self-heals a tmpfs regression and reports any node it couldn't fix.
    converted = ensure_remote_caches_off_ram(nodes)
    if converted:
        # An umount emptied those nodes' caches; the staging checksum-skip only
        # checks LOCAL presence, so it would skip re-pushing to them. Drop the
        # persisted source checksums to force a full re-stage this run.
        import test_config
        for s in srcs:
            try:
                test_config.set_datapack_checksum(s, "")
            except (OSError, ValueError):
                pass
        print(f"[INFO] {converted} node(s) converted → forcing full re-stage")
    print(f"[INFO] staging {len(srcs)} datapack(s) to local disk cache "
          f"({cache}, {datapack_stage.fstype_of(cache)}) and {len(nodes)} "
          f"exec node(s) in parallel")
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
