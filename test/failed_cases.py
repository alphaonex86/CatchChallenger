"""Shared loader/saver for test/failed.json.

Format is a dict keyed by script-name (the same string the script uses
when writing — usually os.path.basename(__file__), sometimes a label like
"compile testingmap4client").  Each value is itself a dict from test
name to a detail object:

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
import json, os

import test_config


def _read():
    path = test_config.FAILED_JSON
    if not os.path.isfile(path):
        return {}
    try:
        with open(path, "r") as f:
            return json.load(f)
    except (json.JSONDecodeError, IOError):
        return {}


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
    nothing to retry"."""
    data = _read()
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
    with open(test_config.FAILED_JSON, "w") as f:
        json.dump(data, f, indent=2)


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
_extras = {}


def set_extras(name, *, cmd=None, host=None,
               compile_output=None, run_output=None):
    """Record extras for `name`.  Repeated calls merge — last write wins
    per field.  Empty string is treated as "no value" so a caller can
    safely pass cmd="" without overwriting a previously-recorded cmd."""
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
    return _extras.pop(name, {})


def clear_extras():
    """Reset the side channel.  Tests that re-run inside the same
    process (resume mode, --bisect smoke tests) call this between
    iterations so old entries don't leak forward."""
    _extras.clear()
