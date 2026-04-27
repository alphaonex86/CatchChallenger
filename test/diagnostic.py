"""
diagnostic.py — clang+sanitizer / gcc+valgrind helpers for testing*.py.

Two mutually-exclusive run modes that catch memory-safety / undefined-behavior
/ threading bugs at the cost of extra CPU. Both sit alongside the regular
build and propagate to remote nodes.

  --sanitize asan|lsan|msan      clang-built binary with sanitizer
                                 instrumentation (~2x slowdown). asan also
                                 turns on UBSan.
  --valgrind memcheck|helgrind|drd
                                 gcc-built debug binary wrapped in valgrind
                                 (10-50x slowdown).

The two modes cannot be combined — argparse rejects when both are set.
Per-node compiler availability is declared in remote_nodes.json under
"compilers": ["gcc"] / ["gcc", "clang"]. node_supports() filters nodes
that don't have the compiler the active diagnostic mode requires.

Public API used by testing*.py:

  parse_diag_args(argv)        parse --sanitize/--valgrind, return diag dict
  is_sanitize / is_valgrind    diagnostic-mode predicates
  scale_timeout(diag, t)       multiply per-test timeout by 10x for valgrind
  build_dir_suffix(diag)       directory tag, e.g. "-sanitize-asan"
  compiler_name(diag)          "clang" / "gcc" / None
  compiler_spec(diag)          "linux-clang" / "linux-g++" / None
  qmake_extra_args(diag)       extra qmake args (sanitizer flags)
  make_py_env(diag)            env additions for make*.py companions
  runtime_env(diag)            env additions for running an instrumented bin
  runtime_wrapper(diag)        argv prefix to wrap a binary with valgrind
  label_suffix(diag)           short tag appended to test labels
  node_supports(node, diag)    True iff the node has the required compiler
"""

import argparse
import os
import sys


SANITIZE_TOOLS = ("asan", "lsan", "msan")
VALGRIND_TOOLS = ("memcheck", "helgrind", "drd")


def parse_diag_args(argv=None):
    """Parse --sanitize / --valgrind from argv and return a config dict.

    Returns {} for the regular (non-diagnostic) run. Calls sys.exit on
    invalid combinations / values, mirroring argparse behaviour.
    """
    if argv is None:
        argv = sys.argv[1:]
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--sanitize", choices=SANITIZE_TOOLS, default=None)
    parser.add_argument("--valgrind", choices=VALGRIND_TOOLS, default=None)
    args, _rest = parser.parse_known_args(argv)
    if args.sanitize and args.valgrind:
        sys.stderr.write(
            "error: --sanitize and --valgrind are mutually exclusive "
            "(sanitizer instruments the binary; valgrind virtualises it)\n")
        sys.exit(2)
    if args.sanitize:
        return {"mode": "sanitize", "tool": args.sanitize}
    if args.valgrind:
        return {"mode": "valgrind", "tool": args.valgrind}
    return {}


def is_active(diag):
    return bool(diag) and diag.get("mode") in ("sanitize", "valgrind")


def is_sanitize(diag):
    return bool(diag) and diag.get("mode") == "sanitize"


def is_valgrind(diag):
    return bool(diag) and diag.get("mode") == "valgrind"


def timeout_mult(diag):
    """Valgrind imposes 10-50x slowdown — scale every per-test timeout 10x."""
    if is_valgrind(diag):
        return 10
    return 1


def scale_timeout(diag, seconds):
    if seconds is None:
        return None
    return int(seconds * timeout_mult(diag))


def build_dir_suffix(diag):
    """Returns a directory suffix to keep diagnostic builds separate so they
    don't fight with the regular build over object files."""
    if is_sanitize(diag):
        return "-sanitize-" + diag["tool"]
    if is_valgrind(diag):
        return "-valgrind-" + diag["tool"]
    return ""


def compiler_name(diag):
    """The compiler we *must* use for the active diagnostic mode.
    Sanitizer uses clang (broadest UBSan/MSan support); valgrind uses gcc.
    Returns None when no diagnostic is active (caller's default applies)."""
    if is_sanitize(diag):
        return "clang"
    if is_valgrind(diag):
        return "gcc"
    return None


def compiler_spec(diag):
    """The qmake -spec value for the diagnostic mode's compiler."""
    name = compiler_name(diag)
    if name == "clang":
        return "linux-clang"
    if name == "gcc":
        return "linux-g++"
    return None


def _sanitize_cxx_flags(tool):
    flags = ["-fno-omit-frame-pointer", "-fno-sanitize-recover=all", "-O1"]
    if tool == "asan":
        flags.append("-fsanitize=address,undefined")
    elif tool == "lsan":
        flags.append("-fsanitize=leak")
    elif tool == "msan":
        flags.append("-fsanitize=memory,undefined")
        flags.append("-fsanitize-memory-track-origins=2")
    return flags


def _sanitize_ld_flags(tool):
    if tool == "asan":
        return ["-fsanitize=address,undefined"]
    if tool == "lsan":
        return ["-fsanitize=leak"]
    if tool == "msan":
        return ["-fsanitize=memory,undefined"]
    return []


def qmake_extra_args(diag):
    """Extra qmake args (sanitizer compile + link flags). Empty list for
    valgrind: valgrind requires only the regular debug build."""
    if not is_sanitize(diag):
        return []
    cxx = _sanitize_cxx_flags(diag["tool"])
    ld = _sanitize_ld_flags(diag["tool"])
    args = []
    fi = 0
    while fi < len(cxx):
        f = cxx[fi]
        args.append("QMAKE_CXXFLAGS+=" + f)
        args.append("QMAKE_CFLAGS+=" + f)
        fi += 1
    fi = 0
    while fi < len(ld):
        f = ld[fi]
        args.append("QMAKE_LFLAGS+=" + f)
        args.append("LIBS+=" + f)
        fi += 1
    return args


def make_py_env(diag):
    """Env additions for invoking make*.py companion scripts.

    The companion scripts read COMPILER, EXTRA_DEFINES, USE_MOLD, BUILD_DIR.
    We force COMPILER to match the active diagnostic mode. Sanitizer flags
    flow through EXTRA_CXXFLAGS/EXTRA_LDFLAGS — companion scripts that have
    not been extended to read those vars simply ignore them; the qmake
    build path is the primary target for sanitizer/valgrind anyway.
    """
    env = {}
    name = compiler_name(diag)
    if name:
        env["COMPILER"] = name
    if is_sanitize(diag):
        env["EXTRA_CXXFLAGS"] = " ".join(_sanitize_cxx_flags(diag["tool"]))
        env["EXTRA_LDFLAGS"] = " ".join(_sanitize_ld_flags(diag["tool"]))
    return env


def runtime_env(diag):
    """Env additions for running an instrumented binary so it aborts on the
    first finding instead of carrying on past the bug."""
    env = {}
    if is_sanitize(diag):
        common_opts = "halt_on_error=1:abort_on_error=1:print_stacktrace=1"
        if diag["tool"] == "asan":
            env["ASAN_OPTIONS"] = common_opts
            env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        elif diag["tool"] == "lsan":
            env["LSAN_OPTIONS"] = "exitcode=23"
        elif diag["tool"] == "msan":
            env["MSAN_OPTIONS"] = common_opts
            env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    return env


def runtime_wrapper(diag):
    """Argv prefix to wrap a binary with valgrind. [] when not in valgrind
    mode (sanitizer-instrumented binaries run unwrapped)."""
    if not is_valgrind(diag):
        return []
    base = ["valgrind",
            "--error-exitcode=23",
            "--child-silent-after-fork=yes"]
    tool = diag["tool"]
    if tool == "memcheck":
        return base + [
            "--tool=memcheck",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--track-origins=yes",
            "--errors-for-leak-kinds=definite,possible",
        ]
    if tool == "helgrind":
        return base + ["--tool=helgrind"]
    if tool == "drd":
        return base + ["--tool=drd"]
    return base


def label_suffix(diag):
    """Short suffix appended to test labels so the PASS/FAIL log is
    distinguishable per diagnostic run."""
    if is_sanitize(diag):
        return " [sanitize:" + diag["tool"] + "]"
    if is_valgrind(diag):
        return " [valgrind:" + diag["tool"] + "]"
    return ""


def node_supports(node, diag):
    """True when the per-node `compilers` list contains the compiler the
    active diagnostic mode needs (gcc for valgrind, clang for sanitize).
    Treats missing/empty `compilers` as ["gcc"] for backwards compat.
    """
    needed = compiler_name(diag)
    if needed is None:
        return True
    raw = node.get("compilers")
    if not raw:
        compilers = ["gcc"]
    else:
        compilers = list(raw)
    return needed in compilers


def forward_args(diag):
    """The CLI flags used to reproduce the active diagnostic mode. Used by
    all.sh-style orchestrators that re-invoke other testing*.py scripts."""
    if is_sanitize(diag):
        return ["--sanitize", diag["tool"]]
    if is_valgrind(diag):
        return ["--valgrind", diag["tool"]]
    return []


def describe(diag):
    """Human-readable description for the run banner."""
    if is_sanitize(diag):
        return "clang + sanitizer (" + diag["tool"] + ")"
    if is_valgrind(diag):
        return "gcc + valgrind (" + diag["tool"] + "), per-test timeouts x10"
    return "regular run"
