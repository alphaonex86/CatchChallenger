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
import shlex
import subprocess
import sys
import time

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

# Metadata refresh run ONCE per node before any install (None = the
# package manager refreshes implicitly / a sync would be too costly,
# e.g. `emerge --sync` rebuilds the whole portage tree).
REFRESH_CMD = {
    "apt":    "export DEBIAN_FRONTEND=noninteractive; apt-get update -qq",
    "dnf":    None,
    "zypper": "zypper --non-interactive refresh",
    "pacman": "pacman -Sy --noconfirm",
    "apk":    "apk update",
    "emerge": None,
    "xbps":   None,
}

# Install ONE package ({pkg}). Per-package, never batched: a single
# masked/unavailable tool (e.g. dev-debug/valgrind has no keyword on
# mips2r2) must NOT block the installable ones (time/perf/heaptrack).
INSTALL_ONE_CMD = {
    "apt":    "export DEBIAN_FRONTEND=noninteractive; "
              "apt-get install -y --no-install-recommends {pkg}",
    "dnf":    "dnf install -y {pkg}",
    "zypper": "zypper --non-interactive install -y {pkg}",
    "pacman": "pacman -S --noconfirm --needed {pkg}",
    "apk":    "apk add --no-cache {pkg}",
    "emerge": "emerge --noreplace --quiet-build=y {pkg}",
    "xbps":   "xbps-install -Sy {pkg}",
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

    def __init__(self, label, ssh=None, deadline=None):
        self.label = label
        self.ssh = ssh            # None => local; else (user, host, port)
        # Absolute time.monotonic() past which NO command may run. None =>
        # unbounded. Every run() clamps its per-command timeout to the
        # budget still left so one slow node (flaky apt mirror, emerge
        # from source) can never push the whole fleet sweep past the
        # global --deadline the operator set.
        self.deadline = deadline

    def run(self, cmd, timeout):
        """Run a shell command; return (rc, combined_output). The
        per-command timeout is clamped to whatever is left of the global
        deadline; once the deadline has passed the command is refused
        outright instead of started and timed out."""
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
            remote = (
                'if command -v setsid >/dev/null 2>&1; then '
                'O=$(mktemp 2>/dev/null || echo /tmp/.cc_setup.$$); '
                'setsid sh -c %s >"$O" 2>&1 & P=$!; '
                '( sleep %d; kill -TERM -$P 2>/dev/null; sleep 10; '
                'kill -KILL -$P 2>/dev/null ) & W=$!; '
                'wait $P 2>/dev/null; RC=$?; '
                'kill $W 2>/dev/null; wait $W 2>/dev/null; '
                'cat "$O" 2>/dev/null; rm -f "$O"; exit $RC; '
                'else sh -c %s 2>&1; fi'
            ) % (qcmd, int(timeout), qcmd)
            argv = ["ssh", "-o", "ConnectTimeout=3", "-o", "BatchMode=yes",
                    "-o", "StrictHostKeyChecking=accept-new",
                    "-o", "ServerAliveInterval=15", "-o", "ServerAliveCountMax=3",
                    "-p", str(p), "%s@%s" % (u, h), remote]
        try:
            # Host-side backstop = remote budget + 30s slack so the
            # remote watchdog (TERM then KILL-after-10s) is the primary
            # killer and we still collect its output; the host timeout
            # only trips if ssh itself wedged.
            host_to = timeout if self.ssh is None else timeout + 30
            cp = subprocess.run(argv, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=host_to)
            return cp.returncode, cp.stdout.decode("utf-8", "replace")
        except subprocess.TimeoutExpired:
            return 124, "[timeout after %ss]" % timeout
        except Exception as e:                               # pragma: no cover
            return 255, "[exec error: %s]" % e


def parse_os_release(text):
    osr = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, _, v = line.partition("=")
        osr[k.strip()] = v.strip().strip('"').strip("'")
    return osr


def have_tool_probe(tool):
    """Shell snippet that prints Y/N for one tool. /usr/bin/time is a
    file (the shell builtin `time` is NOT GNU time and has no -v)."""
    if tool.startswith("/"):
        return '[ -x %s ] && echo Y || echo N' % tool
    return 'command -v %s >/dev/null 2>&1 && echo Y || echo N' % tool


# ---------------------------------------------------------------------------
# Provision one target.
# ---------------------------------------------------------------------------
def provision(tgt, dry_run, conn_timeout, install_timeout):
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

    # Are we root? If not, prefix sudo (non-interactive).
    rc, who = tgt.run("id -u", conn_timeout)
    is_root = who.strip() == "0"
    sudo = "" if is_root else "sudo -n "

    print("  distro=%s  pkg-manager=%s  root=%s"
          % (distro, family, "yes" if is_root else "no(sudo)"))

    # Gentoo: many profiling tools (valgrind/perf/heaptrack) carry only a
    # ~ARCH (testing) keyword on the constrained arches (mips2r2, ...).
    # Grab the node's own portage ARCH so a missing-keyword failure can be
    # retried once with ACCEPT_KEYWORDS="~<arch>" -- a real install
    # attempt, scoped to the single package, not a global unmask.
    portage_arch = None
    if family == "emerge":
        rc, pa = tgt.run("portageq envvar ARCH 2>/dev/null", conn_timeout)
        portage_arch = pa.strip().split("\n")[-1].strip() or None

    # One round-trip: probe every tool we care about. CORE_TOOLS,
    # PROFILER_TOOLS and OPTIONAL_TOOLS are disjoint by construction.
    probe_tools = CORE_TOOLS + PROFILER_TOOLS + OPTIONAL_TOOLS
    probe = " ; ".join('printf "%%s " %s ; %s'
                        % (t, have_tool_probe(t)) for t in probe_tools)
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
    for t in PROFILER_TOOLS + OPTIONAL_TOOLS:
        if present.get(t, False):
            have.append(t)
        else:
            missing.append(t)
            pkg = pmap.get(TOOL_TO_PKG.get(t, ""), "")
            if pkg and pkg not in opt_missing_pkgs:
                opt_missing_pkgs.append(pkg)

    print("  present : %s" % (", ".join(sorted(have)) or "(none)"))
    print("  missing : %s" % (", ".join(sorted(missing)) or "(none -- all set)"))

    status = "ok"            # ok < partial < fail
    if not req_missing_pkgs and not opt_missing_pkgs:
        print("  %s[NOTHING TO DO]%s" % (C_GREEN, C_RESET))
        return status

    # Metadata refresh ONCE (apt-get update / pacman -Sy / ...).
    refresh = REFRESH_CMD.get(family)
    if refresh:
        cmd = sudo + refresh
        if dry_run:
            print("  %s[DRY-RUN refresh]%s %s" % (C_YELLOW, C_RESET, cmd))
        else:
            print("  %srefreshing package metadata%s" % (C_CYAN, C_RESET))
            rrc, rout = tgt.run(cmd, install_timeout)
            if rrc != 0:
                # Non-fatal: install-one may still work from a warm cache.
                print("  %s[WARN refresh rc=%s]%s %s"
                      % (C_YELLOW, rrc, C_RESET,
                         (rout.strip().splitlines() or [""])[-1][:160]))

    def _install_pkg(pkg, kind):
        nonlocal status
        cmd = sudo + INSTALL_ONE_CMD[family].format(pkg=pkg)
        if dry_run:
            print("  %s[DRY-RUN %s]%s %s" % (C_YELLOW, kind, C_RESET, cmd))
            return
        # Breadcrumb BEFORE the call: if the install wedges or the
        # timeout/deadline fires, the console still shows exactly what was
        # running on which node (the operator asked: a timeout must say
        # where it was stuck, not die mute).
        budget_left = ("" if tgt.deadline is None
                       else " (budget %ds left)"
                            % max(0, int(tgt.deadline - time.monotonic())))
        print("  %s-> installing %s on %s%s ...%s"
              % (C_CYAN, pkg, tgt.label, budget_left, C_RESET))
        t0 = time.monotonic()
        rc, out = tgt.run(cmd, install_timeout)
        # Gentoo "masked by: missing keyword" => the ebuild carries no
        # keyword for this arch *at all* (valgrind/perf/heaptrack on
        # mips2r2 were never keyworded upstream), so an inline
        # ACCEPT_KEYWORDS="~mips" can't lift it (no ~mips keyword to
        # accept) and would also be dropped by `sudo -n`'s env reset.
        # Lift it the canonical way: write the atom into
        # /etc/portage/package.accept_keywords/<slug> as root via a
        # file, then retry a plain emerge. Two bounded steps: first the
        # node's own ~ARCH (covers ebuilds that ARE ~arch-keyworded but
        # stable-masked), then "**" which accepts an entirely
        # un-keyworded ebuild -- the only way to even try compiling it
        # on an arch upstream never marked. The "zz-benchmark-setup-"
        # file persists on purpose: next run the atom is already
        # accepted, so emerge isn't masked again (idempotent).
        if family == "emerge" and portage_arch:
            akw = "/etc/portage/package.accept_keywords"
            slug = "zz-benchmark-setup-" + pkg.replace("/", "_")
            for kw in ("~%s" % portage_arch, "**"):
                if rc == 0 or "missing keyword" not in out.lower():
                    break
                entry = "%s %s" % (pkg, kw)
                # package.accept_keywords is a dir on modern profiles;
                # fall back to appending to the single-file form so an
                # operator's existing file is never clobbered.
                write = ('if [ -d %s ]; then printf "%%s\\n" "%s" > %s/%s; '
                         'else printf "%%s\\n" "%s" >> %s; fi'
                         % (akw, entry, akw, slug, entry, akw))
                print("  %sretry %s via %s/%s (%s)%s"
                      % (C_YELLOW, pkg, akw, slug, kw, C_RESET))
                wrc, wout = tgt.run("%ssh -c %s"
                                    % (sudo, shlex.quote(write)),
                                    conn_timeout)
                if wrc != 0:
                    print("  %s[WARN keyword-file rc=%s]%s %s"
                          % (C_YELLOW, wrc, C_RESET,
                             (wout.strip().splitlines() or [""])[-1][:160]))
                rc, out = tgt.run(cmd, install_timeout)
        if rc == 0:
            print("  %s[OK %s]%s %s" % (C_GREEN, kind, C_RESET, pkg))
        else:
            tail = "\n".join("    " + l
                             for l in out.strip().splitlines()[-6:])
            if rc == 124:
                # timeout / global-deadline: say WHERE it was stuck.
                print("  %s[STUCK %s]%s was installing %s on %s -- gave up "
                      "after %ds (per-cmd/deadline budget)\n%s"
                      % (C_RED, kind, C_RESET, pkg, tgt.label,
                         int(time.monotonic() - t0), tail))
            else:
                print("  %s[FAIL %s rc=%s]%s %s\n%s"
                      % (C_RED, kind, rc, C_RESET, pkg, tail))
            if kind == "required":
                status = "fail"
            elif status == "ok":
                status = "partial"

    # Per-package: one masked/unavailable tool never blocks the rest.
    for pkg in req_missing_pkgs:
        _install_pkg(pkg, "required")
    for pkg in opt_missing_pkgs:
        _install_pkg(pkg, "optional")
    return status


# ---------------------------------------------------------------------------
def collect_targets(args):
    targets = []
    seen = set()        # dedupe by ssh tuple; the local host is its own key

    def _add(label, ssh):
        key = ssh if ssh is not None else ("__local__",)
        if key in seen:
            return
        seen.add(key)
        targets.append(Target(label, ssh=ssh))

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
