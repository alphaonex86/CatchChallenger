#!/usr/bin/env python3
"""benchmark_t2100.py -- the Toshiba T2100 laptop as a comparison-only node.

The laptop is a real i486DX2-50 with 8 MB RAM running Linux 3.16 + a Buildroot
uClibc-ng userland, reached over a ~10 KB/s SLIP link. It has no compile node:
binaries are cross-built HERE with the Buildroot toolchain of the T2100
project, checked for non-486 opcodes, stripped, and pushed with rsync
(compressed, skipped when the content is unchanged). Like the ESP32 it is
injected programmatically (remote_nodes.json is operator-owned) and self-skips
when the toolchain or the laptop is absent.

Run directly to check the toolchain and the link:
    python3 benchmark_t2100.py
"""
import atexit
import os
import shlex
import shutil
import socket
import subprocess
import time

import benchmark_helpers as bh

T2100_BUILD   = os.environ.get("CC_T2100_BUILD", "/mnt/data/perso/retro/t2100-build")
T2100_PROJECT = os.environ.get("CC_T2100_PROJECT", "/mnt/data/perso/retro/toshiba T2100 260/IA")
T2100_PREFIX  = os.environ.get("CC_T2100_PREFIX", "/mnt/data/perso/progs/catchchallenger-T2100")
TOOLCHAIN_FILE = os.path.join(T2100_BUILD, "output", "host", "share", "buildroot", "toolchainfile.cmake")
TOOL_PREFIX    = os.path.join(T2100_BUILD, "output", "host", "bin", "i486-buildroot-linux-uclibc-")
CHECK_486      = os.path.join(T2100_PROJECT, "board", "t2100", "check-486.sh")

# uClibc-ng is built without SSP, so the common -fstack-protector-all is off.
# The Buildroot toolchain file puts -Os in CMAKE_<LANG>_FLAGS and leaves the
# Release flags without -O, so Release means -Os there: restore the fleet's -O3.
CMAKE_DEFS = {"CMAKE_BUILD_TYPE": "Release",
              "CMAKE_TOOLCHAIN_FILE": TOOLCHAIN_FILE,
              "CMAKE_C_FLAGS_RELEASE": "-O3 -DNDEBUG",
              "CMAKE_CXX_FLAGS_RELEASE": "-O3 -DNDEBUG",
              "CATCHCHALLENGER_NO_STACK_PROTECTOR": "ON"}
TARGET_FLAGS = ["-march=i486", "-DCATCHCHALLENGER_NO_STACK_PROTECTOR=ON"]
COMPILE_FLAGS = ["-O3"] + TARGET_FLAGS

T2100_NODE = {
    "label":    "toshiba-T2100",
    "arch":     "i486",
    "t2100":    True,     # marker: local cross build, comparison-only
    "ssh_host": os.environ.get("CC_T2100_HOST", "192.168.158.136"),
    "ssh_user": "root",
    "ssh_port": 22,
    "work_dir": "/root/cc-bench",   # on the disk: /tmp is a 1 MB tmpfs
}

# Every login costs ~9 s of dropbear key exchange on the 486 (far more while
# it loads a datapack), so all ssh/rsync calls share one master connection,
# a child of this process for its whole life (see _master_up).
CONTROL_PATH = os.path.join(T2100_PREFIX, "ssh-control")
SSH_OPTS = ["-o", "ConnectTimeout=60", "-o", "BatchMode=yes",
            "-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=/dev/null",
            "-o", "LogLevel=ERROR", "-o", f"ControlPath={CONTROL_PATH}"]
_master = None


def exec_node():
    """The node in the shape benchmark_remote's run helpers expect."""
    return {"label": T2100_NODE["label"], "user": T2100_NODE["ssh_user"],
            "host": T2100_NODE["ssh_host"], "port": T2100_NODE["ssh_port"],
            "work_dir": T2100_NODE["work_dir"], "t2100": True}


def _target():
    return ["-p", str(T2100_NODE["ssh_port"]), f"{T2100_NODE['ssh_user']}@{T2100_NODE['ssh_host']}"]


def _master_up():
    """Start the shared master connection if it is not running. Without it
    every call still works, only slower."""
    global _master
    if _master is not None and _master.poll() is None:
        return
    os.makedirs(T2100_PREFIX, exist_ok=True)
    if os.path.exists(CONTROL_PATH):  # left by a killed run
        os.unlink(CONTROL_PATH)
    _master = subprocess.Popen(["ssh", *SSH_OPTS, "-M", "-N", *_target()],
                               stdin=subprocess.DEVNULL)
    atexit.register(_master.terminate)
    deadline = time.monotonic() + 90
    while not os.path.exists(CONTROL_PATH) and _master.poll() is None and time.monotonic() < deadline:
        time.sleep(0.5)
    if not os.path.exists(CONTROL_PATH):
        print("[t2100] no shared ssh connection: every call logs in on its own")


def ssh(cmd, timeout=60):
    """(rc, stdout, stderr) of `cmd` run on the laptop."""
    _master_up()
    argv = ["ssh", *SSH_OPTS, *_target(), cmd]
    try:
        rc, sout, serr, _ = bh.run_capture(argv, timeout=timeout)
    except subprocess.TimeoutExpired:
        return -1, "", f"ssh timed out after {timeout} s"
    return rc, sout, serr


def runner(cmd, timeout=10):
    """history_recorder runner contract: (rc, stdout); list or string cmd."""
    if isinstance(cmd, list):
        cmd = " ".join(shlex.quote(a) for a in cmd)
    rc, sout, _ = ssh(cmd, timeout=max(timeout, 60))
    return rc, sout


def toolchain_status():
    """(ok, detail): ok when the Buildroot toolchain and the 486 checker exist."""
    for path in (TOOLCHAIN_FILE, TOOL_PREFIX + "strip", CHECK_486):
        if not os.path.exists(path):
            return False, f"missing {path} (Buildroot output of the T2100 project)"
    return True, "toolchain present"


def measure_microbench():
    """Cross-build benchmark/microbench, run it on the laptop and return the
    parsed result ({} on failure, with the cause printed). Cached 24 h by
    benchmark_microbench.cached()."""
    import benchmark_microbench as mb
    bin_path, err = cross_build(mb.SRC_SUBDIR, "build-microbench", mb.BIN_NAME)
    if bin_path is None:
        print(f"[t2100] microbench build failed: {err}")
        return {}
    ok, detail = push([bin_path])
    if not ok:
        print(f"[t2100] microbench push failed: {detail}")
        return {}
    rc, sout, serr = ssh(f"cd {T2100_NODE['work_dir']} && {mb.GATED_RUN}", timeout=300)
    if rc != 0:
        print(f"[t2100] microbench run failed (rc {rc}): {serr.strip()[-300:]}")
        return {}
    return mb.parse_gated(T2100_NODE["label"], sout)


def platform_fields():
    """Build-side fields collect() cannot read on the laptop (it has no
    compiler and no ldd): the cross compiler and libc versions."""
    fields = {"net_link": "slip", "net_link_detail": "SLIP over RS232 115200 baud"}
    rc, sout, _, _ = bh.run_capture([TOOL_PREFIX + "gcc", "-dumpversion"], timeout=30)
    if rc == 0:
        fields["compiler"] = f"i486-buildroot-linux-uclibc-gcc {sout.strip()}"
    version = {}
    try:
        with open(os.path.join(T2100_BUILD, "output", "staging", "usr", "include", "bits", "uClibc_config.h")) as f:
            for line in f:
                parts = line.split()
                if len(parts) == 3 and parts[0] == "#define" and parts[1].startswith("__UCLIBC_") \
                        and parts[1].endswith("__") and parts[2].isdigit():
                    version[parts[1]] = parts[2]
    except OSError:
        pass
    if all(k in version for k in ("__UCLIBC_MAJOR__", "__UCLIBC_MINOR__", "__UCLIBC_SUBLEVEL__")):
        fields["libc"] = "uClibc-ng " + ".".join(version[k] for k in
                                                ("__UCLIBC_MAJOR__", "__UCLIBC_MINOR__", "__UCLIBC_SUBLEVEL__"))
    return fields


def cross_build(src_subdir, build_name, bin_name, cmake_defs=None, timeout=1800):
    """Configure + build `src_subdir` (repo-relative) for the T2100, then strip
    the binary into <build>/t2100/<bin_name> and reject it if it holds code a
    486 cannot run. Returns (path, None) or (None, error)."""
    ok, detail = toolchain_status()
    if not ok:
        return None, detail
    build_dir = os.path.join(T2100_PREFIX, build_name)
    defs = dict(CMAKE_DEFS)
    defs.update(cmake_defs or {})
    cfg = ["cmake", "-S", os.path.join(bh.REPO_ROOT, src_subdir), "-B", build_dir]
    if shutil.which("ninja"):
        cfg += ["-G", "Ninja"]
    cfg += [f"-D{k}={v}" for k, v in defs.items()]
    bh.drop_stale_cmake_cache(build_dir, os.path.join(bh.REPO_ROOT, src_subdir))
    for step in (cfg, ["cmake", "--build", build_dir, "-j", str(os.cpu_count() or 1)]):
        rc, sout, serr, _ = bh.run_capture(step, timeout=timeout)
        if rc != 0:
            return None, f"{' '.join(step[:2])} failed:\n{(sout + serr)[-3000:]}"
    out_dir = os.path.join(build_dir, "t2100")
    os.makedirs(out_dir, exist_ok=True)
    stripped = os.path.join(out_dir, bin_name)
    rc, sout, serr, _ = bh.run_capture([TOOL_PREFIX + "strip", "-o", stripped,
                                        os.path.join(build_dir, bin_name)], timeout=120)
    if rc != 0:
        return None, f"strip failed: {serr.strip()}"
    rc, sout, serr, _ = bh.run_capture(["sh", CHECK_486, TOOL_PREFIX + "objdump", out_dir],
                                       timeout=600)
    if rc != 0:
        return None, f"non-486 code in the build:\n{serr.strip()}"
    return stripped, None


def push(local_paths, timeout=1800):
    """rsync files or dirs into the laptop's work_dir. -c skips unchanged
    content (a rebuild re-strips the binary, so mtimes always change); -z
    matters at ~10 KB/s. Returns (ok, detail)."""
    import benchmark_remote as br
    br.sync_exec_node_time(exec_node())  # no valid CMOS clock: it boots in 1970
    rc, _, serr = ssh(f"mkdir -p {T2100_NODE['work_dir']}")
    if rc != 0:
        return False, f"laptop unreachable: {serr.strip()}"
    argv = ["rsync", "-rtcz", "--chmod=Du+rwx,Fu+rwx",
            "-e", "ssh " + " ".join(SSH_OPTS),
            *local_paths, f"{T2100_NODE['ssh_user']}@{T2100_NODE['ssh_host']}:{T2100_NODE['work_dir']}/"]
    rc, sout, serr, wall = bh.run_capture(argv, timeout=timeout)
    if rc != 0:
        return False, f"rsync failed (rc {rc}): {serr.strip()[-500:]}"
    return True, f"pushed in {wall:.0f} s"


def push_datapack(local_datapack, maincode, timeout=1800):
    """Mirror the server-side datapack (no media, skins kept, only the
    `maincode` map set: the others are 80 % of it) into work_dir/datapack.
    Returns (ok, detail)."""
    import benchmark_remote as br
    _master_up()
    only_maincode = [f"--include=/map/main/{maincode}/", "--exclude=/map/main/*"]
    argv = ["rsync", "-rtcz", "--delete", *only_maincode,
            *br.server_datapack_excludes(keep_skins=True),
            "-e", "ssh " + " ".join(SSH_OPTS),
            local_datapack.rstrip("/") + "/",
            f"{T2100_NODE['ssh_user']}@{T2100_NODE['ssh_host']}:{T2100_NODE['work_dir']}/datapack/"]
    rc, sout, serr, wall = bh.run_capture(argv, timeout=timeout)
    if rc != 0:
        return False, f"datapack rsync failed (rc {rc}): {serr.strip()[-500:]}"
    return True, f"datapack pushed in {wall:.0f} s"


def start_server(bin_name):
    """Start `bin_name` detached in work_dir (log: server.log) with ONE ssh
    login: each login costs ~9 s of dropbear key exchange on the 486, so the
    fleet's start_server_popen polling would pile up logins. Returns
    (pid, None) or (None, error). A leftover instance would hold the port."""
    rc, sout, serr = ssh(f"killall {bin_name} 2>/dev/null && while pidof {bin_name} >/dev/null; do sleep 1; done; "
                         f"cd {T2100_NODE['work_dir']} && rm -f server.log && "
                         f"{{ ./{bin_name} </dev/null >server.log 2>&1 & echo $!; }}")
    pid = sout.strip()
    if rc != 0 or not pid.isdigit():
        return None, f"rc {rc}: {(sout + serr).strip()[-300:]}"
    return pid, None


def wait_bound(port, timeout):
    """True once the laptop's server accepts on `port`. Polled from here: an
    ssh login while the 486 loads the datapack takes over 30 s."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((T2100_NODE["ssh_host"], port), timeout=3):
                return True
        except OSError:
            time.sleep(2)
    return False


def stop_server(pid):
    """Stop the server and wait for it to exit: it writes its BENCH dump on
    the way out."""
    rc, _, serr = ssh(f"kill {pid} && while kill -0 {pid} 2>/dev/null; do sleep 1; done", timeout=120)
    if rc != 0:
        print(f"[t2100] stopping server {pid} failed: {serr.strip()}")


if __name__ == "__main__":
    ok, detail = toolchain_status()
    print(f"toolchain: {detail}")
    rc, sout, serr = ssh("uname -a; free")
    print(sout if rc == 0 else f"laptop unreachable: {serr.strip()}")
    raise SystemExit(0 if ok and rc == 0 else 1)
