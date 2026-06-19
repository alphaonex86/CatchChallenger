"""
build_paths.py — shared "where do testing*.py drop their build artifacts" rule.

Every testing*.py used to materialise its build / staging dirs *inside* the
source tree (e.g. server/cli/build/testing-filedb*, client/qtopengl/build/…,
tools/map2png/build/…). That polluted /home/user/Desktop/CatchChallenger/working/
with *.o, moc_*, ui_*, qrc_*.cpp, CMakeFiles/, CMakeCache.txt, __pycache__/.

This module funnels every test-driven build to a per-operator scratch dir
(<tmpfs_root>/cc-build, see test_config.py / config.json) while keeping the
same sub-tree layout, so existing path math (e.g. relative include dirs,
datapack staging dirs anchored on the build dir) keeps working.

Public API:
  TMPFS_BUILD_ROOT       — absolute root, ends with "cc-build".
  build_path(*parts)     — absolute path under TMPFS_BUILD_ROOT joining the
                           given parts. Use the same path components you'd
                           have passed to os.path.join(ROOT, ...).
  src_to_build(src_dir, root, *tail)
                         — convenience: take a directory inside the source
                           tree and return its build counterpart under
                           TMPFS_BUILD_ROOT, optionally with extra tail
                           components appended. Equivalent to
                           build_path(os.path.relpath(src_dir, root), *tail).
  pycache_env(env=None)  — return an env dict that includes
                           PYTHONPYCACHEPREFIX pointing inside TMPFS_BUILD_ROOT
                           so subprocesses we spawn don't drop __pycache__/
                           dirs inside the source tree.
"""

import os
import test_config

TMPFS_BUILD_ROOT = test_config.TMPFS_BUILD_ROOT
PYCACHE_PREFIX = test_config.PYCACHE_DIR


def build_path(*parts):
    p = os.path.join(TMPFS_BUILD_ROOT, *parts)
    # Auto-register every build dir a testing*.py asks for, so the script
    # tears down its OWN tmpfs scratch on SUCCESSFUL exit (cleanup_helpers
    # keeps it on failure/signal for inspection). Without this a script that
    # forgets register_build_dir() leaks multi-GiB build trees into RAM —
    # which is exactly what happened (testingserver et al. never registered).
    # The harness cleans up itself; no manual sweep. Skip the root.
    if parts:
        _auto_register(p)
    return p


def src_to_build(src_dir, root, *tail):
    rel = os.path.relpath(src_dir, root)
    p = os.path.join(TMPFS_BUILD_ROOT, rel, *tail)
    _auto_register(p)
    return p


def _auto_register(path):
    """Mark `path` for removal-on-success via cleanup_helpers. Defensive:
    lazy import (avoid an import cycle), never raise, and never register the
    build root itself (only dirs strictly under it)."""
    try:
        ap = os.path.abspath(path)
        root = os.path.abspath(TMPFS_BUILD_ROOT)
        if ap == root or os.path.commonpath([ap, root]) != root:
            return
        import cleanup_helpers
        cleanup_helpers.register_build_dir(ap)
    except Exception:
        pass


def pycache_env(env=None):
    e = dict(env) if env is not None else os.environ.copy()
    e["PYTHONPYCACHEPREFIX"] = PYCACHE_PREFIX
    return e


def ensure_root():
    """Create TMPFS_BUILD_ROOT (and PYCACHE_PREFIX) if missing. Cheap to call
    from each testing*.py at import time."""
    os.makedirs(TMPFS_BUILD_ROOT, exist_ok=True)
    os.makedirs(PYCACHE_PREFIX, exist_ok=True)
