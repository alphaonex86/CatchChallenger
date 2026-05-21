#!/usr/bin/env python3
"""halt.py -- stop everything the benchmark harness started on the REMOTE
execution nodes.

NEVER touches the local host: this box is the operator's workstation /
orchestrator, killing its processes could take down the very run that
launched the halt (or unrelated local work). Only remote execution_nodes
are halted.

A no-arg run also SKIPS virtual / container nodes (real_hardware=false:
the mips-lxc / x86-lxc / amd64-lxc containers hosted on THIS box) -- they
share the local host, so halting them is as intrusive as halting local.
An explicitly named node is still halted (operator override).

For every remote execution_node in remote_nodes.json it:
  * kills any leftover benchmark / server / bot processes
    (benchmark_min_network, catchchallenger-server-cli, bot-actions);
  * for an NFS-LXC node (lxc_nfs.enabled), tears the container down
    (lxc-stop -k + unmount the NFS rootfs and its /proc /sys /dev binds)
    via benchmark_remote.nfs_lxc_teardown -- which also kills every
    process inside the guest.

Usage:
    python3 halt.py                 # halt every remote exec node
    python3 halt.py rtl9607c        # halt only the named exec node(s)

Best-effort + idempotent: an unreachable or already-clean node is
reported and skipped, never fatal. flock-held locks (singleton / network)
release on their owning process's death -- halt does not touch them.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "test"))

import benchmark_helpers as bh
import benchmark_remote as br

# Process names every benchmark may leave running on an exec node / locally.
BENCH_PROCS = [
    "benchmark_min_network",
    "catchchallenger-server-cli",
    "bot-actions",
]


def _kill_script():
    """Portable best-effort kill (TERM then KILL) of the bench process
    names via ps|grep|kill. Never fails the shell.

    Self-safe: the script's own shell cmdline contains the process names
    (in NAMES=...), so a naive `pkill -f`/`ps|grep` would match and kill
    ITSELF. We exclude $$ (this shell) and $PPID, skip the grep itself,
    and require a numeric pid (drops the `PID` header line). No `pkill -f`
    for the same reason -- it would match this very command."""
    names = " ".join(BENCH_PROCS)
    return (
        f'NAMES="{names}"; SELF=$$; PARENT=$PPID; '
        'list_pids() { ps -e -o pid,args 2>/dev/null || '
        '              ps -w 2>/dev/null || ps 2>/dev/null; }; '
        'kill_pass() { '
        '  sig="$1"; '
        '  for n in $NAMES; do '
        '    for p in $(list_pids | grep "$n" | grep -v grep | '
        '               awk \'{print $1}\'); do '
        '      case "$p" in (*[!0-9]*) continue;; esac; '
        '      [ "$p" = "$SELF" ] && continue; '
        '      [ "$p" = "$PARENT" ] && continue; '
        '      kill "-$sig" "$p" 2>/dev/null || true; '
        '    done; '
        '  done; '
        '}; '
        'kill_pass TERM; sleep 1; kill_pass KILL; echo HALT_OK'
    )


def _exec_node_dict(ex):
    """Shape an execution_nodes[] entry into the dict the helpers expect."""
    return {
        "label":    ex.get("label"),
        "user":     ex.get("user"),
        "host":     ex.get("host"),
        "port":     ex.get("port", 22),
        "work_dir": ex.get("work_dir"),
        "lxc_nfs":  ex.get("lxc_nfs"),
    }


def _halt_exec(node):
    label = node["label"]
    cfg = node.get("lxc_nfs")
    is_lxc = isinstance(cfg, dict) and bool(cfg.get("enabled", False))
    if is_lxc:
        # Two-step, GUEST then HOST:
        #   1. ssh into the container (guest_ipv6/_ipv4) and kill the bench
        #      processes -- a graceful in-guest stop before the box is
        #      yanked out from under them.
        #   2. teardown on the device pre-chroot SSH: lxc-stop -k (force-
        #      stops whatever's left) + unmount the NFS rootfs + binds.
        guest = br._runtime_exec_node(node)   # host -> guest addr, lxc_nfs cleared
        if guest is not node and guest.get("host"):
            gu, gh, gp = guest["user"], guest["host"], guest.get("port", 22)
            print(f"{bh.C_CYAN}[halt]{bh.C_RESET} {label}: halting guest "
                  f"{gu}@{gh} (kill bench processes)")
            rc, out, err = br.ssh_run(gu, gh, gp, _kill_script(), timeout=40)
            if rc == 0 and "HALT_OK" in (out or ""):
                print(f"{bh.C_GREEN}[halt]{bh.C_RESET} {label}: guest "
                      f"processes killed")
            else:
                tail = (err or "").strip().splitlines()
                why = tail[-1] if tail else f"rc={rc}"
                print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} {label}: guest "
                      f"unreachable / already down ({why})", file=sys.stderr)
        print(f"{bh.C_CYAN}[halt]{bh.C_RESET} {label}: host teardown "
              f"(lxc-stop + umount)")
        try:
            br.nfs_lxc_teardown(node, verbose=True)
            print(f"{bh.C_GREEN}[halt]{bh.C_RESET} {label}: torn down")
        except Exception as e:
            print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} {label}: teardown "
                  f"error: {e}", file=sys.stderr)
        return
    # Ordinary exec node: ssh in and kill the bench processes.
    u, h, p = node.get("user"), node.get("host"), node.get("port", 22)
    if not u or not h:
        print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} {label}: no ssh user/host, "
              f"skipping", file=sys.stderr)
        return
    print(f"{bh.C_CYAN}[halt]{bh.C_RESET} {label}: killing bench processes "
          f"on {u}@{h}")
    rc, out, err = br.ssh_run(u, h, p, _kill_script(), timeout=40)
    if rc == 0 and "HALT_OK" in (out or ""):
        print(f"{bh.C_GREEN}[halt]{bh.C_RESET} {label}: done")
    else:
        # classify_ssh_error already fired a banner on auth failure
        tail = (err or "").strip().splitlines()
        why = tail[-1] if tail else f"rc={rc}"
        print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} {label}: unreachable / "
              f"not killed ({why})", file=sys.stderr)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    wanted = set(argv)   # empty => all REMOTE exec nodes

    # The local host is NEVER halted (operator workstation / orchestrator).
    # Refuse it explicitly so `halt.py local` doesn't silently no-op-look
    # like it did something.
    if "local" in wanted:
        print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} refusing to halt 'local' -- "
              f"halt.py only stops REMOTE exec nodes", file=sys.stderr)
        wanted.discard("local")
        if not wanted:
            return 0

    cfg = bh.load_remote_nodes()
    seen = []
    skipped_virtual = []
    for node in cfg.get("nodes", []):
        for ex in node.get("execution_nodes", []):
            label = ex.get("label")
            if wanted and label not in wanted:
                continue
            # Skip virtual / container nodes (real_hardware == false: the
            # mips-lxc / x86-lxc / amd64-lxc containers hosted on THIS box).
            # They share the local host -- halting them is as intrusive as
            # halting local, and the user excludes them. An explicitly named
            # node is still halted (operator override). real_hardware
            # defaults to True so an unknown node is never silently skipped.
            if not wanted and not ex.get("real_hardware", True):
                skipped_virtual.append(label)
                print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} {label}: skipped "
                      f"(virtual / container, real_hardware=false)")
                continue
            seen.append(label)
            _halt_exec(_exec_node_dict(ex))

    if wanted:
        missing = wanted - set(seen)
        if missing:
            print(f"{bh.C_YELLOW}[halt]{bh.C_RESET} no such exec node(s): "
                  f"{', '.join(sorted(missing))}", file=sys.stderr)
    extra = (f"; {len(skipped_virtual)} virtual skipped"
             if skipped_virtual else "")
    print(f"{bh.C_CYAN}[halt]{bh.C_RESET} fleet halt complete "
          f"({len(seen)} remote exec node(s) processed; local untouched"
          f"{extra})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
