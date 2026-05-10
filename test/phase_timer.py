"""
phase_timer.py — Cumulative wall-clock timing for testing*.py scripts.

Two outputs:

1. **Console prefix** — every log line gets `[t=N.Ns]` so the operator
   can see at a glance which phase of which test is dominating
   elapsed time, without having to re-read the script and add ad-hoc
   prints.

2. **Persistent JSON** — every phase block (entered via the `phase()`
   context manager OR every log_pass / log_fail when wired through
   `record_event`) appends one record to `<tmpfs>/time.json`. Survives
   the all.sh tmpfs wipe (all.sh keeps it like failed.json) so the
   operator can post-mortem the whole run after it finishes.

Record schema (each line is one JSON object — JSONL, NOT a single
JSON document, so parallel testing*.py can append safely without
locking):

    {"script": "testingclient.py", "kind": "phase",
     "name": "compile qtopengl", "ok": true, "dt": 124.3,
     "t_end": 188.7, "ts": 1714998877.123}

`kind` is "phase" (from `with phase(...)`), "pass" (log_pass),
"fail" (log_fail), or "info" (log_info).

Usage in a testing*.py:

    from phase_timer import t as _t, record_event
    ...
    def log_info(msg):
        print(f"{_t()} {C_CYAN}[INFO]{C_RESET} {msg}")
        record_event("info", msg, ok=True, dt=0.0)

    def log_pass(name, detail=""):
        ...
        record_event("pass", name, ok=True, dt=elapsed, detail=detail)

For phase-block timing (e.g. "configure", "build", "rsync"):

    with phase("rsync to mips-lxc"):
        ...

The exit line is emitted even when the block raises.

The module-level _t0 is captured at import time, which happens once
per script run because every testing*.py is invoked as a fresh python
process by all.sh / test/all.sh. Sub-imports of this module reuse the
same _t0 (Python caches modules), so every helper that imports
phase_timer agrees on the same start time.
"""

import fcntl
import json
import os
import sys
import time
from contextlib import contextmanager

_t0 = time.monotonic()
_t0_wall = time.time()

# Resolve TIME_JSON lazily — test_config has its own lazy lookup that
# may raise if ~/.config/catchchallenger-testing/config.json is
# missing on some node where this module is imported (e.g. an
# exec_node that only runs a binary). We want phase_timer to stay
# importable everywhere; logging just no-ops when the path can't be
# resolved.
_TIME_JSON_PATH = None
_TIME_JSON_RESOLVED = False
_SCRIPT_NAME = os.path.basename(sys.argv[0]) if sys.argv and sys.argv[0] else "?"


def _time_json_path():
    global _TIME_JSON_PATH, _TIME_JSON_RESOLVED
    if _TIME_JSON_RESOLVED:
        return _TIME_JSON_PATH
    _TIME_JSON_RESOLVED = True
    try:
        import test_config
        _TIME_JSON_PATH = test_config.TIME_JSON
    except Exception:
        _TIME_JSON_PATH = None
    return _TIME_JSON_PATH


def since_start_s():
    """Return seconds since this module was first imported (float)."""
    return time.monotonic() - _t0


def t():
    """Return formatted cumulative-time prefix, e.g. '[t=12.3s]'."""
    return f"[t={since_start_s():.1f}s]"


def record_event(kind, name, ok=True, dt=0.0, detail=""):
    """Append one JSONL record to <tmpfs>/time.json.

    Best-effort: on any error (file system unwritable, JSON encode
    failure, etc.) the call silently drops — never break a test on a
    logging-side problem.

    Concurrency: testing*.py may run in parallel (per CLAUDE.md
    "Run testing*.py scripts in parallel where they don't conflict").
    We rely on TWO mechanisms together to avoid a corrupt time.json:

      1. `O_APPEND` — every write goes to current EOF atomically
         (kernel adjusts the offset under the inode lock).
      2. `fcntl.flock(LOCK_EX)` — serialises the actual write() call
         across processes on the same host. Belt-and-braces on top of
         O_APPEND so a record longer than PIPE_BUF (4KiB on Linux),
         which can split into multiple page-sized writes, still lands
         contiguously. `detail` may be long, so we don't trust the
         single-write atomic ceiling.

    The lock is dropped before the file is closed so other writers
    don't queue behind a slow flush().
    """
    path = _time_json_path()
    if path is None:
        return
    rec = {
        "script": _SCRIPT_NAME,
        "kind": kind,
        "name": name,
        "ok": bool(ok),
        "dt": float(dt),
        "t_end": since_start_s(),
        "ts": time.time(),
    }
    if detail:
        rec["detail"] = detail
    try:
        line = (json.dumps(rec, default=str) + "\n").encode(
            "utf-8", errors="replace")
        fd = os.open(path,
                     os.O_WRONLY | os.O_CREAT | os.O_APPEND,
                     0o644)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX)
            try:
                os.write(fd, line)
            finally:
                fcntl.flock(fd, fcntl.LOCK_UN)
        finally:
            os.close(fd)
    except Exception:
        pass


@contextmanager
def phase(name, log=None):
    """Context manager that brackets a phase with timing prints + JSON.

    Prints `[t=Ns] >> <name>` on entry and
    `[t=Ns] << <name> done in Δ.Δs` (or `... FAILED in Δ.Δs`) on exit.

    `log` is an optional logger callable (e.g. log_info) — when None,
    plain print() is used so phase_timer stays usable from helper
    modules that don't import the script-local log_info.

    Always emits a JSON record, regardless of `log`.
    """
    emit = log if log is not None else print
    emit(f"{t()} >> {name}")
    t_in = time.monotonic()
    ok = True
    try:
        yield
    except BaseException:
        ok = False
        raise
    finally:
        dt = time.monotonic() - t_in
        verdict = "done" if ok else "FAILED"
        emit(f"{t()} << {name} {verdict} in {dt:.1f}s")
        record_event("phase", name, ok=ok, dt=dt)
