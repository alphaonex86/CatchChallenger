"""
wall_cap.py — internal per-script wall-clock cap.

Every testing*.py imports this at the top. arm() installs a SIGALRM
handler that prints a clear TIMEOUT line and exits 124 (the same
code GNU `timeout` uses, so all.sh's run_test reports it as
[TIMEOUT]) when the cap fires. Cap values mirror the PER_TEST_TIMEOUT
table in test/all.sh so a direct `python3 testingX.py` invocation
gets the same wall-clock budget as `timeout Xm python3 testingX.py`.

Why: when an operator runs a testing*.py outside all.sh (e.g. while
iterating on a single failure), the external `timeout` wrapper is
absent and a hung script can spin forever. Internal arm() pins the
ceiling from the script side. The external `timeout` in all.sh
still acts as a backstop in case Python ignores the signal (e.g.
mid-system-call) — same kill-after grace period applies there.
"""

import os
import signal
import sys


# Seconds. Keep in sync with PER_TEST_TIMEOUT_MAP in test/all.sh.
# "fmt: <script>: <seconds>" — adding a new testing*.py? Update BOTH
# places.
_CAPS_SECONDS = {
    "testingbots.py":                 15 * 60,
    "testingbyIA.py":                 30 * 60,
    "testingclient.py":               30 * 60,
    "testingcluster.py":              10 * 60,
    "testingcmake.py":                30 * 60,
    "testingcompilationandroid.py":   15 * 60,
    "testingcompilationmac.py":       15 * 60,
    "testingcompilationwindows.py":   15 * 60,
    "testingfight.py":                15 * 60,
    "testinggateway.py":              15 * 60,
    "testinghttp.py":                 15 * 60,
    "testingmap2png.py":              15 * 60,
    "testingmap4client.py":           30 * 60,
    "testingmulti.py":                30 * 60,
    "testingqtserver.py":             15 * 60,
    "testingremote.py":               45 * 60,
    "testingserver.py":               30 * 60,
    "testingstats.py":                10 * 60,
    "testingtools.py":                15 * 60,
    "testingwebsocket.py":            30 * 60,
}
_DEFAULT_CAP_SECONDS = 30 * 60


def cap_for(script_name):
    """Look up the cap for a script basename (e.g.
    'testingcompilationwindows.py'). Falls back to the 30-minute
    default when the script isn't listed."""
    return _CAPS_SECONDS.get(script_name, _DEFAULT_CAP_SECONDS)


def _alarm_handler(_signo, _frame):
    cap = cap_for(os.path.basename(sys.argv[0]))
    sys.stderr.write(
        f"\n[wall_cap] TIMEOUT after {cap}s — exiting with code 124 "
        f"(same as GNU `timeout`) so all.sh tags this as "
        f"[TIMEOUT].\n"
    )
    sys.stderr.flush()
    # Exit 124 — what GNU `timeout` returns on cap. test/all.sh's
    # run_test() special-cases 124/137 as TIMEOUT.
    os._exit(124)


def arm(extra_seconds=0):
    """Arm the wall-clock cap for the CURRENT script. Idempotent —
    call once near module-top. Adds optional `extra_seconds` of
    slack on top of the table value (useful when a script knows it
    will spend a non-trivial chunk of its budget on setup that
    doesn't count against the matrix). Safe on platforms without
    SIGALRM (Windows): no-ops with a stderr note."""
    script = os.path.basename(sys.argv[0])
    secs = cap_for(script) + max(0, int(extra_seconds))
    if not hasattr(signal, "SIGALRM") or not hasattr(signal, "alarm"):
        sys.stderr.write(f"[wall_cap] SIGALRM unavailable on this "
                         f"platform; cap ({secs}s) NOT armed.\n")
        return
    try:
        signal.signal(signal.SIGALRM, _alarm_handler)
        signal.alarm(secs)
    except (OSError, ValueError) as e:
        sys.stderr.write(f"[wall_cap] failed to arm {secs}s cap: "
                         f"{e!r}\n")
