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
        out = subprocess.check_output(argv, timeout=10).decode().strip()
        return out == "Y"
    except Exception:
        return None


def _local_have_tool(tool):
    if tool.startswith("/"):
        return os.path.exists(tool) and os.access(tool, os.X_OK)
    return shutil.which(tool) is not None


def _ask_install(node_label, tool, profiler):
    """Interactive prompt: 'have you installed <tool> on <node>?'.

    Returns True when the user confirms (y/yes), False otherwise. When
    stdin is not a TTY we treat the answer as 'no' so unattended runs
    don't hang. The caller persists the 'no' answer into the appropriate
    benchmark_disabled_tools list so we never re-prompt for it."""
    sys.stdout.flush()
    prompt = (f"[bench] node={node_label!r} missing tool {tool!r} "
              f"(needed by profiler {profiler!r}).\n"
              f"        Install it now, then answer y. "
              f"Answer n to skip profiler {profiler!r} permanently on this node.\n"
              f"        Have you installed it? [y/N] ")
    if not sys.stdin or not sys.stdin.isatty():
        print(prompt + "n  (non-interactive)", file=sys.stderr)
        return False
    try:
        ans = input(prompt).strip().lower()
    except (EOFError, KeyboardInterrupt):
        print("n  (interrupted)", file=sys.stderr)
        return False
    return ans in ("y", "yes")


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
        # 3) prompt the operator
        unresolved = []
        for tool in missing_runtime:
            if _ask_install(node.get("label", "?"), tool, prof):
                # operator says installed -> re-probe
                if is_local:
                    ok = _local_have_tool(tool)
                else:
                    ok = _remote_have_tool(node.get("ssh_host"),
                                           node.get("ssh_user"),
                                           node.get("ssh_port", 22), tool)
                if not ok:
                    print(f"[bench] still missing after operator confirmed install: {tool!r} "
                          f"on {node.get('label')!r}; recording as disabled.",
                          file=sys.stderr)
                    unresolved.append(tool)
                    if is_local:
                        _persist_disabled_tool_local(tool)
                    else:
                        _persist_disabled_tool_remote(node.get("label"), tool)
                    disabled.add(tool)
            else:
                # operator answered 'n' -> persist + skip
                unresolved.append(tool)
                if is_local:
                    _persist_disabled_tool_local(tool)
                else:
                    _persist_disabled_tool_remote(node.get("label"), tool)
                disabled.add(tool)
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
        out = subprocess.check_output(cmd, timeout=10).decode().strip().split()
        return float(out[0])
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


# ---- profiler wrappers ---------------------------------------------------

def have_tool(name):
    return shutil.which(name) is not None


def run_capture(cmd, env=None, timeout=None):
    """Run a subprocess; return (rc, stdout, stderr, wall_seconds)."""
    t0 = time.monotonic()
    p  = subprocess.run(cmd, env=env, timeout=timeout,
                        capture_output=True, text=True)
    dt = time.monotonic() - t0
    return p.returncode, p.stdout, p.stderr, dt


def measure_time_v(cmd, env=None, timeout=None):
    """`/usr/bin/time -v` -> dict with {wall_s, user_s, sys_s, max_rss_kb,
    voluntary_ctx_switches, involuntary_ctx_switches, page_faults}.
    Falls back to wall-only measurement when /usr/bin/time is missing."""
    out = {"wall_s": None, "user_s": None, "sys_s": None,
           "max_rss_kb": None, "vol_ctx": None, "invol_ctx": None,
           "minor_pf": None, "major_pf": None, "rc": None}
    if not os.path.exists("/usr/bin/time"):
        rc, _, _, dt = run_capture(cmd, env=env, timeout=timeout)
        out["wall_s"] = dt; out["rc"] = rc
        return out
    full = ["/usr/bin/time", "-v"] + list(cmd)
    rc, _, err, _ = run_capture(full, env=env, timeout=timeout)
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


def measure_perf_stat(cmd, env=None, timeout=None):
    """perf stat -e cycles,instructions,branch-misses,cache-misses
    -> dict; None when perf is unavailable or kernel.perf_event_paranoid
    blocks the user-space counters."""
    if not have_tool("perf"):
        return None
    full = ["perf", "stat", "-x", ",", "-e",
            "cycles,instructions,branch-misses,cache-misses"] + list(cmd)
    rc, _, err, _ = run_capture(full, env=env, timeout=timeout)
    if rc != 0:
        return None
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
    callgrind_annotate succeeds, else None."""
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
    rc, _, _, _ = run_capture(full, env=env, timeout=timeout)
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

def champion_path(bench_name, arch):
    return os.path.join(RESULTS, bench_name, arch, "champion.json")


def candidate_path(bench_name, arch, sha):
    return os.path.join(RESULTS, bench_name, arch, f"candidate-{sha}.json")


def load_champion(bench_name, arch):
    p = champion_path(bench_name, arch)
    if not os.path.isfile(p):
        return None
    try:
        with open(p) as f: return json.load(f)
    except Exception:
        return None


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
