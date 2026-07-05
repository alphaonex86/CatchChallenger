"""Shared loader/saver for test/failed.json.

Format is a dict keyed by script FILENAME (os.path.basename(__file__)) —
all.sh --onlyfailed gates on exactly that key, so a script writing any
other top-level label can never be resumed.  Each value is itself a dict
from test name to a detail object:

    {
      "<script_name>": {
        "<test_name>": {
          "detail": "human-readable failure message (last 25 lines etc.)",
          "cmd":    "exact shell-quoted command to reproduce" | null,
          "host":   "remote.host" | null,
          "error":  "stdout+stderr tail" | null
        },
        ...
      },
      ...
    }

`detail` is mandatory.  `cmd`/`host`/`error` are optional and populated
when the producing testing*.py knows them.

Resume-mode logic in each testing*.py only needs the *names* of failing
tests, not the details — so load_names() returns a flat list.  When the
failed.json was written in the legacy list-of-strings shape, that list
is returned unchanged so older caches keep working until they're
rewritten by the next run.
"""
import fcntl, json, os, tempfile

import test_config


# Cross-process lock file. Multiple testing*.py may run in parallel
# (per CLAUDE.md "Run testing*.py scripts in parallel where they
# don't conflict") and each updates its own slice of failed.json via
# read-modify-write. Without locking, the second writer overwrites
# the first writer's slice — classic lost-update bug. We grab an
# exclusive flock on a sidecar file (NOT failed.json itself, so a
# crashed test doesn't leave a stale fcntl-record on the data file)
# for the entire RMW critical section.
def _lock_path():
    return test_config.FAILED_JSON + ".lock"


class _Locked:
    """Context manager: holds an exclusive flock for the whole RMW.

    Lock file is opened O_RDWR | O_CREAT once per call. Released on
    exit (close drops the lock). Stale lock files don't block — they
    have no fcntl record once the holder exits."""
    def __enter__(self):
        path = _lock_path()
        # Make sure the directory exists. failed.json's parent is the
        # tmpfs root which all.sh creates, but a fresh testing*.py run
        # outside all.sh might race.
        os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
        self._fd = os.open(path, os.O_RDWR | os.O_CREAT, 0o644)
        fcntl.flock(self._fd, fcntl.LOCK_EX)
        return self
    def __exit__(self, *exc):
        try:
            fcntl.flock(self._fd, fcntl.LOCK_UN)
        finally:
            os.close(self._fd)


def _read_unlocked():
    path = test_config.FAILED_JSON
    if not os.path.isfile(path):
        return {}
    try:
        with open(path, "r") as f:
            return json.load(f)
    except (json.JSONDecodeError, IOError):
        return {}


def _read():
    # Public reads are stale-tolerant (load_names / load_detail) — we
    # take the lock briefly to avoid reading mid-write, but a writer
    # that's slow won't block readers for long because save() does the
    # write under a temp file + atomic rename.
    with _Locked():
        return _read_unlocked()


def load_names(script_name):
    """Return the list of failing test names for `script_name`.

    None  → no entry, run every test (cold cache).
    []    → entry exists but empty, every previous test passed → skip all.
    [...] → only re-run these (resume mode).

    Tolerates both the legacy [name, name, ...] list shape and the new
    {name: {...}} dict shape."""
    data = _read()
    if script_name not in data:
        return None
    entry = data[script_name]
    if isinstance(entry, list):
        return list(entry)
    if isinstance(entry, dict):
        return list(entry.keys())
    return None


def load_detail(script_name, test_name):
    """Return the detail dict for a single (script, test) pair, or None."""
    data = _read()
    entry = data.get(script_name)
    if isinstance(entry, dict):
        d = entry.get(test_name)
        if isinstance(d, dict):
            return d
    return None


def save(script_name, failures):
    """Persist `failures` (a list of (name, detail_dict) tuples) under
    `script_name` in failed.json.  Other scripts' entries are preserved.

    `detail_dict` SHOULD contain at least {"detail": "<str>"}; optional
    keys: cmd / host / error.  An empty list of failures clears the
    entry — the next resume-mode pass will see "everything passed,
    nothing to retry".

    Concurrency-safe: the whole read-modify-write executes under an
    exclusive flock so parallel testing*.py runs cannot lose updates,
    and the new contents land via temp-file + atomic rename so a
    reader that opens the file mid-write either sees the old version
    or the new one (never a half-written one)."""
    with _Locked():
        data = _read_unlocked()
        if not failures:
            # Preserve the key with an empty value so resume mode still
            # reports "previously all passed, skipping" — that's the
            # established testing*.py contract.
            data[script_name] = {}
        else:
            out = {}
            for name, detail in failures:
                if not isinstance(detail, dict):
                    detail = {"detail": str(detail) if detail else ""}
                out[name] = detail
            data[script_name] = out
        path = test_config.FAILED_JSON
        d = os.path.dirname(path) or "."
        # tempfile in the same directory so os.replace is a same-filesystem
        # atomic rename. NamedTemporaryFile(delete=False) so we can rename
        # it without it being unlinked first.
        fd, tmp = tempfile.mkstemp(prefix=".failed.", suffix=".tmp", dir=d)
        try:
            with os.fdopen(fd, "w") as f:
                json.dump(data, f, indent=2)
            os.replace(tmp, path)
        except Exception:
            try:
                os.unlink(tmp)
            except OSError:
                pass
            raise


def make_detail(detail="", *, cmd=None, host=None,
                compile_output=None, run_output=None):
    """Build a detail dict with only the populated fields.

    Operator policy: failed.json captures the FULL stdout/stderr of any
    failing command — never truncated.  Two separate fields so a human
    can tell which phase failed even when both phases ran:

      compile_output — full output of the cmake/qmake/cross-compile step
      run_output     — full output of the actual binary / test step

    A producer with only a tail should still pass it (better than
    nothing); the convention is "best-effort full output" — the caller
    must pipe stderr to stdout (`2>&1`) and capture without trimming.
    """
    d = {"detail": detail or ""}
    if cmd is not None:
        d["cmd"] = cmd
    if host is not None:
        d["host"] = host
    if compile_output is not None:
        d["compile_output"] = compile_output
    if run_output is not None:
        d["run_output"] = run_output
    return d


# Side channel for richer failure metadata.  Scripts that produce a test
# failure deep inside a helper (cmake_helpers, remote_build, ...) call
# `set_extras(name, cmd=..., host=..., compile_output=..., run_output=...)`
# next to log_fail(); the per-script save_failed_cases() pulls the
# extras at persist time and merges them into the detail dict so the
# operator sees command + host + full per-phase output in failed.json
# without each helper having to know about every script's logging
# convention.
#
# A process-local threading.Lock guards _extras because a testing*.py
# may push to it from multiple threads (most notably testingmulti.py's
# per-client worker threads). Cross-process safety is handled by
# `_Locked` above; threads inside one process need this in-memory lock.
import threading
_extras = {}
_extras_lock = threading.Lock()


def set_extras(name, *, cmd=None, host=None,
               compile_output=None, run_output=None):
    """Record extras for `name`.  Repeated calls merge — last write wins
    per field.  Empty string is treated as "no value" so a caller can
    safely pass cmd="" without overwriting a previously-recorded cmd."""
    with _extras_lock:
        cur = _extras.setdefault(name, {})
        if cmd:
            cur["cmd"] = cmd
        if host:
            cur["host"] = host
        if compile_output:
            cur["compile_output"] = compile_output
        if run_output:
            cur["run_output"] = run_output


def pop_extras(name):
    """Return + clear the extras dict for `name`.  save_failed_cases() in
    each testing*.py calls this once per failure to drain the side
    channel as it builds the per-failure dict."""
    with _extras_lock:
        return _extras.pop(name, {})


def clear_extras():
    """Reset the side channel.  Tests that re-run inside the same
    process (resume mode, --bisect smoke tests) call this between
    iterations so old entries don't leak forward."""
    with _extras_lock:
        _extras.clear()
