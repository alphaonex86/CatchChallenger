"""benchmark_helpers.py — shared helpers used by every benchmarkXXXX.py.

Per benchmark/CLAUDE.md the harness must:
  * pick exec_nodes dynamically per batch (loadavg<1.0 gate),
  * run several profilers (perf-stat, /usr/bin/time -v, rusage, callgrind,
    binary-size) per cell,
  * print a one-line progress update per cell:
        [bench] D/T done -- profiler: X, SIMD: Y, execution_node: Z
  * compare every metric against champion.json with the strict
    KEEP/DISCARD/ESCALATE matrix.

This module groups the bits every benchmark Python script reuses; each
benchmarkXXX.py keeps its workload-specific bits (build, run, parse).
"""
import fcntl
import json
import os
import random
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

REPO_ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BENCH_ROOT = os.path.join(REPO_ROOT, "benchmark")
RESULTS    = os.path.join(BENCH_ROOT, "results")
REMOTE_NODES_JSON = os.path.join(os.path.dirname(REPO_ROOT), "remote_nodes.json")

# Sidecar file for the "local" host (no entry in remote_nodes.json). Same
# semantics as the per-execution_node benchmark_disabled_tools field; lives
# under ~/.cache so it survives but doesn't pollute the repo.
LOCAL_DISABLED_TOOLS_JSON = os.path.join(
    os.path.expanduser("~"),
    ".cache", "catchchallenger-benchmark", "local_disabled_tools.json",
)

# Lab storage: persistent {container-key -> MAC} so a started LXC guest
# keeps the SAME MAC across every run (stable ARP/NDP, no DHCP churn).
# Same ~/.cache convention -- survives, never pollutes the repo, never
# touches remote_nodes.json.
LAB_MACS_JSON = os.path.join(
    os.path.expanduser("~"),
    ".cache", "catchchallenger-benchmark", "lab_macs.json",
)

# Map profiler name (as used inside benchmark*.py) -> list of CLI tools
# that must be present on the runtime for that profiler to function. A
# profiler is considered "available" when EVERY required tool resolves.
# Keep this in sync with the documented benchmark_disabled_tools values
# in remote_nodes.json's _doc block.
PROFILER_REQUIRED_TOOLS = {
    "rusage":      [],   # bash is always available; /usr/bin/time is preferred but optional
    "perf-stat":   ["perf"],
    "perf-record": ["perf"],
    "callgrind":   ["valgrind", "callgrind_annotate"],
    "cachegrind":  ["valgrind", "cg_annotate"],
    "heaptrack":   ["heaptrack"],
    "binary-size": [],   # always available (just os.path.getsize)
}

C_GREEN  = "\033[92m"
C_YELLOW = "\033[93m"
C_RED    = "\033[91m"
C_CYAN   = "\033[96m"
C_BLUE   = "\033[94m"
C_BOLD   = "\033[1m"
C_RESET  = "\033[0m"


def log_stamp():
    """`H:M d/m/y` wall-clock prefix for live progress / remote-dispatch
    lines, so a multi-hour unattended batch log shows WHEN each cell ran.
    Kept outside the colour codes so the line stays greppable."""
    return time.strftime("%H:%M %d/%m/%y")


# ---- --node filter -------------------------------------------------------
# Optional per-invocation restriction to specific execution nodes. None =
# run every benchmark-enabled node (the default). Set once from each
# benchmark's main() out of parse_bench_args().node. A node matches when its
# execution_nodes[].label OR its arch equals one of the tokens; the literal
# token 'local' keeps the host baseline.
_NODE_FILTER = None


def set_node_filter(tokens):
    """Restrict benchmark runs to the named nodes (list of label/arch strings,
    or 'local'); None/empty restores the default 'every node'."""
    global _NODE_FILTER
    _NODE_FILTER = set(t.lower() for t in tokens) if tokens else None


def node_allowed(label, arch):
    """True when (label, arch) passes the active --node filter. No filter ->
    everything allowed; otherwise the node's label OR arch must match a
    token."""
    if _NODE_FILTER is None:
        return True
    return bool({str(label).lower(), str(arch).lower()} & _NODE_FILTER)


def node_filter_active():
    """True when a --node filter restricts this run to a subset of the fleet.
    The KEEP/DISCARD/ESCALATE decision + champion promotion need the WHOLE
    fleet, so callers must skip them when this is True (a partial run can't
    confirm a change helps/regresses everywhere)."""
    return _NODE_FILTER is not None


def rerun_command():
    """The shell command that re-runs JUST this benchmark, echoed in every
    error banner so the operator can reproduce a single failing benchmark
    without re-running the whole fleet.  Derived from sys.argv (the script
    path + the flags it was launched with -- only --comment is supported)."""
    script = sys.argv[0] or "benchmark*.py"
    parts = ["python3", script] + sys.argv[1:]
    return " ".join(shlex.quote(p) for p in parts)


def _error_banner(colour, headline, detail_sout=None, detail_serr=None):
    """Shared error/skip banner: a coloured headline + the captured output +
    the re-run command.  Always flushed to stderr so it survives a `tee`."""
    bar = "=" * 72
    print(f"{colour}{bar}{C_RESET}", file=sys.stderr)
    print(f"{colour}{headline}{C_RESET}", file=sys.stderr)
    print(f"{colour}{bar}{C_RESET}", file=sys.stderr)
    if detail_sout and detail_sout.strip():
        print(detail_sout)
    if detail_serr and detail_serr.strip():
        print(detail_serr, file=sys.stderr)
    print(f"{C_CYAN}re-run just this benchmark:{C_RESET} {rerun_command()}",
          file=sys.stderr)
    print(f"{colour}{bar}{C_RESET}", file=sys.stderr, flush=True)


def print_local_build_error(label, stage, sout, serr):
    """Print the captured cmake/compiler output of a FAILED local build under
    a clear banner.  A local compile failure aborts the whole benchmark*.py
    (the caller returns non-zero), so the operator must see the actual error
    here rather than a bare '[build] FAILED'.  `stage` is e.g. 'cmake
    configure' / 'cmake build'; sout/serr are the captured streams."""
    _error_banner(
        C_RED,
        f"[{label}] LOCAL {stage} FAILED — aborting benchmark "
        f"(compilation error below)",
        sout, serr)


def print_node_error(benchmark, node_label, status, detail):
    """Print the FULL error of a remote/infra failure on ONE exec node under a
    banner (the live progress line only carries a truncated reason).  `status`
    is 'SKIP' (infra: compile/push/bring-up/server-launch — node unmeasured,
    not a regression) or 'FAIL' (benchmark ran but produced bad data).  The
    fleet continues on other nodes; the banner shows the actual cause + the
    re-run command for this benchmark."""
    colour = C_YELLOW if status == "SKIP" else C_RED
    _error_banner(
        colour,
        f"[{benchmark}] {node_label} [{status}]",
        detail_serr=detail)


# ---- exec node enumeration -----------------------------------------------

def host_arch():
    """Return a coarse arch label matching remote_nodes.json convention."""
    m = os.uname().machine
    if m in ("x86_64", "amd64"):
        return "amd64"
    if m in ("aarch64", "arm64"):
        return "arm64"
    if m.startswith("arm"):
        return "armv7"
    if m.startswith("mips"):
        return "mips2r2"
    if m.startswith("riscv"):
        return "riscv64"
    if m in ("i386", "i486", "i586", "i686"):
        return "i686"
    return m


def load_remote_nodes():
    if not os.path.isfile(REMOTE_NODES_JSON):
        return {"nodes": []}
    try:
        with open(REMOTE_NODES_JSON) as f:
            return json.load(f)
    except Exception as e:
        print(f"[bench] WARN: cannot read remote_nodes.json: {e}", file=sys.stderr)
        return {"nodes": []}


def benchmark_exec_nodes():
    """Return the list of execution_nodes whose `benchmark: true` AND whose
    1-min loadavg is < 1.0 right now. Built per call, never cached.

    Each entry: {"label","ssh_host","ssh_user","ssh_port","work_dir",
                 "arch","compile_node","has_gui","client_run_mode",
                 "disabled_tools","ninja"}
    """
    cfg = load_remote_nodes()
    out = []
    for node in cfg.get("nodes", []):
        if not node.get("enabled", False):
            continue
        for ex in node.get("execution_nodes", []):
            if not ex.get("enabled", False):
                continue
            if not ex.get("benchmark", False):
                continue
            # --node filter: skip non-matching nodes BEFORE the loadavg SSH
            # probe so a narrowed run doesn't waste time reaching boxes it
            # won't use.
            if not node_allowed(ex.get("label"), node.get("arch")):
                continue
            check_la = ex.get("checkloadaverage", True)
            if check_la:
                la = _ssh_loadavg(ex)
                if la is None:
                    print(f"[bench] skip exec_node={ex.get('label')} -- loadavg unreachable"
                          f" (checkloadaverage=true, threshold<1.0)", file=sys.stderr)
                    continue
                if la >= 1.0:
                    print(f"[bench] skip exec_node={ex.get('label')} -- loadavg={la:.2f}"
                          f" >= 1.0 threshold (checkloadaverage=true)", file=sys.stderr)
                    continue
            out.append({
                "label":          ex.get("label"),
                "ssh_host":       ex.get("host"),
                "ssh_user":       ex.get("user"),
                "ssh_port":       ex.get("port", 22),
                "work_dir":       ex.get("work_dir"),
                "arch":           node.get("arch"),
                "compile_node":   node,
                "has_gui":        ex.get("has_gui", False),
                "client_run_mode": ex.get("client_run_mode", "none"),
                "disabled_tools": list(ex.get("benchmark_disabled_tools", [])),
                "ninja":          ex.get("ninja"),  # None -> auto-detect, True/False -> override
                # Per-CPU compile flags: the compile node builds this node's
                # binary WITH them before pushing (see exec_node_flag_defs).
                # Absent -> the build is unchanged / generic.
                "cflags":         ex.get("cflags"),
                "cxxflags":       ex.get("cxxflags"),
                "ldflags":        ex.get("ldflags"),
                # lxc_nfs (diskless NFS-LXC bring-up) must reach the
                # benchmark's exec_node dict so the harness can stop/mount/
                # lxc-start the container and then swap rsync/ssh to the
                # guest address. Absent on ordinary nodes -> stays None.
                "lxc_nfs":        ex.get("lxc_nfs"),
            })
    return out


# ---- ssh failure classification -----------------------------------------
# A dead node (port closed / unreachable) and a node that is UP but
# rejects our key look identical to a naive caller -- both just "ssh
# failed" -> node silently skipped. The second case is an operator
# misconfig the benchmark MUST shout about, not hide.

_SSH_DOWN_SIGNS = (
    "connection refused", "no route to host", "network is unreachable",
    "connection timed out", "operation timed out", "ssh timeout after",
    "could not resolve hostname", "name or service not known",
    "temporary failure in name resolution",
)
_SSH_AUTHKEY_SIGNS = (
    "permission denied (publickey", "permission denied (",
    "permission denied,", "permission denied.",
    "too many authentication failures", "host key verification failed",
    "remote host identification has changed", "no mutual signature algorithm",
    "no matching host key type", "authentication failed",
    "userauth", "connection closed by ",
)

_ssh_warned = set()   # dedupe: one banner per node per process


def classify_ssh_error(rc, stderr):
    """'ok' (rc==0) | 'down' (host/port unreachable -- a genuinely
    offline node) | 'authkey' (sshd IS up and the port IS open but our
    SSH key/auth could not be used -- the fixable case) | 'unknown'."""
    if rc == 0:
        return "ok"
    s = (stderr or "").lower()
    if any(x in s for x in _SSH_DOWN_SIGNS):
        return "down"
    if any(x in s for x in _SSH_AUTHKEY_SIGNS):
        return "authkey"
    return "unknown"


def warn_ssh_key_failure(node_label, host, stderr):
    """Big, hard-to-miss banner: ssh reachable but the KEY/auth is the
    problem. Deduped per node so the many probes (loadavg, tool checks,
    collect) don't spam it. Returns True when it classified as authkey
    (so callers can branch)."""
    key = str(node_label or host)
    if key in _ssh_warned:
        return True
    _ssh_warned.add(key)
    tail = " | ".join(l.strip() for l in (stderr or "").splitlines()
                      if l.strip())[:300] or "(no stderr captured)"
    bar = "=" * 74
    print(f"\n{C_RED}{bar}\n"
          f"  !!! SSH KEY / AUTH FAILURE  --  node {node_label!r} ({host})\n"
          f"  sshd IS running and port IS open, but the SSH key could NOT\n"
          f"  be used (publickey denied / host-key changed / key not in\n"
          f"  authorized_keys / agent not forwarded). This is NOT a dead\n"
          f"  node -- it is a fixable misconfig. The node is SKIPPED and\n"
          f"  its benchmark data is MISSING from this batch until fixed.\n"
          f"  ssh said: {tail}\n"
          f"{bar}{C_RESET}", file=sys.stderr, flush=True)
    return True


def note_ssh_failure(node_label, host, rc, stderr):
    """Classify an ssh result and fire the banner on the authkey case.
    Call from every ssh wrapper's failure path. No-op when rc==0 or the
    node is merely down."""
    if stderr and "Host key verification failed" in stderr:
        return  # StrictHostKeyChecking=no is set everywhere; this is harmless noise
    if classify_ssh_error(rc, stderr) == "authkey":
        warn_ssh_key_failure(node_label, host, stderr)


# ---- lab storage: stable per-container MAC ------------------------------
# When an LXC guest is (re)started and no MAC is saved for it, mint a
# "half-random" MAC -- fixed 00:16:3e LXC half + 3 random octets --
# loop until it collides with NOTHING the orchestrator already knows
# (IPv4 ARP + IPv6 NDP neighbour caches, local link MACs, and other
# MACs already in lab storage), then persist it. Every later start of
# that container reuses the SAME MAC.

_MAC_OUI = "00:16:3e"            # the conventional LXC half


def _lab_macs_load():
    try:
        with open(LAB_MACS_JSON) as f:
            d = json.load(f)
            return d if isinstance(d, dict) else {}
    except Exception:
        return {}


def _lab_macs_save(d):
    os.makedirs(os.path.dirname(LAB_MACS_JSON), exist_ok=True)
    tmp = LAB_MACS_JSON + ".tmp"
    with open(tmp, "w") as f:
        json.dump(d, f, indent=2, sort_keys=True)
    os.replace(tmp, LAB_MACS_JSON)


_MAC_RE = re.compile(r"([0-9a-f]{2}(?::[0-9a-f]{2}){5})", re.I)


def known_macs_local():
    """Every MAC the orchestrator host can see: IPv4 ARP + IPv6 NDP
    neighbour caches plus local link addresses. Used so a freshly
    minted guest MAC can't clash with anything already on the lab
    network. Best-effort -- missing `ip` just yields a smaller set."""
    seen = set()
    for cmd in (["ip", "-4", "neigh", "show"],
                ["ip", "-6", "neigh", "show"],
                ["ip", "link", "show"]):
        try:
            out = subprocess.run(cmd, capture_output=True, text=True,
                                  timeout=5).stdout
        except Exception:
            continue
        for m in _MAC_RE.findall(out):
            seen.add(m.lower())
    return seen


def lab_mac_for(key):
    """Return the persisted MAC for `key` (an LXC container / exec-node
    label), minting + saving a fresh collision-free one the first time.
    Stable forever after: same key -> same MAC across every run."""
    store = _lab_macs_load()
    cur = store.get(key)
    if cur:
        return cur
    taken = known_macs_local() | {v.lower() for v in store.values()}
    mac = None
    for _ in range(10000):
        cand = "%s:%02x:%02x:%02x" % (_MAC_OUI, random.randint(0, 255),
                                      random.randint(0, 255),
                                      random.randint(0, 255))
        if cand.lower() not in taken:
            mac = cand
            break
    if mac is None:                       # 2^24 space exhausted -> absurd
        mac = "%s:%02x:%02x:%02x" % (_MAC_OUI, random.randint(0, 255),
                                     random.randint(0, 255),
                                     random.randint(0, 255))
    store[key] = mac
    _lab_macs_save(store)
    print(f"[lab] minted stable MAC {mac} for container {key!r} "
          f"-> {LAB_MACS_JSON}", file=sys.stderr)
    return mac


# ---- runtime tool detection ---------------------------------------------

def _local_disabled_tools_load():
    p = LOCAL_DISABLED_TOOLS_JSON
    if not os.path.isfile(p):
        return []
    try:
        with open(p) as f:
            return list(json.load(f) or [])
    except Exception:
        return []


def _local_disabled_tools_save(tools):
    p = LOCAL_DISABLED_TOOLS_JSON
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w") as f:
        json.dump(sorted(set(tools)), f, indent=2, sort_keys=True)


def _remote_have_tool(ssh_host, ssh_user, ssh_port, tool):
    """Return True/False/None when SSH unreachable. Uses `command -v`
    rather than `which` because the latter is missing on some minimal
    Alpine/musl exec nodes."""
    try:
        argv = ["ssh", "-o", "ConnectTimeout=4", "-o", "BatchMode=yes",
                "-o", "StrictHostKeyChecking=no",
                "-o", "UserKnownHostsFile=/dev/null",
                "-o", "LogLevel=ERROR",
                "-p", str(ssh_port), f"{ssh_user}@{ssh_host}",
                f"command -v {tool} >/dev/null 2>&1 && echo Y || echo N"]
        p = subprocess.run(argv, capture_output=True, text=True, timeout=10)
        if p.returncode != 0:
            note_ssh_failure(ssh_host, ssh_host, p.returncode, p.stderr)
            return None
        return p.stdout.strip() == "Y"
    except Exception:
        return None


def _local_have_tool(tool):
    if tool.startswith("/"):
        return os.path.exists(tool) and os.access(tool, os.X_OK)
    return shutil.which(tool) is not None


def _persist_disabled_tool_local(tool):
    cur = set(_local_disabled_tools_load())
    cur.add(tool)
    _local_disabled_tools_save(sorted(cur))


def _persist_disabled_tool_remote(node_label, tool):
    """Append `tool` to the named exec node's benchmark_disabled_tools list
    in remote_nodes.json. Idempotent. Preserves the file's key ordering
    by doing an in-place dict mutation + json.dump."""
    if not os.path.isfile(REMOTE_NODES_JSON):
        return
    try:
        with open(REMOTE_NODES_JSON) as f:
            cfg = json.load(f)
    except Exception as e:
        print(f"[bench] WARN: cannot persist disabled_tool to {REMOTE_NODES_JSON}: {e}",
              file=sys.stderr)
        return
    changed = False
    for node in cfg.get("nodes", []):
        for ex in node.get("execution_nodes", []):
            if ex.get("label") != node_label:
                continue
            cur = list(ex.get("benchmark_disabled_tools", []))
            if tool in cur:
                continue
            cur.append(tool)
            cur.sort()
            ex["benchmark_disabled_tools"] = cur
            changed = True
    if not changed:
        return
    with open(REMOTE_NODES_JSON, "w") as f:
        json.dump(cfg, f, indent=2, sort_keys=False)
        f.write("\n")


def disabled_tools_for_node(node):
    """node is either an entry from benchmark_exec_nodes() or the local
    sentinel {"label": "local", ...}. Returns the list of TOOL names
    (not profiler names) currently disabled for that node."""
    if node.get("label") == "local":
        return _local_disabled_tools_load()
    return list(node.get("disabled_tools", []))


def profilers_runnable_on(node, all_profilers):
    """Filter `all_profilers` against the node's runtime tool availability:

      1. honour the persisted benchmark_disabled_tools list (no prompt).
      2. for any tool still unknown, probe (local or via ssh).
      3. when a probe says missing → ask the operator once; on 'n',
         persist the tool name back into the right storage (local sidecar
         or remote_nodes.json) so the next batch skips silently.

    Returns (available_profilers, skip_reasons_by_profiler).
    """
    disabled  = set(disabled_tools_for_node(node))
    available = []
    skips     = {}   # profiler -> "missing tool: X"
    is_local  = (node.get("label") == "local")
    for prof in all_profilers:
        need = PROFILER_REQUIRED_TOOLS.get(prof, [])
        # 1) explicitly disabled in remote_nodes.json / sidecar
        if any(t in disabled for t in need):
            missing = [t for t in need if t in disabled]
            skips[prof] = f"tool-disabled:{','.join(missing)}"
            continue
        # 2) runtime probe
        missing_runtime = []
        for tool in need:
            if is_local:
                ok = _local_have_tool(tool)
            else:
                ok = _remote_have_tool(node.get("ssh_host"),
                                       node.get("ssh_user"),
                                       node.get("ssh_port", 22), tool)
                if ok is None:
                    # SSH unreachable -> conservative: skip everything
                    skips[prof] = "ssh-unreachable"
                    missing_runtime = [tool]   # short-circuit out of the for
                    break
            if not ok:
                missing_runtime.append(tool)
        if not missing_runtime:
            available.append(prof)
            continue
        if skips.get(prof) == "ssh-unreachable":
            continue
        # 3) genuinely missing -> SKIP (never block the batch on stdin).
        #    Provisioning is setup.py's job, not the harness's: a
        #    benchmark*.py must "just run without argument, 1h timeout"
        #    unattended (benchmark/CLAUDE.md), so we never prompt. Persist
        #    the tool into the right benchmark_disabled_tools store so the
        #    next batch is silent, and point the operator at setup.py.
        unresolved = list(missing_runtime)
        for tool in missing_runtime:
            if is_local:
                _persist_disabled_tool_local(tool)
            else:
                _persist_disabled_tool_remote(node.get("label"), tool)
            disabled.add(tool)
        print(f"[bench] node={node.get('label','?')!r} missing "
              f"{','.join(unresolved)} (needed by profiler {prof!r}); "
              f"profiler SKIPPED for this node. Run "
              f"`python3 benchmark/setup.py exec --node {node.get('label','?')}` "
              f"to provision it, then re-run the batch.", file=sys.stderr)
        if unresolved:
            skips[prof] = f"missing-tool:{','.join(unresolved)}"
        else:
            available.append(prof)
    return available, skips


def _ssh_loadavg(ex):
    host = ex.get("host"); user = ex.get("user"); port = ex.get("port", 22)
    if not host or not user:
        return None
    try:
        # Per CLAUDE.md, sleep 60 then read loadavg. We skip the sleep here
        # because exec_node selection happens BEFORE rsync — the hot rsync
        # spike that polluted the 1-min window doesn't apply yet. Caller is
        # responsible for the post-rsync 60s sleep + recheck.
        cmd = ["ssh", "-o", "ConnectTimeout=4", "-o", "BatchMode=yes",
               "-o", "StrictHostKeyChecking=no",
               "-o", "UserKnownHostsFile=/dev/null",
               "-o", "LogLevel=ERROR",
               "-p", str(port), f"{user}@{host}", "cat /proc/loadavg"]
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if p.returncode != 0:
            # Key/auth failure here is the worst: the node would be
            # silently dropped from benchmark_exec_nodes() as if busy.
            note_ssh_failure(ex.get("label"), host, p.returncode, p.stderr)
            return None
        return float(p.stdout.strip().split()[0])
    except Exception:
        return None


# ---- progress display ----------------------------------------------------

class Progress:
    def __init__(self, total, bench_name="benchmark"):
        self.total = total
        self.done  = 0
        self.bench_name = bench_name

    def emit(self, profiler, simd, node, status="OK", extra=""):
        if status == "SKIP" and extra.startswith("tool-disabled:"):
            return  # silenced: tool explicitly disabled for this node
        self.done += 1
        if status == "PASS" or status == "OK":
            colour = C_GREEN
        elif status == "SKIP":
            colour = C_YELLOW
        else:
            colour = C_RED
        prefix = f"{log_stamp()} {colour}[bench]{C_RESET}"
        node_colour = f"{C_BOLD}{C_RED}" if node == "local" else C_CYAN
        node_str = f"{node_colour}{node}{C_RESET}"
        msg = (f"{prefix} {self.done}/{self.total} done -- "
               f"benchmark: {self.bench_name}, "
               f"profiler: {profiler}, SIMD: {simd}, execution_node: {node_str}")
        if status != "OK" and status != "PASS":
            msg += f"  [{status}{(' '+extra) if extra else ''}]"
        elif extra:
            msg += f"  {extra}"
        print(msg, flush=True)


class FleetDeadline:
    """Per-NODE wall-clock budget -- NOT a single whole-fleet pool. Earlier
    this was one global ceiling for the entire batch; a slow local node
    (e.g. the io_uring + compression sweeps in benchmarkbotactions, ~46min)
    drained it and every remaining node was skipped SKIP(deadline) without
    ever being measured -- losing the cross-arch signal the project exists
    for. Now each node gets its OWN budget: call `start_node()` at the top
    of every node iteration to restart the clock, so one node overrunning
    never starves the nodes after it. Within a node the matrix is still
    bounded by the per-cell RUN_TIMEOUTs; this budget is the coarse backstop
    for a node that wedges with no per-cell progress. Default 3600s per
    node; override with env CATCHCHALLENGER_BENCH_DEADLINE_S (0 or negative
    disables the cap)."""

    def __init__(self, default_s=3600):
        budget = default_s
        env = os.environ.get("CATCHCHALLENGER_BENCH_DEADLINE_S")
        if env is not None:
            try:
                budget = int(env)
            except ValueError:
                pass
        # (label, profiler, t_start) of the cell currently in flight, so a
        # timeout / hard hang can report WHERE it was stuck instead of
        # dying mute. Updated by note() before every cell.
        self.current = None
        if budget and budget > 0:
            self.budget   = budget
            self.deadline = time.monotonic() + budget
            print(f"{C_CYAN}[deadline]{C_RESET} per-node budget = "
                  f"{budget}s; a node exceeding it is skipped, the next "
                  f"node starts fresh", flush=True)
        else:
            self.budget   = None
            self.deadline = None

    def start_node(self, label):
        """Restart the budget clock for a new node. The deadline is PER
        NODE: each node gets the full budget regardless of how long earlier
        nodes took, so a slow node never causes later nodes to be skipped."""
        if self.budget is not None:
            self.deadline = time.monotonic() + self.budget

    def reached(self):
        return self.deadline is not None and time.monotonic() >= self.deadline

    def remaining(self):
        """Seconds left in the budget; a large sentinel when uncapped so
        callers can `min(per_cmd_timeout, deadline.remaining())` blindly."""
        if self.deadline is None:
            return 1 << 30
        return max(0, int(self.deadline - time.monotonic()))

    def note(self, label, profiler):
        """Breadcrumb printed BEFORE a cell starts. Two jobs: (1) if the
        cell wedges with no output, the operator still sees the last
        '-> running ...' line and knows exactly where it hung; (2)
        records self.current so where_stuck() can name it on timeout."""
        self.current = (label, profiler, time.monotonic())
        print(f"{C_CYAN}[bench]{C_RESET} -> running profiler={profiler} "
              f"node={label} (deadline in {self.remaining()}s)", flush=True)

    def where_stuck(self):
        """Human string naming the in-flight cell + how long it had been
        running. Call this from any timeout / deadline branch so the
        console always says WHAT was happening when the clock ran out."""
        if self.current is None:
            return "no cell in flight (timed out between cells)"
        label, profiler, t0 = self.current
        return (f"profiler={profiler} node={label}, "
                f"in-flight {int(time.monotonic() - t0)}s when the "
                f"{'deadline' if self.reached() else 'timeout'} fired")

    def skip_node(self, label, profilers, progress, reason="deadline"):
        """Emit a SKIP cell per profiler for a node we ran out of time to
        reach, and return the per_tool sub-dict the caller splices in so
        the history JSON records every cell as SKIP(deadline). Prints the
        in-flight breadcrumb so the operator sees where the budget went."""
        print(f"{C_YELLOW}[deadline]{C_RESET} budget exhausted -- "
              f"{self.where_stuck()}; skipping node {label} "
              f"(+ any nodes after it)", flush=True)
        blocks = {}
        for prof in profilers:
            progress.emit(prof, "no", label, status="SKIP", extra=reason)
            blocks[prof] = {"status": "SKIP", "metrics": {}}
        return blocks


def maxtime_reached(start_monotonic, maxtime_s, last_label=None):
    """True when the operator's --maxtime total cap (seconds) is set and the
    elapsed wall time since `start_monotonic` has passed it. Checked at the
    node-loop boundary so the batch stops launching MORE nodes once the cap
    is hit, finalising with whatever completed (a partial run -> no champion
    decision). Distinct from FleetDeadline, which is a per-node budget;
    maxtime is one ceiling for the whole batch. maxtime_s None/<=0 => no cap.
    Prints a one-line notice naming the last node finished so the operator
    sees why the batch ended early."""
    if not maxtime_s or maxtime_s <= 0:
        return False
    elapsed = int(time.monotonic() - start_monotonic)
    if elapsed > maxtime_s:
        print(f"{C_YELLOW}[maxtime]{C_RESET} elapsed {elapsed}s > --maxtime "
              f"{maxtime_s}s"
              + (f" after node {last_label}" if last_label else "")
              + "; stopping the batch, finalising the partial run",
              flush=True)
        return True
    return False


# ---- profiler wrappers ---------------------------------------------------

def tmpfs_scratch_dir(name="cc-callgrind-scratch"):
    """A scratch dir under the project's tmpfs root for transient tool
    output (callgrind.out, a stray valgrind vgcore on guest SIGILL, ...).
    NEVER the source tree: a vgcore dropped next to the checked-in
    benchmark sources is exactly the artefact-pollution CLAUDE.md forbids
    ('separate compilation artifacts'; 'fix the tool, don't .gitignore').
    Falls back to /tmp when test_config / its tmpfs path is unavailable."""
    root = None
    try:
        test_dir = os.path.join(REPO_ROOT, "test")
        if test_dir not in sys.path:
            sys.path.insert(0, test_dir)
        import test_config
        root = test_config.TMPFS_ROOT
    except Exception:
        root = os.environ.get("CATCHCHALLENGER_TMPFS_ROOT") or "/tmp"
    d = os.path.join(root, name)
    try:
        os.makedirs(d, exist_ok=True)
    except Exception:
        d = os.path.join("/tmp", name)
        os.makedirs(d, exist_ok=True)
    return d


_HELD_LOCKS = []   # keep lock fds alive for the whole process lifetime


def acquire_singleton_lock(bench_name):
    """Prevent two concurrent runs of the SAME benchmark. Takes an
    advisory exclusive flock on a per-benchmark lockfile in tmpfs (NOT
    the repo). Returns the open fd on success (recorded so it stays open
    until process exit -- the lock is held for the whole run); returns
    None when another instance already holds it, after printing a clear
    message naming the holder pid.

    flock is released automatically by the kernel when the process dies
    (normal exit, crash, or kill -9), so there is never a stale lock to
    clean up -- no pidfile races, no manual unlock."""
    lock_dir = tmpfs_scratch_dir("cc-bench-locks")
    path = os.path.join(lock_dir, f"{bench_name}.lock")
    fd = open(path, "a+")
    try:
        fcntl.flock(fd.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        holder = ""
        try:
            fd.seek(0)
            holder = fd.read().strip()
        except Exception:
            pass
        print(f"{C_RED}[lock]{C_RESET} another {bench_name} instance is "
              f"already running"
              + (f" (pid {holder})" if holder else "")
              + f" -- lock {path}. Aborting this run.",
              file=sys.stderr, flush=True)
        fd.close()
        return None
    try:
        fd.seek(0)
        fd.truncate()
        fd.write(f"{os.getpid()}\n")
        fd.flush()
    except Exception:
        pass
    _HELD_LOCKS.append(fd)
    return fd


def acquire_network_lock(bench_name, timeout=None, poll=2.0):
    """Serialize EVERY network-using benchmark against EACH OTHER. Such a
    benchmark (NETWORK_EXCLUSIVE=True -- e.g. benchmarkbotactions, which
    binds a fixed server port and drives the NIC/loopback) must never run
    concurrently with another network benchmark: they'd collide on ports
    and contend for the link, poisoning the measurement. Non-network
    benchmarks don't call this and run in parallel.

    Unlike acquire_singleton_lock (which ABORTS on contention), this WAITS
    its turn: it polls a single shared exclusive flock until free, so the
    network benchmarks form a serial queue rather than failing. Returns the
    held fd (kept alive for the process lifetime) or None on timeout.
    flock auto-releases on process death, so a crashed benchmark never
    wedges the queue. timeout=None waits indefinitely (the per-batch
    FleetDeadline still bounds total runtime)."""
    lock_dir = tmpfs_scratch_dir("cc-bench-locks")
    path = os.path.join(lock_dir, "cc-bench-network.lock")
    fd = open(path, "a+")
    start = time.monotonic()
    announced = False
    while True:
        try:
            fcntl.flock(fd.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            break
        except OSError:
            if not announced:
                holder = ""
                try:
                    fd.seek(0)
                    holder = fd.read().strip()
                except Exception:
                    pass
                print(f"{C_CYAN}[netlock]{C_RESET} {bench_name}: another "
                      f"network benchmark is running"
                      + (f" ({holder})" if holder else "")
                      + "; waiting for the serial turn...",
                      file=sys.stderr, flush=True)
                announced = True
            if timeout is not None and time.monotonic() - start >= timeout:
                print(f"{C_RED}[netlock]{C_RESET} {bench_name}: timed out after "
                      f"{timeout}s waiting for the network lock. Aborting.",
                      file=sys.stderr, flush=True)
                fd.close()
                return None
            time.sleep(poll)
    try:
        fd.seek(0)
        fd.truncate()
        fd.write(f"{bench_name} pid {os.getpid()}\n")
        fd.flush()
    except Exception:
        pass
    _HELD_LOCKS.append(fd)
    if announced:
        print(f"{C_GREEN}[netlock]{C_RESET} {bench_name}: acquired the network "
              f"lock, proceeding.", file=sys.stderr, flush=True)
    return fd


def have_tool(name):
    return shutil.which(name) is not None


def cmake_accel_defs():
    """Optional build accelerators, picked up ONLY when present (project
    rule: accelerators never hard-coded, fallback otherwise). Returns a
    list of extra `cmake` -D args.

    Without the ccache compiler-launcher every benchmark cold build --
    a fresh node, /tmp tmpfs wiped on reboot, a changed flag, and in
    particular the per-test fresh rsync onto a remote compile node --
    recompiles the whole tree from scratch with plain g++. On a slow or
    qemu-emulated compile node that single cold build can itself blow
    the build timeout / eat the whole FleetDeadline budget. Wiring
    ccache when it exists turns the 2nd+ build into a near-instant
    cache replay; absent ccache the list is empty and cmake behaves
    exactly as before. ccache output is bit-identical to a plain
    compile, so no benchmark metric is affected -- only build wall
    time."""
    defs = []
    if shutil.which("ccache"):
        defs += ["-DCMAKE_C_COMPILER_LAUNCHER=ccache",
                 "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"]
    # Eager symbol binding (-Wl,-z,now -Wl,-Bsymbolic). Unlike ccache this
    # DOES change runtime behaviour -- that is the point: it removes the
    # lazy do_lookup_x / _dl_lookup_symbol_x per-call trampoline cost the
    # profile attributed ~3% Ir to. The flag is opt-in in CCCommon.cmake
    # (default OFF for production); the benchmark always builds the perf
    # variant so the champion history tracks it. ELF-only -- the CMake
    # side skips it on the MinGW/PE target.
    defs += ["-DCATCHCHALLENGER_PERF_LINK=ON"]
    return defs


# ── system-vs-vendored library choice ─────────────────────────────────
# Which of the switchable libs (system .so vs in-tree vendored copy) the
# server binary was actually linked against. CCCommon.cmake decides this
# per build from find_library() against the (cross) sysroot and prints a
# parseable STATUS line; we parse the configure log rather than the binary
# so the answer is exact. Populated at configure time keyed by node label
# (the local host = "local", every remote node by its execution_nodes
# label), and read back by history_recorder.PlatformRecord.write() to
# stamp each per-run history JSON. Keyed per-node because a system lib is
# the node's own (a cross sysroot's .so), so two arches can legitimately
# differ (one system, one vendored).
LIBS_BY_NODE = {}

# The five libs CCCommon.cmake / libzlib.cmake switch system<->vendored.
# zlib prints BOTH branches; the leaf libs print "<name>: system" only
# when the system copy is accepted (silent => vendored fallback); zstd
# prints the fallback line only when vendored (silent => system).
_SWITCHABLE_LIBS = ("zlib", "zstd", "blake3", "xxhash", "tinyxml2")

# lib -> pkg-config module name, for the exact system-version probe.
_LIB_PC_NAME = {"zlib": "zlib", "zstd": "libzstd", "blake3": "libblake3",
                "xxhash": "libxxhash", "tinyxml2": "tinyxml2"}


def _lib_version_from_log(name, text):
    """Pull the version a CCCommon.cmake STATUS line advertises for `name`,
    or None. Covers the leaf libs' "<name>: system <hdr> v<ver>" and the
    "building vendored zlib <ver>" line. Returns the raw token (may be a
    partial like '0.8.x' / '11.x' — the exact pkg-config probe refines a
    system lib later)."""
    import re
    if name == "zlib":
        m = re.search(r"building vendored zlib ([0-9][0-9.]*)", text)
        return m.group(1) if m else None
    m = re.search(name + r": system \S+ v([0-9][0-9.xX]*)", text)
    return m.group(1) if m else None


def detect_libs(configure_stdout):
    """Parse a `cmake -S … -B …` configure log and return
    {lib: {"source": 'system'|'vendored', "version": <str|None>}} for the
    switchable libraries. Returns {} when the log isn't a CatchChallenger
    lib-stack configure (no zlib line) so a non-CC / truncated log never
    yields bogus verdicts. The zlib line is the anchor: when present,
    CCCommon.cmake's full discovery (zstd/blake3/xxhash/tinyxml2) has also
    run, so a missing 'system' line for a leaf lib means the vendored
    fallback was compiled in. Versions here are best-effort from the log;
    record_libs() refines a system lib's version with pkg-config."""
    text = configure_stdout or ""
    if "zlib: using system copy" in text:
        zlib_src = "system"
    elif "building vendored zlib" in text:
        zlib_src = "vendored"
    else:
        return {}   # not a CC lib-stack configure log -> unknown
    src = {"zlib": zlib_src}
    # zstd: only the vendored branch prints a line; system path is silent.
    src["zstd"] = ("vendored" if "falling back to vendored build" in text
                   else "system")
    # leaf libs: "<name>: system <path> v…" printed iff system accepted.
    for name in ("blake3", "xxhash", "tinyxml2"):
        src[name] = "system" if (name + ": system") in text else "vendored"
    libs = {}
    for name in _SWITCHABLE_LIBS:
        libs[name] = {"source": src[name],
                      "version": _lib_version_from_log(name, text)}
    return libs


def _local_shell(cmd, timeout=10):
    """Run `cmd` in a shell on the local host; return stdout ('' on error).
    The default `shell` for record_libs (remote builds pass an ssh one)."""
    try:
        return subprocess.run(cmd, shell=True, capture_output=True,
                              text=True, timeout=timeout).stdout
    except Exception:
        return ""


def _pkgconfig_version(pc_name, shell):
    """Exact installed version of a system lib via `pkg-config
    --modversion`, run through `shell` (local or ssh). None when
    pkg-config can't answer (absent / no .pc)."""
    out = (shell("pkg-config --modversion %s 2>/dev/null" % pc_name) or "")
    for line in out.splitlines():
        line = line.strip()
        if line:
            return line
    return None


def record_libs(node_label, configure_stdout, shell=None):
    """Detect the system-vs-vendored lib choice + version from a configure
    log and stash it under `node_label` in LIBS_BY_NODE. For each lib
    resolved to the SYSTEM copy, refine the version with an exact
    `pkg-config --modversion` run through `shell` (defaults to the local
    host; remote builds pass an ssh runner so the version is the compile
    node's, not the host's). No-op (keeps any prior value) when the log
    yields nothing parseable. Returns the dict."""
    libs = detect_libs(configure_stdout)
    if not libs:
        return {}
    sh = shell or _local_shell
    for name, info in libs.items():
        if info["source"] == "system":
            exact = _pkgconfig_version(_LIB_PC_NAME[name], sh)
            if exact:
                info["version"] = exact
    LIBS_BY_NODE[node_label] = libs
    return libs


def alias_libs(from_label, to_label):
    """Copy a recorded lib map from one node label to another (used to
    carry a compile node's verdict onto each exec node it built for)."""
    if from_label in LIBS_BY_NODE:
        LIBS_BY_NODE[to_label] = LIBS_BY_NODE[from_label]


def _drop_core_rlimit():
    """preexec_fn: set RLIMIT_CORE=0 in the child before exec. A
    benchmarked binary that dies on a signal (the harness SIGINT/timeout
    that stops the server, or a genuine crash) must NOT litter a core in
    the cwd -- and under valgrind that core is a `vgcore.<pid>` written
    next to the checked-in benchmark sources. We already detect the
    failure via rc; the dump has zero diagnostic value here and is pure
    source-tree pollution (CLAUDE.md: fix the tool, never .gitignore the
    artefact). Wrapped so a platform without RLIMIT_CORE can't break the
    run."""
    try:
        import resource
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    except Exception:
        pass


def run_capture(cmd, env=None, timeout=None, cwd=None, preexec_fn=None):
    """Run a subprocess; return (rc, stdout, stderr, wall_seconds). cwd
    lets callers pin the child to a tmpfs scratch dir so anything it
    drops (status files, valgrind vgcore, callgrind.out) never lands in
    the source tree; both new args default to None => behaviour
    unchanged for every existing caller."""
    t0 = time.monotonic()
    p  = subprocess.run(cmd, env=env, timeout=timeout,
                        capture_output=True, text=True,
                        cwd=cwd, preexec_fn=preexec_fn)
    dt = time.monotonic() - t0
    return p.returncode, p.stdout, p.stderr, dt


def measure_time_v(cmd, env=None, timeout=None, cwd=None):
    """`/usr/bin/time -v` -> dict with {wall_s, user_s, sys_s, max_rss_kb,
    voluntary_ctx_switches, involuntary_ctx_switches, page_faults}.
    Falls back to wall-only measurement when /usr/bin/time is missing.
    cwd pins the child to a scratch dir; core dumps are disabled so a
    killed/crashed child never drops a core in the source tree."""
    out = {"wall_s": None, "user_s": None, "sys_s": None,
           "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
           "minor_pf": None, "major_pf": None, "rc": None}
    if not os.path.exists("/usr/bin/time"):
        rc, _, _, dt = run_capture(cmd, env=env, timeout=timeout,
                                   cwd=cwd, preexec_fn=_drop_core_rlimit)
        out["wall_s"] = dt; out["rc"] = rc
        return out
    full = ["/usr/bin/time", "-v"] + list(cmd)
    rc, _, err, _ = run_capture(full, env=env, timeout=timeout,
                                cwd=cwd, preexec_fn=_drop_core_rlimit)
    out["rc"] = rc
    for line in err.splitlines():
        s = line.strip()
        if s.startswith("Elapsed (wall clock) time"):
            out["wall_s"] = _parse_wall(s.split(": ", 1)[1])
        elif s.startswith("User time (seconds):"):
            try: out["user_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("System time (seconds):"):
            try: out["sys_s"] = float(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Maximum resident set size (kbytes):"):
            try: out["max_rss_kb"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Voluntary context switches:"):
            try: out["vol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Involuntary context switches:"):
            try: out["invol_ctx"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Minor (reclaiming a frame) page faults:"):
            try: out["minor_pf"] = int(s.split(":", 1)[1])
            except: pass
        elif s.startswith("Major (requiring I/O) page faults:"):
            try: out["major_pf"] = int(s.split(":", 1)[1])
            except: pass
    return out


def _parse_wall(s):
    # h:mm:ss or m:ss.ss
    parts = s.split(":")
    try:
        if len(parts) == 3:
            return int(parts[0])*3600 + int(parts[1])*60 + float(parts[2])
        if len(parts) == 2:
            return int(parts[0])*60 + float(parts[1])
        return float(parts[0])
    except Exception:
        return None


def perf_no_hw_skip(node_label=None):
    """Call when perf stat ran but returned no data (all hardware counters
    <not supported>/<not counted>). Persists 'perf' into the node's
    disabled-tool list so the next batch skips immediately without probing.
    Returns the skip-reason string 'no-hw-counters'."""
    if node_label is None or node_label == "local":
        _persist_disabled_tool_local("perf")
    else:
        _persist_disabled_tool_remote(node_label, "perf")
    return "no-hw-counters"


def measure_perf_stat(cmd, env=None, timeout=None, cwd=None):
    """perf stat -e cycles,instructions,branch-misses,cache-misses
    -> dict; None when perf is unavailable or kernel.perf_event_paranoid
    blocks the user-space counters.

    Note: when the wrapped command exits non-zero (e.g. `timeout 30 X`
    that timed-out returns 124, or a Qt app that exits on SIGINT
    returns 128+SIGINT=130), perf STILL writes the counter lines to
    stderr before propagating the rc. The early-return on rc!=0 was
    therefore discarding good data on every `timeout`-wrapped run --
    parse the events regardless and only fail when stderr was empty
    or unparseable."""
    if not have_tool("perf"):
        return None
    full = ["perf", "stat", "-x", ",", "-e",
            "cycles,instructions,branch-misses,cache-misses"] + list(cmd)
    rc, _, err, _ = run_capture(full, env=env, timeout=timeout,
                                cwd=cwd, preexec_fn=_drop_core_rlimit)
    out = {}
    for line in err.splitlines():
        f = line.split(",")
        if len(f) < 3:
            continue
        val = f[0].strip()
        evt = f[2].strip() if len(f) > 2 else ""
        if val in ("<not supported>", "<not counted>", ""):
            continue
        try:
            out[evt] = int(val)
        except ValueError:
            try: out[evt] = float(val)
            except ValueError: pass
    return out or None


def measure_callgrind(cmd, env=None, timeout=None, outdir=None,
                      toggle_collect=None):
    """valgrind --tool=callgrind. Returns instruction count when
    callgrind_annotate succeeds, else None.

    toggle_collect: a function-name glob (e.g. '*min_network*'). When set,
    runs with `--collect-atstart=no --toggle-collect=<glob>` so only the
    matched function (and its callees) is counted — excludes one-time
    startup (dynamic linking) from the IR. Header-free and needs no source
    markers: valgrind resolves the glob against the binary's symbols at
    runtime, so it works on every node regardless of cross-sysroot headers.
    None (default) = collect everything (whole-program IR).

    valgrind dumps `vgcore.<pid>` into the *process cwd* (NOT
    --callgrind-out-file's dir) whenever the guest is killed by a
    core-dumping signal -- which is exactly what the harness does to
    stop the server (SIGINT / timeout). Run valgrind WITH cwd=outdir
    (a tmpfs scratch dir) AND RLIMIT_CORE=0 so a vgcore is never
    written, and if anything is, it lands in scratch -- never next to
    the checked-in benchmark sources (CLAUDE.md: fix the tool, don't
    .gitignore the artefact).

    AVX-512 workaround: modern glibc IFUNC-dispatches memset/memcpy to
    AVX-512 variants at startup on capable CPUs (zmm regs, EVEX prefix).
    valgrind <= 3.26 chokes on those with SIGILL inside _dl_aux_init,
    BEFORE main() runs -- the resulting callgrind.out only captures the
    crash prologue (~400 IR, useless). Set GLIBC_TUNABLES to mask out
    AVX-512 hwcaps so glibc picks AVX2/SSE variants under valgrind.
    Same trick used by Fedora/Debian valgrind packaging."""
    if not have_tool("valgrind") or not have_tool("callgrind_annotate"):
        return None
    if outdir is None:
        # NEVER default to os.getcwd(): if the harness is launched from the
        # repo, a guest-SIGILL vgcore would land next to the checked-in
        # sources. Use a tmpfs scratch dir instead.
        outdir = tmpfs_scratch_dir()
    out_file = os.path.join(outdir, "callgrind.out")
    try:
        os.remove(out_file)
    except FileNotFoundError:
        pass
    run_env = dict(env or os.environ)
    # Append to any caller-provided tunables instead of clobbering.
    mask = ("glibc.cpu.hwcaps=-AVX512F,-AVX512VL,-AVX512BW,"
            "-AVX512DQ,-AVX512CD,-AVX512_VBMI,-AVX512_VBMI2,"
            "-AVX512_BITALG,-AVX512_VPOPCNTDQ,-AVX512_VNNI,"
            "-AVX512_BF16,-AVX512_FP16")
    prev = run_env.get("GLIBC_TUNABLES", "")
    run_env["GLIBC_TUNABLES"] = (prev + ":" + mask) if prev else mask
    # Drop --quiet so we capture valgrind's diagnostics on stderr; we
    # only print them when something goes wrong.
    full = ["valgrind", "--tool=callgrind", "--callgrind-out-file=" + out_file]
    if toggle_collect:
        full += ["--collect-atstart=no", f"--toggle-collect={toggle_collect}"]
    full += list(cmd)
    rc, _, verr, _ = run_capture(full, env=run_env, timeout=timeout,
                                 cwd=outdir, preexec_fn=_drop_core_rlimit)
    # Accept the run when callgrind.out was written even though the
    # child exited non-zero (e.g. the harness sent SIGINT to stop a
    # server). Reject only if the file is missing or so tiny it can't
    # have captured anything past startup -- that's the AVX-512-SIGILL
    # / unsupported-insn / very-early-crash signature.
    if not os.path.isfile(out_file):
        return None
    lowered = (verr or "").lower()
    # valgrind couldn't decode a host instruction: typically AVX-512
    # baked into the system ld-linux / glibc (gentoo, custom builds)
    # combined with a valgrind that doesn't yet support every EVEX
    # variant. Surface a precise, actionable message AND persist
    # `valgrind` into the local disabled-tools sidecar so subsequent
    # batches skip this profiler without dragging the operator back.
    if ("unhandled instruction bytes" in lowered or
            "unrecognised instruction" in lowered):
        bar = "=" * 74
        snippet = "\n".join(l for l in (verr or "").splitlines()
                            if ("unhandled instruction" in l.lower()
                                or "unrecognised instruction" in l.lower()
                                or "at 0x" in l.lower()))[:600]
        print(f"\n{C_RED}{bar}\n"
              f"  callgrind unusable on this host: valgrind cannot decode an\n"
              f"  instruction the system ld-linux/glibc uses (typically an\n"
              f"  AVX-512 EVEX store baked into ld-linux's _dl_start).\n"
              f"  Auto-disabling 'valgrind' for the local node so the batch\n"
              f"  can continue.\n"
              f"  Two ways to actually re-enable callgrind on this host:\n"
              f"    1. Wait for / install a valgrind release that decodes\n"
              f"       this EVEX variant. As of 3.28 some AVX-512BW store\n"
              f"       forms still SIGILL inside ld-linux.\n"
              f"    2. Hide AVX-512 from userspace at the kernel level: add\n"
              f"         clearcpuid=304\n"
              f"       (=AVX512F) to the kernel cmdline, regenerate the\n"
              f"       bootloader config, reboot. glibc's IFUNC dispatcher\n"
              f"       then picks AVX2/SSE paths and valgrind is happy.\n"
              f"       Trade-off: every process on the box loses AVX-512.\n"
              f"  Then remove 'valgrind' from\n"
              f"    {LOCAL_DISABLED_TOOLS_JSON}\n"
              f"  valgrind said:\n{snippet}\n"
              f"{bar}{C_RESET}", file=sys.stderr)
        _persist_disabled_tool_local("valgrind")
        return None
    # Judge validity on the PARSED instruction count, not the file's byte
    # size: with --toggle-collect only the matched function is recorded, so
    # a perfectly valid run produces a SMALL file (~3 KB) holding a huge Ir
    # total. The failure signatures we must reject are the AVX-512-SIGILL /
    # very-early-crash cases, which collect only the startup prologue
    # (~hundreds of Ir). A floor on the Ir total separates the two cleanly.
    if not os.path.isfile(out_file):
        return None
    rc2, sout, _, _ = run_capture(["callgrind_annotate", out_file], timeout=60)
    if rc2 != 0:
        return None
    ir = None
    for line in sout.splitlines():
        s = line.strip()
        if s.endswith("PROGRAM TOTALS"):
            try:
                ir = int(s.split()[0].replace(",", ""))
            except Exception:
                ir = None
            break
    # ~hundreds of Ir = crash-prologue-only (SIGILL before main / nothing
    # collected); a real workload is orders of magnitude larger.
    if ir is None or ir < 100000:
        tail = "\n".join((verr or "").splitlines()[-8:])
        print(f"[callgrind] no usable data (Ir={ir}) -- valgrind said:\n"
              f"{tail}", file=sys.stderr)
        return None
    return ir


def binary_size(path):
    try: return os.path.getsize(path)
    except OSError: return None


# ---- aggregation ---------------------------------------------------------

def stats_of(samples):
    """Median + stddev of a list of numbers; (None,None) when empty.
    The benchmark CLAUDE.md decision matrix uses median for the central
    tendency and stddev for the noise band."""
    samples = [s for s in samples if s is not None]
    if not samples:
        return None, None
    if len(samples) == 1:
        return samples[0], 0.0
    return statistics.median(samples), statistics.pstdev(samples)


# ---- champion compare ----------------------------------------------------

def node_path_parts(node_label):
    """Map a node label to the (compile_label, exec_label) pair that
    keys the on-disk history/results layout:

        history/<bench>/<compile_label>/<exec_label>/<stamp>.json
        results/<bench>/<compile_label>/<exec_label>/{champion,candidate-*}.json

    * 'local' / None  -> ('local','local')  (the workstation is both the
      amd64 compile node and the amd64 baseline exec node).
    * a remote execution_nodes[].label -> (parent nodes[].label, label)
      e.g. 'rtl9607c' -> ('mips-lxc','rtl9607c').
    * an unknown label -> (label, label) so a record is NEVER dropped on
      the floor just because remote_nodes.json drifted."""
    if not node_label or node_label == "local":
        return ("local", "local")
    cfg = load_remote_nodes()
    for node in cfg.get("nodes", []):
        for ex in node.get("execution_nodes", []):
            if ex.get("label") == node_label:
                return (node.get("label") or node_label, node_label)
    return (node_label, node_label)


def champion_path(bench_name):
    """Per-benchmark champion — stores metrics from ALL platforms so the
    decision is whether a change is better for the whole fleet, not just
    one arch (benchmark/CLAUDE.md decision matrix)."""
    return os.path.join(RESULTS, bench_name, "champion.json")


def candidate_path(bench_name, stamp):
    """Per-benchmark candidate — one JSON per run containing every
    platform's metrics. <stamp> is the run's started_utc with ':'
    replaced by '-' (path-portable)."""
    return os.path.join(RESULTS, bench_name, f"candidate-{stamp}.json")


def load_champion(bench_name):
    p = champion_path(bench_name)
    if not os.path.isfile(p):
        return None
    try:
        with open(p) as f: return json.load(f)
    except Exception:
        return None


PROFILE_TOOLS = ("callgrind", "massif", "perf")


def parse_bench_args(argv=None):
    """Common CLI for every benchmark*.py. The benchmarks otherwise take
    no args (they run unattended). Two flags:

    --comment   a free-text tag persisted into this run's candidate /
                champion / per-platform-history JSON so a score can be
                attributed to the change under test, e.g.
                --comment="Invert loop". Empty string when omitted (the
                field is still written, as "").

    --profile [TOOL]
                deep-dive mode: instead of the full regression batch,
                run the benchmark's command ONCE under TOOL
                (callgrind | massif | perf; default callgrind), drop the
                profiler artefact under the tmpfs profile dir and print
                its path. Local-only — profiling perturbs timing, so it
                is never the before/after datum (run without --profile
                for the numbers)."""
    import argparse
    ap = argparse.ArgumentParser(
        description="CatchChallenger benchmark (run with no args for the "
                    "regression batch; --comment tags the recorded score; "
                    "--profile [tool] does a one-shot local profile).")
    ap.add_argument("--comment", default="",
                    help='free-text tag stored in every result/history '
                         'JSON for this run (e.g. "Invert loop")')
    ap.add_argument("--profile", choices=PROFILE_TOOLS + ("all",), nargs="?",
                    const="all", default=None,
                    help="profile mode: run the benchmark's command under a "
                         "profiler on the LOCAL host AND every benchmark "
                         "exec node, writing one artefact per (node,tool) to "
                         "the tmpfs profile dir. Bare --profile (or "
                         "--profile all) runs BOTH callgrind and perf; pass a "
                         "single tool to narrow.")
    ap.add_argument("--node", action="append", default=None, metavar="LABEL|ARCH",
                    help="restrict the run to specific execution node(s) by "
                         "execution_nodes[].label or arch (repeatable; "
                         "'local' = host baseline only). Default: every "
                         "benchmark-enabled node.")
    ap.add_argument("--maxtime", type=int, default=None, metavar="SECONDS",
                    help="overall wall-clock ceiling for the whole batch "
                         "(seconds). Checked at each node-loop boundary: once "
                         "the elapsed time exceeds it the loop stops launching "
                         "more nodes and the run finalises with whatever "
                         "completed (partial -> no champion decision). Unlike "
                         "the per-node FleetDeadline this is one cap for the "
                         "entire run. Default: no cap.")
    return ap.parse_args(argv)


def profile_tools(arg):
    """Resolve the --profile argument to the list of profilers to run.
    'all' (or the bare-flag default) => both callgrind and perf, which the
    operator always wants together (callgrind = instruction-accurate,
    noise-free; perf = real sampled cycles/cache). A specific tool name
    narrows to just that one."""
    if arg in (None,):
        return []
    if arg == "all":
        return ["callgrind", "perf"]
    return [arg]


def profile_node_tag(node_label, cpu_cores):
    """Filename-safe '<node>-CPU-<cores>' tag embedded in every profile
    artefact so an operator can tell at a glance which exec node and core
    count produced it (e.g. 'rpi-zero-w-CPU-4', 'local-CPU-32'). cpu_cores
    None => 'CPU-NA'."""
    safe = "".join(c if (c.isalnum() or c in "-_") else "_"
                   for c in str(node_label))
    cores = str(cpu_cores) if cpu_cores else "NA"
    return f"{safe}-CPU-{cores}"


def profiler_wrap(cmd, tool, env=None, node_label="local", cpu_cores=None,
                  toggle_collect=None):
    """Build the (full_argv, out_path, env) tuple to run `cmd` under
    `tool` (callgrind | massif | perf). Returns (None, None, None) when
    the underlying tool isn't installed. The artefact path lives under
    the tmpfs profile scratch dir and embeds the '<node>-CPU-<cores>' tag
    so artefacts from different exec nodes never collide. Shared by
    profile_once() (one-shot commands) and benchmarks that drive a
    long-running server process they wrap themselves (botactions)."""
    import datetime
    outdir = tmpfs_scratch_dir("cc-bench-profile")
    if cpu_cores is None and node_label == "local":
        cpu_cores = os.cpu_count()
    tag = profile_node_tag(node_label, cpu_cores)
    stamp = datetime.datetime.utcnow().strftime("%Y%m%dT%H-%M-%SZ")
    if tool in ("callgrind", "massif"):
        if not have_tool("valgrind"):
            print(f"{C_RED}[profile] valgrind not found — cannot run "
                  f"--profile {tool}{C_RESET}", file=sys.stderr)
            return None, None, None
        out = os.path.join(outdir, f"{tool}.out.{tag}.{stamp}")
        valgrind_opts = ["valgrind", f"--tool={tool}",
                         (f"--callgrind-out-file={out}" if tool == "callgrind"
                          else f"--massif-out-file={out}")]
        # toggle_collect glob -> count only that function (+ callees),
        # excluding startup (dynamic linking) from the profile. Header-free:
        # valgrind resolves the glob against the binary's symbols, so it
        # works on every node regardless of cross-sysroot valgrind headers.
        if tool == "callgrind" and toggle_collect:
            valgrind_opts += ["--collect-atstart=no",
                              f"--toggle-collect={toggle_collect}"]
        full = valgrind_opts + list(cmd)
        # Same AVX-512 hwcap mask measure_callgrind() uses so valgrind
        # doesn't SIGILL in ld-linux before main() on modern glibc.
        run_env = dict(env or os.environ)
        mask = ("glibc.cpu.hwcaps=-AVX512F,-AVX512VL,-AVX512BW,"
                "-AVX512DQ,-AVX512CD,-AVX512_VBMI,-AVX512_VBMI2,"
                "-AVX512_BITALG,-AVX512_VPOPCNTDQ,-AVX512_VNNI,"
                "-AVX512_BF16,-AVX512_FP16")
        prev = run_env.get("GLIBC_TUNABLES", "")
        run_env["GLIBC_TUNABLES"] = (prev + ":" + mask) if prev else mask
        return full, out, run_env
    if tool == "perf":
        if not have_tool("perf"):
            print(f"{C_RED}[profile] perf not found — cannot run "
                  f"--profile perf{C_RESET}", file=sys.stderr)
            return None, None, None
        out = os.path.join(outdir, f"perf.data.{tag}.{stamp}")
        # --call-graph=dwarf (not -g/frame-pointer): DWARF CFI unwinding
        # resolves stacks even when the binary omits frame pointers, which
        # is the default on i386/armv6 Release builds (where -g produced
        # the broken [unknown] 0x0 stacks). No -fno-omit-frame-pointer
        # rebuild needed, so the measurement build stays unperturbed.
        full = ["perf", "record", "--call-graph=dwarf", "-o", out] + list(cmd)
        return full, out, (env or dict(os.environ))
    print(f"{C_RED}[profile] unknown tool {tool}{C_RESET}", file=sys.stderr)
    return None, None, None


def profile_artefact_hint(tool, out):
    """One-line 'open with: …' hint + the GREEN artefact banner, shared so
    every benchmark prints the same thing for --profile."""
    hint = {"callgrind": f"callgrind_annotate {out}",
            "massif":    f"ms_print {out}",
            "perf":      f"perf report -i {out}"}.get(tool, "")
    print(f"{C_GREEN}[profile] {tool} artefact: {out}{C_RESET}\n"
          f"          open with: {hint}")


def profile_once(cmd, tool, cwd=None, timeout=None, env=None,
                 node_label="local", cpu_cores=None, toggle_collect=None):
    """--profile implementation for one-shot commands (run to completion):
    run `cmd` once under `tool`, drop the artefact under the tmpfs profile
    scratch dir (filename tagged '<node>-CPU-<cores>'), print + return its
    path (None on failure / missing tool). Lower-level than
    measure_callgrind(): we keep the raw profiler output for the operator
    to open, we don't reduce it to a single metric."""
    full, out, run_env = profiler_wrap(cmd, tool, env=env,
                                       node_label=node_label,
                                       cpu_cores=cpu_cores,
                                       toggle_collect=toggle_collect)
    if full is None:
        return None
    run_capture(full, env=run_env, timeout=timeout, cwd=cwd or os.path.dirname(out),
                preexec_fn=_drop_core_rlimit)
    if not os.path.isfile(out):
        print(f"{C_RED}[profile] {tool} produced no artefact "
              f"(expected {out}){C_RESET}", file=sys.stderr)
        return None
    profile_artefact_hint(tool, out)
    return out


def write_record(path, record):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f: json.dump(record, f, indent=2, sort_keys=True)


def git_sha():
    try:
        out = subprocess.check_output(["git", "rev-parse", "HEAD"],
                                       cwd=REPO_ROOT, timeout=4).decode().strip()
        return out or "nogit"
    except Exception:
        return "nogit"


def _prepare_decision_metrics(metrics):
    """Return a copy of a node's flat metric dict with throughput-coupled
    cumulative counters made comparable PER-UNIT-OF-WORK before the matrix.

    bytes_sent and the raw perf_* counters accumulate over a FIXED-TIME
    window, so a faster variant runs MORE ticks in that window and shows
    HIGHER totals. Comparing those raw totals flags a speed-up as a
    regression (benchmark/CLAUDE.md: "CPU%/RSS are never a conclusion on
    their own ... contrast resource usage against the work-done metric").
      * <pfx>_bytes_sent -> <pfx>_bytes_per_tick (divided by <pfx>_ticks):
        the real per-tick network cost. A genuine per-tick network
        regression still trips the matrix; "sent more only because it ran
        more ticks" no longer does. No tick pairing -> dropped (can't
        compare fairly).
      * raw perf_* (instructions/branch-misses/cache-misses/cycles) are
        dropped from the decision: cumulative over the perf window with no
        recorded tick count to normalise against. Per-tick speed is already
        carried by the latency + ticks_per_s metrics; perf_* stays in the
        JSON for diagnostics only.
    """
    out = {}
    for name, m in metrics.items():
        if name.startswith("perf_"):
            continue
        if name.endswith("_bytes_sent"):
            pfx = name[:-len("_bytes_sent")]
            ticks = metrics.get(pfx + "_ticks")
            bs = m.get("median")
            if ticks and ticks.get("median") and bs is not None:
                out[pfx + "_bytes_per_tick"] = {
                    "median": bs / ticks["median"],
                    "stddev": 0.0,
                    "unit": "bytes/tick",
                    "better": "lower",
                }
            continue
        out[name] = m
    return out


def _is_tail_jitter(name, improved_map, noise_band):
    """True when a pX_p95/p99 tail-latency metric regressed but the SAME
    workload's median moved the OTHER way (improved beyond the noise band).

    That is tail jitter, not a systematic regression: the distribution
    centre got faster and only an outlier sample wobbled — typical of a
    sub-microsecond op at the smallest player counts. A tail regression at
    a FLAT or regressed median is NOT suppressed, so the documented
    'p99 up at flat p50' real-regression case still trips the matrix."""
    for tail in ("_p95_tick_ns", "_p99_tick_ns", "_p999_tick_ns"):
        if name.endswith(tail):
            med = improved_map.get(name[:-len(tail)] + "_median_tick_ns")
            return med is not None and med >= noise_band
    return False


def decide_multi_node(champion, candidate, *, keep_threshold=0.30,
                      discard_threshold=0.20, noise_band=0.10):
    """Cross-platform KEEP/DISCARD/ESCALATE decision.

    Compares EVERY node's metrics between champion and candidate. A
    single metric improvement on one node can trigger KEEP, and a
    regression on any node can trigger DISCARD — per the decision
    matrix in benchmark/CLAUDE.md.

    champion / candidate schema:
        { "commit": "<sha>", "nodes": { "<label>": { "arch": "<arch>",
            "metrics": { "<name>": {"median": n, "stddev": n,
                        "unit": "...", "better": "lower|higher"} }
        } } }

    Returns (decision, [per_metric_summary]).
    """
    if champion is None:
        return "KEEP", [("first-run", None, None, "no champion to compare")]

    ch_nodes = champion.get("nodes", {})
    ca_nodes = candidate.get("nodes", {})

    summary = []
    any_better = False
    any_worse  = False
    all_within_noise = True

    all_node_labels = sorted(set(list(ch_nodes.keys()) + list(ca_nodes.keys())))
    for label in all_node_labels:
        ch_metrics = _prepare_decision_metrics((ch_nodes.get(label) or {}).get("metrics", {}))
        ca_metrics = _prepare_decision_metrics((ca_nodes.get(label) or {}).get("metrics", {}))
        if not ch_metrics and not ca_metrics:
            continue
        arch = (ch_nodes.get(label) or ca_nodes.get(label) or {}).get("arch", "?")
        all_metric_names = sorted(set(list(ch_metrics.keys()) +
                                       list(ca_metrics.keys())))
        # first pass: per-metric improvement fraction (the tail-jitter rule
        # needs a workload's median delta while judging its p95/p99 sibling).
        improved_map = {}
        for name in all_metric_names:
            ch = ch_metrics.get(name)
            ca = ca_metrics.get(name)
            if not ch or not ca:
                continue
            chm = ch.get("median")
            cm  = ca.get("median")
            if chm is None or cm is None:
                continue
            better = ch.get("better", "lower")
            delta = 0.0 if chm == 0 else (cm - chm) / chm
            improved_map[name] = -delta if better == "lower" else delta
        # second pass: summary + KEEP/DISCARD triggers
        for name in all_metric_names:
            ch = ch_metrics.get(name)
            ca = ca_metrics.get(name)
            key = f"{label}({arch})/{name}"
            if ch is None and ca is None:
                continue
            if ch is None:
                summary.append((key, None, ca.get("median"),
                                "new metric (not in champion)"))
                continue
            if ca is None:
                summary.append((key, ch.get("median"), None,
                                "missing in candidate"))
                continue
            chm = ch.get("median")
            cm  = ca.get("median")
            if chm is None or cm is None:
                continue
            better = ch.get("better", "lower")
            if chm == 0:
                delta = 0.0
            else:
                delta = (cm - chm) / chm
            if better == "lower":
                improved = -delta
            else:
                improved = delta
            within_noise = abs(delta) <= noise_band
            if not within_noise:
                all_within_noise = False
            if improved >= keep_threshold:
                any_better = True
            note = ""
            if improved <= -discard_threshold:
                if _is_tail_jitter(name, improved_map, noise_band):
                    note = " [tail-jitter: median improved, not counted]"
                else:
                    any_worse = True
            tag = ("better" if improved > 0 else "worse" if improved < 0
                   else "flat")
            summary.append((key, chm, cm,
                            f"delta={delta:+.2%} ({tag}){note}"))

    if all_within_noise:
        decision = "DISCARD"
    elif any_better and not any_worse:
        decision = "KEEP"
    elif any_worse and not any_better:
        decision = "DISCARD"
    else:
        decision = "ESCALATE"
    return decision, summary


def decide(champion, candidate, *, keep_threshold=0.30, discard_threshold=0.20,
           noise_band=0.10):
    """Strict KEEP/DISCARD/ESCALATE matrix from benchmark/CLAUDE.md.

    Returns (decision, [per_metric_summary]). A metric direction is
    determined by the champion's `better` field ("lower" | "higher")."""
    if champion is None:
        return "KEEP", [("first-run", None, None, "no champion to compare")]
    summary = []
    any_better = False
    any_worse  = False
    all_within_noise = True
    ch_metrics = _prepare_decision_metrics(champion["metrics"])
    ca_metrics = _prepare_decision_metrics(candidate["metrics"])
    improved_map = {}
    for name, ch in ch_metrics.items():
        cand = ca_metrics.get(name)
        if cand is None:
            continue
        chm = ch.get("median"); cm = cand.get("median")
        if chm is None or cm is None:
            continue
        better = ch.get("better", "lower")
        delta = 0.0 if chm == 0 else (cm - chm) / chm
        improved_map[name] = -delta if better == "lower" else delta
    for name, ch in ch_metrics.items():
        cand = ca_metrics.get(name)
        if cand is None:
            summary.append((name, ch["median"], None, "missing in candidate"))
            continue
        cm = cand["median"]; chm = ch["median"]
        better = ch.get("better", "lower")
        if chm == 0:
            delta = 0.0
        else:
            delta = (cm - chm) / chm
        # interpret delta sign relative to "better"
        if better == "lower":
            improved = -delta
        else:
            improved =  delta
        within_noise = abs(delta) <= noise_band
        if not within_noise:
            all_within_noise = False
        if improved >= keep_threshold:
            any_better = True
        note = ""
        if improved <= -discard_threshold:
            if _is_tail_jitter(name, improved_map, noise_band):
                note = " [tail-jitter: median improved, not counted]"
            else:
                any_worse = True
        summary.append((name, chm, cm, f"delta={delta:+.2%} ({'better' if improved>0 else 'worse' if improved<0 else 'flat'}){note}"))
    if all_within_noise:
        decision = "DISCARD"   # within ±10% noise band on every metric
    elif any_better and not any_worse:
        decision = "KEEP"
    elif any_worse and not any_better:
        decision = "DISCARD"
    else:
        decision = "ESCALATE"
    return decision, summary


def print_decision(bench_name, decision, summary, arch_hint=None):
    colour = {"KEEP": C_GREEN, "DISCARD": C_YELLOW, "ESCALATE": C_RED}.get(decision, "")
    tag = f"{bench_name}@{arch_hint}" if arch_hint else bench_name
    print(f"\n{colour}[{tag}] DECISION: {decision}{C_RESET}")
