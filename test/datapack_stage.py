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

Every staged slot is FILTERED to CATCHCHALLENGER_EXTENSION_ALLOWED
(general/base/GeneralVariable.hpp, read at runtime): cruft the engine never
loads (README.md, .git, .xcf, .sample, …) is dropped from every slot. Each
datapack is staged TWICE: the full client slot (all allowed extensions, incl.
images/audio) and a media-stripped headless-SERVER slot (`<id>-server-headless`,
images/audio removed, skin/fighter/ kept as empty folders) for server-only
consumers. Pass `server=True` to the accessors to reach the stripped slot.

Public API:
    LOCAL_CACHE_ROOT          test_config.LOCAL_CACHE_ROOT
                              (<paths.tmpfs_root>/cc-datapack)
    DEFAULT_REMOTE_CACHE      /home/user/datapack-cache
    allowed_extensions()      → CATCHCHALLENGER_EXTENSION_ALLOWED (runtime)
    datapack_id(src, server=False)        → stable slot id
    staged_local(src, server=False)       → path under LOCAL_CACHE_ROOT
    staged_remote(node, src, server=False)→ path under node['datapack_cache']
    stage_all(srcs, exec_nodes, log_info=print) → stage both slots for every
                                src in parallel, wait, raise on any failure.

Idempotent: safe to call stage_all() at every all.sh start.
"""

import os
import subprocess
import threading
import hashlib
import sys
import test_config


LOCAL_CACHE_ROOT = test_config.LOCAL_CACHE_ROOT
DEFAULT_REMOTE_CACHE = "/home/user/datapack-cache"

_RSYNC_LOCAL_TIMEOUT = 600
_RSYNC_REMOTE_TIMEOUT = 1200

# ---- extension filtering -------------------------------------------------
# CATCHCHALLENGER_EXTENSION_ALLOWED (general/base/GeneralVariable.hpp) is the
# ONLY set of extensions the engine ever loads from a datapack. We stage a
# FILTERED mirror, not a 1:1 copy:
#   * EVERY slot is filtered to the allowed list -> non-datapack cruft
#     (README.md, .git, GIMP .xcf, .sample, ...) is dropped on client and
#     server alike (the engine ignores it anyway). "Respect
#     CATCHCHALLENGER_EXTENSION_ALLOWED in all cases."
#   * The server-headless slot additionally drops the MEDIA subset (images
#     png/jpg/gif, audio ogg/opus): a headless server needs no rendering or
#     sound. skin/fighter/ survives as EMPTY folders (--include=*/) for the
#     XML-parse fighter-skin enumeration.
# Loaded from the header AT RUNTIME so updating the #define updates staging.
_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_MEDIA_EXT = {"png", "jpg", "jpeg", "gif", "bmp", "webp", "ico", "svg",
              "tga", "psd", "ogg", "opus", "wav", "mp3", "mid", "midi",
              "flac", "aac", "m4a", "mod", "xm", "s3m"}
_ALLOWED_EXT_FALLBACK = ["tmx", "xml", "tsx", "js", "png", "jpg", "gif",
                         "ogg", "opus"]
_ALLOWED_EXT_CACHE = None
# Bump when the staging filter logic changes: it is mixed into the source
# checksum so a logic change invalidates every cached checksum and forces
# one full re-stage (otherwise an unchanged source would skip the rsync and
# the new filtering / new server slot would never be applied).
_STAGE_SCHEMA = "v2-extfilter"


def allowed_extensions():
    """CATCHCHALLENGER_EXTENSION_ALLOWED parsed from GeneralVariable.hpp at
    runtime (memoized); falls back to the known set if unreadable."""
    global _ALLOWED_EXT_CACHE
    if _ALLOWED_EXT_CACHE is not None:
        return _ALLOWED_EXT_CACHE
    allowed = None
    hpp = os.path.join(_REPO_ROOT, "general", "base", "GeneralVariable.hpp")
    try:
        with open(hpp, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if "CATCHCHALLENGER_EXTENSION_ALLOWED" in line and "#define" in line:
                    q1 = line.find('"')
                    q2 = line.find('"', q1 + 1)
                    if q1 != -1 and q2 != -1:
                        allowed = [e.strip().lower()
                                   for e in line[q1 + 1:q2].split(";")
                                   if e.strip()]
                    break
    except OSError as e:
        print(f"[datapack_stage] WARN: cannot read {hpp}: {e}", file=sys.stderr)
    if not allowed:
        allowed = list(_ALLOWED_EXT_FALLBACK)
    _ALLOWED_EXT_CACHE = allowed
    return allowed


def keep_extensions(server):
    """Extensions to KEEP for a slot: the allowed list, minus the media
    subset when `server` (headless: no images/audio)."""
    allowed = allowed_extensions()
    if server:
        return [e for e in allowed if e not in _MEDIA_EXT]
    return list(allowed)


def rsync_filter_args(server):
    """rsync whitelist for a staged datapack slot: keep only the wanted
    extensions, drop everything else (cruft + media-when-server) with
    --delete-excluded so already-staged strays are pruned. --include=*/
    keeps the full directory tree (empty folders where a dir held only
    excluded files -- e.g. server skin/fighter/)."""
    # Prune .git BEFORE the dir-include: the source datapack is a git
    # checkout, and --include=*/ would otherwise recreate its whole object
    # tree as hundreds of empty folders. First-match-wins, so this comes
    # first.
    args = ["--delete-excluded", "--exclude=.git", "--exclude=.git/",
            "--include=*/"]
    for ext in keep_extensions(server):
        args.append(f"--include=*.{ext}")
    args.append("--exclude=*")
    return args


# Suffix for the headless-server cache slot, kept distinct from the full
# (client) slot so client tests still get images/audio.
SERVER_SLOT_SUFFIX = "-server-headless"

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
    # Mix in the staging-filter schema so a change to the filter logic (new
    # extension rules / new server slot) invalidates the cached checksum and
    # forces one full re-stage instead of skipping on an unchanged source.
    h.update(_STAGE_SCHEMA.encode("ascii"))
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


def datapack_id(src, server=False):
    """Return a stable cache-key for a source datapack path. The
    basename works because every checked-in datapack lives in its own
    top-level directory (CatchChallenger-datapack, datapack-pkmn, …);
    if two sources somehow had the same basename the caller should
    rename one. Trailing slashes are stripped first so
    /home/user/datapack-pkmn and /home/user/datapack-pkmn/ map to the
    same id. `server=True` returns the headless-server slot id (a
    media-stripped sibling of the full client slot)."""
    base = os.path.basename(os.path.normpath(src))
    return base + SERVER_SLOT_SUFFIX if server else base


def staged_local(src, server=False):
    """Return the local cache path corresponding to `src` (the
    media-stripped server slot when `server=True`). Does NOT verify the
    path exists — call stage_all() first."""
    return os.path.join(LOCAL_CACHE_ROOT, datapack_id(src, server))


def remote_cache_for(exec_node):
    """Per-node cache root, with the documented default."""
    return (exec_node.get("datapack_cache") or DEFAULT_REMOTE_CACHE).rstrip("/")


def staged_remote(exec_node, src, server=False):
    """Return the remote-side cache path on `exec_node` for `src` (the
    media-stripped server slot when `server=True`). Does NOT verify;
    stage_all() rsyncs the source there first."""
    return remote_cache_for(exec_node) + "/" + datapack_id(src, server)


def _rsync_local(src, log_info, server=False):
    """Local rsync source → LOCAL_CACHE_ROOT/<id>. --delete keeps the
    cache slot a true mirror; if a file disappears upstream we drop it
    here too. The slot is FILTERED to CATCHCHALLENGER_EXTENSION_ALLOWED
    (server slot also drops media) via rsync_filter_args() so cruft
    (README.md, .git, .xcf, ...) never lands in the cache. The
    source-checksum gate is enforced one level up in stage_all."""
    dst = staged_local(src, server)
    os.makedirs(LOCAL_CACHE_ROOT, exist_ok=True)
    os.makedirs(dst, exist_ok=True)
    args = (["rsync", "-art", "--delete"] + rsync_filter_args(server)
            + [src.rstrip("/") + "/", dst + "/"])
    log_info(f"datapack stage local{' [server]' if server else ''}: "
             f"{src} -> {dst}")
    try:
        p = subprocess.run(args, timeout=_RSYNC_LOCAL_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"local rsync TIMEOUT after {_RSYNC_LOCAL_TIMEOUT}s ({src})"
    if p.returncode == 0:
        return True, ""
    return False, (f"local rsync rc={p.returncode} for {src}: " +
                   p.stdout.decode(errors="replace").strip()[-300:])


def _rsync_remote(exec_node, src, log_info, server=False):
    """SSH-rsync source → exec_node:<datapack_cache>/<id>. The remote
    cache persists across runs (we never clean it); rsync --delete only
    drops files that are gone upstream so the diff stays small. The slot
    is FILTERED to CATCHCHALLENGER_EXTENSION_ALLOWED (server slot also
    drops media) via rsync_filter_args() + --delete-excluded so cruft and
    media are pruned from the cache, not just skipped."""
    from cmd_helpers import RSYNC_SSH_E, SSH_OPTS_LIST
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    cache = remote_cache_for(exec_node)
    name = datapack_id(src, server)
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
        # ssh hung trying to reach the box — treat as unreachable, not a
        # hard staging error (see rc==255 note below).
        log_info(f"datapack stage SKIP {label}: ssh mkdir TIMEOUT (node unreachable)")
        return None, f"ssh mkdir TIMEOUT for {label}:{dst} (node unreachable, skipped)"
    if m.returncode != 0:
        out = m.stdout.decode(errors="replace").strip()
        # rc==255 is ssh's OWN failure code (couldn't connect): "No route
        # to host", "Connection refused", "Connection timed out", "Could
        # not resolve hostname". A mkdir failing on a REACHABLE box returns
        # the remote command's rc (e.g. 1), never 255. An offline exec node
        # — typically an NFS-LXC box that is only brought up later during
        # its own test phase — must NOT abort the whole all.sh: skip it
        # here (return None) and let that node's own test phase surface a
        # FAIL/SKIP if it is actually needed. A genuine error on a
        # reachable node still returns False and aborts.
        if m.returncode == 255:
            log_info(f"datapack stage SKIP {label}: ssh unreachable "
                     f"(rc=255): {out[-160:]}")
            return None, (f"ssh unreachable rc=255 for {label}:{dst} "
                          f"(skipped): {out[-160:]}")
        return False, (f"ssh mkdir rc={m.returncode} for {label}:{dst}: " +
                       out[-200:])
    rsync_args = [
        "rsync", "-art", "--delete",
        *rsync_filter_args(server),
        "-e", f"{RSYNC_SSH_E} -p {port}",
        src.rstrip("/") + "/", f"{rsync_host}:{dst}/",
    ]
    log_info(f"datapack stage remote{' [server]' if server else ''}: "
             f"{src} -> {label}:{dst}")
    try:
        p = subprocess.run(rsync_args, timeout=_RSYNC_REMOTE_TIMEOUT,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"remote rsync TIMEOUT after {_RSYNC_REMOTE_TIMEOUT}s for {label}"
    if p.returncode == 0:
        return True, ""
    # rsync rc=23 = partial transfer. The pkmn datapack ships files whose
    # XXH32-hash-as-mtime sits in the year-2081–2106 range, which 32-bit
    # time_t receivers (mips-lxc, older LXC images) can't fully represent
    # — rsync emits "Time value of <path> truncated on receiver" warnings
    # and exits 23 even though every file's bytes did transfer. Treat
    # that one-warning case as success so the staging step doesn't abort
    # the entire all.sh; the truncated mtime won't help the gateway's
    # hash-cache shortcut on the remote node but the datapack content is
    # still correct.
    out = p.stdout.decode(errors="replace")
    if p.returncode == 23 and "truncated on receiver" in out:
        non_trunc_errors = []
        oi = 0
        lines = out.splitlines()
        while oi < len(lines):
            ln = lines[oi]
            oi += 1
            stripped = ln.strip()
            if not stripped:
                continue
            if "truncated on receiver" in stripped:
                continue
            if stripped.startswith("rsync warning: some files vanished"):
                continue
            if stripped.startswith("rsync error: some files/attrs were not transferred"):
                # The umbrella summary line that rsync appends when the
                # only individual error was a time-truncation warning.
                continue
            non_trunc_errors.append(stripped)
        if not non_trunc_errors:
            log_info(f"remote rsync {label}: ignoring rc=23 (mtime truncation only)")
            return True, ""
    return False, (f"remote rsync rc={p.returncode} for {label}: " +
                   out.strip()[-300:])


def stage_all(srcs, exec_nodes=None, log_info=print):
    """Run every (src) and (src, exec_node) rsync in parallel. Blocks
    until every thread is done. Returns (ok_total, errors) where errors
    is a list of strings — empty on full success. A per-worker status of
    None means "skipped" (an exec node was unreachable, e.g. an offline
    NFS-LXC box brought up only during its own test phase); skips are
    logged but never abort the run.

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

    def runner(src, fn, args, server):
        ok, detail = fn(*args, log_info, server=server)
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
        # Cached checksum is only authoritative when BOTH staged copies (the
        # full client slot AND the media-stripped server slot) actually exist
        # on disk. If a previous all.sh wipe deleted the cache, or the server
        # slot has never been created (first run after this feature landed),
        # the checksum alone would skip the rsync and a consumer side would
        # still be empty. Force a re-stage when either local slot is
        # missing/empty, regardless of checksum.
        def _present(p):
            return os.path.isdir(p) and len(os.listdir(p)) > 0
        local_present = (_present(staged_local(src, server=False))
                         and _present(staged_local(src, server=True)))
        if (cur_sum and prev_sum == cur_sum and local_present):
            log_info(f"datapack {src}: checksum match (cached), skipping all rsyncs")
            with lock:
                results.append((True, ""))
                per_src_results.setdefault(src, []).append((True, ""))
            continue
        # Stage BOTH slots: full (client, server=False) + headless server
        # (media-stripped, server=True). Each is its own worker so they run
        # in parallel and a failure of one is reported independently.
        for srv in (False, True):
            t = threading.Thread(target=runner,
                                  args=(src, _rsync_local, (src,), srv),
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
                                          args=(src, _rsync_remote, (en, src), srv),
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
        # A None status (unreachable node, skipped) is not a failure: a
        # genuine failure is ok is False. Persist the checksum as long as
        # nothing actually failed.
        if rs and all(r[0] is not False for r in rs):
            try:
                test_config.set_datapack_checksum(src, cur_sum)
            except (OSError, ValueError):
                pass  # non-fatal; the optimisation just doesn't kick in next run

    # ok is False  -> genuine error (aborts the run)
    # ok is None   -> skipped (unreachable node), logged but non-fatal
    # ok is True    -> success
    errors = [d for ok, d in results if ok is False]
    return (len(errors) == 0), errors
