#!/usr/bin/env python3
"""setup.py -- install the tools every benchmark*.py needs, on the local
host AND on the ../remote_nodes.json boxes selected by the positional
MODE argument.

  MODE              boxes provisioned (besides the local host)
  ----------------  ---------------------------------------------------
  all  (default)    every compilation node (nodes[].ssh) AND every
                    execution_node
  compile           only the compilation nodes (nodes[].ssh)
  exec              only the execution_nodes

The local host is always provisioned regardless of MODE: per
benchmark/CLAUDE.md it is simultaneously the amd64 compile node and the
amd64 baseline execution node, so it is never a compile-only / exec-only
box. A box that is BOTH a compilation node and an execution_node (same
ssh user/host/port -- e.g. odroid-n2, cb2, rpi-zero-w) is provisioned
once, not twice (dedupe by ssh tuple).

Design goals (see benchmark/CLAUDE.md):

  * One command, no arguments: `python3 setup.py` provisions the whole
    fleet (local host + every compile node + every execution_node).
    No per-node hand-holding.
  * IDEMPOTENT -- a tool is installed ONLY when its binary is missing
    (`command -v` / file check). An already-provisioned node is a fast
    no-op, never a reinstall.
  * OS / OS-version autodetect -- every target is probed via
    /etc/os-release (ID, ID_LIKE, VERSION_ID); the matching package
    manager + per-distro package names are chosen automatically. Same
    script provisions Debian, Ubuntu, Gentoo, Fedora/RHEL, Arch,
    Alpine, openSUSE, Void.
  * Never pip-installs anything (project rule: no pip without ask).
    Charts use the hand-rolled SVG path, so matplotlib is NOT required.

Only the build/orchestration basics (cmake, gcc/g++, make, rsync, git,
python3, size) are REQUIRED -- a benchmark batch cannot start without
them, so a failed install there is a hard FAIL (rc=1).

The profiler back-ends are pulled from benchmark_helpers.PROFILER_
REQUIRED_TOOLS so this can never drift from what the profilers
actually call, but they are best-effort: the harness already SKIPs a
profiler whose binary is absent (benchmark/CLAUDE.md decision matrix
-- a missing metric is "unknown", never a FAIL), so a box that simply
cannot install one (raspbian valgrind vs libc6 Breaks, no portage
~mips keyword for valgrind/perf/heaptrack, heaptrack unpackaged on
gentoo/alpine, sudo password required) degrades to "partial" instead
of failing the whole fleet sweep. The optional accelerators (ninja,
ccache, mold, lld, clang) are best-effort the same way.

Only ENABLED nodes are touched by default (enabled:false compile nodes
and execution_nodes are skipped); ssh connect timeout is 3s so a
powered-off node fails fast instead of stalling the fleet sweep.

Usage:
  python3 setup.py                    # all: local + compile + exec nodes
  python3 setup.py exec               # local + every ENABLED execution_node
  python3 setup.py compile            # local + every ENABLED compile node
  python3 setup.py --dry-run          # print what WOULD run, install nothing
  python3 setup.py --local-only       # only this host
  python3 setup.py --include-disabled # also hit enabled:false nodes
  python3 setup.py exec --node LABEL  # only the named node(s), repeatable
"""
import argparse
import json
import os
import select
import shlex
import signal
import subprocess
import sys
import time

# A remote command silent for this many seconds is presumed wedged
# (hung apt mirror, qemu-mips compile stall, dead ssh channel) and
# killed -- distinct from the per-command/global deadline. The command
# is reported so the operator sees exactly what stalled.
#
# Two tiers: 120s for fast probes (os-release / id -u / tool probe
# should answer in < 1s; 120s = wedged), and a much longer window for
# package install/refresh -- dpkg's "Processing triggers for man-db"
# step on slow ARM boards can sit genuinely silent for several minutes
# (man-db rebuilds its index without printing). 120s killed installs
# that were actually progressing on the far side, so the host reported
# FAIL on a package the remote did finish installing.
REMOTE_IDLE_TIMEOUT         = 120
REMOTE_INSTALL_IDLE_TIMEOUT = 600

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(HERE)
REMOTE_NODES_JSON = os.path.join(os.path.dirname(REPO_ROOT), "remote_nodes.json")

# Reuse the profiler->tools map so this never drifts from the harness.
try:
    sys.path.insert(0, HERE)
    from benchmark_helpers import PROFILER_REQUIRED_TOOLS, load_remote_nodes
except Exception:                                            # pragma: no cover
    PROFILER_REQUIRED_TOOLS = {
        "rusage":      ["/usr/bin/time"],
        "perf-stat":   ["perf"],
        "perf-record": ["perf"],
        "callgrind":   ["valgrind", "callgrind_annotate"],
        "cachegrind":  ["valgrind", "cg_annotate"],
        "heaptrack":   ["heaptrack"],
        "binary-size": [],
    }

    def load_remote_nodes():
        if not os.path.isfile(REMOTE_NODES_JSON):
            return {"nodes": []}
        with open(REMOTE_NODES_JSON) as f:
            return json.load(f)

C_GREEN, C_YELLOW, C_RED, C_CYAN, C_RESET = (
    "\033[92m", "\033[93m", "\033[91m", "\033[96m", "\033[0m")


def _disabled_tools_for_label(label):
    """benchmark_disabled_tools from remote_nodes.json for a node label.

    Same field the benchmark harness honours -- setup.py now reads it
    too so a tool can be turned off purely by config (e.g. valgrind on
    the mips boxes: no ~mips portage keyword => emerge-from-source under
    qemu = the mips-lxc disaster). Matched by label across BOTH
    execution_nodes[] and the parent nodes[] compile entry that shares
    the label (the mips-lxc box is both); union so either placement of
    the field works."""
    if not label or label == "local":
        return set()
    out = set()
    try:
        cfg = load_remote_nodes()
    except Exception:
        return out
    for node in cfg.get("nodes", []):
        node_lbl = node.get("label")
        execs = node.get("execution_nodes", []) or []
        if node_lbl == label:
            out.update(node.get("benchmark_disabled_tools", []) or [])
            for ex in execs:
                if ex.get("label") == label:
                    out.update(ex.get("benchmark_disabled_tools", []) or [])
        else:
            for ex in execs:
                if ex.get("label") == label:
                    out.update(ex.get("benchmark_disabled_tools", []) or [])
    return out

# ---------------------------------------------------------------------------
# Logical tools the benchmark workspace needs.
#
#   key   -> the binary we test for with `command -v` (or a file path).
#   value -> ("required"|"optional", why)
#
# valgrind ships callgrind_annotate + cg_annotate, so testing the
# `valgrind` binary covers the callgrind/cachegrind profilers too; the
# *_annotate names are still listed so a distro that splits them gets
# them pulled in via the package map.
# ---------------------------------------------------------------------------
def _profiler_tool_set():
    tools = set()
    for need in PROFILER_REQUIRED_TOOLS.values():
        for t in need:
            tools.add(t)
    return tools


# Build / orchestration basics every benchmark*.py shells out to. A
# benchmark batch literally cannot run without these, so a failed
# install here is a hard FAIL (rc=1, red in the summary).
CORE_TOOLS = sorted({"cmake", "gcc", "g++", "make", "rsync", "git",
                     "python3", "size"})

# Profiler back-ends (perf, valgrind + callgrind/cachegrind annotators,
# heaptrack) plus the rusage /usr/bin/time. "Meaningful where
# available": the harness already SKIPs a profiler whose binary is
# absent -- per benchmark/CLAUDE.md the decision matrix treats a
# missing metric as "unknown", never a FAIL. So a box that cannot
# install one (raspbian valgrind vs libc6 Breaks, no portage ~mips
# keyword for valgrind/perf/heaptrack, heaptrack unpackaged on
# gentoo/alpine, sudo password required) must degrade to "partial",
# not fail the whole fleet sweep. Installed best-effort, like the
# optional accelerators below.
PROFILER_TOOLS = sorted(_profiler_tool_set())

OPTIONAL_TOOLS = ["ninja", "ccache", "mold", "ld.lld", "clang",
                  "gdb", "sensors"]

# Per-package-manager-family tool exclusions: tools that simply do not
# exist for that distro and must not be probed, listed as "missing", or
# install-requested. heaptrack has no ebuild in the Gentoo tree, so on
# every emerge (Gentoo) node it is excluded outright -- the bench harness
# falls back to callgrind/massif there.
FAMILY_EXCLUDED_TOOLS = {
    "emerge": {"heaptrack"},
}

# Map a probe binary -> the logical package key used in PKG_MAP below.
TOOL_TO_PKG = {
    "/usr/bin/time": "time",
    "perf": "perf",
    "valgrind": "valgrind",
    "callgrind_annotate": "valgrind",
    "cg_annotate": "valgrind",
    "heaptrack": "heaptrack",
    "size": "binutils",
    "cmake": "cmake",
    "gcc": "gcc",
    "g++": "gxx",
    "make": "make",
    "rsync": "rsync",
    "git": "git",
    "python3": "python3",
    "ninja": "ninja",
    "ccache": "ccache",
    "mold": "mold",
    "ld.lld": "lld",
    "clang": "clang",
    "gdb": "gdb",
    "sensors": "lm_sensors",
}

# Per package-manager family: logical package key -> distro package name.
# `None` means "skip on this distro" (not packaged / provided elsewhere).
PKG_MAP = {
    "apt": {
        "time": "time", "perf": "linux-perf", "valgrind": "valgrind",
        "heaptrack": "heaptrack", "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "g++", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python3", "ninja": "ninja-build",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "lm-sensors",
    },
    "dnf": {
        "time": "time", "perf": "perf", "valgrind": "valgrind",
        "heaptrack": "heaptrack", "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "gcc-c++", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python3", "ninja": "ninja-build",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "lm_sensors",
    },
    "zypper": {
        "time": "time", "perf": "perf", "valgrind": "valgrind",
        "heaptrack": "heaptrack", "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "gcc-c++", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python3", "ninja": "ninja",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "sensors",
    },
    "pacman": {
        "time": "time", "perf": "perf", "valgrind": "valgrind",
        "heaptrack": "heaptrack", "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "gcc", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python", "ninja": "ninja",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "lm_sensors",
    },
    "apk": {
        "time": "time", "perf": "perf", "valgrind": "valgrind",
        "heaptrack": None, "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "g++", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python3", "ninja": "ninja",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "lm-sensors",
    },
    "emerge": {
        "time": "sys-process/time", "perf": "dev-util/perf",
        "valgrind": "dev-debug/valgrind", "heaptrack": None,
        "binutils": "sys-devel/binutils", "cmake": "dev-build/cmake",
        "gcc": "sys-devel/gcc", "gxx": "sys-devel/gcc",
        "make": "dev-build/make", "rsync": "net-misc/rsync",
        "git": "dev-vcs/git", "python3": "dev-lang/python",
        "ninja": "dev-build/ninja", "ccache": "dev-util/ccache",
        "mold": "sys-devel/mold", "lld": "llvm-core/lld",
        "clang": "llvm-core/clang", "gdb": "dev-debug/gdb",
        "lm_sensors": "sys-apps/lm-sensors",
    },
    "xbps": {
        "time": "time", "perf": "perf", "valgrind": "valgrind",
        "heaptrack": "heaptrack", "binutils": "binutils", "cmake": "cmake",
        "gcc": "gcc", "gxx": "gcc", "make": "make", "rsync": "rsync",
        "git": "git", "python3": "python3", "ninja": "ninja",
        "ccache": "ccache", "mold": "mold", "lld": "lld", "clang": "clang",
        "gdb": "gdb", "lm_sensors": "lm_sensors",
    },
}

# Build-time library dependencies (headers + .pc/.so) for compiling the
# server / benchmark binaries: blake3, xxhash, zlib, zstd, tinyxml2.
# These are BEST-EFFORT (added to opt_missing_pkgs, not required): the
# build prefers the system .so but falls back to the in-tree vendored
# copy when find_library() can't locate it (general/CCCommon.cmake
# system-first/embed-fallback). So a node missing libblake3-dev simply
# compiles general/blake3 instead -- it never fails the fleet sweep.
# hps is NOT listed: it is header-only and vendored-only (general/hps),
# upstream ships no system package.
# `None` entry => that lib isn't packaged on the distro (e.g. no official
# libblake3 on Arch/Alpine) -> skipped, vendored copy used.
# One descriptor per build-time lib. `pc`/`hdr` drive a presence probe
# (pkg-config name(s) + header(s)) so an already-installed lib is NEVER
# re-requested -- only genuinely-absent libs reach the install command.
# `pkg` maps the package-manager family to its atom; None => that lib is
# not packaged on the distro (e.g. no official libblake3 on Arch/Alpine/
# Void) -> skipped, the in-tree vendored copy is used.
BUILD_DEP_LIBS = [
    {"name": "zlib", "pc": ["zlib"], "hdr": ["zlib.h"],
     "pkg": {"apt": "zlib1g-dev", "dnf": "zlib-devel", "zypper": "zlib-devel",
             "pacman": "zlib", "apk": "zlib-dev", "emerge": "sys-libs/zlib",
             "xbps": "zlib-devel"}},
    {"name": "zstd", "pc": ["libzstd"], "hdr": ["zstd.h"],
     "pkg": {"apt": "libzstd-dev", "dnf": "libzstd-devel", "zypper": "libzstd-devel",
             "pacman": "zstd", "apk": "zstd-dev", "emerge": "app-arch/zstd",
             "xbps": "libzstd-devel"}},
    {"name": "blake3", "pc": ["libblake3"], "hdr": ["blake3.h"],
     "pkg": {"apt": "libblake3-dev", "dnf": "libblake3-devel", "zypper": "libblake3-devel",
             "pacman": None, "apk": None, "emerge": "dev-libs/blake3",
             "xbps": None}},
    {"name": "xxhash", "pc": ["libxxhash"], "hdr": ["xxhash.h"],
     "pkg": {"apt": "libxxhash-dev", "dnf": "xxhash-devel", "zypper": "xxhash-devel",
             "pacman": "xxhash", "apk": "xxhash-dev", "emerge": "dev-libs/xxhash",
             "xbps": "xxhash-devel"}},
    {"name": "tinyxml2", "pc": ["tinyxml2"], "hdr": ["tinyxml2.h"],
     "pkg": {"apt": "libtinyxml2-dev", "dnf": "tinyxml2-devel", "zypper": "tinyxml2-devel",
             "pacman": "tinyxml2", "apk": "tinyxml2-dev", "emerge": "dev-libs/tinyxml2",
             "xbps": "tinyxml2-devel"}},
]

# NOTE: setup.py NEVER refreshes package metadata. No `apt-get update`,
# `pacman -Sy`, `apk update`, `zypper refresh`, `emerge --sync` -- these
# hit the network to rebuild the index (and `emerge --sync` rebuilds the
# whole portage tree). The install commands run against the index already
# present on the node; a stale index simply means a package is skipped
# (best-effort libs fall back to the in-tree vendored copy). Keeping the
# whole flow offline-as-possible avoids the long network stalls seen on
# slow / firewalled boxes.

# Install ONE package ({pkg}). Per-package, never batched: a single
# masked/unavailable tool (e.g. dev-debug/valgrind has no keyword on
# mips2r2) must NOT block the installable ones (time/perf/heaptrack).
INSTALL_ONE_CMD = {
    "apt":    "export DEBIAN_FRONTEND=noninteractive; "
              "apt-get install -y --no-install-recommends "
              "-o DPkg::Lock::Timeout=0 {pkg}",
    "dnf":    "dnf install -y {pkg}",
    "zypper": "zypper --non-interactive install -y {pkg}",
    "pacman": "pacman -S --noconfirm --needed {pkg}",
    "apk":    "apk add --no-cache {pkg}",
    "emerge": "emerge --noreplace --quiet-build=y {pkg}",
    "xbps":   "xbps-install -Sy {pkg}",
}

# "Is {pkg} available in the index already on this node?" -- prints nothing,
# exit 0 = available. Queries the LOCAL package metadata only (no network /
# no refresh, matching the no-refresh install policy). Used to skip a
# best-effort lib that simply isn't packaged on this OS *version* (e.g.
# libblake3-dev exists on Debian 13 but not 12) so it is never listed as a
# missing install -- the in-tree vendored copy is used instead. A family
# with no entry here (or emerge, whose atoms we trust) is assumed available.
PKG_AVAILABLE_CMD = {
    "apt":    "apt-cache show {pkg} 2>/dev/null | grep -q '^Package:'",
    "dnf":    "dnf -C -q list {pkg} >/dev/null 2>&1",
    "zypper": "zypper -C -n info {pkg} >/dev/null 2>&1",
    "pacman": "pacman -Si {pkg} >/dev/null 2>&1",
    "apk":    "apk list {pkg} 2>/dev/null | grep -q .",
    "xbps":   "xbps-query -R {pkg} >/dev/null 2>&1",
}

# os-release ID / ID_LIKE token  ->  package manager family.
ID_TO_FAMILY = {
    "debian": "apt", "ubuntu": "apt", "linuxmint": "apt", "raspbian": "apt",
    "devuan": "apt", "pop": "apt",
    "fedora": "dnf", "rhel": "dnf", "centos": "dnf", "rocky": "dnf",
    "almalinux": "dnf", "amzn": "dnf", "ol": "dnf",
    "opensuse": "zypper", "opensuse-leap": "zypper",
    "opensuse-tumbleweed": "zypper", "sles": "zypper", "suse": "zypper",
    "arch": "pacman", "archarm": "pacman", "manjaro": "pacman",
    "endeavouros": "pacman",
    "alpine": "apk",
    "gentoo": "emerge",
    "void": "xbps",
}


def family_from_os_release(osr):
    """osr is a dict parsed from /etc/os-release. Resolve via ID first,
    then each ID_LIKE token. Return (family, distro_label) or (None, label)."""
    idv = (osr.get("ID") or "").strip().lower()
    label = "%s %s" % (idv or "unknown", osr.get("VERSION_ID", "") or "")
    if idv in ID_TO_FAMILY:
        return ID_TO_FAMILY[idv], label.strip()
    for tok in (osr.get("ID_LIKE") or "").lower().split():
        if tok in ID_TO_FAMILY:
            return ID_TO_FAMILY[tok], label.strip()
    return None, label.strip()


# ---------------------------------------------------------------------------
# Command execution -- local and over ssh, same interface.
# ---------------------------------------------------------------------------
class Target:
    """A provisioning target: either the local host or an ssh node."""

    def __init__(self, label, ssh=None, deadline=None, lxc_node=None):
        self.label = label
        self.ssh = ssh            # None => local; else (user, host, port)
        # Absolute time.monotonic() past which NO command may run. None =>
        # unbounded. Every run() clamps its per-command timeout to the
        # budget still left so one slow node (flaky apt mirror, emerge
        # from source) can never push the whole fleet sweep past the
        # global --deadline the operator set.
        self.deadline = deadline
        # For exec nodes whose `lxc_nfs.enabled` is true (e.g. rtl9607c):
        # the FULL execution_nodes dict from remote_nodes.json. provision()
        # uses it to drive nfs_lxc_bring_up before any probe -- the outer
        # box is an OpenWrt router with no useful pkg manager; setup must
        # happen INSIDE the container, so `ssh` already points at
        # lxc_nfs.guest_ipv6 (collect_targets does that retargeting).
        self.lxc_node = lxc_node

    def run(self, cmd, timeout, idle_timeout=None):
        """Run a shell command; return (rc, combined_output). The
        per-command timeout is clamped to whatever is left of the global
        deadline; once the deadline has passed the command is refused
        outright instead of started and timed out. `idle_timeout`
        (remote only, default REMOTE_IDLE_TIMEOUT) kills the command
        early when it produces NO output for that many seconds -- pass
        REMOTE_INSTALL_IDLE_TIMEOUT for package install/refresh, whose
        dpkg/emerge phases can be silent for minutes on slow ARM."""
        if self.deadline is not None:
            remaining = self.deadline - time.monotonic()
            if remaining <= 1:
                return 124, "[global deadline reached -- command skipped]"
            timeout = min(timeout, int(remaining))
        if self.ssh is None:
            argv = ["sh", "-c", cmd]
        else:
            u, h, p = self.ssh
            # REMOTE-side self-bounding. The mips-lxc disaster: setup.py
            # ran `emerge dev-debug/valgrind` over ssh; the host process
            # died but the remote emerge+make -j32 tree (under qemu-mips)
            # kept compiling for hours -- ssh-client death does NOT kill
            # the remote command, and the host can't reach into a
            # container's pid-ns / different-uid procs to clean it up.
            # Fix: run the remote command in its OWN session (setsid =>
            # process-group leader) with a remote watchdog that, after
            # the budget, signals the WHOLE group (kill -TERM -$P, then
            # -KILL). Now the remote box kills its own build tree
            # regardless of what happens to the ssh client -- no orphan
            # can outlive the budget. setsid absent (ultra-minimal
            # rootfs) => fall back to the bare command (rare; those
            # nodes have no emerge-from-source tarpit anyway).
            qcmd = "'" + cmd.replace("'", "'\\''") + "'"
            # The watchdog ALSO runs under its own setsid session with its
            # fds sent to /dev/null. Two reasons, both learned the hard way:
            #  (1) A plain `( sleep N; kill ... ) &` subshell inherits ssh's
            #      stdout, and `kill $W` kills only the subshell -- its
            #      `sleep` child is orphaned and KEEPS the ssh channel's
            #      stdout open, so the client never sees EOF and blocks for
            #      the full idle-timeout even though the real command already
            #      finished (observed as a "stuck doing nothing" install of
            #      an unavailable package that errored out instantly).
            #  (2) setsid makes W a process-group leader, so `kill -TERM -$W`
            #      reaps the whole watchdog group (the `sleep` included), not
            #      just the leader. /dev/null redirection guarantees that
            #      even a stray survivor can't hold the channel open.
            remote = (
                'if command -v setsid >/dev/null 2>&1; then '
                'O=$(mktemp 2>/dev/null || echo /tmp/.cc_setup.$$); '
                'setsid sh -c %s >"$O" 2>&1 & P=$!; '
                'setsid sh -c "sleep %d; kill -TERM -$P 2>/dev/null; '
                'sleep 10; kill -KILL -$P 2>/dev/null" >/dev/null 2>&1 '
                '</dev/null & W=$!; '
                'wait $P 2>/dev/null; RC=$?; '
                'kill -TERM -$W 2>/dev/null; '
                'cat "$O" 2>/dev/null; rm -f "$O"; exit $RC; '
                'else sh -c %s 2>&1; fi'
            ) % (qcmd, int(timeout), qcmd)
            argv = ["ssh", "-o", "ConnectTimeout=3", "-o", "BatchMode=yes",
                    "-o", "StrictHostKeyChecking=accept-new",
                    "-o", "ServerAliveInterval=15", "-o", "ServerAliveCountMax=3",
                    "-p", str(p), "%s@%s" % (u, h), remote]
        # Host-side backstop = remote budget + 30s slack so the remote
        # watchdog (TERM then KILL-after-10s) is the primary killer and
        # we still collect its output; the host cap only trips if ssh
        # itself wedged. On top of that, a REMOTE command that produces
        # no output for REMOTE_IDLE_TIMEOUT is presumed hung and killed
        # early -- a stalled apt mirror / qemu compile can sit silent
        # for the full budget otherwise. Stream the output so we can
        # measure inter-output gaps; kill the whole local process-group
        # (start_new_session) on idle/total, and report the command.
        host_to = timeout if self.ssh is None else timeout + 30
        if self.ssh is None:
            idle = None
        else:
            idle = idle_timeout if idle_timeout is not None else REMOTE_IDLE_TIMEOUT
        try:
            p = subprocess.Popen(argv, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT,
                                  start_new_session=True)
        except Exception as e:                               # pragma: no cover
            return 255, "[exec error: %s]" % e
        buf = []
        fd = p.stdout.fileno()
        start = time.monotonic()
        last = start
        killed = None                       # None | "idle" | "total"
        while True:
            now = time.monotonic()
            if now - start >= host_to:
                killed = "total"
                break
            waits = [host_to - (now - start)]
            if idle is not None:
                waits.append(idle - (now - last))
            r, _, _ = select.select([fd], [], [], max(0.5, min(waits)))
            if r:
                try:
                    chunk = os.read(fd, 65536)
                except OSError:
                    chunk = b""
                if not chunk:
                    break                   # EOF -> process done writing
                buf.append(chunk)
                last = time.monotonic()
            elif idle is not None and time.monotonic() - last >= idle:
                killed = "idle"
                break
            elif time.monotonic() - start >= host_to:
                killed = "total"
                break
        if killed:
            # SIGTERM then SIGKILL the whole local group (ssh + kids);
            # the remote setsid watchdog cleans the remote tree itself.
            for sig in (signal.SIGTERM, signal.SIGKILL):
                try:
                    os.killpg(p.pid, sig)
                except Exception:
                    pass
                try:
                    p.wait(timeout=5)
                    break
                except Exception:
                    continue
            out = b"".join(buf).decode("utf-8", "replace")
            if killed == "idle":
                msg = ("[KILLED: no output for %ds on %s -- command: %s]"
                       % (idle, self.label, cmd))
                print("  %s%s%s" % (C_RED, msg, C_RESET),
                      file=sys.stderr, flush=True)
                return 124, (out + "\n" + msg).strip()
            return 124, (out + "\n[timeout after %ss]" % timeout).strip()
        try:
            rest = p.stdout.read()
            if rest:
                buf.append(rest)
        except Exception:
            pass
        p.wait()
        return p.returncode, b"".join(buf).decode("utf-8", "replace")


def parse_os_release(text):
    osr = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, _, v = line.partition("=")
        osr[k.strip()] = v.strip().strip('"').strip("'")
    return osr


def have_tool_probe(tool, world_pkg=None):
    """Shell snippet that prints Y/N for one tool. /usr/bin/time is a
    file (the shell builtin `time` is NOT GNU time and has no -v).

    On Gentoo a tool can be installed yet `command -v` still miss it:
    clang/lld land in /usr/lib/llvm/*/bin and the unversioned name is
    an eselect-managed symlink that may be unset, so `command -v clang`
    fails even though llvm-core/clang IS emerged -> setup.py wrongly
    re-emerges it every run (the "not detected but exists" case). When
    an emerge package atom is known, ALSO accept the tool as present if
    that atom is recorded in /var/lib/portage/world."""
    if tool.startswith("/"):
        base = '[ -x %s ]' % tool
    else:
        base = 'command -v %s >/dev/null 2>&1' % tool
    if world_pkg:
        base = ('%s || grep -qF -- %s /var/lib/portage/world 2>/dev/null'
                % (base, shlex.quote(world_pkg)))
    return 'if %s; then echo Y; else echo N; fi' % base


def have_lib_probe(pcs, hdrs, world_pkg=None):
    """Shell snippet that prints Y/N for one build-time library. A lib is
    "present" if pkg-config knows it OR its header is on the include path
    (covers distros that ship the .so+headers without a .pc, and headers
    dropped under /usr/include/<triplet>/). On Gentoo also accept the
    atom recorded in /var/lib/portage/world. Mirrors have_tool_probe so
    an already-installed lib is detected and never re-requested."""
    checks = []
    pi = 0
    while pi < len(pcs):
        checks.append('pkg-config --exists %s 2>/dev/null' % shlex.quote(pcs[pi]))
        pi += 1
    hi = 0
    while hi < len(hdrs):
        h = hdrs[hi]
        checks.append('ls /usr/include/%s /usr/local/include/%s '
                      '/usr/include/*/%s 2>/dev/null | grep -q .'
                      % (h, h, h))
        hi += 1
    base = ' || '.join('{ %s ; }' % c for c in checks)
    if world_pkg:
        base = ('%s || grep -qF -- %s /var/lib/portage/world 2>/dev/null'
                % (base, shlex.quote(world_pkg)))
    return 'if %s; then echo Y; else echo N; fi' % base


def _priv_cmd(cmd):
    """Wrap a full shell pipeline so the WHOLE thing runs under one
    `sh -c` (the old `sudo -n EXPORT; apt-get` only sudo'd the export,
    so apt-get ran unprivileged -> dpkg-lock failure) and strictly
    non-interactive (`</dev/null` => any tool that still reads stdin
    gets immediate EOF, never hangs).

    No `sudo`: setup.py installs packages ONLY when it is already root.
    A non-root node with missing packages is failed up-front by the
    caller with the exact root command to run by hand -- setup.py never
    escalates privilege itself (project rule: never auto-install;
    request it from the operator)."""
    return "sh -c %s </dev/null" % shlex.quote(cmd)


def _install_many(family, pkgs):
    """ONE command installing ALL pkgs in a single invocation (not
    one-per-package). Every INSTALL_ONE_CMD template ends with the
    package argument, so a space-joined list just works. emerge gets
    --keep-going so one masked/unbuildable atom can't abort the whole
    batch -- the rest still install and the bench harness SKIPs an
    absent optional profiler on its own."""
    tmpl = INSTALL_ONE_CMD[family]
    if family == "emerge":
        # --keep-going: one bad atom doesn't abort the batch.
        # --noreplace: skip atoms already installed / up to date (don't
        # rebuild them) -- idempotent re-runs stay fast.
        for flag in ("--keep-going", "--noreplace"):
            if flag not in tmpl:
                tmpl = tmpl.replace("emerge ", "emerge %s " % flag, 1)
    return tmpl.format(pkg=" ".join(pkgs))


# ---------------------------------------------------------------------------
# Provision one target.
# ---------------------------------------------------------------------------
def provision(tgt, dry_run, conn_timeout, install_timeout):
    """Public entry point. For lxc_nfs nodes (e.g. rtl9607c) the outer
    box is an OpenWrt router with no useful pkg manager; setup MUST run
    INSIDE the container. Bring up the LXC (mount NFS + lxc-start --
    defensively stop+umount first, then mount+start), provision the
    GUEST, then tear down (best-effort). All ssh-to-guest work flows
    through the existing pipeline because collect_targets already
    retargeted tgt.ssh to lxc_nfs.guest_ipv6."""
    if tgt.lxc_node is not None:
        try:
            import benchmark_remote as br
        except Exception as e:
            print("%s=== %s ===%s" % (C_CYAN, tgt.label, C_RESET))
            print("  %s[SKIP]%s lxc_nfs node but benchmark_remote "
                  "unavailable: %s" % (C_YELLOW, C_RESET, e))
            return "skip"
        print("%s[lxc_nfs %s]%s bring-up: stop/umount -> mount NFS -> "
              "lxc-start (defensive remount of catchchallenger guest) ..."
              % (C_CYAN, tgt.label, C_RESET))
        _ok, _tail = br.nfs_lxc_bring_up(tgt.lxc_node, verbose=True)
        if not _ok:
            print("%s[lxc_nfs %s]%s bring-up FAILED -- not provisioning"
                  % (C_YELLOW, tgt.label, C_RESET))
            return "unreachable"
    try:
        return _provision_inner(tgt, dry_run, conn_timeout, install_timeout)
    finally:
        if tgt.lxc_node is not None:
            try:
                import benchmark_remote as br
                br.nfs_lxc_teardown(tgt.lxc_node, verbose=True)
                print("%s[lxc_nfs %s]%s stopped + unmounted (host clean)"
                      % (C_CYAN, tgt.label, C_RESET))
            except Exception as e:
                print("%s[lxc_nfs %s]%s teardown failed (best-effort): %s"
                      % (C_YELLOW, tgt.label, C_RESET, e))


def _provision_inner(tgt, dry_run, conn_timeout, install_timeout):
    print("%s=== %s ===%s" % (C_CYAN, tgt.label, C_RESET))

    if tgt.deadline is not None and time.monotonic() >= tgt.deadline:
        print("  %s[SKIP]%s global deadline reached before this node "
              "-- not provisioned" % (C_YELLOW, C_RESET))
        return "deadline"

    # `|| true` keeps rc==0 on a reachable box even with no os-release, so a
    # non-zero rc (or an ssh diagnostic on stdout) means the box is down.
    rc, out = tgt.run("cat /etc/os-release 2>/dev/null || true", conn_timeout)
    if "global deadline reached" in out:
        print("  %s[SKIP]%s global deadline reached -- not provisioned"
              % (C_YELLOW, C_RESET))
        return "deadline"
    low = out.lower()
    unreachable_sig = ("connection refused", "connection timed out",
                       "no route to host", "could not resolve",
                       "operation timed out", "permission denied",
                       "host key verification failed", "connection closed")
    if rc != 0 or any(s in low for s in unreachable_sig):
        print("  %s[UNREACHABLE]%s %s"
              % (C_YELLOW, C_RESET, out.strip().splitlines()[-1][:200]
                 if out.strip() else "(no response, rc=%s)" % rc))
        return "unreachable"
    osr = parse_os_release(out)
    family, distro = family_from_os_release(osr)
    if family is None:
        print("  %s[SKIP]%s unknown distro (ID=%r ID_LIKE=%r) -- no package "
              "manager mapping" % (C_YELLOW, C_RESET,
                                   osr.get("ID"), osr.get("ID_LIKE")))
        return "skip"

    # Are we root? setup.py never sudos: root installs directly, a
    # non-root node with missing packages is failed with the exact
    # AS-ROOT commands for the operator to run by hand.
    rc, who = tgt.run("id -u", conn_timeout)
    is_root = who.strip() == "0"

    print("  distro=%s  pkg-manager=%s  root=%s"
          % (distro, family, "yes" if is_root else "no"))


    # Config-driven tool disable: a node's benchmark_disabled_tools list
    # in remote_nodes.json (matched by node label) is honoured here too,
    # so e.g. valgrind on the mips boxes is skipped purely by config --
    # never emerge-from-source under qemu (the mips-lxc disaster), no
    # arch logic hard-coded in Python.
    node_disabled = _disabled_tools_for_label(tgt.label)

    # One round-trip: probe every tool we care about. CORE_TOOLS,
    # PROFILER_TOOLS and OPTIONAL_TOOLS are disjoint by construction.
    # Drop tools this distro family doesn't package (e.g. heaptrack on
    # Gentoo) so they are never probed, listed, or install-requested.
    family_excluded = FAMILY_EXCLUDED_TOOLS.get(family, set())
    probe_tools = [t for t in (CORE_TOOLS + PROFILER_TOOLS + OPTIONAL_TOOLS)
                   if t not in family_excluded]

    def _world_pkg(t):
        # only emerge has /var/lib/portage/world; map tool -> atom
        if family != "emerge":
            return None
        return PKG_MAP["emerge"].get(TOOL_TO_PKG.get(t, ""), "") or None

    probe_parts = ['printf "%%s " %s ; %s'
                   % (t, have_tool_probe(t, _world_pkg(t)))
                   for t in probe_tools]
    # Same round-trip: probe each build-time lib so an already-installed
    # one is detected and dropped from the install request below; AND probe
    # whether its package is available in this node's index so a lib not
    # packaged on this OS version is skipped, not install-requested.
    avail_tmpl = PKG_AVAILABLE_CMD.get(family)
    for lib in BUILD_DEP_LIBS:
        wpkg = lib["pkg"].get("emerge") if family == "emerge" else None
        probe_parts.append(
            'printf "%%s " lib_%s ; %s'
            % (lib["name"], have_lib_probe(lib["pc"], lib["hdr"], wpkg)))
        pkg = lib["pkg"].get(family)
        if pkg and avail_tmpl:
            chk = avail_tmpl.format(pkg=shlex.quote(pkg))
            probe_parts.append(
                'printf "%%s " avail_%s ; if %s; then echo Y; else echo N; fi'
                % (lib["name"], chk))
    probe = " ; ".join(probe_parts)
    rc, out = tgt.run(probe, conn_timeout)
    present = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2:
            present[parts[0]] = (parts[1] == "Y")

    pmap = PKG_MAP[family]
    req_missing_pkgs, opt_missing_pkgs = [], []
    have, missing = [], []
    for t in CORE_TOOLS:
        if present.get(t, False):
            have.append(t)
        else:
            missing.append(t)
            pkg = pmap.get(TOOL_TO_PKG.get(t, ""), "")
            if pkg and pkg not in req_missing_pkgs:
                req_missing_pkgs.append(pkg)
    # Profilers + accelerators are best-effort: a FAIL here only marks
    # the node "partial" (see _install_pkg), never "fail" -- the bench
    # harness SKIPs an absent profiler on its own.
    skipped_cfg = []
    for t in PROFILER_TOOLS + OPTIONAL_TOOLS:
        if t in family_excluded:
            continue                        # unpackaged on this distro family
        if t in node_disabled:
            skipped_cfg.append(t)           # benchmark_disabled_tools config
            continue
        if present.get(t, False):
            have.append(t)
        else:
            missing.append(t)
            pkg = pmap.get(TOOL_TO_PKG.get(t, ""), "")
            if pkg and pkg not in opt_missing_pkgs:
                opt_missing_pkgs.append(pkg)
    if skipped_cfg:
        print("  %s[SKIP]%s benchmark_disabled_tools (remote_nodes.json): %s"
              % (C_YELLOW, C_RESET, ", ".join(sorted(skipped_cfg))))

    # Build-time library deps (blake3/xxhash/zlib/zstd/tinyxml2) for
    # compiling the server. BEST-EFFORT, not required: every one has an
    # in-tree vendored fallback (general/*), so a node that can't install
    # the system dev package just compiles the embedded copy. Added to
    # opt_missing_pkgs => an uninstallable lib degrades the node to
    # "partial", never hard-fails the fleet sweep. The package manager is
    # idempotent, so already-present packages are silently skipped.
    lib_have, lib_missing, lib_unavail = [], [], []
    for lib in BUILD_DEP_LIBS:
        pkg = lib["pkg"].get(family)
        if pkg is None:
            continue                        # not packaged here -> vendored copy
        if present.get("lib_" + lib["name"], False):
            lib_have.append(lib["name"])    # already installed -> don't re-request
        elif avail_tmpl and not present.get("avail_" + lib["name"], True):
            # missing AND not in the index on this OS version (e.g.
            # libblake3-dev on Debian 12) -> use the vendored copy, never
            # request an install that would just error "Unable to locate".
            lib_unavail.append(lib["name"])
        else:
            lib_missing.append(lib["name"])
            if pkg not in opt_missing_pkgs and pkg not in req_missing_pkgs:
                opt_missing_pkgs.append(pkg)
    print("  libs    : present %s | missing %s%s"
          % (", ".join(lib_have) or "(none)",
             ", ".join(lib_missing) or "(none)",
             (" | not-packaged-here " + ", ".join(lib_unavail))
             if lib_unavail else ""))

    print("  present : %s" % (", ".join(sorted(have)) or "(none)"))
    print("  missing : %s" % (", ".join(sorted(missing)) or "(none -- all set)"))

    status = "ok"            # ok < partial < fail
    if not req_missing_pkgs and not opt_missing_pkgs:
        print("  %s[NOTHING TO DO]%s" % (C_GREEN, C_RESET))
        return status

    # Install each missing package on ITS OWN, never as one batch. A single
    # unavailable atom must not abort the installable ones -- e.g.
    # libblake3-dev exists on Debian 13 but NOT Debian 12, and a batched
    # `apt-get install libblake3-dev libtinyxml2-dev` aborts on the missing
    # one so neither lands. Per-package: the unavailable best-effort lib is
    # SKIPPED (the in-tree vendored copy is used) and the installable ones
    # still go in. Required (core-tool) failures fail the node; optional
    # (best-effort lib) failures only degrade it to "partial".
    req_set     = set(req_missing_pkgs)
    all_pkgs    = list(dict.fromkeys(req_missing_pkgs + opt_missing_pkgs))
    fail_status = "fail" if req_missing_pkgs else "partial"

    # No sudo, ever. Not root + packages missing => do NOT escalate; hand
    # the operator the per-package commands to run AS ROOT (project rule:
    # never auto-install -- request it).
    if not is_root:
        print("  %s[NEEDS ROOT]%s not root and packages are missing; "
              "setup.py does not sudo. Run AS ROOT on %s:"
              % (C_RED, C_RESET, tgt.label))
        pi = 0
        while pi < len(all_pkgs):
            print("    %s" % _install_many(family, [all_pkgs[pi]]))
            pi += 1
        return fail_status

    if dry_run:
        pi = 0
        while pi < len(all_pkgs):
            print("  %s[DRY-RUN install]%s %s"
                  % (C_YELLOW, C_RESET,
                     _priv_cmd(_install_many(family, [all_pkgs[pi]]))))
            pi += 1
        return status

    # "package not available on this OS / OS version" sentinels, per family.
    # An optional lib that trips one of these is skipped to vendored, not
    # treated as a hard error (no network refresh masked a fresh package --
    # the index simply has no such atom on this release).
    unavail_sigs = ("unable to locate package", "no match for argument",
                    "no candidate version", "unable to find a match",
                    "target not found", "no such package",
                    "no package", "not found in the repositories",
                    "couldn't find any package")

    ok_pkgs, skipped_opt, failed_req = [], [], []
    # No metadata refresh: each install runs against the index already on
    # the node (see the "NEVER refreshes package metadata" note above).
    pi = 0
    while pi < len(all_pkgs):
        pkg = all_pkgs[pi]
        pi += 1
        required    = pkg in req_set
        install_raw = _install_many(family, [pkg])
        cmd         = _priv_cmd(install_raw)
        budget_left = ("" if tgt.deadline is None
                       else " (budget %ds left)"
                            % max(0, int(tgt.deadline - time.monotonic())))
        print("  %s-> installing %s on %s%s ...%s"
              % (C_CYAN, pkg, tgt.label, budget_left, C_RESET))
        print("    $ %s" % install_raw)
        t0 = time.monotonic()
        rc, out = tgt.run(cmd, install_timeout,
                          idle_timeout=REMOTE_INSTALL_IDLE_TIMEOUT)
        low = out.lower()

        if rc == 0:
            print("  %s[OK install]%s %s" % (C_GREEN, C_RESET, pkg))
            ok_pkgs.append(pkg)
        else:
            # dpkg frontend lock held by another apt/dpkg -> fatal for the
            # whole node, do NOT keep trying the rest.
            if family == "apt" and ("could not get lock" in low
                                    or "unable to acquire the dpkg frontend "
                                       "lock" in low):
                print("  %s[FAIL install]%s dpkg frontend lock held by another"
                      " process on %s -- not waiting/retrying\n    $ %s"
                      % (C_RED, C_RESET, tgt.label, cmd))
                return fail_status

            tail = "\n".join("    " + l
                             for l in out.strip().splitlines()[-8:])
            if required:
                if rc == 124:
                    print("  %s[STUCK install]%s required %s on %s -- gave up "
                          "after %ds\n%s\n    $ %s"
                          % (C_RED, C_RESET, pkg, tgt.label,
                             int(time.monotonic() - t0), tail, cmd))
                else:
                    print("  %s[FAIL install rc=%s]%s required %s\n%s\n    $ %s"
                          % (C_RED, rc, C_RESET, pkg, tail, cmd))
                failed_req.append(pkg)
            else:
                if any(s in low for s in unavail_sigs):
                    print("  %s[SKIP install]%s %s not available on this OS "
                          "version -> in-tree vendored copy used"
                          % (C_YELLOW, C_RESET, pkg))
                elif rc == 124:
                    print("  %s[STUCK install]%s optional %s on %s -- gave up "
                          "after %ds -> vendored copy used\n%s\n    $ %s"
                          % (C_YELLOW, C_RESET, pkg, tgt.label,
                             int(time.monotonic() - t0), tail, cmd))
                else:
                    print("  %s[WARN install rc=%s]%s optional %s failed -> "
                          "vendored copy used\n%s\n    $ %s"
                          % (C_YELLOW, rc, C_RESET, pkg, tail, cmd))
                skipped_opt.append(pkg)

    if ok_pkgs:
        print("  %s[OK install]%s %d package(s): %s"
              % (C_GREEN, C_RESET, len(ok_pkgs), " ".join(ok_pkgs)))
    if failed_req:
        return "fail"
    if skipped_opt:
        return "partial"
    return status


# ---------------------------------------------------------------------------
def collect_targets(args):
    targets = []
    seen = set()        # dedupe by ssh tuple; the local host is its own key

    def _add(label, ssh, lxc_node=None):
        key = ssh if ssh is not None else ("__local__",)
        if key in seen:
            return
        seen.add(key)
        targets.append(Target(label, ssh=ssh, lxc_node=lxc_node))

    # The local host is simultaneously the amd64 compile node and the
    # amd64 baseline execution node (benchmark/CLAUDE.md) -- it belongs to
    # every MODE, never a compile-only / exec-only box.
    if not args.node or "local" in args.node:
        _add("local", None)
    if args.local_only:
        return targets

    cfg = load_remote_nodes()
    want_compile = args.mode in ("all", "compile")
    want_exec = args.mode in ("all", "exec")

    def _skip(lbl):
        print("%s=== %s ===%s\n  %s[SKIP]%s enabled:false "
              "(pass --include-disabled to force)"
              % (C_CYAN, lbl, C_RESET, C_YELLOW, C_RESET))

    for node in cfg.get("nodes", []):
        # Compilation node == the nodes[].ssh box itself. Its `enabled`
        # default is true (remote_nodes.json _doc), unlike execution_nodes.
        if want_compile:
            lbl = node.get("label", "?")
            if not (args.node and lbl not in args.node):
                if not args.include_disabled and not node.get("enabled", True):
                    _skip(lbl)
                else:
                    ssh = node.get("ssh", {})
                    _add(lbl, (ssh.get("user"), ssh.get("host"),
                               ssh.get("port", 22)))
        if want_exec:
            for ex in node.get("execution_nodes", []):
                lbl = ex.get("label", "?")
                if args.node and lbl not in args.node:
                    continue
                if not args.include_disabled and not ex.get("enabled", False):
                    _skip(lbl)
                    continue
                # lxc_nfs node? Provision the GUEST, not the outer box
                # (the outer is e.g. an OpenWrt router with no pkg
                # manager we map). Retarget ssh to lxc_nfs.guest_ipv6
                # and stash the exec dict so provision() can drive
                # nfs_lxc_bring_up / nfs_lxc_teardown around the work.
                lxc = (ex.get("lxc_nfs") or {}) if isinstance(
                    ex.get("lxc_nfs"), dict) else {}
                if lxc.get("enabled") and lxc.get("guest_ipv6"):
                    _add(lbl, (ex.get("user"), lxc["guest_ipv6"],
                               ex.get("port", 22)),
                         lxc_node=ex)
                else:
                    _add(lbl, (ex.get("user"), ex.get("host"),
                               ex.get("port", 22)))
    return targets


def main():
    # Line-buffer stdout/stderr. Without this, `python3 setup.py | tee log`
    # block-buffers stdout (4 KiB) because the pipe is not a TTY, so the
    # serial fleet sweep (each remote apt-get takes seconds) shows ZERO
    # output for minutes and looks hung when it is merely slow. The
    # per-node "=== label ===" header is printed before each node's work,
    # so line buffering alone gives a live progress trail. (reconfigure is
    # Python 3.7+; the orchestrator host always has it -- guard anyway.)
    try:
        sys.stdout.reconfigure(line_buffering=True)
        sys.stderr.reconfigure(line_buffering=True)
    except (AttributeError, ValueError):
        pass
    ap = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", nargs="?", choices=["all", "exec", "compile"],
                    default="all",
                    help="all (default): local + compilation nodes + "
                         "execution_nodes; exec: local + execution_nodes "
                         "only; compile: local + compilation nodes only")
    ap.add_argument("--dry-run", action="store_true",
                    help="print the install commands, change nothing")
    ap.add_argument("--local-only", action="store_true",
                    help="provision only this host")
    ap.add_argument("--include-disabled", action="store_true",
                    help="also provision execution_nodes with enabled:false "
                         "(default: enabled nodes only)")
    ap.add_argument("--node", action="append", default=[],
                    help="restrict to this node label (repeatable; "
                         "'local' selects the host)")
    ap.add_argument("--conn-timeout", type=int, default=30,
                    help="seconds for probe/ssh round-trips (default 30)")
    ap.add_argument("--install-timeout", type=int, default=1800,
                    help="seconds for one package-install command "
                         "(default 1800; emerge from source is slow)")
    ap.add_argument("--deadline", type=int, default=3600,
                    help="hard wall-clock ceiling in seconds for the WHOLE "
                         "fleet sweep (default 3600 = 1h). Every remote "
                         "command is clamped to the budget still left; "
                         "nodes not reached before the deadline are marked "
                         "'deadline' and skipped. 0 disables the ceiling.")
    args = ap.parse_args()

    targets = collect_targets(args)
    if not targets:
        print("no targets matched", file=sys.stderr)
        return 2

    deadline = None
    if args.deadline and args.deadline > 0:
        deadline = time.monotonic() + args.deadline
        print("%s[deadline]%s whole-fleet ceiling = %ds; nodes not reached "
              "by then are skipped" % (C_CYAN, C_RESET, args.deadline))
    for t in targets:
        t.deadline = deadline

    summary = {}
    for t in targets:
        if deadline is not None and time.monotonic() >= deadline:
            print("%s=== %s ===%s" % (C_CYAN, t.label, C_RESET))
            print("  %s[SKIP]%s global deadline reached -- not provisioned"
                  % (C_YELLOW, C_RESET))
            summary[t.label] = "deadline"
            print()
            continue
        summary[t.label] = provision(t, args.dry_run,
                                     args.conn_timeout, args.install_timeout)
        print()

    print("%s===== summary =====%s" % (C_CYAN, C_RESET))
    rc = 0
    for lbl, st in summary.items():
        col = {"ok": C_GREEN, "skip": C_YELLOW, "unreachable": C_YELLOW,
               "partial": C_YELLOW, "deadline": C_YELLOW,
               "fail": C_RED}.get(st, C_RESET)
        print("  %s%-20s %s%s" % (col, lbl, st, C_RESET))
        if st == "fail":
            rc = 1
    return rc


if __name__ == "__main__":
    sys.exit(main())
