"""
datapack_stage.py — one-shot pre-staging of every test datapack onto a
fast cache, in parallel.

Why: the test scripts copy ~hundreds of MB of XML / TMX every time they
re-run a server or set up a remote client. shutil.copytree on real disk
takes 5-15s per copy and is repeated dozens of times per all.sh. Two
sources of slowdown disappear when we stage once and symlink afterwards:

  * Local (test box): rsync each source datapack to
      <test_config.LOCAL_CACHE_ROOT>/<basename>/
    once at all.sh startup. Tests then `os.symlink(<staged>, <build>/datapack)`
    instead of copytree(). tmpfs reads are RAM-speed; symlinks cost zero.

  * Remote (each enabled execution_node): rsync each source datapack via
    SSH to <datapack_cache>/<basename>/ on the node. Cache path comes
    from the per-exec-node `datapack_cache` field in remote_nodes.json
    (default /home/user/datapack-cache/). The remote cache PERSISTS
    across runs — never wiped by the test harness; subsequent rsync
    --delete passes only ship the diff. testingremote.py's exec phase
    then `ln -sfn <cache>/<id> <build>/datapack` instead of rsync'ing
    the whole tree every test.

All rsyncs run concurrently (one thread per (target, source) pair) and
this module's `stage_all()` blocks until every one completes.

Public API:
    LOCAL_CACHE_ROOT          test_config.LOCAL_CACHE_ROOT
                              (<paths.tmpfs_root>/cc-datapack)
    DEFAULT_REMOTE_CACHE      /home/user/datapack-cache
    datapack_id(src)          → stable id for a source datapack (basename)
    staged_local(src)         → path under LOCAL_CACHE_ROOT for `src`
    staged_remote(node, src)  → path under node['datapack_cache'] for `src`
    stage_all(srcs, exec_nodes, log_info=print) → run every rsync in
                                parallel, wait, raise on any failure.

Idempotent: safe to call stage_all() at every all.sh start.
"""

import os
import subprocess
import threading
import hashlib
import test_config


LOCAL_CACHE_ROOT = test_config.LOCAL_CACHE_ROOT
DEFAULT_REMOTE_CACHE = "/home/user/datapack-cache"

_RSYNC_LOCAL_TIMEOUT = 600
_RSYNC_REMOTE_TIMEOUT = 1200

# Checksum-skip — at the start of stage_all we recompute every source
# datapack's checksum and compare it to test_config.get_datapack_checksum
# (persisted to config.json under paths.datapack_checksums). When the
# source is unchanged since the last successful stage_all, every rsync
# (local + each remote LXC) is SKIPPED; on a no-change all.sh start
# this saves the multi-hundred-MB host→LXC SSH transfer entirely.
# Stored in config.json so the cache hint survives across sessions, and
# nothing is written into the staged cache trees themselves — they stay
# pristine 1:1 mirrors of the source.


def _src_checksum(src):
    """Compute a fast content-aware checksum over a datapack tree.

    We hash (relative-path, size, mtime_ns) tuples for every regular file
    and symlink under `src`. This catches every change rsync would
    propagate (content, deletes, renames, perm changes via mtime) without
    actually reading file bytes — typical hundreds-of-MB datapack hashes
    in well under a second.

    Returns a hex sha256 string. Empty string on any I/O error so the
    caller falls back to "always rsync"."""
    h = hashlib.sha256()
    src = os.path.normpath(src)
    base_len = len(src) + 1
    try:
        for root, dirs, files in os.walk(src, followlinks=False):
            dirs.sort()
            files.sort()
            fi = 0
            while fi < len(files):
                fn = files[fi]
                fi += 1
                full = os.path.join(root, fn)
                rel = full[base_len:]
                try:
                    st = os.lstat(full)
                except OSError:
                    continue
                line = f"{rel}\0{st.st_size}\0{st.st_mtime_ns}\n"
                h.update(line.encode("utf-8", "surrogateescape"))
    except OSError:
        return ""
    return h.hexdigest()

# Shared SSH options + rsync-via-ssh string. Imported lazily so the
# module loads even when cmd_helpers (and thus diagnostic) hasn't been
# imported yet — useful when stage_all is invoked very early in
# all.sh before the per-test helpers initialise.


def datapack_id(src):
    """Return a stable cache-key for a source datapack path. The
    basename works because every checked-in datapack lives in its own
    top-level directory (CatchChallenger-datapack, datapack-pkmn, …);
    if two sources somehow had the same basename the caller should
    rename one. Trailing slashes are stripped first so
    /home/user/datapack-pkmn and /home/user/datapack-pkmn/ map to the
    same id."""
    return os.path.basename(os.path.normpath(src))


def staged_local(src):
    """Return the local cache path corresponding to `src`. Does NOT
    verify the path exists — call stage_all() first."""
    return os.path.join(LOCAL_CACHE_ROOT, datapack_id(src))


def remote_cache_for(exec_node):
    """Per-node cache root, with the documented default."""
    return (exec_node.get("datapack_cache") or DEFAULT_REMOTE_CACHE).rstrip("/")


def staged_remote(exec_node, src):
    """Return the remote-side cache path on `exec_node` for `src`.
    Does NOT verify; stage_all() rsyncs the source there first."""
    return remote_cache_for(exec_node) + "/" + datapack_id(src)


def _rsync_local(src, log_info):
    """Local rsync source → LOCAL_CACHE_ROOT/<id>. --delete keeps the
    cache slot a true mirror; if a file disappears upstream we drop it
    here too. The source-checksum gate is enforced one level up in
    stage_all — when the recomputed source checksum matches the value
    stored in config.json, this function is not called at all. The
    staged tree stays a pristine 1:1 mirror of the source (no sentinel
    or hidden checksum file written into it)."""
    dst = staged_local(src)
    os.makedirs(LOCAL_CACHE_ROOT, exist_ok=True)
    os.makedirs(dst, exist_ok=True)
    args = ["rsync", "-art", "--delete", src.rstrip("/") + "/", dst + "/"]
    log_info(f"datapack stage local: {src} -> {dst}")
    try:
        p = subprocess.run(args, timeout=_RSYNC_LOCAL_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"local rsync TIMEOUT after {_RSYNC_LOCAL_TIMEOUT}s ({src})"
    if p.returncode == 0:
        return True, ""
    return False, (f"local rsync rc={p.returncode} for {src}: " +
                   p.stdout.decode(errors="replace").strip()[-300:])


def _rsync_remote(exec_node, src, log_info):
    """SSH-rsync source → exec_node:<datapack_cache>/<id>. The remote
    cache persists across runs (we never clean it); rsync --delete only
    drops files that are gone upstream so the diff stays small. The
    source-checksum gate is enforced one level up in stage_all — when
    the recomputed source checksum matches the value stored in
    config.json, this function is not called for any target. The
    staged tree stays a pristine 1:1 mirror of the source (no sentinel
    or hidden checksum file written into it)."""
    from cmd_helpers import RSYNC_SSH_E, SSH_OPTS_LIST
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    cache = remote_cache_for(exec_node)
    name = datapack_id(src)
    dst = cache + "/" + name
    label = exec_node.get("label", host)
    rsync_host = f"{user}@{host}"
    if ":" in host and not host.startswith("["):
        rsync_host = f"{user}@[{host}]"
    mkdir_args = ["ssh"] + SSH_OPTS_LIST + ["-p", str(port),
                                             f"{user}@{host}",
                                             f"mkdir -p {dst}"]
    try:
        m = subprocess.run(mkdir_args, timeout=30,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"ssh mkdir TIMEOUT for {label}:{dst}"
    if m.returncode != 0:
        return False, (f"ssh mkdir rc={m.returncode} for {label}:{dst}: " +
                       m.stdout.decode(errors="replace").strip()[-200:])
    rsync_args = [
        "rsync", "-art", "--delete",
        "-e", f"{RSYNC_SSH_E} -p {port}",
        src.rstrip("/") + "/", f"{rsync_host}:{dst}/",
    ]
    log_info(f"datapack stage remote: {src} -> {label}:{dst}")
    try:
        p = subprocess.run(rsync_args, timeout=_RSYNC_REMOTE_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"remote rsync TIMEOUT after {_RSYNC_REMOTE_TIMEOUT}s for {label}"
    if p.returncode == 0:
        return True, ""
    return False, (f"remote rsync rc={p.returncode} for {label}: " +
                   p.stdout.decode(errors="replace").strip()[-300:])


def stage_all(srcs, exec_nodes=None, log_info=print):
    """Run every (src) and (src, exec_node) rsync in parallel. Blocks
    until every thread is done. Returns (ok_total, errors) where errors
    is a list of strings — empty on full success.

    `srcs` is a list of source datapack paths.
    `exec_nodes` is the flattened list from
    remote_build.all_enabled_exec_nodes(); pass None / [] to skip the
    remote staging entirely (e.g. when the harness has no SSH set up).

    Per-source checksum gate: for each src we recompute a fast
    metadata-based checksum and look up the previously-stored value via
    test_config.get_datapack_checksum. On match, every rsync (local +
    every remote LXC) is SKIPPED; on mismatch we run all rsyncs in
    parallel and, if every target succeeds, persist the new checksum
    via test_config.set_datapack_checksum so the next all.sh start
    short-circuits."""
    threads = []
    results = []
    per_src_results = {}   # src -> list[(ok, detail)] of its workers
    src_sums = {}          # src -> recomputed checksum
    lock = threading.Lock()

    def runner(src, fn, *args):
        ok, detail = fn(*args, log_info)
        with lock:
            results.append((ok, detail))
            per_src_results.setdefault(src, []).append((ok, detail))

    si = 0
    while si < len(srcs):
        src = srcs[si]
        si += 1
        cur_sum = _src_checksum(src)
        src_sums[src] = cur_sum
        prev_sum = test_config.get_datapack_checksum(src) if cur_sum else ""
        if cur_sum and prev_sum == cur_sum:
            log_info(f"datapack {src}: checksum match (cached), skipping all rsyncs")
            with lock:
                results.append((True, ""))
                per_src_results.setdefault(src, []).append((True, ""))
            continue
        t = threading.Thread(target=runner, args=(src, _rsync_local, src),
                             daemon=True)
        t.start()
        threads.append(t)
        if exec_nodes:
            ei = 0
            while ei < len(exec_nodes):
                en = exec_nodes[ei]
                ei += 1
                if not bool(en.get("enabled", True)):
                    continue
                t2 = threading.Thread(target=runner,
                                      args=(src, _rsync_remote, en, src),
                                      daemon=True)
                t2.start()
                threads.append(t2)

    ti = 0
    while ti < len(threads):
        threads[ti].join()
        ti += 1

    # Persist checksum for sources where every target succeeded. Skip
    # write when any target failed so the next run retries those.
    for src in srcs:
        cur_sum = src_sums.get(src, "")
        if not cur_sum:
            continue
        rs = per_src_results.get(src, [])
        if rs and all(r[0] for r in rs):
            try:
                test_config.set_datapack_checksum(src, cur_sum)
            except (OSError, ValueError):
                pass  # non-fatal; the optimisation just doesn't kick in next run

    errors = [d for ok, d in results if not ok]
    return (len(errors) == 0), errors
