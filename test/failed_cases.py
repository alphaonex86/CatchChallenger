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


def make_detail(detail="", *, cmd=None, host=None, error=None):
    """Build a detail dict with only the populated fields."""
    d = {"detail": detail or ""}
    if cmd is not None:
        d["cmd"] = cmd
    if host is not None:
        d["host"] = host
    if error is not None:
        d["error"] = error
    return d
