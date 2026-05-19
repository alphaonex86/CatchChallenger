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
import json
import os
import random
import re
import shutil
import statistics
import subprocess
import sys
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
    "rusage":      ["/usr/bin/time"],
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
C_RESET  = "\033[0m"


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
                 "disabled_tools"}
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
            la = _ssh_loadavg(ex)
            if la is None or la >= 1.0:
                print(f"[bench] skip exec_node={ex.get('label')} loadavg={la}", file=sys.stderr)
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
    def __init__(self, total):
        self.total = total
        self.done  = 0

    def emit(self, profiler, simd, node, status="OK", extra=""):
        self.done += 1
        if status == "PASS" or status == "OK":
            colour = C_GREEN
        elif status == "SKIP":
            colour = C_YELLOW
        else:
            colour = C_RED
        prefix = f"{colour}[bench]{C_RESET}"
        msg = f"{prefix} {self.done}/{self.total} done -- profiler: {profiler}, SIMD: {simd}, execution_node: {node}"
        if status != "OK" and status != "PASS":
            msg += f"  [{status}{(' '+extra) if extra else ''}]"
        elif extra:
            msg += f"  {extra}"
        print(msg, flush=True)


class FleetDeadline:
    """Hard wall-clock ceiling for a whole benchmark batch -- the same
    class of bug fixed in setup.py: the {profiler}x{simd}x{arch}x
    {workload}x{node} matrix is unbounded (a slow/qemu-emulated node, a
    callgrind run at ~30x, a wedged remote ssh) so without a global cap
    `benchmark*.py` can run far past the 1h the operator expects
    (benchmark/CLAUDE.md: 'just run this command without argument with
    1h timeout'). Checked cooperatively at each node boundary; remaining
    cells are emitted SKIP(deadline) so the progress counter and the
    per-platform history JSON stay honest (a skipped cell is recorded,
    never silently dropped). Default 3600s; override with env
    CATCHCHALLENGER_BENCH_DEADLINE_S (0 or negative disables the cap)."""

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
            print(f"{C_CYAN}[deadline]{C_RESET} whole-fleet ceiling = "
                  f"{budget}s; nodes not reached by then are skipped",
                  flush=True)
        else:
            self.budget   = None
            self.deadline = None

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


# ---- profiler wrappers ---------------------------------------------------

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
    return defs


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


def measure_callgrind(cmd, env=None, timeout=None, outdir=None):
    """valgrind --tool=callgrind. Returns instruction count when
    callgrind_annotate succeeds, else None.

    valgrind dumps `vgcore.<pid>` into the *process cwd* (NOT
    --callgrind-out-file's dir) whenever the guest is killed by a
    core-dumping signal -- which is exactly what the harness does to
    stop the server (SIGINT / timeout). Run valgrind WITH cwd=outdir
    (a tmpfs scratch dir) AND RLIMIT_CORE=0 so a vgcore is never
    written, and if anything is, it lands in scratch -- never next to
    the checked-in benchmark sources (CLAUDE.md: fix the tool, don't
    .gitignore the artefact)."""
    if not have_tool("valgrind") or not have_tool("callgrind_annotate"):
        return None
    if outdir is None:
        outdir = os.getcwd()
    out_file = os.path.join(outdir, "callgrind.out")
    try:
        os.remove(out_file)
    except FileNotFoundError:
        pass
    full = ["valgrind", "--tool=callgrind", "--callgrind-out-file=" + out_file,
            "--quiet"] + list(cmd)
    rc, _, _, _ = run_capture(full, env=env, timeout=timeout,
                              cwd=outdir, preexec_fn=_drop_core_rlimit)
    if rc != 0 or not os.path.isfile(out_file):
        return None
    rc2, sout, _, _ = run_capture(["callgrind_annotate", out_file], timeout=60)
    if rc2 != 0:
        return None
    for line in sout.splitlines():
        s = line.strip()
        if s.endswith("PROGRAM TOTALS"):
            try:
                return int(s.split()[0].replace(",", ""))
            except Exception:
                return None
    return None


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


def champion_path(bench_name, node_label):
    c, e = node_path_parts(node_label)
    return os.path.join(RESULTS, bench_name, c, e, "champion.json")


def candidate_path(bench_name, node_label, stamp):
    """stamp = the run's started_utc with ':' -> '-' (path-portable).
    Filename is candidate-<stamp>.json (not -<sha>): the commit is
    already inside the JSON, and one run == one timestamp per node."""
    c, e = node_path_parts(node_label)
    return os.path.join(RESULTS, bench_name, c, e, f"candidate-{stamp}.json")


def load_champion(bench_name, node_label):
    p = champion_path(bench_name, node_label)
    if not os.path.isfile(p):
        return None
    try:
        with open(p) as f: return json.load(f)
    except Exception:
        return None


def parse_bench_args(argv=None):
    """Common CLI for every benchmark*.py. The benchmarks otherwise take
    no args (they run unattended); the only flag is --comment, a
    free-text tag persisted into this run's candidate / champion /
    per-platform-history JSON so a score can be attributed to the change
    under test, e.g. --comment="Invert loop". Empty string when omitted
    (the field is still written, as "")."""
    import argparse
    ap = argparse.ArgumentParser(
        description="CatchChallenger benchmark (run with no args; "
                    "--comment tags the recorded score).")
    ap.add_argument("--comment", default="",
                    help='free-text tag stored in every result/history '
                         'JSON for this run (e.g. "Invert loop")')
    return ap.parse_args(argv)


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
    for name, ch in champion["metrics"].items():
        cand = candidate["metrics"].get(name)
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
        if improved <= -discard_threshold:
            any_worse = True
        summary.append((name, chm, cm, f"delta={delta:+.2%} ({'better' if improved>0 else 'worse' if improved<0 else 'flat'})"))
    if all_within_noise:
        decision = "DISCARD"   # within ±10% noise band on every metric
    elif any_better and not any_worse:
        decision = "KEEP"
    elif any_worse and not any_better:
        decision = "DISCARD"
    else:
        decision = "ESCALATE"
    return decision, summary


def print_decision(bench_name, arch, decision, summary):
    colour = {"KEEP": C_GREEN, "DISCARD": C_YELLOW, "ESCALATE": C_RED}.get(decision, "")
    print(f"\n{colour}[{bench_name}@{arch}] DECISION: {decision}{C_RESET}")
    for name, ch, cm, note in summary:
        print(f"  - {name}: champion={ch} candidate={cm}  {note}")
