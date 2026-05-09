#!/usr/bin/env python3
"""
testingfight.py — CatchChallenger fight-engine unit tests.

Builds the standalone test binary at test/fight/ (subclasses
CommonFightEngine via MockFightEngine, loads a real datapack via
CommonDatapack::parseDatapack(), then exercises:
   - isInFight / wild-monster lifecycle
   - random fight seed accounting
   - attack / useSkill / hpChange
   - tryEscape success + failure
   - tryCapture
   - KO detection / dropKOOtherMonster / healAllMonsters
   - addPlayerMonster / moveUp / moveDown / removeMonsterByPosition
   - static helpers (getStat, generateWildSkill)

Per the user's brief: no main-code changes — the test lives entirely
under test/fight/ and overrides CommonFightEngine via subclass. The
build dir lands under tmpfs as /mnt/data/perso/tmpfs/cc-build/test/fight/build/...
(actually wherever build_paths.build_path() routes, mirroring every other
testing*.py).

Each datapack listed in config.json:paths.datapacks is exercised in
turn; the binary's PASS/FAIL lines are reported back to the harness.
"""
import sys
sys.dont_write_bytecode = True

import os, sys, signal, subprocess, threading, multiprocessing, json
import shutil, time

import diagnostic
import build_paths
from cmd_helpers import clamp_local


build_paths.ensure_root()

DIAG = diagnostic.parse_diag_args()
_DIAG_SUFFIX = diagnostic.build_dir_suffix(DIAG)

_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
with open(_CONFIG_PATH, "r") as _f:
    _config = json.load(_f)

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

# Same two datapacks used everywhere else in the harness, plus any extra
# entries config.json:paths.datapacks lists.
DATAPACKS = []
_dpcfg = _config.get("paths", {}).get("datapacks") or []
i = 0
while i < len(_dpcfg):
    p = os.path.expanduser(_dpcfg[i])
    if os.path.isdir(p):
        DATAPACKS.append(p)
    i += 1
# Fall back to the two well-known absolute paths if config didn't supply them.
for fallback in (
        "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack",
        "/home/user/Desktop/CatchChallenger/datapack-pkmn"):
    if os.path.isdir(fallback) and fallback not in DATAPACKS:
        DATAPACKS.append(fallback)

PRO        = os.path.join(ROOT, "test/fight/testfightengine.pro")  # virtual
BUILD_DIR  = build_paths.build_path("test/fight/build/testing-fight" + _DIAG_SUFFIX)
BIN_NAME   = "testfightengine"

COMPILE_TIMEOUT = 600
RUN_TIMEOUT     = 120

NICE_PREFIX = ["nice", "-n", "19", "ionice", "-c", "3"]

C_GREEN  = "\033[92m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_RESET  = "\033[0m"

results = []
total_expected = [0]
_last_log_time = [time.monotonic()]

SCRIPT_NAME = os.path.basename(__file__)
import failed_cases as _fc


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    if failed_cases is None:
        return True
    return test_name in failed_cases


def save_failed_cases():
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")


def log_fail(name, detail=""):
    now = time.monotonic()
    elapsed = now - _last_log_time[0]
    _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} "
          f"{name}  {detail}  ({elapsed:.1f}s)")
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li])
        li += 1


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(NICE_PREFIX + list(args), cwd)
    try:
        p = subprocess.run(NICE_PREFIX + list(args), cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def build_project(pro_file, build_dir, label):
    import cmake_helpers as _ch
    return _ch.build_project(
        pro_file, build_dir, label,
        root=ROOT, nproc=NPROC,
        log_info=log_info, log_pass=log_pass, log_fail=log_fail,
        ensure_dir=ensure_dir, run_cmd=run_cmd,
        diag=DIAG, diag_module=diagnostic,
    )


def run_binary_for_datapack(datapack):
    binary = os.path.join(BUILD_DIR, BIN_NAME)
    if not os.path.isfile(binary):
        log_fail(f"run {os.path.basename(datapack)}",
                 f"binary missing: {binary}")
        return False
    env = os.environ.copy()
    timeout = diagnostic.scale_timeout(DIAG, RUN_TIMEOUT)
    wrapper = diagnostic.runtime_wrapper(DIAG)
    cmd = NICE_PREFIX + wrapper + [binary, datapack]
    diagnostic.record_cmd(cmd, BUILD_DIR)
    log_info(f"running: {BIN_NAME} {datapack}")
    try:
        p = subprocess.run(cmd, cwd=BUILD_DIR, env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=clamp_local(timeout))
    except subprocess.TimeoutExpired:
        log_fail(f"run {os.path.basename(datapack)}",
                 f"timeout after {timeout}s")
        return False
    out = p.stdout.decode(errors="replace")
    # Echo the binary's output so a human reading the harness log can
    # see what each scenario printed without re-running.
    for line in out.splitlines():
        print(f"  | {line}")
    # The binary prints "PASS <name>" / "FAIL <name>" lines; lift each
    # into the harness's pass/fail accounting under a per-datapack
    # namespace so failed.json stays readable.
    dp_label = os.path.basename(datapack)
    any_fail = False
    for raw in out.splitlines():
        line = raw.strip()
        if line.startswith("PASS "):
            rest = line[5:].split(" ", 1)
            name = rest[0]
            detail = rest[1] if len(rest) > 1 else ""
            log_pass(f"{dp_label}:{name}", detail)
        elif line.startswith("FAIL "):
            rest = line[5:].split(" ", 1)
            name = rest[0]
            detail = rest[1] if len(rest) > 1 else ""
            log_fail(f"{dp_label}:{name}", detail)
            any_fail = True
    if p.returncode != 0:
        log_fail(f"{dp_label}:exit",
                 f"binary exit code {p.returncode}")
        any_fail = True
    return not any_fail


def cleanup(*_args):
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


def main():
    print(f"\n{C_CYAN}{'='*60}")
    print("  CatchChallenger — Fight Engine Unit Tests")
    print(f"{'='*60}{C_RESET}\n")

    if not DATAPACKS:
        log_fail("setup", "no datapack found (config.json paths.datapacks "
                          "and well-known fallbacks both missing)")
        summary()
        return

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed (delete failed.json to re-run)")
        return

    # Build once.
    if not build_project(PRO, BUILD_DIR, "testfightengine"):
        summary()
        return

    di = 0
    while di < len(DATAPACKS):
        dp = DATAPACKS[di]
        di += 1
        print(f"\n{C_CYAN}--- datapack: {dp} ---{C_RESET}")
        run_binary_for_datapack(dp)

    summary()


def summary():
    print(f"\n{C_CYAN}{'='*60}")
    print("  Summary")
    print(f"{'='*60}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for name, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {name}  {detail}  ({elapsed:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    save_failed_cases()
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
