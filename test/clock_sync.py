"""clock_sync.py -- keep a fleet node's clock correct.

Several boards here have no RTC, or a dead RTC battery: they come up at
whatever their firmware defaults to. Measured on this fleet, in one run:
p1mmx was ~29.6 YEARS behind, geode ~18.5 years, rpi-4 ~91 days, rpi-3 ~61
days. Nothing on those boards ever corrects it -- they are diskless / minimal
rootfs with no NTP client running.

That is not cosmetic. Every file the harness rsyncs onto such a board lands
with an mtime far in its FUTURE, which is exactly what breaks incremental
staging and any mtime-based cache; ninja refuses to build ("manifest still
dirty after 100 tries") when the clock is wrong; and a build's own timestamps
become unusable for diagnosing anything.

So every python path that touches a node fixes the clock first: the benchmark
harness before it stages, the correctness harness before it builds, and
setup.py while provisioning. Cheap (one ssh round trip, skipped when the node
is within tolerance) and idempotent.

Persistence is deliberately NOT attempted beyond a best-effort `hwclock -w`:
a board with no RTC, or with a dead cell, cannot keep the time across a power
cycle whatever we write, and installing an NTP daemon is the operator's call
(project rule: never install packages on a node). Correcting at every contact
is what makes the fleet work unattended.

Correcting the clock is only half of it: files staged while it was wrong
keep their future mtime forever, so `repair_future_mtimes()` pulls those back
to now over the harness-owned directories. Both run together.

Callers pass their own ssh runner -- a callable(cmd, timeout) -> (rc, stdout)
-- so this module depends on neither harness.
"""
import time


TOLERANCE_S = 5
# A file in a tree the harness re-creates every run cannot legitimately be
# this old: on those, an ancient mtime means the clock was wrong when it was
# written, not that the file is genuinely from another decade.
DRIFT_OLD_DAYS = 365


def repair_drifted_mtimes(run_ssh, paths, label="", log=None,
                          old_days=None, now=None):
    """Pull any file whose mtime cannot be right back to now.

    Correcting the clock is only half the job: everything staged while it was
    wrong keeps its bogus date forever, and BOTH directions break the same
    "is this newer than that" decisions the toolchain lives on:

      * FUTURE  -- an artefact dated ahead of its source looks up to date, so
                   make/ninja/ccache skip the rebuild and the run measures the
                   PREVIOUS binary. Always repaired: a future mtime is never
                   legitimate.
      * ANCIENT -- a source dated decades back (a board that boots in 1996)
                   looks older than the artefact built from it, with the same
                   silent outcome, and it makes ninja loop on "manifest still
                   dirty". Repaired only when `old_days` is given, and only on
                   trees the harness re-creates every run (sources, build
                   dirs). NEVER pass it for a datapack cache: rsync -a keeps
                   the SOURCE mtimes there, so genuinely old files are correct
                   and touching them would force a full re-transfer.

    The comparison is done HERE, not on the node. A 32-bit board cannot even
    look at the dates that matter: p1mmx's `find -newer` dies with "Value too
    large for defined data type" on a file stamped past 2038, so a shell-side
    test silently skips exactly the pathological files. `find` with no
    predicate lists them (it never stats), `stat -c %Y` reads them, and
    `touch` fixes them -- so the node only enumerates and we decide.

    Only harness-owned directories are ever touched -- never the node's system
    tree. Best effort throughout: an unreadable path is skipped rather than
    failing the run. Returns the number of files repaired.

    (Paths containing a newline are skipped: the enumeration is line-based.
    Build trees and datapack caches do not have any, and a mis-parsed path
    must never turn into a touch of the wrong file.)"""
    paths = [p for p in (paths or []) if p]
    if not paths:
        return 0
    if now is None:
        now = int(time.time())
    future_floor = now + TOLERANCE_S
    old_floor = (now - int(old_days) * 86400) if old_days else None
    quoted = " ".join(_shq(p) for p in paths)
    # find with NO predicate does not stat, so it lists even the files whose
    # timestamp overflows the node's 32-bit time_t; stat then reports them.
    listing = (f'for p in {quoted}; do [ -e "$p" ] || continue; '
               f'find "$p" -print 2>/dev/null; done | '
               f'while IFS= read -r f; do stat -c "%Y %n" "$f" 2>/dev/null; done')
    try:
        rc, out = run_ssh(listing, 180)
    except Exception as e:                      # noqa: BLE001 - reported
        if log is not None:
            log(f"{label}: mtime scan failed: {e}")
        return 0
    bad = []
    for line in (out or "").splitlines():
        stamp, _sep, path = line.partition(" ")
        if not path:
            continue
        try:
            mtime = int(stamp)
        except ValueError:
            continue
        if mtime > future_floor or (old_floor is not None and mtime < old_floor):
            bad.append(path)
    if not bad:
        return 0
    # Chunked so the remote command line never overflows ARG_MAX.
    fixed = 0
    i = 0
    while i < len(bad):
        chunk = bad[i:i + 100]
        i += 100
        cmd = "touch -- " + " ".join(_shq(p) for p in chunk) + " 2>/dev/null"
        try:
            run_ssh(cmd, 60)
            fixed += len(chunk)
        except Exception:                       # noqa: BLE001 - best effort
            pass
    if fixed and log is not None:
        log(f"{label}: pulled {fixed} misdated file(s) back to now "
            f"(staged while the clock was wrong)")
    return fixed


def _shq(path):
    """POSIX single-quote a path for the remote shell."""
    return "'" + str(path).replace("'", "'\\''") + "'"


# Kept as the historical name; future-only repair.
def repair_future_mtimes(run_ssh, paths, label="", log=None):
    return repair_drifted_mtimes(run_ssh, paths, label=label, log=log)


def sync_clock(run_ssh, label="", tolerance_s=TOLERANCE_S, log=None,
               repair_paths=None, rebuildable_paths=None,
               old_days=DRIFT_OLD_DAYS):
    """Compare the node's clock with ours and set it when it differs.

    Returns (ok, detail):
      (True,  "in sync")        already within tolerance
      (True,  "corrected ...")  was off, and we set it
      (False, "<reason>")       unreachable, unreadable, or not settable
                                (an unprivileged login without sudo)

    Never raises: a clock we cannot fix must not take the run down with it --
    the caller keeps working with a known-bad clock rather than losing the
    measurement entirely."""
    try:
        local_ts = int(time.time())
    except Exception as e:                      # noqa: BLE001 - reported
        return False, f"local clock unreadable: {e}"
    try:
        rc, out = run_ssh("date +%s", 15)
    except Exception as e:                      # noqa: BLE001 - reported
        return False, f"clock probe failed: {e}"
    if rc != 0:
        return False, "clock probe failed (node unreachable?)"
    try:
        remote_ts = int((out or "").strip().split()[0])
    except (ValueError, IndexError, AttributeError):
        return False, f"unparseable `date +%s` output: {(out or '')[:40]!r}"
    diff = abs(local_ts - remote_ts)
    if diff <= tolerance_s:
        return True, "in sync"
    # sudo -n first (unprivileged login), then plain (root / CAP_SYS_TIME).
    rc, _out = run_ssh(f"sudo -n date -s '@{local_ts}' >/dev/null 2>&1", 15)
    if rc != 0:
        rc, _out = run_ssh(f"date -s '@{local_ts}' >/dev/null 2>&1", 15)
    if rc != 0:
        detail = (f"clock is {diff}s off and could not be set "
                  f"(no privilege to set the time?)")
        if log is not None:
            log(f"{label}: {detail}")
        return False, detail
    # Best effort only: writes the corrected time to the RTC when the board
    # has one. A board with no RTC (or a dead cell) fails here and that is
    # fine -- the running clock is already right, which is what the staging
    # and the build care about.
    run_ssh("hwclock -w >/dev/null 2>&1 || true", 15)
    detail = f"clock was {diff}s off, corrected to {local_ts}"
    if log is not None:
        log(f"{label}: {detail}")
    # The clock is right from here on, but whatever was staged while it was
    # wrong still carries a future mtime -- repair it in the same breath, so
    # no later step has to reason about a timeline that never happened.
    if repair_paths:
        repair_drifted_mtimes(run_ssh, repair_paths, label=label, log=log)
    if rebuildable_paths:
        repair_drifted_mtimes(run_ssh, rebuildable_paths, label=label,
                              log=log, old_days=old_days)
    return True, detail
