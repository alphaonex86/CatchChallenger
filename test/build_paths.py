"""
build_paths.py — shared "where do testing*.py drop their build artifacts" rule.

Every testing*.py used to materialise its build / staging dirs *inside* the
source tree (e.g. server/epoll/build/testing-filedb*, client/qtopengl/build/…,
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
    return os.path.join(TMPFS_BUILD_ROOT, *parts)


def src_to_build(src_dir, root, *tail):
    rel = os.path.relpath(src_dir, root)
    return os.path.join(TMPFS_BUILD_ROOT, rel, *tail)


def pycache_env(env=None):
    e = dict(env) if env is not None else os.environ.copy()
    e["PYTHONPYCACHEPREFIX"] = PYCACHE_PREFIX
    return e


def ensure_root():
    """Create TMPFS_BUILD_ROOT (and PYCACHE_PREFIX) if missing. Cheap to call
    from each testing*.py at import time."""
    os.makedirs(TMPFS_BUILD_ROOT, exist_ok=True)
    os.makedirs(PYCACHE_PREFIX, exist_ok=True)
