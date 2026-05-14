"""
process_helpers.py — shared subprocess.Popen preexec helpers.

`setsid_and_pdeathsig` puts the spawned child in its own session
(so `os.killpg` targets only this child's process tree) AND arms
prctl(PR_SET_PDEATHSIG, SIGTERM) so the kernel SIGTERMs the child
the moment its parent dies. Covers the SIGKILL-of-the-harness case
where no Python finalizer ever runs — without it, every
`timeout --kill-after=… python3 testingfoo.py` invocation that
trips its wall limit leaves the spawned children (catchchallenger
clients, server binaries, makensis sub-processes, wine64 etc.)
re-parented to PID 1 and burning CPU forever.

Use as `preexec_fn=process_helpers.setsid_and_pdeathsig` in every
subprocess.Popen call that launches a process we own. Cheap (one
prctl syscall per spawn) and safe (prctl is a no-op on non-Linux
kernels; we silently ignore the OSError).
"""

import ctypes
import os
import signal

_PR_SET_PDEATHSIG = 1
try:
    _libc = ctypes.CDLL("libc.so.6", use_errno=True)
except OSError:
    _libc = None


def setsid_and_pdeathsig():
    os.setsid()
    if _libc is not None:
        try:
            _libc.prctl(_PR_SET_PDEATHSIG, signal.SIGTERM, 0, 0, 0)
        except Exception:
            pass


# Exit codes that indicate an in-process abort() — both the raw
# Python -N signed form (subprocess.returncode is -SIGNUM for
# signal-killed children) and the 128+SIGNUM bash convention.
# SIGABRT is signal 6.
_SIGABRT_RC = {-6, 134}
_SIGSEGV_RC = {-11, 139}
_CRASH_RC = _SIGABRT_RC | _SIGSEGV_RC

# Stderr signature of the CATCHCHALLENGER_HARDENED-guarded abort in
# general/base/ProtocolParsingInput.cpp. Used to recognise a parse-
# layer crash separately from generic abort()s (e.g. C++ exceptions,
# assertion failures) so the harness can label the row PROTOCOL-PARSE-
# FAILURE and direct the operator at the right code path.
PROTOCOL_PARSE_FAILURE_MARKER = "error: the protocol parsing was wrong"


def rerun_under_gdb(argv, cwd=None, env=None, stdin_input=None,
                    timeout=120):
    """Re-run an already-aborted binary under `gdb -batch` to capture
    a thread-by-thread backtrace.

    Called by testing*.py from the post-mortem path: a server/client
    process exited with SIGABRT (rc in _SIGABRT_RC, almost always
    from CATCHCHALLENGER_HARDENED's abort() in
    ProtocolParsingInput.cpp), the harness has already collected the
    stderr message identifying which packet tripped the parser, and
    now needs the C++-side stack to localize the offset-mismatch /
    invariant breakdown to the exact line.

    Re-running is preferred over attaching gdb during the original
    spawn because:
      1. Attaching slows the process by 10-50× under valgrind/asan
         and shifts timing-dependent bugs out of reproducibility.
      2. abort() races the gdb attach — by the time the signal handler
         lands, the relevant stack is already partially unwound.
      3. Catch-by-batch gdb on a re-run reproduces deterministically
         when HARDENED is the cause (the same packet re-triggers the
         same parser branch).

    Returns the gdb stdout (full bt) as a str; on internal failure
    (gdb not installed, timeout) returns a one-line diagnostic
    string the caller can include in its [FAIL] detail.
    """
    import subprocess
    if not argv:
        return "rerun_under_gdb: empty argv"
    if not os.path.exists(argv[0]):
        return f"rerun_under_gdb: binary missing: {argv[0]}"
    gdb_argv = [
        "gdb", "-batch",
        "-ex", "set pagination off",
        "-ex", "handle SIGTERM nostop noprint pass",
        "-ex", "handle SIGPIPE nostop noprint pass",
        "-ex", "run",
        "-ex", "thread apply all bt 60",
        "-ex", "info registers",
        "--args",
    ] + list(argv)
    try:
        p = subprocess.run(
            gdb_argv,
            cwd=cwd,
            env=env,
            input=stdin_input,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
        out = p.stdout.decode(errors="replace") if p.stdout else ""
        return out
    except FileNotFoundError:
        return "rerun_under_gdb: gdb not installed on this host"
    except subprocess.TimeoutExpired:
        return f"rerun_under_gdb: timeout after {timeout}s"
    except OSError as exc:
        return f"rerun_under_gdb: OSError: {exc}"


def is_sigabrt(returncode):
    """Was this process killed by SIGABRT? Accepts both -6 (Python)
    and 134 (shell, 128+SIGABRT) conventions."""
    return returncode in _SIGABRT_RC


def is_crash(returncode):
    """Was this process killed by SIGABRT or SIGSEGV?"""
    return returncode in _CRASH_RC


def looks_like_protocol_parse_failure(stderr_text):
    """Cheap stderr grep — returns True when the binary aborted
    because CATCHCHALLENGER_HARDENED tripped on a parse* false return."""
    if not stderr_text:
        return False
    return PROTOCOL_PARSE_FAILURE_MARKER in stderr_text
