#!/usr/bin/env python3
"""
testingcompilationESP32.py — CatchChallenger ESP32 (ESP-IDF) datapack-cpp port.

The ESP32 has very little RAM (~520 KB) and no general filesystem, so the
all-in-one game server is built to run with NOTHING on disk: the datapack AND
the server settings are compiled into the executable as human-readable C++ const
data that lives in flash (.rodata / XIP) and is read in place — no filesystem,
no byte blob, no deserialization at startup. Account/character state uses the
in-memory backend (CATCHCHALLENGER_DB_INTERNAL_VARS). See test/ESP32.md.

Two host-verifiable stages + two ESP-IDF stages (self-skip when the toolchain or
emulator is absent, like testingcompilation{mac,android}.py):

  stage1  build server-cli with CATCHCHALLENGER_DATAPACK_CPP_EMIT, run `--save`
          on the staged datapack (maincode "test"); the server re-emits the
          whole cache as datapack_cpp.{cpp,hpp} into
          /mnt/data/perso/tmpfs/datapack-cpp/ (human-readable typed const arrays).

  stage2  build server-cli with CATCHCHALLENGER_NOXML + CATCHCHALLENGER_DATAPACK_CPP
          + CATCHCHALLENGER_DB_INTERNAL_VARS + CATCHCHALLENGER_SELECT, compiling
          that generated .cpp in. Run it in a directory holding ONLY the binary
          (no datapack/, no server-properties.xml, no database/) and assert it
          loads the datapack + settings from flash, binds, and starts the LAN
          announce — proving the no-filesystem path. Then (best-effort) point the
          native client at it.

  esp-idf (self-skip)  idf.py build the server/cli/esp32 component project for the
          xtensa-esp32 target, passing the same generated datapack-cpp dir.

  qemu    (self-skip)  boot the firmware under qemu-system-xtensa (Espressif fork)
          headless with a host TCP port-forward, wait for 'correctly bind:', then
          connect qtcpu800x600 --host 127.0.0.1 --port <p> --autologin. Mirrors
          the operator's real ESP32 hardware.

  --realhardware (opt-in)  flash the connected board (idf.py flash), read its
          serial for the DHCP IP + 'correctly bind:', then connect qtcpu800x600
          --host <board-ip> --port <p> --autologin. --serial <dev> pins the port;
          without it the ESP32's USB serial is AUTODETECTED (ranks /dev/ttyUSB*
          + /dev/ttyACM* by ESP32 USB-serial vendor id, confirmed with esptool
          chip_id). Self-skips when the flag is absent or no board is present.

stage1+stage2 are pure host builds (no ESP-IDF needed) and are the part that
actually exercises the new datapack-cpp + settings-in-flash + LAN-announce code.
The connect phases assert the client reaches protocol-good/login; the fileless
server serves no datapack files so a fresh client does not reach the map.
"""

import sys
sys.dont_write_bytecode = True

import os, signal, subprocess, json, time, shutil, multiprocessing, glob
import build_paths
import diagnostic
import wall_cap
wall_cap.arm()
import cleanup_helpers
from cmd_helpers import clamp_local
import datapack_stage

build_paths.ensure_root()

# ── config ────────────────────────────────────────────────────────────────────
_CONFIG_PATH = os.path.join(os.path.expanduser("~"),
                            ".config", "catchchallenger-testing", "config.json")
try:
    with open(_CONFIG_PATH, "r") as _f:
        _config = json.load(_f)
except (OSError, ValueError):
    _config = {}

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NPROC = str(multiprocessing.cpu_count())

# ESP-IDF + qemu-xtensa prefix (operator-built, kept out of the repo like MXE /
# the MS-DOS prefix). esp-idf/ under it; export.sh sets IDF_PATH + the toolchain.
ESP32_PREFIX = os.environ.get("CC_ESP32_PREFIX",
                              "/mnt/data/perso/progs/catchchallenger-ESP32")
IDF_PATH     = os.environ.get("IDF_PATH", os.path.join(ESP32_PREFIX, "esp-idf"))
IDF_PY       = os.path.join(IDF_PATH, "tools", "idf.py")
# Espressif's qemu fork (qemu-system-xtensa with the esp32 machine).
QEMU_XTENSA  = shutil.which("qemu-system-xtensa")

SERVER_SRC   = os.path.join(ROOT, "server", "cli")
ESP_PROJECT  = os.path.join(SERVER_SRC, "esp32")

# Where stage1 writes the generated human-readable datapack-cpp (the path the
# user pinned). stage2 + the ESP-IDF build both compile it in.
DATAPACK_CPP_DIR = "/mnt/data/perso/tmpfs/datapack-cpp"

STAGE1_BUILD = build_paths.build_path("server/cli/build/testing-esp32-stage1")
STAGE2_BUILD = build_paths.build_path("server/cli/build/testing-esp32-stage2")
cleanup_helpers.register_build_dir(STAGE1_BUILD)
cleanup_helpers.register_build_dir(STAGE2_BUILD)
STAGE1_EXE   = os.path.join(STAGE1_BUILD, "catchchallenger-server-cli")
STAGE2_EXE   = os.path.join(STAGE2_BUILD, "catchchallenger-server-cli")

# Native qtcpu800x600 client for the connect phase (reused from testingclient if
# already built; we do NOT build Qt here — this is a server compile test).
CLIENT_CPU_BIN = build_paths.build_path(
    "client/qtcpu800x600/build/testing-cpu/catchchallenger")

DATAPACKS = _config["paths"]["datapacks"] if "paths" in _config else []

# server-properties.xml the stage1 --save bakes into the flash cache: max 10
# clients, the min-CPU visibility algorithm, the "test" maincode, and a LAN
# broadcast name (so the announce timer turns on — empty name = disabled).
SERVER_PORT  = 42498
BROADCAST_NAME = "CatchChallenger ESP32"
MAIN_DATAPACK_CODE = "test"

NICE_PREFIX_COMPILE = ["nice", "-n", "19", "ionice", "-c", "3"]

COMPILE_TIMEOUT  = 1800
SAVE_TIMEOUT     = 300
BIND_TIMEOUT     = 60
QEMU_BIND_TIMEOUT= 240   # qemu-system-xtensa boot + wifi + bind (TCG is slow)
# Connect: a fileless server can't serve the datapack, so a fresh client that
# lacks the matching datapack connects + logs in but stalls before the map. The
# client never exits on its own then, so the connect waits out this cap, then we
# grep its log for the connected/protocol-good marker (CLIENT_OK_MARKERS).
CLIENT_TIMEOUT   = 45
IDF_TIMEOUT      = 2400   # esp-idf full build is slow (xtensa gcc, big component)

# --realhardware: flash the connected board (idf.py flash) then connect a client
# to its DHCP address. Operator-only (needs a board on the serial port); absent
# or unset => self-skip. Optional --serial <dev> overrides the port.
def _argval(flag, default):
    if flag in sys.argv:
        i = sys.argv.index(flag)
        if i + 1 < len(sys.argv):
            return sys.argv[i + 1]
    return default
REALHARDWARE = "--realhardware" in sys.argv
# --serial <dev> pins the port; without it we autodetect which USB serial is the
# ESP32 (see _detect_esp32_serial). None => autodetect.
SERIAL_PORT  = _argval("--serial", None)

# USB vendor IDs of the serial bridges found on ESP32 boards (and the ESP32-S2/
# S3/C3 native USB-CDC). Used to rank candidate ttys before the esptool probe.
_ESP32_USB_VIDS = {
    "10c4",   # Silicon Labs CP210x (CP2102/CP2104) — most common on ESP32 devkits
    "1a86",   # WCH CH340/CH341/CH9102 — common on cheap boards
    "0403",   # FTDI FT231X etc.
    "303a",   # Espressif (native USB-CDC on ESP32-S2/S3/C3/C6)
}

IDF_BUILD = build_paths.build_path("server/cli/build/testing-esp32-idf")
IDF_BIN   = os.path.join(IDF_BUILD, "catchchallenger-server-esp32.bin")

# Client-connect success markers, in decreasing strength. The no-FS server
# serves no datapack files, so a fresh client connects + protocol-good + logs in
# but never reaches the map; protocol_is_good is always reached on a good connect.
CLIENT_OK_MARKERS = (
    ("MapVisualiserPlayer::mapDisplayedSlot()", "reached the map"),
    ("new token to go on game server:",         "connected + logged in (no map: fileless server serves no datapack files)"),
    ("BaseWindow::protocol_is_good()",          "connected, protocol handshake good (no datapack files served)"),
)

# ── colours / logging (same shape as the sibling scripts) ─────────────────────
C_GREEN = "\033[92m"; C_RED = "\033[91m"; C_CYAN = "\033[96m"; C_RESET = "\033[0m"
results = []
_last_log_time = [time.monotonic()]
total_expected = [0]
SCRIPT_NAME = os.path.basename(__file__)
from test_config import FAILED_JSON
import failed_cases as _fc
import phase_timer


def load_failed_cases():
    return _fc.load_names(SCRIPT_NAME)


def should_run(test_name, failed_cases):
    return failed_cases is None or test_name in failed_cases


def save_failed_cases():
    failed = []
    for name, ok, detail, _elapsed in results:
        if not ok:
            d = _fc.make_detail(detail)
            d.update(_fc.pop_extras(name))
            failed.append((name, d))
    _fc.save(SCRIPT_NAME, failed)


def log_info(msg):
    print(f"{phase_timer.t()} {C_CYAN}[INFO]{C_RESET} {msg}")


def log_pass(name, detail=""):
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, True, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_GREEN}[PASS]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("pass", name, ok=True, dt=elapsed, detail=detail)


def log_fail(name, detail=""):
    now = time.monotonic(); elapsed = now - _last_log_time[0]; _last_log_time[0] = now
    results.append((name, False, detail, elapsed))
    if len(results) > total_expected[0]:
        total_expected[0] = len(results)
    print(f"{phase_timer.t()} {C_RED}[FAIL]{C_RESET} {len(results)}/{total_expected[0]} {name}  {detail}  ({elapsed:.1f}s)")
    phase_timer.record_event("fail", name, ok=False, dt=elapsed, detail=detail)
    li = 0
    _ctx = diagnostic.last_cmd_lines()
    while li < len(_ctx):
        print(_ctx[li]); li += 1


def run_cmd(args, cwd, timeout=COMPILE_TIMEOUT, env=None):
    timeout = clamp_local(timeout)
    diagnostic.record_cmd(args, cwd)
    try:
        p = subprocess.run(args, cwd=cwd, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           env=env or os.environ)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"TIMEOUT after {timeout}s"


def datapack_available():
    return bool(DATAPACKS) and os.path.isdir(DATAPACKS[0])


def stage_server_datapack():
    """Symlink target for the server's datapack/ (headless, media-stripped) —
    the staged copy, never the read-only source."""
    return datapack_stage.staged_local(DATAPACKS[0], server=True)


def stage_client_datapack():
    """Full staged datapack (with images) for the rendering client."""
    return datapack_stage.staged_local(DATAPACKS[0], server=False)


def write_server_properties(path):
    """Partial server-properties.xml. NormalServerGlobal::checkSettingsFile()
    fills every key we omit with sane defaults; we only pin the few the ESP32
    profile needs (10 clients, min-CPU visibility, the "test" maincode and the
    LAN broadcast name). --save bakes the whole result into the flash cache."""
    with open(path, "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{SERVER_PORT}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                f'    <max-players value="10"/>\n'
                f'    <broadcastName value="{BROADCAST_NAME}"/>\n'
                '    <content>\n'
                f'        <mainDatapackCode value="{MAIN_DATAPACK_CODE}"/>\n'
                '    </content>\n'
                '    <mapVisibility>\n'
                '        <minimize value="cpu"/>\n'
                '    </mapVisibility>\n'
                '</configuration>\n')


# ── stage1: build EMIT server + --save -> datapack_cpp.{cpp,hpp} ──────────────
def build_stage1_and_emit():
    name = "stage1 datapack-cpp emit (server-cli --save -> datapack_cpp.cpp)"
    if not datapack_available():
        log_fail(name, "datapack source missing (config paths.datapacks)")
        return False
    if os.path.isdir(STAGE1_BUILD):
        shutil.rmtree(STAGE1_BUILD, ignore_errors=True)
    os.makedirs(STAGE1_BUILD, exist_ok=True)
    log_info("stage1 cmake configure (EMIT + DB_INTERNAL_VARS + SELECT)")
    cfg = NICE_PREFIX_COMPILE + [
        "cmake", "-S", SERVER_SRC, "-B", STAGE1_BUILD,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCATCHCHALLENGER_SELECT=ON",
        "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
        "-DCATCHCHALLENGER_DATAPACK_CPP_EMIT=ON",
        "-DCATCHCHALLENGER_HARDENED=ON",
    ]
    rc, out = run_cmd(cfg, STAGE1_BUILD)
    if rc != 0:
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"cmake configure failed (rc={rc})"); print(out[-3000:]); return False
    log_info(f"stage1 cmake --build -j{NPROC}")
    rc, out = run_cmd(NICE_PREFIX_COMPILE + ["cmake", "--build", STAGE1_BUILD, "-j", NPROC],
                      STAGE1_BUILD)
    if rc != 0 and shutil.which("ccache"):
        log_info("stage1 build failed; flushing ccache + retry once")
        subprocess.run(["ccache", "-C"], capture_output=True, text=True, timeout=120)
        rc, out = run_cmd(NICE_PREFIX_COMPILE + ["cmake", "--build", STAGE1_BUILD, "-j", NPROC],
                          STAGE1_BUILD)
    if rc != 0 or not os.path.isfile(STAGE1_EXE):
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"stage1 build failed (rc={rc})"); print(out[-3000:]); return False

    # Run --save in a scratch dir: binary + staged datapack symlink + the
    # partial server-properties.xml. The server emits datapack_cpp.{cpp,hpp}
    # next to the binary; we move them to DATAPACK_CPP_DIR.
    work = os.path.join(STAGE1_BUILD, "save-run")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    shutil.copy2(STAGE1_EXE, os.path.join(work, "catchchallenger-server-cli"))
    os.symlink(stage_server_datapack(), os.path.join(work, "datapack"))
    write_server_properties(os.path.join(work, "server-properties.xml"))
    log_info("stage1 running --save (parse datapack, emit human-readable C++)")
    rc, out = run_cmd([os.path.join(work, "catchchallenger-server-cli"), "--save"],
                      work, timeout=SAVE_TIMEOUT)
    cpp = os.path.join(work, "datapack_cpp.cpp")
    hpp = os.path.join(work, "datapack_cpp.hpp")
    if not (os.path.isfile(cpp) and os.path.isfile(hpp)):
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"--save did not emit datapack_cpp.{{cpp,hpp}} (rc={rc})")
        print(out[-3000:]); return False
    os.makedirs(DATAPACK_CPP_DIR, exist_ok=True)
    shutil.copy2(cpp, os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.cpp"))
    shutil.copy2(hpp, os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.hpp"))
    size_kib = os.path.getsize(cpp) // 1024
    log_pass(name, f"-> {DATAPACK_CPP_DIR}/datapack_cpp.cpp ({size_kib} KiB human-readable)")
    return True


# ── stage2: build NOXML+DATAPACK_CPP server, run no-FS, connect client ────────
def build_stage2():
    name = "stage2 build no-FS server (NOXML + DATAPACK_CPP + DB_INTERNAL_VARS)"
    if not os.path.isfile(os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.cpp")):
        log_fail(name, "stage1 output missing — run stage1 first"); return None
    if os.path.isdir(STAGE2_BUILD):
        shutil.rmtree(STAGE2_BUILD, ignore_errors=True)
    os.makedirs(STAGE2_BUILD, exist_ok=True)
    log_info("stage2 cmake configure (NOXML + DATAPACK_CPP, no filesystem)")
    cfg = NICE_PREFIX_COMPILE + [
        "cmake", "-S", SERVER_SRC, "-B", STAGE2_BUILD,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCATCHCHALLENGER_SELECT=ON",
        "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
        "-DCATCHCHALLENGER_NOXML=ON",
        "-DCATCHCHALLENGER_DATAPACK_CPP=ON",
        "-DCATCHCHALLENGER_DATAPACK_CPP_DIR=" + DATAPACK_CPP_DIR,
        "-DCATCHCHALLENGER_HARDENED=ON",
    ]
    rc, out = run_cmd(cfg, STAGE2_BUILD)
    if rc != 0:
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"cmake configure failed (rc={rc})"); print(out[-3000:]); return None
    log_info(f"stage2 cmake --build -j{NPROC}")
    rc, out = run_cmd(NICE_PREFIX_COMPILE + ["cmake", "--build", STAGE2_BUILD, "-j", NPROC],
                      STAGE2_BUILD)
    if rc != 0 and shutil.which("ccache"):
        log_info("stage2 build failed; flushing ccache + retry once")
        subprocess.run(["ccache", "-C"], capture_output=True, text=True, timeout=120)
        rc, out = run_cmd(NICE_PREFIX_COMPILE + ["cmake", "--build", STAGE2_BUILD, "-j", NPROC],
                          STAGE2_BUILD)
    if rc != 0 or not os.path.isfile(STAGE2_EXE):
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"stage2 build failed (rc={rc})"); print(out[-3000:]); return None
    log_pass(name, f"-> {os.path.relpath(STAGE2_EXE, ROOT)} ({os.path.getsize(STAGE2_EXE)//1024} KiB)")
    return STAGE2_EXE


def run_stage2_no_fs():
    """Run the stage2 server in a directory with ONLY the binary — no datapack/,
    no server-properties.xml, no database/. Assert it loads the datapack +
    settings from flash, binds, and arms the LAN announce. This is the core
    no-filesystem proof and needs no ESP-IDF."""
    name = "stage2 run no-FS: datapack+settings from flash, bind, LAN announce"
    work = os.path.join(STAGE2_BUILD, "nofs-run")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    shutil.copy2(STAGE2_EXE, os.path.join(work, "catchchallenger-server-cli"))
    log_info("stage2 launching server with empty working dir (no filesystem)")
    logpath = os.path.join(work, "server.log")
    with open(logpath, "wb") as lf:
        proc = subprocess.Popen([os.path.join(work, "catchchallenger-server-cli")],
                                cwd=work, stdout=lf, stderr=subprocess.STDOUT)
    bound = False
    try:
        deadline = time.monotonic() + clamp_local(BIND_TIMEOUT)
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                break
            try:
                with open(logpath, "rb") as f:
                    if b"correctly bind" in f.read():
                        bound = True
                        break
            except OSError:
                pass
            time.sleep(0.5)
        with open(logpath, "rb") as f:
            log = f.read().decode(errors="replace")
    finally:
        try:
            proc.terminate(); proc.wait(timeout=10)
        except (OSError, subprocess.SubprocessError):
            try:
                proc.kill()
            except OSError:
                pass
    # The datapack-cpp deserialize prints the per-section sizes; require both a
    # non-trivial datapack load and the bind, plus the baked LAN-announce.
    loaded = ("commonDatapack size:" in log) and ("Loaded the common datapack" in log)
    announce = "LAN announce enabled:" in log
    if bound and loaded and announce:
        log_pass(name, "datapack+settings loaded from flash, bound, LAN announce armed")
        return True
    log_fail(name, f"bound={bound} datapackLoaded={loaded} lanAnnounce={announce}")
    print(log[-3000:])
    return False


def _poll_file_for_marker(path, marker, timeout, proc=None):
    """Poll a growing log file for a marker (mirrors testingcompilationmsdos
    _wait_for_bind). Returns (found, text). If proc is given, give up early when
    it exits."""
    deadline = time.monotonic() + clamp_local(timeout)
    while time.monotonic() < deadline:
        if proc is not None and proc.poll() is not None:
            break
        try:
            with open(path, "rb") as f:
                data = f.read()
            if marker.encode() in data:
                return True, data.decode(errors="replace")
        except OSError:
            pass
        time.sleep(0.5)
    try:
        with open(path, "rb") as f:
            return False, f.read().decode(errors="replace")
    except OSError:
        return False, ""


def _poll_serial_for_marker(tty, marker, timeout):
    """Read a real board's serial tty for a marker. `timeout cat <tty>` in short
    bursts is script-friendly and needs no pyserial (pure stdlib). Returns
    (found, accumulated_text)."""
    deadline = time.monotonic() + clamp_local(timeout)
    buf = []
    while time.monotonic() < deadline:
        try:
            p = subprocess.run(["timeout", "2", "cat", tty],
                               stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                               timeout=4)
            chunk = p.stdout.decode(errors="replace")
            if chunk:
                buf.append(chunk)
                if marker in "".join(buf):
                    return True, "".join(buf)
        except (subprocess.TimeoutExpired, OSError):
            pass
    return False, "".join(buf)


def _client_connect(host, port):
    """Run qtcpu800x600 --host/--port --autologin against host:port. Returns
    (ok, detail, out). ok when the client reaches the map OR (the no-FS server
    case) connects + protocol-good / logs in (CLIENT_OK_MARKERS).

    The fileless server never delivers the map, so the client (without
    --closewhenonmap firing) never exits on its own — we STREAM its log, return
    as soon as a marker appears (fast), and kill it. (subprocess.run+timeout
    would discard the output, so we cannot use run_cmd here.)"""
    import tempfile
    cenv = os.environ.copy()
    cenv["QT_QPA_PLATFORM"] = "offscreen"
    cargs = [CLIENT_CPU_BIN, "--host", host, "--port", str(port),
             "--autologin", "--character", "PlayerCPU", "--closewhenonmap"]
    diagnostic.record_cmd(cargs, os.path.dirname(CLIENT_CPU_BIN))
    fd, logp = tempfile.mkstemp(prefix="cc-esp32-client-", suffix=".log")
    os.close(fd)
    found = None
    with open(logp, "wb") as lf:
        cp = subprocess.Popen(cargs, cwd=os.path.dirname(CLIENT_CPU_BIN),
                              env=cenv, stdout=lf, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + clamp_local(CLIENT_TIMEOUT)
        while time.monotonic() < deadline and found is None:
            done = cp.poll() is not None
            try:
                with open(logp, "rb") as f:
                    data = f.read().decode(errors="replace")
            except OSError:
                data = ""
            for marker, detail in CLIENT_OK_MARKERS:
                if marker in data:
                    found = detail
                    break
            if found is not None or done:
                break
            time.sleep(0.5)
        with open(logp, "rb") as f:
            out = f.read().decode(errors="replace")
    finally:
        try:
            cp.terminate(); cp.wait(timeout=10)
        except (OSError, subprocess.SubprocessError):
            try:
                cp.kill()
            except OSError:
                pass
        try:
            os.remove(logp)
        except OSError:
            pass
    if found is None:
        for marker, detail in CLIENT_OK_MARKERS:
            if marker in out:
                found = detail
                break
    if found is not None:
        return True, found, out
    return False, "client did not connect/handshake", out


def run_stage2_client_connect():
    """End-to-end: point the native qtcpu800x600 client at the host no-FS server
    (same binary the ESP32 runs). Asserts the client connects + protocol-good.
    The fileless server serves no datapack files, so the client logs in but does
    not reach the map (see ESP32.md). Self-skips if the native client is absent."""
    name = "qtcpu800x600 --host/--port --autologin -> ESP32 no-FS server (host)"
    if not os.path.isfile(CLIENT_CPU_BIN):
        log_info("client connect skipped — native qtcpu800x600 not built "
                 "(run testingclient.py first)")
        return False
    work = os.path.join(STAGE2_BUILD, "connect-run")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    shutil.copy2(STAGE2_EXE, os.path.join(work, "catchchallenger-server-cli"))
    logpath = os.path.join(work, "server.log")
    with open(logpath, "wb") as lf:
        srv = subprocess.Popen([os.path.join(work, "catchchallenger-server-cli")],
                               cwd=work, stdout=lf, stderr=subprocess.STDOUT)
    try:
        bound, _ = _poll_file_for_marker(logpath, "correctly bind", BIND_TIMEOUT, srv)
        if not bound:
            log_fail(name, "server did not bind")
            return False
        ok, detail, out = _client_connect("127.0.0.1", SERVER_PORT)
        if ok:
            log_pass(name, detail)
            return True
        log_fail(name, detail)
        print(out[-2500:])
        return False
    finally:
        try:
            srv.terminate(); srv.wait(timeout=10)
        except (OSError, subprocess.SubprocessError):
            try:
                srv.kill()
            except OSError:
                pass


# ── esp-idf compile (self-skip when the toolchain is absent) ──────────────────
def esp_idf_available():
    missing = []
    if not os.path.isfile(IDF_PY):
        missing.append("esp-idf (IDF_PATH/tools/idf.py)")
    if not os.path.isdir(ESP_PROJECT):
        missing.append("server/cli/esp32 project")
    return missing


def idf_env():
    """Fresh env for idf.py: only the minimum + IDF_PATH + IDF_TOOLS_PATH. The
    operator's esp-idf/export.sh prepends the xtensa toolchain to PATH; we re-run
    it. export.sh resolves the installed tools via IDF_TOOLS_PATH (default
    ~/.espressif) -- this prefix keeps them under ESP32_PREFIX/tools, so we MUST
    pass IDF_TOOLS_PATH or export.sh finds nothing and `idf.py` is not on PATH
    (rc=127). Honour an operator override, else default to the prefix convention."""
    env = {}
    for k in ("HOME", "USER", "LANG", "TERM", "TMPDIR"):
        if k in os.environ:
            env[k] = os.environ[k]
    for k, v in os.environ.items():
        if k.startswith("LC_"):
            env[k] = v
    env["IDF_PATH"] = IDF_PATH
    env["IDF_TOOLS_PATH"] = os.environ.get(
        "IDF_TOOLS_PATH", os.path.join(ESP32_PREFIX, "tools"))
    env["PATH"] = os.environ.get("PATH", "/usr/bin:/bin")
    return env


def build_esp_idf():
    name = "esp-idf build catchchallenger-server (xtensa-esp32)"
    missing = esp_idf_available()
    if missing:
        log_info("esp-idf build skipped — missing: " + ", ".join(missing)
                 + f" (prefix {ESP32_PREFIX}); see test/ESP32.md")
        return False
    if not os.path.isfile(os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.cpp")):
        log_info("esp-idf build skipped — stage1 datapack-cpp not generated")
        return False
    idf_build = build_paths.build_path("server/cli/build/testing-esp32-idf")
    env = idf_env()
    # idf.py is run through esp-idf/export so the toolchain is on PATH; the
    # project picks up the generated datapack-cpp via CATCHCHALLENGER_DATAPACK_CPP_DIR.
    cmd = ["bash", "-lc",
           f". '{IDF_PATH}/export.sh' >/dev/null 2>&1; "
           f"idf.py -C '{ESP_PROJECT}' -B '{idf_build}' "
           f"-DCATCHCHALLENGER_DATAPACK_CPP_DIR='{DATAPACK_CPP_DIR}' build"]
    log_info("esp-idf idf.py build (xtensa-esp32, Release)")
    rc, out = run_cmd(cmd, ESP_PROJECT, timeout=IDF_TIMEOUT, env=env)
    binpath = os.path.join(idf_build, "catchchallenger-server-esp32.bin")
    if rc == 0 and os.path.isfile(binpath):
        log_pass(name, f"-> {os.path.relpath(binpath, ROOT)} ({os.path.getsize(binpath)//1024} KiB)")
        return True
    _fc.set_extras(name, compile_output=out)
    log_fail(name, f"idf.py build failed (rc={rc})")
    print(out[-4000:])
    return False


def run_qemu_xtensa():
    """Boot the ESP-IDF firmware headless under qemu-system-xtensa (Espressif
    fork) with a host TCP port-forward into the guest's listener, wait for
    'correctly bind:' on the emulated serial, then connect the native client to
    127.0.0.1:<port> (slirp hostfwd). Self-skips cleanly when qemu-xtensa, the
    firmware image, or the native client is absent. The emulator mirrors the real
    board (see test/ESP32.md §5)."""
    name = "qtcpu800x600 --host/--port --autologin -> ESP32 server (qemu-xtensa)"
    if QEMU_XTENSA is None:
        log_info("qemu-xtensa connect skipped — qemu-system-xtensa not found "
                 "(Espressif qemu fork; see test/ESP32.md §5)")
        return False
    if not os.path.isfile(IDF_BIN):
        log_info("qemu-xtensa connect skipped — no ESP-IDF firmware image "
                 "(esp-idf build was skipped or failed)")
        return False
    if not os.path.isfile(CLIENT_CPU_BIN):
        log_info("qemu-xtensa connect skipped — native qtcpu800x600 not built")
        return False
    work = os.path.join(IDF_BUILD, "qemu-run")
    if os.path.isdir(work):
        shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work, exist_ok=True)
    serial_log = os.path.join(work, "serial.log")
    accel = ["-accel", "kvm"] if os.path.exists("/dev/kvm") else []
    qargs = [QEMU_XTENSA] + accel + [
        "-m", "4", "-machine", "esp32",
        "-drive", f"file={IDF_BIN},format=raw,if=mtd",
        "-netdev", f"user,id=n0,hostfwd=tcp::{SERVER_PORT}-:{SERVER_PORT}",
        "-device", "virtio-net-device,netdev=n0",
        "-serial", f"file:{serial_log}",
        "-display", "none", "-nographic", "-no-reboot"]
    log_info("booting ESP32 firmware under qemu-system-xtensa "
             + ("(KVM)" if accel else "(TCG)")
             + f", slirp hostfwd {SERVER_PORT}")
    diagnostic.record_cmd(qargs, work)
    qproc = subprocess.Popen(qargs, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        bound, slog = _poll_file_for_marker(serial_log, "correctly bind",
                                            QEMU_BIND_TIMEOUT, qproc)
        if not bound:
            log_fail(name, f"firmware never reached 'correctly bind:' under qemu "
                           f"(within {QEMU_BIND_TIMEOUT}s)")
            print(slog[-2500:])
            return False
        log_info("ESP32 firmware bound under qemu; connecting native client via slirp")
        ok, detail, out = _client_connect("127.0.0.1", SERVER_PORT)
        if ok:
            log_pass(name, "qemu: " + detail)
            return True
        log_fail(name, "qemu: " + detail)
        print(out[-2500:])
        return False
    finally:
        try:
            qproc.kill(); qproc.wait(timeout=10)
        except (OSError, subprocess.SubprocessError):
            pass


def _usb_vidpid(tty):
    """Return (vid,pid) lowercase hex for a /dev/tty* USB serial, or None. Walks
    /sys/class/tty/<name>/device up to the USB device node holding idVendor/idProduct."""
    d = os.path.realpath("/sys/class/tty/" + os.path.basename(tty) + "/device")
    hops = 0
    while hops < 8:
        vidf = os.path.join(d, "idVendor")
        pidf = os.path.join(d, "idProduct")
        if os.path.isfile(vidf) and os.path.isfile(pidf):
            try:
                with open(vidf) as f:
                    v = f.read().strip().lower()
                with open(pidf) as f:
                    p = f.read().strip().lower()
                return v, p
            except OSError:
                return None
        nd = os.path.dirname(d)
        if nd == d:
            break
        d = nd
        hops += 1
    return None


def _esptool_is_esp32(port, env):
    """Definitive check: ask esptool what chip is on `port`. True iff it
    identifies an ESP32-family chip. Run through esp-idf/export.sh (esptool is
    only on PATH there); a non-ESP/empty port just fails to connect -> False."""
    cmd = ["bash", "-lc",
           f". '{IDF_PATH}/export.sh' >/dev/null 2>&1; "
           f"esptool.py --port '{port}' --connect-attempts 1 chip_id 2>&1 || true"]
    rc, out = run_cmd(cmd, ESP_PROJECT, timeout=30, env=env)
    return ("Chip is ESP32" in out) or ("Detecting chip type" in out and "ESP32" in out)


def _detect_esp32_serial(env):
    """Autodetect the ESP32's USB serial port (when --serial was not given).
    Enumerate /dev/ttyUSB*+/dev/ttyACM*, rank by known ESP32 USB-serial vendor
    IDs, then confirm with esptool chip_id (probes known-VID ports first to avoid
    resetting unrelated devices). Returns (port, how) or (None, reason)."""
    cands = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    if not cands:
        return None, "no /dev/ttyUSB*|/dev/ttyACM* present — connect the ESP32"
    ranked = []
    i = 0
    while i < len(cands):
        vp = _usb_vidpid(cands[i])
        known = vp is not None and vp[0] in _ESP32_USB_VIDS
        ranked.append((0 if known else 1, cands[i], vp))
        i += 1
    ranked.sort(key=lambda x: x[0])
    # esptool-confirm in rank order (known-VID first) — return the first ESP32.
    j = 0
    while j < len(ranked):
        tty = ranked[j][1]; vp = ranked[j][2]
        if _esptool_is_esp32(tty, env):
            tag = (vp[0] + ":" + vp[1] + " ") if vp else ""
            return tty, tag + "esptool-confirmed ESP32"
        j += 1
    # esptool found none (boards in run mode / no auto-reset): fall back to the
    # first known-VID candidate as a heuristic.
    k = 0
    while k < len(ranked):
        if ranked[k][0] == 0:
            vp = ranked[k][2]
            return ranked[k][1], (vp[0] + ":" + vp[1] +
                   " (ESP32 USB-serial vendor id; esptool could not auto-connect)")
        k += 1
    return None, ("none of " + ", ".join(cands) +
                  " is an ESP32 (unknown USB vendor id, esptool no response)")


def flash_real_hardware():
    """--realhardware: flash the connected board (idf.py flash over the serial
    port), read its serial for the DHCP IP + 'correctly bind:', then connect the
    native client to the board's IP. The port is --serial if given, else
    autodetected (_detect_esp32_serial). Only runs when --realhardware was passed
    AND a board is present; self-skips otherwise."""
    name = "real ESP32: idf.py flash + qtcpu800x600 --autologin connect"
    if not REALHARDWARE:
        return False                       # operator opt-in only
    missing = esp_idf_available()
    if missing:
        log_info("--realhardware skipped — missing: " + ", ".join(missing))
        return False
    if not os.path.isfile(IDF_BIN):
        log_info("--realhardware skipped — no firmware image "
                 "(esp-idf build was skipped or failed)")
        return False
    if not os.path.isfile(CLIENT_CPU_BIN):
        log_info("--realhardware skipped — native qtcpu800x600 not built")
        return False
    env = idf_env()
    # Resolve the serial port: --serial pins it, otherwise autodetect the ESP32.
    if SERIAL_PORT is not None:
        port = SERIAL_PORT
        if not os.path.exists(port):
            log_info(f"--realhardware skipped — no board on {port} "
                     f"(connect one or pass --serial <dev>)")
            return False
    else:
        port, how = _detect_esp32_serial(env)
        if port is None:
            log_info(f"--realhardware skipped — {how}")
            return False
        log_info(f"--realhardware: autodetected ESP32 serial {port} ({how})")
    cmd = ["bash", "-lc",
           f". '{IDF_PATH}/export.sh' >/dev/null 2>&1; "
           f"idf.py -C '{ESP_PROJECT}' -B '{IDF_BUILD}' -p '{port}' flash"]
    log_info(f"--realhardware: idf.py flash -> {port}")
    rc, out = run_cmd(cmd, ESP_PROJECT, timeout=IDF_TIMEOUT, env=env)
    if rc != 0:
        _fc.set_extras(name, compile_output=out)
        log_fail(name, f"idf.py flash failed (rc={rc})")
        print(out[-3000:])
        return False
    log_info(f"flashed; reading {port} for DHCP IP + bind")
    found, serial = _poll_serial_for_marker(port, "correctly bind", QEMU_BIND_TIMEOUT)
    import re
    m = re.search(r"DHCP got IP (\d+\.\d+\.\d+\.\d+)", serial)
    if not found or m is None:
        log_fail(name, "board did not print 'DHCP got IP <addr>' + 'correctly bind:' on serial")
        print(serial[-2000:])
        return False
    board_ip = m.group(1)
    log_info(f"board bound at {board_ip}:{SERVER_PORT}; connecting native client")
    ok, detail, cout = _client_connect(board_ip, SERVER_PORT)
    if ok:
        log_pass(name, f"{board_ip}: {detail}")
        return True
    log_fail(name, f"{board_ip}: {detail}")
    print(cout[-2500:])
    return False


# ── main ──────────────────────────────────────────────────────────────────────
def cleanup(*_a):
    sys.exit(1)


signal.signal(signal.SIGINT, cleanup)
signal.signal(signal.SIGTERM, cleanup)


def main():
    print(f"\n{C_CYAN}{'='*64}")
    print("  CatchChallenger — ESP32 (ESP-IDF) datapack-cpp no-filesystem port")
    print(f"{'='*64}{C_RESET}\n")

    failed_cases = load_failed_cases()
    if failed_cases is not None and len(failed_cases) == 0:
        log_info("all previously passed, skipping (delete failed.json for full re-run)")
        return

    emit_ok = False
    if should_run("stage1 datapack-cpp emit (server-cli --save -> datapack_cpp.cpp)",
                  failed_cases):
        emit_ok = build_stage1_and_emit()
    elif os.path.isfile(os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.cpp")):
        emit_ok = True

    exe = None
    if emit_ok and should_run(
            "stage2 build no-FS server (NOXML + DATAPACK_CPP + DB_INTERNAL_VARS)",
            failed_cases):
        exe = build_stage2()
    elif os.path.isfile(STAGE2_EXE):
        exe = STAGE2_EXE

    if exe is not None:
        if should_run("stage2 run no-FS: datapack+settings from flash, bind, LAN announce",
                      failed_cases):
            run_stage2_no_fs()
        if should_run("qtcpu800x600 --host/--port --autologin -> ESP32 no-FS server (host)",
                      failed_cases):
            run_stage2_client_connect()

    # ESP-IDF build, then connect a client to the firmware under the emulator and
    # (with --realhardware) on the real board. All self-skip cleanly when the
    # toolchain / emulator / board is absent.
    if should_run("esp-idf build catchchallenger-server (xtensa-esp32)", failed_cases):
        build_esp_idf()
    if should_run("qtcpu800x600 --host/--port --autologin -> ESP32 server (qemu-xtensa)",
                  failed_cases):
        run_qemu_xtensa()
    if REALHARDWARE and should_run(
            "real ESP32: idf.py flash + qtcpu800x600 --autologin connect", failed_cases):
        flash_real_hardware()

    save_failed_cases()
    summary()


def summary():
    print(f"\n{C_CYAN}{'='*64}")
    print("  Summary")
    print(f"{'='*64}{C_RESET}")
    passed = sum(1 for r in results if r[1])
    failed = sum(1 for r in results if not r[1])
    total_elapsed = sum(r[3] for r in results)
    for nm, ok, detail, elapsed in results:
        tag = f"{C_GREEN}PASS{C_RESET}" if ok else f"{C_RED}FAIL{C_RESET}"
        print(f"  [{tag}] {nm}  {detail}  ({elapsed:.1f}s)")
    print(f"  total elapsed: {total_elapsed:.1f}s")
    print()
    print(f"  {C_GREEN}{passed} passed{C_RESET}, {C_RED}{failed} failed{C_RESET}")
    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
