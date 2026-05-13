"""
cleanup_helpers.py — shared build-artifact cleanup for testing*.py.

The harness builds each binary into its own per-test build dir under
/mnt/data/perso/tmpfs/cc-build/. That tmpfs is RAM-backed; without
active cleanup it fills up after ~3 testingcompilation* + cluster
runs (saw 41 GiB across one all.sh in practice).

Two clean stages:

  prune_intermediate_build_artifacts(build_dir)
    Called immediately after a successful build. Drops every .o / .d
    / .rsp / .dwo style scratch file the compiler produced. The
    binaries (.exe / no-extension ELF / .so) AND the CMake state
    (CMakeCache.txt, CMakeFiles/ structure, build.ninja) are kept —
    so a subsequent incremental rebuild (e.g. when testing*.py is
    re-run with a different define) still works without paying full
    configure cost, while ~95% of the bulk on disk (the .o files)
    is freed.

  remove_build_dir(build_dir)
    Called when ALL test phases are done with build_dir — typically
    from an atexit / SIGTERM / SIGINT teardown registered via
    `register_build_dir(path)`. Wipes the subtree.

  cleanup_tmp_build_caches()
    Sweeps the host-/tmp scratch dirs the toolchain leaks into
    (`/tmp/cores`, `/tmp/sslcheck`, `/tmp/cc-test-mxe-cpu`,
    `/tmp/cc-test-mxe-sp`).

The module is self-contained: no third-party deps, safe to call when
the target dir is absent (no-op).
"""

import atexit
import os
import shutil
import signal
import sys

try:
    import test_config as _tc
    _TMPFS_ROOT = _tc.TMPFS_BUILD_ROOT.rstrip("/")
    if _TMPFS_ROOT.endswith("/cc-build"):
        _TMPFS_ROOT = _TMPFS_ROOT[: -len("/cc-build")]
except (ImportError, AttributeError):
    _TMPFS_ROOT = "/mnt/data/perso/tmpfs"


def promote_artifact(src_path, dst_name=None):
    """Copy a final shipping artifact (.exe / .msi / .dmg / .apk /
    .aab) from inside the build dir to the root of the tmpfs so it
    survives the build-dir teardown. After all.sh finishes the
    operator should see ONLY these promoted artifacts plus the
    bookkeeping JSON at the tmpfs root.

    Returns the destination path on success, or None when src is
    missing / unreadable."""
    if not src_path or not os.path.isfile(src_path):
        return None
    if dst_name is None:
        dst_name = os.path.basename(src_path)
    try:
        os.makedirs(_TMPFS_ROOT, exist_ok=True)
        dst = os.path.join(_TMPFS_ROOT, dst_name)
        shutil.copy2(src_path, dst)
        return dst
    except OSError:
        return None


# Suffixes that are pure compiler/linker scratch. Removing them does
# NOT invalidate CMake state — the next `cmake --build` re-creates
# them via ccache (per-TU cache hit, ~free) and re-links if needed.
_PRUNE_FILE_SUFFIXES = (
    ".o", ".obj",
    ".dwo",            # dwarf-fission split-debug objects
    ".gcno", ".gcda",  # gcov coverage scratch
    ".d",              # ninja/gcc dependency files
    ".ninja_dep",
    ".rsp",            # ninja link/compile response files
    ".manifest",
)


def prune_intermediate_build_artifacts(build_dir):
    """Drop *.o / *.d / *.rsp / ... scratch from build_dir, keep
    binaries AND CMake state (CMakeCache, CMakeFiles tree
    structure, build.ninja). Safe to call when build_dir is missing.
    Returns the count of files removed."""
    if not os.path.isdir(build_dir):
        return 0
    removed_files = 0
    # Files only — never recurse-delete a directory here, because
    # CMakeFiles/<target>.dir/ holds the dependency graph CMake
    # needs to drive an incremental rebuild. Removing the .o files
    # *inside* that tree is fine; the .dir itself stays.
    for root, _, files in os.walk(build_dir, topdown=False):
        for name in files:
            if _file_is_scratch(name):
                try:
                    os.remove(os.path.join(root, name))
                    removed_files += 1
                except OSError:
                    pass
    return removed_files


def _file_is_scratch(name):
    low = name.lower()
    si = 0
    while si < len(_PRUNE_FILE_SUFFIXES):
        if low.endswith(_PRUNE_FILE_SUFFIXES[si]):
            return True
        si += 1
    return False


def remove_build_dir(build_dir):
    """Recursively rm -rf build_dir. No-op when missing."""
    if not build_dir:
        return
    if not os.path.isdir(build_dir):
        return
    try:
        shutil.rmtree(build_dir, ignore_errors=True)
    except OSError:
        pass


# Host /tmp paths that the MXE wine smoke test, the openssl probe and
# the ASAN core-dump capture leak into. Wipe them at end-of-script so
# the host /tmp doesn't accumulate stale scratch across runs.
_TMP_CACHE_PATHS = (
    "/tmp/cores",
    "/tmp/sslcheck",
    "/tmp/cc-test-mxe-cpu",
    "/tmp/cc-test-mxe-sp",
)


def cleanup_tmp_build_caches():
    """Wipe /tmp/cores, /tmp/sslcheck, /tmp/cc-test-mxe-{cpu,sp}.
    Safe when missing."""
    pi = 0
    while pi < len(_TMP_CACHE_PATHS):
        path = _TMP_CACHE_PATHS[pi]
        if os.path.isdir(path):
            try:
                shutil.rmtree(path, ignore_errors=True)
            except OSError:
                pass
        pi += 1


# ── atexit / signal wiring ────────────────────────────────────────────
# Each testing*.py adds its build dirs to this set via
# `register_build_dir(path)`. On normal exit (atexit) AND on SIGTERM/
# SIGINT (signal handler), every registered dir is removed plus the
# /tmp scratch is swept. Idempotent — calling more than once is safe.

_registered_dirs = set()
_prev_sigterm = None
_prev_sigint = None
_already_armed = False
_run_successful = False


def register_build_dir(path):
    """Mark a build dir for removal when the current script exits
    *successfully*. See mark_run_successful() — without it, the
    registered dirs are KEPT at exit so the operator can inspect the
    failure."""
    if path:
        _registered_dirs.add(os.path.abspath(path))
    _arm_handlers()


def mark_run_successful():
    """Signal that the testing*.py finished without failures.

    Only when this state is set BEFORE the script exits do we tear
    down the registered build dirs in the atexit / signal handler.
    Otherwise we leave the dirs in place so the failure-investigation
    workflow has the binary + logs to look at. Idempotent.

    The state is also set automatically by the wrapped sys.exit when
    called with code 0 or no argument, AND by the atexit handler when
    no exception is in flight and no explicit failure was signalled —
    so most testing*.py scripts don't need to call this directly."""
    global _run_successful
    _run_successful = True


def mark_run_failed():
    """Explicitly mark the run as failed so the teardown KEEPS the
    build dirs around for post-mortem. Use this when a script wants
    to log a failure summary without raising."""
    global _run_successful
    _run_successful = False
    global _explicit_failure
    _explicit_failure = True


_explicit_failure = False


def _do_teardown():
    # Decide success vs failure:
    # - explicit failure flag set → keep dirs
    # - exception in flight       → keep dirs
    # - sys.exit() with non-zero  → keep dirs (the wrapped sys.exit
    #                               below clears _run_successful)
    # - otherwise                 → success → wipe dirs
    if _explicit_failure:
        return
    if sys.exc_info() != (None, None, None):
        return
    if not _run_successful:
        return
    di = 0
    snapshot = list(_registered_dirs)
    while di < len(snapshot):
        remove_build_dir(snapshot[di])
        di += 1
    cleanup_tmp_build_caches()


def _signal_teardown(signo, _frame):
    _do_teardown()
    # Re-raise the original signal so the parent's exit code is
    # correct (otherwise SIGTERM becomes exit 0 and downstream
    # tooling believes the test passed when it was actually killed).
    if signo == signal.SIGTERM and callable(_prev_sigterm):
        _prev_sigterm(signo, _frame)
    elif signo == signal.SIGINT and callable(_prev_sigint):
        _prev_sigint(signo, _frame)
    # Default fallback: re-raise.
    signal.signal(signo, signal.SIG_DFL)
    os.kill(os.getpid(), signo)


_orig_sys_exit = sys.exit


def _wrapped_sys_exit(code=0):
    # Numeric 0 / None / "" (posix success): leave both flags alone —
    # the run is considered successful by default unless explicitly
    # marked otherwise.
    # Numeric non-zero: explicit failure → keep dirs at teardown.
    global _explicit_failure
    if code is not None and code != 0 and code != "":
        _explicit_failure = True
    _orig_sys_exit(code)


def _arm_handlers():
    global _prev_sigterm, _prev_sigint, _already_armed, _run_successful
    if _already_armed:
        return
    _already_armed = True
    atexit.register(_do_teardown)
    # Wrap sys.exit so an explicit non-zero exit marks the run as
    # failed; a normal sys.exit(0) / fall-off-end leaves the success
    # flag at its True default and the dirs get torn down.
    sys.exit = _wrapped_sys_exit
    _run_successful = True
    try:
        _prev_sigterm = signal.signal(signal.SIGTERM, _signal_teardown)
    except (OSError, ValueError):
        _prev_sigterm = None
    try:
        _prev_sigint = signal.signal(signal.SIGINT, _signal_teardown)
    except (OSError, ValueError):
        _prev_sigint = None
