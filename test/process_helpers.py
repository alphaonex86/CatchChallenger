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
