"""history_recorder.py -- per-run, per-platform history dumper.

Per benchmark/CLAUDE.md "Per-run history -- append-only, one JSON per
run", every benchmark*.py drops ONE file per (benchmark, run, platform)
under:

  benchmark/history/<benchmark-name>/<ISO-8601>-<cpu-model-slug>.json

Files are never overwritten. The directory IS the timeline. Files in
the same batch share `batch_id` so per-platform records can be re-
stitched into a single batch view.

This module is the single source of truth for the schema. Benchmark
scripts call:

  rec = history_recorder.PlatformRecord(bench_name, batch_id, runner)
  rec.collect()                       # fills cpu/ram/disk/net/etc
  rec.add_result("rusage",   { ... })
  rec.add_result("perf-stat",{ ... })
  rec.write(started_utc, ended_utc, compile_flags=[...], simd_tier="...")

The runner abstraction lets the same code path collect locally (via
subprocess) or over SSH (via an ssh wrapper) -- the remote dispatch is
not wired in any benchmark today, but the helper is ready for it.
"""
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
import uuid


HISTORY_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "history")


# ---- runner abstraction --------------------------------------------------

def local_runner(cmd, timeout=8):
    """Run `cmd` (list or str) locally; return (rc, stdout_str).
    Stderr is swallowed -- collectors only care about stdout shape."""
    if isinstance(cmd, str):
        argv = ["sh", "-c", cmd]
    else:
        argv = list(cmd)
    try:
        p = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
        return p.returncode, p.stdout
    except Exception:
        return 1, ""


def make_ssh_runner(host, user, port=22):
    """Return a runner that ssh's into host. Same (rc, stdout) contract.
    String commands are quoted; list commands are shlex-joined."""
    def _run(cmd, timeout=10):
        if isinstance(cmd, list):
            remote = " ".join(shlex.quote(a) for a in cmd)
        else:
            remote = cmd
        argv = ["ssh", "-o", "ConnectTimeout=4", "-o", "BatchMode=yes",
                "-p", str(port), f"{user}@{host}", remote]
        try:
            p = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
            return p.returncode, p.stdout
        except Exception:
            return 1, ""
    return _run


# ---- collectors ----------------------------------------------------------
# Each collector returns the value or None. Never raises. Never returns
# fields not in the schema -- the writer enforces the full key set.

_SLUG_RE = re.compile(r"[^a-z0-9]+")

def cpu_model_slug(model):
    if not model:
        return "unknown-cpu"
    s = _SLUG_RE.sub("-", model.lower()).strip("-")
    return s[:60] or "unknown-cpu"


def _read(run, path):
    rc, out = run(["cat", path])
    if rc != 0:
        return ""
    return out


def collect_cpu(run):
    out = {"cpu_model": None, "cpu_cores": None, "cpu_mhz": None, "arch": None}
    txt = _read(run, "/proc/cpuinfo")
    if not txt:
        return out
    cores = 0
    mhz_vals = []
    for line in txt.splitlines():
        if ":" not in line:
            continue
        k, _, v = line.partition(":")
        k = k.strip(); v = v.strip()
        if k == "model name" and out["cpu_model"] is None:
            out["cpu_model"] = v
        elif k == "cpu MHz":
            try: mhz_vals.append(float(v))
            except ValueError: pass
        elif k == "processor":
            cores += 1
        # arm/mips/riscv use different keys
        elif k in ("Hardware", "Processor") and out["cpu_model"] is None:
            out["cpu_model"] = v
        elif k == "uarch" and out["cpu_model"] is None:
            out["cpu_model"] = v
    if cores:
        out["cpu_cores"] = cores
    if mhz_vals:
        out["cpu_mhz"] = max(mhz_vals)
    rc, m = run(["uname", "-m"])
    if rc == 0 and m.strip():
        out["arch"] = m.strip()
    return out


def collect_ram(run):
    out = {"ram_total_mb": None, "ram_type": None}
    txt = _read(run, "/proc/meminfo")
    for line in txt.splitlines():
        if line.startswith("MemTotal:"):
            try:
                kb = int(line.split()[1])
                out["ram_total_mb"] = kb // 1024
            except Exception:
                pass
            break
    # dmidecode needs root and isn't on most boards -- best effort only.
    rc, dmi = run("dmidecode -t memory 2>/dev/null | grep -E 'Type:|Speed:' | head -4")
    if rc == 0 and dmi.strip():
        kind = None; speed = None
        for line in dmi.splitlines():
            s = line.strip()
            if s.startswith("Type:") and "Unknown" not in s and "None" not in s:
                kind = s.split(":", 1)[1].strip()
            elif s.startswith("Speed:") and "Unknown" not in s and speed is None:
                speed = s.split(":", 1)[1].strip().replace(" ", "")
        if kind:
            out["ram_type"] = kind + ("-" + speed if speed else "")
    return out


def _root_block_device(run):
    """Return ("/dev/sda" or "/dev/nvme0n1", parent_dev_name) for /."""
    rc, out = run(["findmnt", "-no", "SOURCE", "/"])
    if rc != 0 or not out.strip():
        return None, None
    src = out.strip()
    # Resolve LVM/encrypted/etc to underlying device when possible.
    rc, real = run(["readlink", "-f", src])
    if rc == 0 and real.strip():
        src = real.strip()
    # Strip partition suffix to find the parent disk (sda1->sda, nvme0n1p1->nvme0n1).
    name = src.rsplit("/", 1)[-1]
    parent = re.sub(r"p?\d+$", "", name)
    return src, parent


def collect_disk(run):
    out = {"disk_root": None, "disk_kind": None}
    src, parent = _root_block_device(run)
    if not src:
        return out
    if src.startswith("tmpfs") or "tmpfs" in src:
        out["disk_root"] = "tmpfs"; out["disk_kind"] = "tmpfs"; return out
    # vendor + model from sysfs
    vendor = model = ""
    rc, v = run(["cat", f"/sys/block/{parent}/device/vendor"])
    if rc == 0: vendor = v.strip()
    rc, m = run(["cat", f"/sys/block/{parent}/device/model"])
    if rc == 0: model = m.strip()
    if vendor or model:
        out["disk_root"] = (vendor + " " + model).strip() or src
    else:
        out["disk_root"] = src
    # kind: nvme / mmc / rotational vs ssd
    if parent.startswith("nvme"):
        out["disk_kind"] = "nvme"
    elif parent.startswith("mmcblk"):
        out["disk_kind"] = "emmc" if parent.startswith("mmcblk0") else "sd"
    else:
        rc, rot = run(["cat", f"/sys/block/{parent}/queue/rotational"])
        if rc == 0:
            out["disk_kind"] = "hdd" if rot.strip() == "1" else "ssd-sata"
    return out


def collect_net(run):
    """Primary NIC model. We pick the interface used for the default route,
    then look up its PCI/USB device model via lspci / sysfs."""
    rc, route = run(["sh", "-c", "ip route show default | head -1"])
    iface = None
    if rc == 0:
        m = re.search(r"\bdev\s+(\S+)", route)
        if m: iface = m.group(1)
    if not iface:
        return None
    # /sys/class/net/<iface>/device/{vendor,device} are numeric ids; resolve
    # via lspci -mm if available, else fall back to the device path string.
    rc, link = run(["readlink", "-f", f"/sys/class/net/{iface}/device"])
    if rc != 0 or not link.strip():
        return iface
    devpath = link.strip()
    # PCI: /sys/devices/pci.../0000:01:00.0
    pci = re.search(r"([0-9a-f]{4}:[0-9a-f]{2}:[0-9a-f]{2}\.[0-9a-f])", devpath)
    if pci and shutil.which("lspci"):
        rc, lspci = local_runner(["lspci", "-mm", "-s", pci.group(1)])
        if rc == 0 and lspci.strip():
            # lspci -mm: slot "class" "vendor" "device" ...
            parts = re.findall(r'"([^"]*)"', lspci)
            if len(parts) >= 3:
                return (parts[1] + " " + parts[2]).strip()
    # USB / generic fallback: read product file
    rc, prod = run(["cat", f"/sys/class/net/{iface}/device/product"])
    if rc == 0 and prod.strip():
        return prod.strip()
    return iface


def collect_kernel(run):
    rc, out = run(["uname", "-r"])
    return out.strip() if rc == 0 else None


def collect_libc(run):
    rc, out = run("ldd --version 2>&1 | head -1")
    if rc == 0 and out.strip():
        s = out.strip()
        # "ldd (Ubuntu GLIBC 2.39-...) 2.39"  /  "musl libc (x86_64)\nVersion 1.2.5"
        m = re.search(r"(glibc|musl|uClibc-ng|uClibc)[^\d]*([\d\.]+)", s, re.I)
        if m:
            return f"{m.group(1).lower()} {m.group(2)}"
        return s
    return None


def collect_compiler(run):
    for cc in ("cc", "gcc", "clang"):
        rc, out = run(f"{cc} --version 2>&1 | head -1")
        if rc == 0 and out.strip():
            return out.strip()
    return None


def collect_loadavg(run):
    rc, out = run(["cat", "/proc/loadavg"])
    if rc != 0 or not out.strip():
        return None
    try:
        return float(out.split()[0])
    except Exception:
        return None


# ---- record ---------------------------------------------------------------

class PlatformRecord:
    """Accumulator for one (benchmark, run, platform) record.

    Fields stay None until `collect()` runs; that way a benchmark can
    pre-allocate the record at batch start and fill it once the cell
    actually runs.
    """

    def __init__(self, benchmark, batch_id, node_label, runner=None,
                 arch_hint=None):
        self.benchmark   = benchmark
        self.batch_id    = batch_id
        self.node_label  = node_label
        self.runner      = runner or local_runner
        self.arch_hint   = arch_hint
        self.platform    = {
            "node":          node_label,
            "arch":          arch_hint,
            "cpu_model":     None,
            "cpu_cores":     None,
            "cpu_mhz":       None,
            "ram_total_mb":  None,
            "ram_type":      None,
            "disk_root":     None,
            "disk_kind":     None,
            "net_card":      None,
            "kernel":        None,
            "libc":          None,
            "compiler":      None,
            "compile_flags": [],
            "simd_tier":     "generic",
            "loadavg_1min_at_start": None,
        }
        self.results = {}

    def collect(self):
        """Best-effort fill of every platform field. Never raises."""
        cpu = collect_cpu(self.runner)
        self.platform["cpu_model"] = cpu["cpu_model"]
        self.platform["cpu_cores"] = cpu["cpu_cores"]
        self.platform["cpu_mhz"]   = cpu["cpu_mhz"]
        if self.platform["arch"] is None and cpu["arch"]:
            self.platform["arch"] = cpu["arch"]
        ram = collect_ram(self.runner)
        self.platform["ram_total_mb"] = ram["ram_total_mb"]
        self.platform["ram_type"]     = ram["ram_type"]
        disk = collect_disk(self.runner)
        self.platform["disk_root"] = disk["disk_root"]
        self.platform["disk_kind"] = disk["disk_kind"]
        self.platform["net_card"]  = collect_net(self.runner)
        self.platform["kernel"]    = collect_kernel(self.runner)
        self.platform["libc"]      = collect_libc(self.runner)
        self.platform["compiler"]  = collect_compiler(self.runner)
        self.platform["loadavg_1min_at_start"] = collect_loadavg(self.runner)
        return self

    def add_result(self, tool, metrics=None, status="PASS",
                   skip_reason=None, artifact=None):
        """Record one profiler/tool block.

        `metrics` is a dict {name -> {value, unit, better, samples?, median?,
        stddev?}}. Missing optional fields are filled with None so the JSON
        is self-describing across tools."""
        norm = {}
        for name, m in (metrics or {}).items():
            entry = {
                "value":   m.get("value", m.get("median")),
                "unit":    m.get("unit"),
                "better":  m.get("better"),
                "samples": m.get("samples"),
                "median":  m.get("median", m.get("value")),
                "stddev":  m.get("stddev"),
            }
            norm[name] = entry
        self.results[tool] = {
            "metrics":     norm,
            "artifact":    artifact,
            "status":      status,
            "skip_reason": skip_reason,
        }
        return self

    def write(self, *, commit, started_utc, ended_utc,
              compile_flags=None, simd_tier=None,
              harness_version=None, decision=None):
        if compile_flags is not None:
            self.platform["compile_flags"] = list(compile_flags)
        if simd_tier is not None:
            self.platform["simd_tier"] = simd_tier
        doc = {
            "benchmark":       self.benchmark,
            "commit":          commit,
            "commit_short":    (commit or "")[:7] or None,
            "started_utc":     started_utc,
            "ended_utc":       ended_utc,
            "harness_version": harness_version,
            "batch_id":        self.batch_id,
            "decision":        decision,
            "results":         self.results,
        }
        doc.update(self.platform)
        slug = cpu_model_slug(self.platform["cpu_model"])
        stamp = started_utc.replace(":", "-")
        outdir = os.path.join(HISTORY_ROOT, self.benchmark)
        os.makedirs(outdir, exist_ok=True)
        path = os.path.join(outdir, f"{stamp}-{slug}.json")
        # Never overwrite -- append a short suffix if the (very unlikely)
        # collision happens (same node, same second, same CPU slug).
        n = 1
        final = path
        while os.path.exists(final):
            final = os.path.join(outdir, f"{stamp}-{slug}-{n}.json")
            n += 1
        with open(final, "w") as f:
            json.dump(doc, f, indent=2, sort_keys=True)
            f.write("\n")
        return final


# ---- post-hoc decision tagging -------------------------------------------

def attach_decision(benchmark, batch_id, decision):
    """Stamp every history file from `batch_id` with the batch's decision.

    Decision is computed AFTER all per-platform records are written (it
    depends on the champion compare), so we update the JSONs in place.
    Only files whose `decision` is still null are touched."""
    outdir = os.path.join(HISTORY_ROOT, benchmark)
    if not os.path.isdir(outdir):
        return 0
    n = 0
    for name in os.listdir(outdir):
        if not name.endswith(".json"):
            continue
        p = os.path.join(outdir, name)
        try:
            with open(p) as f:
                doc = json.load(f)
        except Exception:
            continue
        if doc.get("batch_id") != batch_id:
            continue
        if doc.get("decision") is not None:
            continue
        doc["decision"] = decision
        with open(p, "w") as f:
            json.dump(doc, f, indent=2, sort_keys=True)
            f.write("\n")
        n += 1
    return n


# ---- module-level convenience --------------------------------------------

def new_batch_id():
    return uuid.uuid4().hex


def iso_now():
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def harness_version():
    here = os.path.dirname(os.path.abspath(__file__))
    try:
        out = subprocess.check_output(
            ["git", "log", "-1", "--format=%H", "--", here],
            cwd=os.path.dirname(here), timeout=4).decode().strip()
        return out or None
    except Exception:
        return None


# ---- self-test -----------------------------------------------------------

if __name__ == "__main__":
    rec = PlatformRecord("selftest", new_batch_id(), "local").collect()
    rec.add_result("rusage", {
        "wall_s": {"value": 1.23, "unit": "s", "better": "lower"},
    })
    t = iso_now()
    p = rec.write(commit="0"*40, started_utc=t, ended_utc=t,
                  compile_flags=["-O3"], simd_tier="generic",
                  harness_version=harness_version())
    print(f"wrote {p}")
    with open(p) as f:
        sys.stdout.write(f.read())
