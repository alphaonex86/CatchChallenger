#!/usr/bin/env python3
"""
monitor.py — Background system + per-CC-process sampler.

all.sh starts this in the background at the very top of a test run
and kills it on exit. Every `--interval` seconds (default 60s) it
appends one JSONL record to <tmpfs>/monitor.json with:

  - cpu      : aggregate /proc/stat utilisation since the previous
               sample (user, system, iowait, idle %).
  - mem      : /proc/meminfo (MemTotal, MemAvailable, used, swap).
  - load     : /proc/loadavg (1, 5, 15-min).
  - net      : /proc/net/dev rx/tx bytes since the previous sample,
               keyed by interface.
  - testing  : the CURRENT testing*.py running on the host (parsed
               from `ps -o pid,etimes,cmd` looking for testing*.py).
  - cc_procs : per-process snapshot of every catchchallenger-* binary
               and every cc1plus / clang++ / cmake / make / ninja /
               python3 testing*.py — pid, rss_kib, %cpu (since
               previous sample), state, cmd[:200].

Cross-process safety: appends via O_APPEND + fcntl.flock so a
parallel testing*.py that also writes to monitor.json (none does
today, but cheap insurance) cannot interleave a partial record.

Pair with phase_timer.py's time.json: a slow phase whose monitor.json
samples show CPU < 30% for the whole window is an idle-host phase
that's wasting wall time waiting on something — the candidate for
parallelisation. A slow phase pinned at 100% on one core is a
single-threaded hot loop — needs algorithmic work, not concurrency.

Usage:
    python3 test/monitor.py &   # all.sh starts it like this
    ...                          # the test run
    kill <its-pid>               # all.sh kills it on exit

Exits cleanly on SIGTERM (atexit hook flushes a final record).

Self-contained: only stdlib + /proc reads. No psutil dependency
(can't `pip install` per CLAUDE.md feedback).
"""

import sys
sys.dont_write_bytecode = True

import argparse
import fcntl
import json
import os
import re
import signal
import sys
import time
from contextlib import suppress

# Bypass test_config when ~/.config is missing — the operator may
# launch monitor.py outside an all.sh wrapper, in which case we just
# fall back to a known-tmpfs default. test_config.TIME_JSON path
# format is honoured when available.
def _resolve_default_path():
    try:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import test_config
        return os.path.join(test_config.TMPFS_ROOT, "monitor.json")
    except Exception:
        return "/mnt/data/perso/tmpfs/monitor.json"


# ── per-source readers ────────────────────────────────────────────────

def _read_cpu_stat():
    """Return a dict with the aggregate cpu jiffies since boot.
    /proc/stat 'cpu  ...' line: user nice system idle iowait irq
    softirq steal guest guest_nice"""
    with open("/proc/stat") as f:
        head = f.readline()
    parts = head.split()
    if not parts or parts[0] != "cpu":
        return {}
    j = [int(x) for x in parts[1:]]
    while len(j) < 10:
        j.append(0)
    user, nice, system, idle, iowait, irq, softirq, steal = j[:8]
    total = sum(j)
    return {
        "user": user, "nice": nice, "system": system, "idle": idle,
        "iowait": iowait, "irq": irq, "softirq": softirq, "steal": steal,
        "total": total,
    }


def _cpu_delta(prev, cur):
    """Return per-bucket utilisation in % between two _read_cpu_stat
    snapshots. Defends against backward jiffies (suspend/resume)."""
    if not prev:
        return None
    dt = cur["total"] - prev["total"]
    if dt <= 0:
        return None
    out = {}
    for k in ("user", "nice", "system", "idle", "iowait", "irq",
              "softirq", "steal"):
        out[k] = round(100.0 * (cur[k] - prev[k]) / dt, 2)
    return out


def _read_meminfo():
    out = {}
    try:
        with open("/proc/meminfo") as f:
            for ln in f:
                k, _, v = ln.partition(":")
                v = v.strip().split()
                if not v:
                    continue
                # value is in kB for the rows we care about
                try:
                    out[k] = int(v[0])
                except ValueError:
                    pass
    except OSError:
        return {}
    used = out.get("MemTotal", 0) - out.get("MemAvailable", 0)
    out["MemUsed"] = used
    return {k: out[k] for k in (
        "MemTotal", "MemFree", "MemAvailable", "MemUsed",
        "Buffers", "Cached", "SwapTotal", "SwapFree"
    ) if k in out}


def _read_loadavg():
    try:
        with open("/proc/loadavg") as f:
            parts = f.read().split()
        return {"1m": float(parts[0]), "5m": float(parts[1]),
                "15m": float(parts[2])}
    except (OSError, ValueError, IndexError):
        return {}


def _read_netdev():
    """Return a dict of {iface: (rx_bytes, tx_bytes)} from /proc/net/dev."""
    out = {}
    try:
        with open("/proc/net/dev") as f:
            lines = f.readlines()
    except OSError:
        return out
    # skip 2 header lines
    for ln in lines[2:]:
        if ":" not in ln:
            continue
        name, rest = ln.split(":", 1)
        name = name.strip()
        # Skip loopback to keep noise down; can't carry production traffic.
        if name == "lo":
            continue
        parts = rest.split()
        if len(parts) < 16:
            continue
        rx_bytes = int(parts[0])
        tx_bytes = int(parts[8])
        out[name] = (rx_bytes, tx_bytes)
    return out


def _net_delta(prev, cur, dt_s):
    if not prev or dt_s <= 0:
        return {}
    out = {}
    for iface, (rx, tx) in cur.items():
        prx, ptx = prev.get(iface, (rx, tx))
        out[iface] = {
            "rx_bps": int((rx - prx) / dt_s),
            "tx_bps": int((tx - ptx) / dt_s),
        }
    return out


# ── per-process snapshot ──────────────────────────────────────────────

# Process filtering — strict CC-only.
#
# An operator's machine commonly runs unrelated compilations: a
# `gentoo update` (emerge cc1plus), an ultracopier rebuild on a Mac
# VM via SSH, a /usr/portage-like overlay sync, etc. None of those
# belong in monitor.json — they would skew the post-mortem of which
# CC phase was actually CPU-bound. Every match below requires BOTH
#
#   (a) a known CC-tooling binary in the cmdline / comm, and
#   (b) a CC-context token in the cmdline (a CC repo path, a
#       testing*.py entry point, an SSH endpoint listed in
#       remote_nodes.json, or a CC-binary install layout).
#
# CC binaries we ship are matched unconditionally — there is no
# legitimate reason a non-CC system would run `catchchallenger-*`.

# Tools/compilers/drivers that ALSO need CC-context in their cmdline.
# These are too generic to match alone (every sysadmin task involves
# some of them).
_TOOL_NAME_RE = re.compile(
    r"\b("
    # native compilers
    r"gcc|g\+\+|clang|clang\+\+|cc1|cc1plus|"
    r"ld|ld\.bfd|ld\.gold|ld\.lld|mold|"
    r"ar|ranlib|nm|strip|objcopy|as|"
    # build drivers
    r"cmake|ctest|ninja|gmake|make|"
    # diag tooling
    r"valgrind|memcheck-amd64-linux|memcheck-x86-linux|"
    # cross / packaging tooling
    r"x86_64-w64-mingw32-(?:gcc|g\+\+|ar|ld|ranlib|nm|strip|"
    r"windres|dlltool)|"
    r"osslsigncode|wine|wine64|wine-preloader|wineserver|"
    # remote tools
    r"ssh|scp|rsync|sshd|"
    # python (driving testing*.py)
    r"python3?"
    r")\b"
)

# CC binaries — match unconditionally. These names are unique to us.
_CC_BINARY_RE = re.compile(
    r"\b("
    r"catchchallenger[\w.+-]*|"
    r"server-filedb|server-cli-epoll|server-gui|server-master|"
    r"server-login|server-gateway|game-server-alone|"
    r"map2png|datapack-explorer-generator|filedb-converter|"
    r"qtcpu800x600|catchchallenger800x600"
    r")\b"
)

# CC-context — at least one of these must be in the cmdline for a
# generic compiler / make / ssh / python to count. Built dynamically
# at startup from:
#   - the canonical repo path tokens we ship in CLAUDE.md
#   - all SSH user@host endpoints declared in remote_nodes.json
def _build_cc_context_re():
    tokens = [
        # canonical repo / build tree paths (matched as substrings,
        # not anchored at /).
        r"CatchChallenger",
        r"cc-build",
        r"cc-datapack",
        r"catchchallenger",         # any tmpfs/cc-* path or git/CatchChallenger lowercased
        r"libqtcatchchallenger",
        r"libcatchchallenger",
        r"qtopengl",
        r"qtcpu800x600",
        r"server[/-]epoll",
        r"server[/-]base",
        r"server[/-]gateway",
        r"server[/-]login",
        r"server[/-]master",
        # known testing harness names
        r"testing[^/\s]*\.py",
        # cross-build slot for windows
        r"catchchallenger-mxe",
    ]
    # Pull SSH endpoint hosts (compile + exec nodes) from
    # remote_nodes.json so an outgoing ssh into a CC remote node
    # shows up, but ssh sessions to operator's other machines (mac
    # VM hosting ultracopier, gentoo update box, etc.) are dropped.
    try:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import test_config
        rn_path = test_config.REMOTE_NODES_JSON
        with open(rn_path) as f:
            rn = json.load(f)
        hosts = set()
        for n in rn.get("nodes", []):
            ssh = n.get("ssh") or {}
            h = ssh.get("host")
            if h:
                hosts.add(h)
            for e in n.get("execution_nodes", []):
                h = e.get("host")
                if h:
                    hosts.add(h)
        # Escape every host literal — IPv6 contains :, regex needs \: escape
        # only inside a class, but plain literal works in alternation. Sort
        # longest-first so a more-specific host wins over a prefix match.
        for h in sorted(hosts, key=len, reverse=True):
            tokens.append(re.escape(h))
    except Exception:
        # remote_nodes.json missing → still match repo-path tokens
        # locally; remote-node ssh tracking is best-effort.
        pass
    return re.compile("(" + "|".join(tokens) + ")")


_CC_CONTEXT_RE = _build_cc_context_re()
_TESTING_RE = re.compile(r"\btesting[^/\s]*\.py\b")


def _read_proc_stat(pid):
    """Return (utime, stime, rss_kib, comm, state) for /proc/<pid> or None."""
    try:
        with open(f"/proc/{pid}/stat") as f:
            data = f.read()
    except OSError:
        return None
    # comm is in parens, may contain spaces; use rsplit on ')' to avoid
    # the spaces-in-comm pitfall.
    pre, _, post = data.partition("(")
    comm, _, rest = post.rpartition(")")
    fields = rest.split()
    if len(fields) < 22:
        return None
    state = fields[0]
    # utime, stime are fields 14, 15 (1-indexed in proc(5)) → indices
    # 11, 12 in the post-')' split because the first 2 listed there
    # (state, ppid, ...) shift the count.
    try:
        utime = int(fields[11])
        stime = int(fields[12])
    except (ValueError, IndexError):
        return None
    rss_pages = 0
    try:
        rss_pages = int(fields[21])
    except (ValueError, IndexError):
        pass
    page_kib = os.sysconf("SC_PAGE_SIZE") // 1024
    rss_kib = rss_pages * page_kib
    return (utime, stime, rss_kib, comm, state)


def _read_cmdline(pid):
    try:
        with open(f"/proc/{pid}/cmdline", "rb") as f:
            raw = f.read()
    except OSError:
        return ""
    # cmdline is NUL-separated; trailing NUL.
    parts = raw.rstrip(b"\x00").split(b"\x00")
    return " ".join(p.decode("utf-8", errors="replace") for p in parts)


def _enumerate_pids():
    try:
        return [int(d) for d in os.listdir("/proc") if d.isdigit()]
    except OSError:
        return []


def _scan_cc_processes():
    """Return a list of dicts describing every CC-related process on
    the host right now. Each dict carries enough state to compute
    %CPU on the next sample."""
    out = []
    for pid in _enumerate_pids():
        st = _read_proc_stat(pid)
        if st is None:
            continue
        utime, stime, rss_kib, comm, state = st
        cmd = _read_cmdline(pid) or comm
        # Three independent ways a process counts as CC-related:
        #   1. CC binary unconditionally (server-cli-epoll, etc.).
        #   2. testing*.py entry point.
        #   3. Generic build / SSH tooling (gcc, make, ssh) PLUS a
        #      CC-context token in the cmdline (repo path, remote
        #      node host from remote_nodes.json). This stops
        #      operator-side gentoo `emerge`, an ssh session to a
        #      Mac VM compiling ultracopier, etc. from leaking in.
        if _CC_BINARY_RE.search(cmd) or _CC_BINARY_RE.search(comm):
            pass
        elif _TESTING_RE.search(cmd):
            pass
        elif (_TOOL_NAME_RE.search(cmd) or _TOOL_NAME_RE.search(comm)) \
                and _CC_CONTEXT_RE.search(cmd):
            pass
        else:
            continue
        out.append({
            "pid": pid,
            "comm": comm,
            "state": state,
            "rss_kib": rss_kib,
            "ut": utime,
            "st": stime,
            "cmd": cmd[:200],
        })
    return out


def _diff_cc_procs(prev, cur, dt_s, ticks_per_s):
    """Annotate `cur` entries with %cpu computed against `prev`."""
    if not prev or dt_s <= 0 or ticks_per_s <= 0:
        for c in cur:
            c["pct_cpu"] = None
        return cur
    pmap = {p["pid"]: p for p in prev}
    for c in cur:
        p = pmap.get(c["pid"])
        if p is None:
            c["pct_cpu"] = None
            continue
        d_jiff = (c["ut"] + c["st"]) - (p["ut"] + p["st"])
        if d_jiff < 0:
            c["pct_cpu"] = None
            continue
        c["pct_cpu"] = round(100.0 * d_jiff / (dt_s * ticks_per_s), 2)
    # Strip raw jiffy fields from the output (kept on input for the
    # next-iteration diff).
    return [{k: v for k, v in c.items() if k not in ("ut", "st")}
            for c in cur]


# ── main loop ─────────────────────────────────────────────────────────

_running = True


def _stop(signum, frame):
    global _running
    _running = False


def _append(path, rec):
    line = (json.dumps(rec, default=str) + "\n").encode(
        "utf-8", errors="replace")
    fd = os.open(path,
                 os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o644)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX)
        try:
            os.write(fd, line)
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
    finally:
        os.close(fd)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", default=_resolve_default_path(),
                   help="output JSONL path")
    p.add_argument("--interval", type=float, default=60.0,
                   help="seconds between samples (default 60)")
    p.add_argument("--first-delay", type=float, default=2.0,
                   help="seconds before the first sample (default 2)")
    args = p.parse_args()

    signal.signal(signal.SIGTERM, _stop)
    signal.signal(signal.SIGINT, _stop)

    ticks_per_s = os.sysconf("SC_CLK_TCK") or 100

    # Make sure parent dir exists (tmpfs root from all.sh).
    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)

    # Header record so a parser knows the interval / start time.
    _append(args.out, {
        "kind": "monitor_start",
        "ts": time.time(),
        "interval_s": args.interval,
        "ticks_per_s": ticks_per_s,
        "pid": os.getpid(),
    })

    prev_cpu = None
    prev_net = None
    prev_procs = []
    prev_t = None

    if args.first_delay > 0:
        deadline = time.monotonic() + args.first_delay
        while _running and time.monotonic() < deadline:
            time.sleep(0.5)

    while _running:
        t_now = time.monotonic()
        wall = time.time()
        cpu = _read_cpu_stat()
        mem = _read_meminfo()
        load = _read_loadavg()
        net = _read_netdev()
        cc = _scan_cc_processes()
        dt_s = (t_now - prev_t) if prev_t is not None else 0.0
        rec = {
            "kind": "sample",
            "ts": wall,
            "dt_s": round(dt_s, 3),
            "cpu_pct": _cpu_delta(prev_cpu, cpu),
            "mem_kib": mem,
            "load": load,
            "net_bps": _net_delta(prev_net, net, dt_s),
            "cc_procs": _diff_cc_procs(prev_procs, cc, dt_s, ticks_per_s),
        }
        # Don't emit the first sample's "deltas-against-nothing" row
        # — it would carry no CPU% and confuse the post-mortem. Wait
        # for two samples before we have meaningful deltas.
        if prev_t is not None:
            with suppress(OSError):
                _append(args.out, rec)
        prev_cpu = cpu
        prev_net = net
        prev_procs = cc
        prev_t = t_now

        # Sleep with quick wake-ups so SIGTERM is responsive.
        deadline = t_now + args.interval
        while _running and time.monotonic() < deadline:
            time.sleep(min(1.0, deadline - time.monotonic()))

    with suppress(OSError):
        _append(args.out, {
            "kind": "monitor_stop",
            "ts": time.time(),
            "pid": os.getpid(),
        })


if __name__ == "__main__":
    main()
