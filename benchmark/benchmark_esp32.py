#!/usr/bin/env python3
"""benchmark_esp32.py -- ESP32 serial autodetection (and, when the ESP-IDF
toolchain is present, build+flash) for the CatchChallenger benchmark.

WHY autodetect instead of a hard-coded /dev/ttyUSBN:
  * ttyUSBN numbers are NOT stable -- they renumber across replug/reboot, so a
    pinned "ttyUSB4" silently points at the wrong board next session.
  * Several attached boards share the same ESP32-class USB-serial bridge VID
    (CP210x 10c4, CH34x 1a86, FTDI 0403, Espressif 303a), so VID alone cannot
    disambiguate, and cheap CP2102s report a NON-unique serial ("0001") so
    /dev/serial/by-id collides -- it is unreliable here.
  * esptool (the usual confirmer) is NOT installed on this host.

HOW detection works WITHOUT esptool (the key trick):
  every ESP32 prints a fixed ROM boot banner ("ets ...", "rst:0x...", "boot:0x",
  "ESP-ROM") the instant EN is toggled. We hardware-reset a candidate by pulsing
  RTS (the auto-reset line on ESP32 devkits) via pure termios ioctls -- NO data
  is written, only modem-control lines -- then read the banner back. Presence of
  the ROM markers == "this IS an ESP32". A second content fingerprint then tells
  us it is OUR board: CatchChallenger markers once flashed, or the operator's
  power-meter banner before flashing.

Resolution order (first hit wins), so a steady setup never re-probes the fleet:
  1. explicit override argument / --serial <dev>
  2. $CC_ESP32_SERIAL
  3. cached by-path from a previous successful detect (re-verified)
  4. probe ESP32-VID ttys (skipping ones held by another process) and match.

A by-path handle (e.g. /dev/serial/by-path/...-usb-0:3.2.1.3:1.0-port0) is the
stable thing to pin once found -- it survives renumbering as long as the board
stays in the same physical USB port.

This module is import-only for the benchmark; run directly to test detection:
    python3 benchmark_esp32.py --detect            # autodetect + print the port
    python3 benchmark_esp32.py --detect /dev/ttyUSB4   # confirm one device
"""
import os
import sys
import glob
import time
import json
import array
import fcntl
import select
import termios
import subprocess

# USB vendor IDs of the serial bridges on ESP32 boards (+ native USB-CDC).
ESP32_USB_VIDS = {"10c4", "1a86", "0403", "303a"}

# ROM boot-banner substrings every ESP32 emits on reset -> "is an ESP32"
# (no esptool needed). The exact strings come from the ESP32 first-stage loader.
ROM_MARKERS = ("ets ", "rst:0x", "ESP-ROM", "boot:0x", "SPI_FAST_FLASH_BOOT")
# Fingerprints that identify *our* board specifically.
CC_MARKERS = ("CatchChallenger", "correctly bind", "DHCP got IP",
              "Loaded the common datapack", "LAN announce")
# The operator's board ships (pre-flash) with a current/voltage probe firmware;
# its banner uniquely identifies the board to (re)flash before we overwrite it.
POWERMETER_MARKERS = ("currentProbeInputCalibration", "currentProbeOutputCalibration",
                      "VMainCalibration", "VBatteryCalibration", "VMiddleCalibration")

_CACHE_PATH = os.path.join(os.path.expanduser("~"), ".config",
                           "catchchallenger-testing", "esp32-serial.json")

# Linux modem-control ioctls / line bits (not always exposed by termios module).
_TIOCMGET = 0x5415
_TIOCMSET = 0x5418
_TIOCM_DTR = 0x002
_TIOCM_RTS = 0x004


def _usb_vidpid(tty):
    """(vid, pid) lowercase hex for a /dev/tty* USB serial, or None. Walks
    /sys/class/tty/<name>/device up to the USB node holding idVendor/idProduct."""
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


def bypath_of(tty):
    """The stable /dev/serial/by-path/* handle for a tty (physical USB port),
    or the tty itself if none. Pin THIS to survive ttyUSBN renumbering."""
    real = os.path.realpath(tty)
    bp = "/dev/serial/by-path"
    if os.path.isdir(bp):
        names = sorted(os.listdir(bp))
        i = 0
        while i < len(names):
            link = os.path.join(bp, names[i])
            if os.path.realpath(link) == real:
                return link
            i += 1
    return tty


def _is_busy(tty):
    """True if another process holds the tty (e.g. a ser2net fleet console) --
    we must not reset those."""
    try:
        p = subprocess.run(["fuser", tty], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=5)
        return p.returncode == 0
    except (OSError, subprocess.SubprocessError):
        return False


def read_with_reset(tty, secs=6, baud=termios.B115200):
    """Hardware-reset the board (pulse RTS->EN) and read `secs` of serial. Pure
    termios: only modem-control lines are toggled, no bytes are written. Returns
    decoded text ('' on error). Resetting an ESP32 just reboots it (harmless)."""
    try:
        fd = os.open(tty, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError:
        return ""
    try:
        a = termios.tcgetattr(fd)
        iflag, oflag, cflag, lflag, ispeed, ospeed, cc = a
        cflag = (cflag & ~termios.CSIZE) | termios.CS8 | termios.CLOCAL | termios.CREAD
        cflag &= ~(termios.PARENB | termios.CSTOPB | termios.CRTSCTS)
        iflag &= ~(termios.IXON | termios.IXOFF | termios.IXANY | termios.IGNBRK |
                   termios.BRKINT | termios.PARMRK | termios.ISTRIP | termios.INLCR |
                   termios.IGNCR | termios.ICRNL)
        oflag &= ~termios.OPOST
        lflag &= ~(termios.ECHO | termios.ECHONL | termios.ICANON | termios.ISIG |
                   termios.IEXTEN)
        termios.tcsetattr(fd, termios.TCSANOW,
                          [iflag, oflag, cflag, lflag, baud, baud, cc])

        def _set(dtr, rts):
            buf = array.array('i', [0])
            fcntl.ioctl(fd, _TIOCMGET, buf, True)
            v = buf[0]
            v = (v | _TIOCM_DTR) if dtr else (v & ~_TIOCM_DTR)
            v = (v | _TIOCM_RTS) if rts else (v & ~_TIOCM_RTS)
            fcntl.ioctl(fd, _TIOCMSET, array.array('i', [v]), True)

        # esptool classic reset into NORMAL run mode: EN low then high, IO0 high.
        _set(dtr=False, rts=True)
        time.sleep(0.12)
        _set(dtr=False, rts=False)
        time.sleep(0.05)

        out = bytearray()
        deadline = time.monotonic() + secs
        while time.monotonic() < deadline:
            r, _, _ = select.select([fd], [], [], 1.0)
            if r:
                try:
                    out += os.read(fd, 4096)
                except OSError:
                    pass
        return out.decode("latin-1")
    except (OSError, termios.error):
        return ""
    finally:
        try:
            os.close(fd)
        except OSError:
            pass


def _has_any(text, markers):
    i = 0
    while i < len(markers):
        if markers[i] in text:
            return True
        i += 1
    return False


def _score(text):
    """Rank a probe: -1 not an ESP32; 0 ESP32 but unknown firmware; 1 our board
    pre-flash (power-meter); 2 our board flashed (CatchChallenger)."""
    if not _has_any(text, ROM_MARKERS):
        return -1
    if _has_any(text, CC_MARKERS):
        return 2
    if _has_any(text, POWERMETER_MARKERS):
        return 1
    return 0


def _load_cache():
    try:
        with open(_CACHE_PATH) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def _save_cache(bypath):
    try:
        os.makedirs(os.path.dirname(_CACHE_PATH), exist_ok=True)
        with open(_CACHE_PATH, "w") as f:
            json.dump({"bypath": bypath}, f, indent=2)
    except OSError:
        pass


def candidate_ttys():
    """ESP32-class USB serial ttys not currently held by another process,
    in /dev/serial/by-path order for stability."""
    raw = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    out = []
    i = 0
    while i < len(raw):
        tty = raw[i]
        vp = _usb_vidpid(tty)
        if vp is not None and vp[0] in ESP32_USB_VIDS and not _is_busy(tty):
            out.append(tty)
        i += 1
    return out


def detect_esp32_serial(override=None, probe_secs=6, use_cache=True, verbose=True):
    """Resolve the ESP32 serial port. Returns (dev, how) or (None, reason).
    `dev` is a concrete /dev/tty* path. Resolution order: override -> env ->
    cached by-path -> probe. Probing hardware-resets each candidate (ESP32:
    harmless reboot) and matches the ROM banner + our fingerprint."""
    def _say(m):
        if verbose:
            print("[esp32-detect] " + m)

    # 1) explicit override / 2) env
    cand = override or os.environ.get("CC_ESP32_SERIAL")
    if cand:
        if os.path.exists(cand):
            return cand, "override"
        return None, f"override {cand} does not exist"

    # 3) cached by-path (re-verify it still resolves to a present device)
    if use_cache:
        bp = _load_cache().get("bypath")
        if bp and os.path.exists(bp):
            return os.path.realpath(bp), f"cache {bp}"

    # 4) probe
    cands = candidate_ttys()
    if not cands:
        return None, ("no free ESP32-class USB serial found "
                      "(connect the board, or pass --serial / $CC_ESP32_SERIAL)")
    best = None  # (score, tty)
    i = 0
    while i < len(cands):
        tty = cands[i]
        _say(f"probing {tty} ({bypath_of(tty)}) ...")
        sc = _score(read_with_reset(tty, probe_secs))
        tag = {-1: "not-esp32", 0: "esp32(unknown-fw)",
               1: "esp32(power-meter=our board)", 2: "esp32(CatchChallenger)"}[sc]
        _say(f"  {tty}: {tag}")
        if sc >= 1 and (best is None or sc > best[0]):
            best = (sc, tty)
            if sc == 2:           # flashed CC board -> definitive, stop early
                break
        i += 1
    if best is None:
        return None, ("no ESP32 matched our fingerprint among "
                      + ", ".join(cands) + " (none showed CatchChallenger or the "
                      "power-meter banner)")
    tty = best[1]
    bp = bypath_of(tty)
    _save_cache(bp)
    how = ("CatchChallenger firmware" if best[0] == 2 else "power-meter firmware "
           "(pre-flash board)") + f"; pinned {bp}"
    return tty, how


# --------------------------------------------------------------------------
# Flashing requires the ESP-IDF toolchain + esptool, which are provisioned out
# of tree (like MXE / the MS-DOS prefix). When absent, the caller self-skips.
ESP32_PREFIX = os.environ.get("CC_ESP32_PREFIX",
                              "/mnt/data/perso/progs/catchchallenger-ESP32")
IDF_PATH = os.environ.get("IDF_PATH", os.path.join(ESP32_PREFIX, "esp-idf"))


def toolchain_status():
    """(ok, detail). ok=True only when ESP-IDF is present so build+flash is
    possible. Detection above needs NONE of this -- only flashing does."""
    missing = []
    if not os.path.isfile(os.path.join(IDF_PATH, "tools", "idf.py")):
        missing.append(f"esp-idf ({IDF_PATH}/tools/idf.py)")
    if missing:
        return False, "missing: " + ", ".join(missing) + f" (prefix {ESP32_PREFIX})"
    return True, "esp-idf present"


# --------------------------------------------------------------------------
# Build + flash pipeline. Everything lives under the prefix (operator wish:
# "all stuff for this task in this folder"); WiFi/IP come from the out-of-repo
# cc-esp32.conf so secrets never enter git.
import re
import shlex
import shutil

REPO_ROOT   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER_CLI  = os.path.join(REPO_ROOT, "server", "cli")
ESP_PROJECT = os.path.join(SERVER_CLI, "esp32")
DATAPACK_SRC = os.environ.get("CC_ESP32_DATAPACK",
                              "/home/user/Desktop/CatchChallenger/CatchChallenger-datapack")
CONF_PATH    = os.path.join(ESP32_PREFIX, "cc-esp32.conf")
STAGE1_BUILD = os.path.join(ESP32_PREFIX, "build-stage1")
DATAPACK_CPP_DIR = os.path.join(ESP32_PREFIX, "datapack-cpp")
FW_BUILD     = os.path.join(ESP32_PREFIX, "build-fw")
SDKCFG_OVERLAY = os.path.join(ESP32_PREFIX, "sdkconfig.cc-esp32")
STAGE_DATAPACK = os.path.join(ESP32_PREFIX, "datapack-stage")
FW_BIN       = os.path.join(FW_BUILD, "catchchallenger-server-esp32.bin")

ESP32_SERVER_PORT = int(os.environ.get("CC_ESP32_PORT", "42498"))
ESP32_MAINCODE    = "test"
ESP32_BROADCAST   = "CatchChallenger ESP32"
# ONLYBYMIRROR firmware requires a non-empty datapack mirror URL (the fileless
# server can't serve files). Canonical site; the benchmark bot uses a local
# datapack so it is never actually fetched. Override with CC_ESP32_MIRROR.
ESP32_MIRROR = os.environ.get("CC_ESP32_MIRROR",
                              "https://catchchallenger.herman-brule.com/datapack/")


def load_config():
    """Parse the out-of-repo cc-esp32.conf (WiFi creds + static IP). Returns a
    dict; missing file -> {} (caller self-skips). NEVER logged verbatim."""
    conf = {}
    try:
        with open(CONF_PATH) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#") and "=" in line:
                    k, v = line.split("=", 1)
                    conf[k.strip()] = v.strip()
    except OSError:
        pass
    return conf


def _run_idf(idf_args, cwd=None, timeout=1800):
    """Run `idf.py ...` after sourcing export.sh. Returns (rc, combined_output)."""
    env = os.environ.copy()
    env["IDF_TOOLS_PATH"] = os.path.join(ESP32_PREFIX, "tools")
    cmd = ["bash", "-lc",
           ". " + shlex.quote(os.path.join(IDF_PATH, "export.sh")) + " >/dev/null 2>&1; "
           + " ".join(shlex.quote(a) for a in idf_args)]
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return -1, f"idf.py TIMEOUT after {timeout}s"


def stage_datapack_server():
    """rsync the source datapack -> prefix staging, media-excluded (the server
    only parses it, never serves images). Never writes the read-only source."""
    os.makedirs(STAGE_DATAPACK, exist_ok=True)
    args = ["rsync", "-a", "--delete",
            "--exclude=*.png", "--exclude=*.jpg", "--exclude=*.ogg",
            "--exclude=*.opus", "--exclude=*.wav", "--exclude=.git",
            "--exclude=.claude",
            DATAPACK_SRC.rstrip("/") + "/", STAGE_DATAPACK + "/"]
    p = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    return p.returncode == 0, p.stdout.decode(errors="replace")[-500:]


def stage1_emit(verbose=True):
    """Build host server-cli (DATAPACK_CPP_EMIT) + run --save -> emit the
    human-readable datapack_cpp.{cpp,hpp} the firmware compiles into flash.
    Returns (ok, detail). Mirrors test/testingcompilationESP32.py stage1."""
    def _say(m):
        if verbose:
            print("[esp32-stage1] " + m)
    ok, msg = stage_datapack_server()
    if not ok:
        return False, "datapack staging failed: " + msg
    os.makedirs(STAGE1_BUILD, exist_ok=True)
    os.makedirs(DATAPACK_CPP_DIR, exist_ok=True)
    _say("cmake configure + build (EMIT + DB_INTERNAL_VARS + SELECT)")
    cfg = ["cmake", "-S", SERVER_CLI, "-B", STAGE1_BUILD, "-DCMAKE_BUILD_TYPE=Release",
           "-DCATCHCHALLENGER_SELECT=ON", "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
           "-DCATCHCHALLENGER_DATAPACK_CPP_EMIT=ON"]
    p = subprocess.run(cfg, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    if p.returncode != 0:
        return False, "cmake configure failed:\n" + p.stdout.decode(errors="replace")[-1500:]
    p = subprocess.run(["cmake", "--build", STAGE1_BUILD, "-j", str(os.cpu_count() or 1)],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=1800)
    if p.returncode != 0:
        return False, "build failed:\n" + p.stdout.decode(errors="replace")[-1500:]
    exe = os.path.join(STAGE1_BUILD, "catchchallenger-server-cli")
    work = os.path.join(STAGE1_BUILD, "save-run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    shutil.copy2(exe, os.path.join(work, "catchchallenger-server-cli"))
    shutil.copytree(STAGE_DATAPACK, os.path.join(work, "datapack"))
    with open(os.path.join(work, "server-properties.xml"), "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{ESP32_SERVER_PORT}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                '    <max-players value="10"/>\n'
                f'    <broadcastName value="{ESP32_BROADCAST}"/>\n'
                # ONLYBYMIRROR (the ESP32 firmware profile) refuses to start with
                # an empty mirror: the fileless server can't send the datapack, so
                # clients must fetch it from this URL. The benchmark bot uses a
                # LOCAL datapack and never downloads, but the value must be
                # non-empty to pass settings validation.
                f'    <httpDatapackMirror value="{ESP32_MIRROR}"/>\n'
                f'    <content><mainDatapackCode value="{ESP32_MAINCODE}"/></content>\n'
                '    <mapVisibility><minimize value="cpu"/></mapVisibility>\n'
                '</configuration>\n')
    _say("running --save (emit datapack_cpp.{cpp,hpp})")
    p = subprocess.run([os.path.join(work, "catchchallenger-server-cli"), "--save"],
                       cwd=work, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300)
    cpp = os.path.join(work, "datapack_cpp.cpp")
    hpp = os.path.join(work, "datapack_cpp.hpp")
    if not (os.path.isfile(cpp) and os.path.isfile(hpp)):
        return False, "--save did not emit datapack_cpp.*:\n" + p.stdout.decode(errors="replace")[-1500:]
    shutil.copy2(cpp, os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.cpp"))
    shutil.copy2(hpp, os.path.join(DATAPACK_CPP_DIR, "datapack_cpp.hpp"))
    return True, f"datapack_cpp.cpp ({os.path.getsize(cpp)//1024} KiB)"


def write_overlay(config):
    """Write the out-of-repo sdkconfig overlay (WiFi creds + static IP) with
    0600 perms. Secrets stay in the prefix, never in git."""
    need = ("WIFI_SSID", "WIFI_PASSWORD", "STATIC_IP", "STATIC_NETMASK", "STATIC_GATEWAY")
    for k in need:
        if k not in config:
            return False, f"cc-esp32.conf missing {k}"
    old = os.umask(0o077)
    try:
        with open(SDKCFG_OVERLAY, "w") as f:
            f.write("# GENERATED out-of-repo overlay — WiFi creds + static IP. NEVER commit.\n")
            f.write('CONFIG_CATCHCHALLENGER_WIFI_SSID="%s"\n'      % config["WIFI_SSID"])
            f.write('CONFIG_CATCHCHALLENGER_WIFI_PASSWORD="%s"\n'  % config["WIFI_PASSWORD"])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_IP="%s"\n'      % config["STATIC_IP"])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_NETMASK="%s"\n' % config["STATIC_NETMASK"])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_GATEWAY="%s"\n' % config["STATIC_GATEWAY"])
            # partitions.csv is right-sized (1.875 MiB factory) to fit a 2 MiB
            # flash, so the image is portable to cheap 2 MiB ESP32s as well as the
            # 4 MiB D0WD devkits here (a 2 MiB image flashes fine on a 4 MiB chip).
            f.write('CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y\n')
            f.write('CONFIG_ESPTOOLPY_FLASHSIZE="2MB"\n')
    finally:
        os.umask(old)
    return True, SDKCFG_OVERLAY


def build_firmware(config, verbose=True):
    """idf.py set-target esp32 + build, with the out-of-repo overlay + the
    stage1 datapack-cpp. Returns (ok, detail)."""
    ok, msg = write_overlay(config)
    if not ok:
        return False, msg
    env_sdk = os.path.join(SERVER_CLI, "esp32", "sdkconfig.defaults") + ";" + SDKCFG_OVERLAY
    os.environ["SDKCONFIG_DEFAULTS"] = env_sdk
    shutil.rmtree(FW_BUILD, ignore_errors=True)
    if verbose:
        print("[esp32-build] idf.py build (xtensa-esp32) — single configure with all -D")
    # ALL -D at the first (only) configure: -DIDF_TARGET sets the chip and
    # -DCATCHCHALLENGER_DATAPACK_CPP_DIR must be cached before main/CMakeLists.txt
    # reads it (a separate set-target would cache the default dir first).
    # -DSDKCONFIG out-of-repo: idf.py otherwise writes the RESOLVED sdkconfig
    # (which contains the WiFi password merged from the overlay) into the project
    # dir = inside the git repo. Keep it in the build dir so no secret lands in
    # the tree (also satisfies "separate compilation artifacts").
    rc, out = _run_idf(["idf.py", "-C", ESP_PROJECT, "-B", FW_BUILD,
                        "-DIDF_TARGET=esp32",
                        "-DSDKCONFIG=" + os.path.join(FW_BUILD, "sdkconfig"),
                        "-DCATCHCHALLENGER_DATAPACK_CPP_DIR=" + DATAPACK_CPP_DIR, "build"],
                       timeout=1800)
    if rc != 0 or not os.path.isfile(FW_BIN):
        return False, "build failed:\n" + out[-2000:]
    return True, f"{FW_BIN} ({os.path.getsize(FW_BIN)//1024} KiB)"


def flash(tty, verbose=True):
    """idf.py -p <tty> flash. Returns (ok, detail)."""
    if verbose:
        print(f"[esp32-flash] idf.py -p {tty} flash")
    rc, out = _run_idf(["idf.py", "-C", ESP_PROJECT, "-B", FW_BUILD, "-p", tty, "flash"],
                       timeout=600)
    if rc != 0:
        return False, "flash failed:\n" + out[-2000:]
    return True, "flashed"


def read_serial_ip(tty, timeout=90):
    """Reset the board and read serial until it reports its IP + binds. Returns
    (ip_or_None, bound_bool, text)."""
    text = read_with_reset(tty, secs=min(timeout, 90))
    m = re.search(r"(?:DHCP got IP|static IPv4 configured:?)\s*(\d+\.\d+\.\d+\.\d+)", text)
    ip = m.group(1) if m else None
    bound = "correctly bind" in text
    return ip, bound, text


def ensure_online(config=None, flash_if_needed=True, serial=None, verbose=True):
    """Make the ESP32 server reachable; return (host, detail) or (None, reason).
    Fast path: if host:port already answers, return it (no reflash). Otherwise,
    when the toolchain + creds are present, stage1 -> build -> flash -> wait, and
    return the static IP. Self-skips (None) when anything required is absent."""
    config = config or load_config()
    host = config.get("STATIC_IP") or os.environ.get("CC_ESP32_HOST", "192.168.158.160")
    # already up?
    try:
        import socket
        with socket.create_connection((host, ESP32_SERVER_PORT), timeout=3):
            return host, "already reachable (no reflash)"
    except OSError:
        pass
    if not flash_if_needed:
        return None, f"{host}:{ESP32_SERVER_PORT} unreachable and flash_if_needed=False"
    tc_ok, tc_detail = toolchain_status()
    if not tc_ok:
        return None, "cannot flash: " + tc_detail
    if not config:
        return None, f"cannot flash: missing {CONF_PATH} (WiFi creds)"
    dev = serial
    if dev is None:
        dev, how = detect_esp32_serial(override=config.get("SERIAL") or None, verbose=verbose)
        if dev is None:
            return None, "cannot flash: " + how
        if verbose:
            print(f"[esp32] target serial {dev} ({how})")
    ok, d = stage1_emit(verbose=verbose)
    if not ok:
        return None, "stage1 failed: " + d
    ok, d = build_firmware(config, verbose=verbose)
    if not ok:
        return None, "build failed: " + d
    ok, d = flash(dev, verbose=verbose)
    if not ok:
        return None, "flash failed: " + d
    if verbose:
        print("[esp32] flashed; reading serial for IP + bind ...")
    ip, bound, _txt = read_serial_ip(dev, timeout=90)
    # wait for the game port over the network (the real readiness signal)
    import socket
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, ESP32_SERVER_PORT), timeout=3):
                return host, f"flashed + online (serial ip={ip}, bound={bound})"
        except OSError:
            time.sleep(2)
    return None, (f"flashed but {host}:{ESP32_SERVER_PORT} not reachable in time "
                  f"(serial ip={ip}, bound={bound})")


if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "--detect":
        ov = args[1] if len(args) > 1 else None
        dev, how = detect_esp32_serial(override=ov)
        if dev:
            print(f"DETECTED: {dev}  ({how})")
            print(f"stable handle: {bypath_of(dev)}")
            sys.exit(0)
        print(f"NOT DETECTED: {how}")
        sys.exit(1)
    ok, detail = toolchain_status()
    print(f"toolchain: {'OK' if ok else 'ABSENT'} -- {detail}")
    print("usage: python3 benchmark_esp32.py --detect [/dev/ttyUSBx]")
