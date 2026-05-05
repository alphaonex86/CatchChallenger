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


def exec_node_supports(exec_node, diag, parent_compile_node=None):
    """True when the per-execution-node opt-in flag for the active
    diagnostic mode is set AND the parent compile node has the matching
    compiler available. Two opt-in flags gate diagnostic runs on exec
    nodes (declared in remote_nodes.json as mandatory keys, see
    remote_build._REQUIRED_EXEC_NODE_KEYS):
      sanitizer_gcc=true   → exec node will run --valgrind passes —
                              ONLY if its compile node has gcc in
                              `compilers`. valgrind needs a debug-
                              info-rich gcc-built binary.
      sanitizer_clang=true → exec node will run --sanitize passes —
                              ONLY if its compile node has clang in
                              `compilers`. asan/lsan/msan are clang
                              flags.
    Outside diagnostic mode (no --sanitize / --valgrind), this returns
    True so normal runs are unaffected. Both flags default to false on
    a fresh node so the operator opts in deliberately after checking
    the runtime has the required tooling.

    When `parent_compile_node` is omitted the function checks only the
    exec-side flag — for callers that already filtered by node_supports()
    on the compile side and don't want to re-do that work.
    """
    if is_sanitize(diag):
        if not bool(exec_node.get("sanitizer_clang", False)):
            return False
        if parent_compile_node is not None:
            return node_supports(parent_compile_node, diag)
        return True
    if is_valgrind(diag):
        if not bool(exec_node.get("sanitizer_gcc", False)):
            return False
        if parent_compile_node is not None:
            return node_supports(parent_compile_node, diag)
        return True
    return True


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


# ── last-shell-command tracking (shown by log_fail on failure) ────────────────
# Each testing*.py keeps its own run_cmd() wrapper around subprocess.run.
# We expose record_cmd() / last_cmd_lines() so every wrapper can stash
# (args, cwd) here and every log_fail() can pull it back out — without a
# shared `run_cmd` (the wrappers each build their own NICE_PREFIX, env,
# timeout, gdb wrapping, etc.). The state is process-local; sequentially
# overwriting is fine because failures are reported synchronously after
# the call that produced them.
_LAST_CMD = {"args": None, "cwd": None}


def record_cmd(args, cwd):
    """Stash (args, cwd) of the most recent shell call.
    Call from every run_cmd() wrapper before subprocess.run().

    'args' is anything subprocess.run accepts: list[str] (the common form
    in this repo) or a single shell string. Both render cleanly via str()
    + ' '.join.
    """
    _LAST_CMD["args"] = list(args) if isinstance(args, (list, tuple)) else args
    _LAST_CMD["cwd"] = cwd


def last_cmd_lines():
    """Return printable lines describing the most recent shell command,
    or [] if none has been recorded. Used by log_fail() to attach context
    after the failure detail line so the operator sees pwd + last command
    without rerunning. Two lines: cwd + the command itself."""
    if _LAST_CMD["args"] is None:
        return []
    args = _LAST_CMD["args"]
    if isinstance(args, list):
        # Quote tokens that contain whitespace so the line is paste-safe.
        rendered = []
        for a in args:
            s = str(a)
            if any(ch.isspace() for ch in s) or s == "":
                rendered.append("'" + s.replace("'", "'\\''") + "'")
            else:
                rendered.append(s)
        cmd_str = " ".join(rendered)
    else:
        cmd_str = str(args)
    cwd = _LAST_CMD["cwd"] if _LAST_CMD["cwd"] else os.getcwd()
    return [
        "  last shell command before fail (cwd=" + str(cwd) + "):",
        "  $ " + cmd_str,
    ]
