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


def _default_iface(run):
    rc, route = run(["sh", "-c", "ip route show default | head -1"])
    if rc != 0:
        return None
    m = re.search(r"\bdev\s+(\S+)", route)
    return m.group(1) if m else None


def _wifi_standard_from_flags(flags):
    """Map `iw link` capability tokens to a wifi generation label.
    EHT > HE > VHT > HT > legacy. Tokens come from `iw dev <iface> link`
    output ("RX: ... MCS N", "VHT-MCS N", "HE-MCS N", "EHT-MCS N")."""
    f = flags.upper()
    if "EHT" in f:
        return "wifi-7"
    if "HE" in f:
        return "wifi-6"
    if "VHT" in f:
        return "wifi-5"
    if "HT" in f:
        return "wifi-4"
    return None


def _wifi_band_from_freq(freq_mhz):
    if freq_mhz is None:
        return None
    if 2400 <= freq_mhz <= 2500:
        return "2.4GHz"
    if 5000 <= freq_mhz <= 5895:
        return "5GHz"
    if 5925 <= freq_mhz <= 7125:
        return "6GHz"
    return None


_ARPHRD = {
    1:   "wired",       # ETHER
    512: "ppp",
    256: "slip",
    257: "cslip",
    258: "slip6",
    259: "cslip6",
    772: "loopback",
    768: "tunnel",      # IPIP
    769: "tunnel6",
    773: "sit",
    776: "ipgre",
    778: "gre",
    823: "ieee802154",
}


def _classify_link_kind(run, iface):
    """Return (net_link, net_link_detail) for `iface`.

    net_link is the coarse bucket: wired / wifi / ppp / slip / tunnel /
    vpn / serial / loopback / bridge / virtual / unknown.
    net_link_detail is the driver-level kind when known (wireguard,
    openvpn, gre, vxlan, ppp0, tun0, ser2net, ...).
    """
    detail = None
    # `ip -d link show <iface>` exposes the rtnl `kind` token on a line
    # like "    tun  ..." or "    wireguard ..." for virtual links.
    rc, ipd = run(["sh", "-c", f"ip -d link show {shlex.quote(iface)} 2>/dev/null"])
    kinds = ("wireguard", "openvpn", "tun", "tap", "ppp", "vti", "vti6",
             "gre", "gretap", "ip6gre", "ip6gretap", "sit", "ipip",
             "vxlan", "geneve", "bond", "team", "bridge", "veth",
             "macvlan", "macvtap", "ipvlan", "vlan", "dummy")
    if rc == 0 and ipd.strip():
        for k in kinds:
            if re.search(rf"^\s+{k}\b", ipd, re.M):
                detail = k
                break
    # /sys/class/net/<iface>/type is the ARPHRD numeric.
    arphrd = None
    rc, t = run(["cat", f"/sys/class/net/{iface}/type"])
    if rc == 0:
        try: arphrd = int(t.strip())
        except Exception: pass
    # Wifi takes precedence (caller already handled it, but be safe).
    rc, _ = run(["test", "-d", f"/sys/class/net/{iface}/wireless"])
    if rc == 0:
        return "wifi", detail or iface
    # ser2net / serial tunnels: the iface is usually slipN/pppN over a
    # serial line. Detect ser2net by checking whether the route's gateway
    # is reachable via a /dev/tty* line owner — best effort.
    if detail in ("wireguard", "openvpn", "vti", "vti6", "gre", "gretap",
                  "ip6gre", "ip6gretap", "sit", "ipip", "vxlan", "geneve",
                  "tun", "tap"):
        # tun/tap is generic — could be VPN. wireguard/openvpn are VPN
        # explicitly; gre/sit/ipip/vxlan/geneve are tunnels.
        if detail in ("wireguard", "openvpn"):
            return "vpn", detail
        if detail in ("tun", "tap"):
            # OpenVPN/WireGuard often present as tun; try to disambiguate
            # by looking at which userspace process owns the iface.
            rc, who = run(["sh", "-c",
                           f"ss -tulpnH 2>/dev/null | grep -E '(openvpn|wireguard|wg-)' | head -1"])
            if rc == 0 and who.strip():
                if "openvpn" in who: return "vpn", "openvpn"
                if "wireguard" in who or "wg-" in who: return "vpn", "wireguard"
            return "tunnel", detail
        return "tunnel", detail
    if detail in ("bridge", "bond", "team", "veth", "macvlan", "macvtap",
                  "ipvlan", "vlan", "dummy"):
        return "virtual", detail
    if arphrd is not None:
        bucket = _ARPHRD.get(arphrd)
        if bucket:
            return bucket, detail
    # ser2net heuristic: name pattern slN / sl0
    if re.match(r"^sl\d+$", iface):
        return "slip", detail or iface
    if re.match(r"^ppp\d+$", iface):
        return "ppp", detail or iface
    return "unknown", detail or iface


def collect_net_link(run, iface=None):
    """Network link classification + per-kind detail.

    Coarse bucket goes in `net_link`; the driver/protocol name goes in
    `net_link_detail`. Wifi-specific and wired-specific fields are
    populated when applicable; otherwise they stay None."""
    out = {
        "net_link":         None,
        "net_link_detail":  None,
        "wifi_ssid":        None,
        "wifi_standard":    None,
        "wifi_band":        None,
        "wifi_link_mbps":   None,
        "eth_link_mbps":    None,
    }
    if iface is None:
        iface = _default_iface(run)
    if not iface:
        return out
    kind, detail = _classify_link_kind(run, iface)
    out["net_link"]        = kind
    out["net_link_detail"] = detail
    is_wifi = (kind == "wifi")
    if kind == "wired":
        # /sys/class/net/<iface>/speed is the kernel-reported negotiated
        # rate in Mbps. Returns -1 / fails when link is down or driver
        # doesn't publish it; fall back to ethtool's "Speed:" line.
        rc, sp = run(["cat", f"/sys/class/net/{iface}/speed"])
        if rc == 0:
            try:
                v = int(sp.strip())
                if v > 0:
                    out["eth_link_mbps"] = v
            except Exception:
                pass
        if out["eth_link_mbps"] is None:
            rc, et = run(["sh", "-c",
                          f"ethtool {shlex.quote(iface)} 2>/dev/null | grep -i '^\\s*Speed:'"])
            if rc == 0 and et.strip():
                m = re.search(r"Speed:\s*([\d\.]+)\s*([GM])b/s", et, re.I)
                if m:
                    try:
                        v = float(m.group(1))
                        if m.group(2).upper() == "G":
                            v *= 1000
                        out["eth_link_mbps"] = int(v)
                    except Exception:
                        pass
        return out
    if not is_wifi:
        return out
    # SSID + freq + bitrate from `iw dev <iface> link`
    rc, link = run(["iw", "dev", iface, "link"])
    freq_mhz = None
    flags = ""
    if rc == 0 and link.strip() and "Not connected" not in link:
        for line in link.splitlines():
            s = line.strip()
            if s.startswith("SSID:"):
                out["wifi_ssid"] = s.split(":", 1)[1].strip() or None
            elif s.startswith("freq:"):
                try: freq_mhz = int(float(s.split(":", 1)[1].strip()))
                except Exception: pass
            elif s.startswith("tx bitrate:"):
                v = s.split(":", 1)[1].strip()
                m = re.match(r"([\d\.]+)\s*MBit/s(.*)$", v)
                if m:
                    try: out["wifi_link_mbps"] = int(float(m.group(1)))
                    except Exception: pass
                    flags += " " + m.group(2)
    # nmcli fallback for SSID + rate if iw didn't yield them (no root etc.)
    if out["wifi_ssid"] is None:
        rc, nm = run(["sh", "-c",
                      "nmcli -t -f ACTIVE,SSID,FREQ,RATE dev wifi 2>/dev/null"
                      " | awk -F: '$1==\"yes\"{print; exit}'"])
        if rc == 0 and nm.strip():
            parts = nm.strip().split(":")
            if len(parts) >= 4:
                out["wifi_ssid"] = parts[1] or None
                try:
                    fz = parts[2].replace(" MHz", "").strip()
                    if fz: freq_mhz = int(float(fz))
                except Exception: pass
                m = re.match(r"([\d\.]+)\s*MBit/s", parts[3].strip())
                if m and out["wifi_link_mbps"] is None:
                    try: out["wifi_link_mbps"] = int(float(m.group(1)))
                    except Exception: pass
    # Standard: pull capability tokens from `iw dev <iface> station dump`
    # (the bitrate line carries VHT/HE/EHT-MCS tags) before falling back.
    rc, sta = run(["sh", "-c", f"iw dev {shlex.quote(iface)} station dump 2>/dev/null"])
    if rc == 0 and sta.strip():
        flags += " " + sta
    out["wifi_standard"] = _wifi_standard_from_flags(flags)
    out["wifi_band"] = _wifi_band_from_freq(freq_mhz)
    return out


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


def collect_virt(run):
    """Detect bare-metal vs container vs VM, plus arch-emulation.

    Returns dict:
      virt_kind     : "bare-metal" | "container" | "vm"
      virt_type     : "none" | "lxc" | "docker" | "podman" |
                      "systemd-nspawn" | "openvz" | "qemu" | "kvm" |
                      "vmware" | "virtualbox" | "xen" | "microsoft" |
                      "wsl" | "unknown"
      arch_emulated : bool   (uname -m differs from underlying CPU arch,
                              e.g. qemu-user-static binfmt)
      host_arch     : best-effort underlying CPU arch when emulated
    """
    out = {"virt_kind": None, "virt_type": None,
           "arch_emulated": None, "host_arch": None}

    # systemd-detect-virt is the canonical answer when present.
    rc, c = run("systemd-detect-virt --container 2>/dev/null")
    c = c.strip() if rc == 0 else ""
    rc, v = run("systemd-detect-virt --vm 2>/dev/null")
    v = v.strip() if rc == 0 else ""
    if c and c != "none":
        out["virt_kind"] = "container"; out["virt_type"] = c
    elif v and v != "none":
        out["virt_kind"] = "vm"; out["virt_type"] = v
    elif c == "none" and v == "none":
        out["virt_kind"] = "bare-metal"; out["virt_type"] = "none"
    else:
        # Heuristic fallbacks when systemd-detect-virt isn't installed.
        rc, cg = run(["cat", "/proc/1/cgroup"])
        if rc == 0:
            if "/lxc/" in cg or cg.strip().startswith("0::/lxc"):
                out["virt_kind"] = "container"; out["virt_type"] = "lxc"
            elif "/docker/" in cg or "docker-" in cg:
                out["virt_kind"] = "container"; out["virt_type"] = "docker"
            elif "/kubepods" in cg:
                out["virt_kind"] = "container"; out["virt_type"] = "kubernetes"
        if out["virt_kind"] is None:
            rc, _ = run(["test", "-e", "/.dockerenv"])
            if rc == 0:
                out["virt_kind"] = "container"; out["virt_type"] = "docker"
        if out["virt_kind"] is None:
            # WSL leaves a marker in /proc/sys/kernel/osrelease.
            rc, osr = run(["cat", "/proc/sys/kernel/osrelease"])
            if rc == 0 and ("microsoft" in osr.lower() or "wsl" in osr.lower()):
                out["virt_kind"] = "vm"; out["virt_type"] = "wsl"
        if out["virt_kind"] is None:
            rc, cpuinfo = run(["cat", "/proc/cpuinfo"])
            if rc == 0 and re.search(r"^flags\s*:.*\bhypervisor\b", cpuinfo, re.M):
                out["virt_kind"] = "vm"; out["virt_type"] = "unknown"
        if out["virt_kind"] is None:
            out["virt_kind"] = "bare-metal"; out["virt_type"] = "none"

    # Arch emulation: compare uname -m with the arch implied by
    # /proc/cpuinfo. qemu-user-static makes uname report the emulated
    # arch while /proc/cpuinfo still surfaces the host CPU (procfs is
    # not virtualized by user-mode emulation).
    rc, um = run(["uname", "-m"])
    uname_arch = um.strip() if rc == 0 else None
    rc, cpuinfo = run(["cat", "/proc/cpuinfo"])
    host_arch = None
    if rc == 0 and cpuinfo:
        # x86 has "vendor_id" / "model name : Intel|AMD"; arm reports
        # "CPU implementer" / "CPU architecture"; mips: "cpu model :";
        # riscv: "isa : rv64..."; ppc: "cpu : POWER...". We only need
        # to know if the family disagrees with uname.
        if re.search(r"^vendor_id\s*:\s*(GenuineIntel|AuthenticAMD|HygonGenuine|CentaurHauls)",
                     cpuinfo, re.M) or re.search(r"^model name\s*:.*(Intel|AMD)",
                     cpuinfo, re.M):
            host_arch = "x86_64"
        elif re.search(r"^CPU implementer\s*:", cpuinfo, re.M):
            host_arch = "aarch64"  # could be armv7 too; family-level only
        elif re.search(r"^cpu model\s*:.*MIPS", cpuinfo, re.M):
            host_arch = "mips"
        elif re.search(r"^isa\s*:\s*rv", cpuinfo, re.M):
            host_arch = "riscv64"
        elif re.search(r"^cpu\s*:\s*(POWER|PPC)", cpuinfo, re.M):
            host_arch = "ppc64"
    def _fam(a):
        if not a: return None
        a = a.lower()
        if a in ("x86_64", "amd64", "i386", "i486", "i586", "i686"): return "x86"
        if a.startswith("aarch64") or a.startswith("arm"): return "arm"
        if a.startswith("mips"): return "mips"
        if a.startswith("riscv") or a.startswith("rv"): return "riscv"
        if a.startswith("ppc") or a.startswith("powerpc"): return "ppc"
        return a
    if uname_arch and host_arch:
        emulated = (_fam(uname_arch) != _fam(host_arch))
        out["arch_emulated"] = emulated
        if emulated:
            out["host_arch"] = host_arch
    else:
        out["arch_emulated"] = False
    return out


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
            "net_link":      None,
            "net_link_detail": None,
            "wifi_ssid":     None,
            "wifi_standard": None,
            "wifi_band":     None,
            "wifi_link_mbps": None,
            "eth_link_mbps": None,
            "virt_kind":     None,
            "virt_type":     None,
            "arch_emulated": None,
            "host_arch":     None,
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
        link = collect_net_link(self.runner)
        for k in ("net_link", "net_link_detail", "wifi_ssid", "wifi_standard",
                  "wifi_band", "wifi_link_mbps", "eth_link_mbps"):
            self.platform[k] = link.get(k)
        virt = collect_virt(self.runner)
        for k in ("virt_kind", "virt_type", "arch_emulated", "host_arch"):
            self.platform[k] = virt.get(k)
        self.platform["kernel"]    = collect_kernel(self.runner)
        self.platform["libc"]      = collect_libc(self.runner)
        self.platform["compiler"]  = collect_compiler(self.runner)
        self.platform["loadavg_1min_at_start"] = collect_loadavg(self.runner)
        return self

    @staticmethod
    def _norm_metrics(metrics):
        norm = {}
        for name, m in (metrics or {}).items():
            norm[name] = {
                "value":   m.get("value", m.get("median")),
                "unit":    m.get("unit"),
                "better":  m.get("better"),
                "samples": m.get("samples"),
                "median":  m.get("median", m.get("value")),
                "stddev":  m.get("stddev"),
            }
        return norm

    def add_result(self, tool, metrics=None, status="PASS",
                   skip_reason=None, artifact=None):
        """Record one profiler/tool block.

        `metrics` is a dict {name -> {value, unit, better, samples?, median?,
        stddev?}}. Missing optional fields are filled with None so the JSON
        is self-describing across tools.

        Sub-benchmarks (per-workload slices, e.g. 10/50/200 players) are
        added separately via `add_subbenchmark()` and land under
        `results[tool]["subbenchmarks"][label]`."""
        existing = self.results.get(tool, {})
        self.results[tool] = {
            "metrics":       self._norm_metrics(metrics),
            "subbenchmarks": existing.get("subbenchmarks", {}),
            "artifact":      artifact,
            "status":        status,
            "skip_reason":   skip_reason,
        }
        return self

    def add_subbenchmark(self, tool, label, metrics):
        """Record one workload slice under `tool` (e.g. label="10-players").

        Each slice MUST include a `cpu_percent` metric (unit "%",
        better="lower") so reviewers can see whether throughput gains
        came from less CPU or just from using more of it.

        Convention: the server is intentionally single-threaded
        (epoll event loop), so cpu_percent is bounded at 100 %.
        100 % means one core saturated. A value > 100 % means the
        wrong process was timed (probably the bot-client tree) --
        treat as a measurement bug, not as multi-core usage.

        The helper does not synthesize cpu_percent -- capture it from
        `/usr/bin/time -v` ("Percent of CPU this job got") or from
        rusage (utime+stime)/wall_time."""
        entry = self.results.setdefault(tool, {
            "metrics": {}, "subbenchmarks": {},
            "artifact": None, "status": "PASS", "skip_reason": None,
        })
        entry.setdefault("subbenchmarks", {})[label] = self._norm_metrics(metrics)
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
    for n, cpu in ((10, 18.4), (50, 62.1), (200, 188.7)):
        rec.add_subbenchmark("rusage", f"{n}-players", {
            "cpu_percent": {"value": cpu, "unit": "%", "better": "lower"},
            "wall_s":      {"value": 0.05 * n, "unit": "s", "better": "lower"},
        })
    t = iso_now()
    p = rec.write(commit="0"*40, started_utc=t, ended_utc=t,
                  compile_flags=["-O3"], simd_tier="generic",
                  harness_version=harness_version())
    print(f"wrote {p}")
    with open(p) as f:
        sys.stdout.write(f.read())
