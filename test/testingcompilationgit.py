#!/usr/bin/env python3
"""
testingcompilationgit.py — fresh-checkout compile gate.

Clone the PUBLIC GitHub repo (shallow, recursive) into a tmpfs scratch
dir and build the client/ and server/ binaries from that PRISTINE tree,
then delete the clone so the multi-hundred-MB checkout doesn't linger in
tmpfs RAM.

Why a separate test: every other testing*.py (and testingcmake.py)
builds the LOCAL working tree, which can contain files that exist on
disk but were never `git add`-ed (or are .gitignore'd) — the local build
is green, yet `git clone` gives a tree that is MISSING that file and
fails to configure/compile. This script is the only check that the
PUBLISHED repo actually builds out of the box.

Builds (Debug, native gcc, warm ccache), the three the operator named:
  client/      -> catchchallenger            (qtopengl client)
  server/      -> catchchallenger-server-gui (Qt admin server)
  server/cli/  -> catchchallenger-server-cli (file-db epoll server)

Skip-as-pass (exit 0) when git is missing or the clone fails — a GitHub
hiccup is infra-unreachable, not a missing-file bug, and red CI on a
flaky network is worse than a noted skip (same convention as
testingcompilationmac/android self-skipping when their VPS is down). A
clone that SUCCEEDS but won't compile is a real FAIL.
"""

# Drop the .pyc cache for this process so importing the local helpers
# never lands a __pycache__/ dir in the source tree. Set before the
# first LOCAL import; stdlib bytecode is unaffected.
import sys
sys.dont_write_bytecode = True

import os, subprocess, multiprocessing, time, shutil
import diagnostic
import wall_cap
wall_cap.arm()
import build_paths
from cmd_helpers import clamp_local
import test_config

build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()

# Public, anonymous-clonable URL the operator gave. The local checkout
# uses an SSH remote; this HTTPS form needs no key so a fresh box can
# run the gate.
REPO_URL = "https://github.com/alphaonex86/CatchChallenger.git"

# RAM-backed scratch under the operator's tmpfs root (path comes from
# config.json so it never leaks into this public repo's git history; it
# resolves to <tmpfs_root>/git, e.g. /mnt/data/perso/tmpfs/git here).
# all.sh's pre-run sweep also wipes anything under tmpfs_root that isn't
# on its keep-list, so a clone left by a crashed run is reaped at the
# next start — belt-and-braces on top of the finally: cleanup below.
CLONE_DIR = os.path.join(test_config.TMPFS_ROOT, "git")

NPROC = str(multiprocessing.cpu_count())
NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

CLONE_TIMEOUT = 600   # network clone of source + vendored libs
COMPILE_TIMEOUT = 600

# The targets to build, as legacy .pro keys understood by
# cmake_helpers._PRO_TO_CMAKE (the file need not exist on disk — the
# driver only uses the key to look up the cmake target + source subdir).
TARGET_PRO_RELS = [
    "client/catchchallenger.pro",                  # client/  -> catchchallenger
    "server/server-gui.pro",                       # server/  -> catchchallenger-server-gui
    "server/cli/catchchallenger-server-filedb.pro",# server/cli/ -> catchchallenger-server-cli
]

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_YELLOW = "\033[93m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    """Persist failures (with the build_project side-channel extras —
    failing cmake command + full output) under SCRIPT_NAME."""
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT):
    """Build callback for cmake_helpers.build_project. Spawns under
    nice/ionice (compile policy) and points ccache's CCACHE_BASEDIR at
    the CLONE root so the fresh checkout's absolute paths rewrite to the
    same repo-relative keys the local builds used — identical source
    content then HITS the warm cache instead of cold cc1plus per TU
    (which would blow the wall cap on three full Qt builds)."""
    timeout = clamp_local(timeout)
    env = os.environ.copy()
    env["CCACHE_BASEDIR"] = CLONE_DIR
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(
            NICE_PREFIX + list(args), cwd=cwd, timeout=timeout, env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def ensure_dir(d):
    os.makedirs(d, exist_ok=True)


def clone_repo():
    """Shallow recursive clone of REPO_URL into CLONE_DIR. Returns
    True on success, False to SKIP-as-pass (git missing / network
    failure)."""
    if shutil.which("git") is None:
        log_info(f"{C_YELLOW}[SKIP]{C_RESET} git not on PATH — cannot test "
                 f"the published tree; skipping (counts as pass)")
        return False
    # git clone refuses a non-empty target; clear any leftover first.
    if os.path.exists(CLONE_DIR):
        shutil.rmtree(CLONE_DIR, ignore_errors=True)
    ensure_dir(os.path.dirname(CLONE_DIR))
    cmd = ["git", "clone", "--depth=1", "--recursive", REPO_URL, CLONE_DIR]
    log_info(f"cloning {REPO_URL} -> {CLONE_DIR} (depth=1, recursive)")
    with phase_timer.phase("git clone", log=log_info):
        try:
            p = subprocess.run(
                cmd, timeout=clamp_local(CLONE_TIMEOUT),
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            )
        except subprocess.TimeoutExpired:
            log_info(f"{C_YELLOW}[SKIP]{C_RESET} clone timed out "
                     f"(>{CLONE_TIMEOUT}s) — treating as network-unreachable; "
                     f"skipping (counts as pass)")
            return False
    if p.returncode != 0:
        tail = "\n    ".join(p.stdout.decode(errors="replace")
                             .rstrip().splitlines()[-8:])
        log_info(f"{C_YELLOW}[SKIP]{C_RESET} clone failed (rc={p.returncode}) "
                 f"— treating as network/infra-unreachable; skipping "
                 f"(counts as pass):\n    {tail}")
        return False
    # Sanity: the two dirs we are about to build MUST exist in the
    # clone. If git produced a tree without them the whole point of the
    # test is moot — surface it as a real failure, not a skip.
    missing = [d for d in ("client", "server")
               if not os.path.isdir(os.path.join(CLONE_DIR, d))]
    if missing:
        log_fail("git clone layout",
                 f"cloned tree is missing top-level dir(s): {missing}")
        return False
    return True


def build_target(pro_rel):
    """Configure + build one target from the CLONE, rooted at CLONE_DIR.
    Returns True on success."""
    import cmake_helpers as _ch
    pro_file = os.path.join(CLONE_DIR, pro_rel)
    suffix = diagnostic.build_dir_suffix(DIAG)
    safe = pro_rel.replace("/", "_")
    # Build dir lives under the tmpfs build root (auto-registered for
    # cleanup-on-success), NOT inside the clone — keeps the clone a
    # pristine source tree and lets the finally: rmtree the clone alone.
    build_dir = build_paths.build_path("compilation-git", safe + suffix)
    ensure_dir(build_dir)
    label = "git " + pro_rel + diagnostic.label_suffix(DIAG)
    return _ch.build_project(
        pro_file, build_dir, label,
        root=CLONE_DIR, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — fresh git-clone compile gate")
    print(f"{'='*60}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping "
                 "(delete failed.json for full re-run)")
        return

    if diagnostic.is_active(DIAG):
        log_info(f"diagnostic mode: {diagnostic.describe(DIAG)}")

    if not clone_repo():
        # clone_repo logged either a SKIP (no results appended → exit 0)
        # or a real FAIL (results has an entry → exit 1 below).
        save_failed_cases()
        if any(not r[1] for r in results):
            sys.exit(1)
        return

    for pro_rel in TARGET_PRO_RELS:
        name = "compile git " + pro_rel + diagnostic.label_suffix(DIAG)
        if should_run(name, failed_cases):
            build_target(pro_rel)
        print()

    # summary
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print(f"\n  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    finally:
        # Free the tmpfs RAM the clone held, pass or fail. The clone is a
        # dedicated dir we created under tmpfs_root, so removing it can
        # never touch the source tree or the datapack cache.
        if os.path.isdir(CLONE_DIR):
            shutil.rmtree(CLONE_DIR, ignore_errors=True)
