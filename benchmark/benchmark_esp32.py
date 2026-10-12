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

import benchmark_helpers as bh

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
    we must not reset those. fuser only sees this user's processes, so a port
    held by root also counts as busy when it carries a UUCP lock file (what
    ser2net and the serial tools leave). The device itself is never opened
    here: an open raises DTR/RTS, and a neighbouring port drives a relay."""
    name = os.path.basename(tty)
    for lock_dir in ("/var/lock", "/run/lock"):
        if os.path.exists(os.path.join(lock_dir, "LCK.." + name)):
            return True
    try:
        p = subprocess.run(["fuser", tty], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=5)
        return p.returncode == 0
    except (OSError, subprocess.SubprocessError) as e:
        print(f"[esp32-detect] fuser unavailable ({e}): cannot see who holds {tty}")
        return False


def read_with_reset(tty, secs=6, baud=termios.B115200, until=None):
    """Hardware-reset the board (pulse RTS->EN) and read `secs` of serial (less
    once `until` has been read). Pure termios: only modem-control lines are
    toggled, no bytes are written. Returns decoded text ('' on error).
    Resetting an ESP32 just reboots it (harmless)."""
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

        return _read_fd(fd, secs, until)
    except (OSError, termios.error):
        return ""
    finally:
        try:
            os.close(fd)
        except OSError:
            pass


def _read_fd(fd, secs, until):
    out = bytearray()
    deadline = time.monotonic() + secs
    while time.monotonic() < deadline:
        r, _, _ = select.select([fd], [], [], 1.0)
        if r:
            try:
                out += os.read(fd, 4096)
            except OSError:
                pass
            if until and until.encode() in out:
                break
    return out.decode("latin-1")


def read_native(tty, secs, until=None, stop=None, sink=None):
    """Read a board without a reset pulse (opening only raises DTR+RTS
    together, which the auto-reset circuit ignores). A native-USB board
    (USB-Serial/JTAG) re-enumerates on EVERY reset (the flash's, a panic's, a
    watchdog's), which kills the open descriptor: reopen the port when it comes
    back and keep reading, so a crash loop shows up in the text instead of
    silently ending the capture. The firmware waits a few seconds at boot.
    Ends after `secs`, at `until`, or when the `stop` event is set; `sink`, a
    list, receives each decoded chunk as it arrives."""
    out = bytearray()
    deadline = time.monotonic() + secs
    while time.monotonic() < deadline and not (stop and stop.is_set()):
        while not os.path.exists(tty) and time.monotonic() < deadline:
            time.sleep(0.2)
        try:
            fd = os.open(tty, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        except OSError:
            time.sleep(0.2)
            continue
        try:
            while time.monotonic() < deadline and not (stop and stop.is_set()):
                r, _, _ = select.select([fd], [], [], 1.0)
                if r:
                    chunk = os.read(fd, 4096)
                    if not chunk:
                        break           # hung up: the board reset
                    out += chunk
                    if sink is not None:
                        sink.append(chunk.decode("latin-1"))
                    if until and until.encode() in out:
                        return out.decode("latin-1")
        except OSError:
            out += b"\n[host] serial port lost (board reset), reopening\n"
        finally:
            os.close(fd)
    return out.decode("latin-1")


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


def _save_cache(bypath, target="esp32"):
    """Pin a board's by-path: the classic ESP32 under "bypath" (as before),
    native-USB chips under "native"/<target>, without dropping the others."""
    cache = _load_cache()
    if target == "esp32":
        cache["bypath"] = bypath
    else:
        cache.setdefault("native", {})[target] = bypath
    try:
        os.makedirs(os.path.dirname(_CACHE_PATH), exist_ok=True)
        with open(_CACHE_PATH, "w") as f:
            json.dump(cache, f, indent=2)
    except OSError as e:
        print(f"[esp32-detect] cannot save {_CACHE_PATH}: {e}")


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
            pinned = os.path.realpath(bp)
            # A pinned port another process holds is not our board any more
            # (the neighbouring ports are other lab gear): say so instead of
            # flashing into it, and do not fall through to probing, which
            # would reset-pulse every free USB serial on the bench.
            if _is_busy(pinned):
                return None, (f"pinned port {pinned} ({bp}) is held by another "
                              f"process, so the ESP32 is not there any more: "
                              f"reconnect it, or set CC_ESP32_SERIAL=/dev/ttyUSBn "
                              f"(or delete the cached by-path) to point at it")
            return pinned, f"cache {bp}"

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
# The boards benchmarked, by ESP-IDF target. "esp32" is the classic board on a
# CP2102 bridge (detected by ROM banner, above); the others are native-USB
# chips (USB-Serial/JTAG, Espressif VID 303a), identified with esptool, which
# is safe there: an Espressif device is never one of the bench's other
# CP2102-driven consoles or relays.
BOARDS = {
    "esp32": {"arch": "xtensa-lx6", "cpu_model": "Espressif ESP32 (Xtensa LX6 dual-core)",
              "cpu_cores": 2, "cpu_mhz": 240.0, "ram_type": "SRAM ~520KiB (no DRAM)",
              "net_card": "ESP32 WiFi (802.11 b/g/n)", "compiler": "xtensa-esp32-elf-gcc",
              "console": "uart0", "native_usb": False},
    "esp32c6": {"arch": "riscv32", "cpu_model": "Espressif ESP32-C6 (RISC-V single-core HP)",
                "cpu_cores": 1, "cpu_mhz": 160.0, "ram_type": "SRAM 512KiB HP + 16KiB LP (no DRAM)",
                "net_card": "ESP32-C6 WiFi 6 (802.11ax)", "compiler": "riscv32-esp-elf-gcc",
                "console": "usb-serial-jtag", "native_usb": True},
}
NATIVE_USB_VID = "303a"


def board_label(target):
    """History/node label of a board: the classic ESP32 keeps "esp32"."""
    return target


def _esptool_chip(tty):
    """ESP-IDF target name of the native-USB chip on `tty` (e.g. "esp32c6"),
    or None. esptool resets it into its ROM loader and back."""
    rc, out = _run_idf(["esptool.py", "--port", tty, "chip_id"], timeout=60)
    found = re.search(r"Detecting chip type\.\.\. (ESP32[-\w]*)", out)
    if rc != 0 or not found:
        return None
    return found.group(1).lower().replace("-", "")


def detect_board(target, verbose=True):
    """(tty, how) of the board for `target`, or (None, reason)."""
    if target == "esp32":
        return detect_esp32_serial(verbose=verbose)
    override = os.environ.get("CC_" + target.upper() + "_SERIAL")
    if override:
        return (override, "override") if os.path.exists(override) else \
               (None, f"override {override} does not exist")
    bp = (_load_cache().get("native") or {}).get(target)
    if bp and os.path.exists(bp) and not _is_busy(os.path.realpath(bp)):
        vp = _usb_vidpid(bp)
        if vp is not None and vp[0] == NATIVE_USB_VID:
            return os.path.realpath(bp), f"cache {bp}"
    for tty in sorted(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")):
        vp = _usb_vidpid(tty)
        if vp is not None and vp[0] == NATIVE_USB_VID and not _is_busy(tty):
            chip = _esptool_chip(tty)
            if verbose:
                print(f"[esp32-detect] {tty}: {chip or 'no answer'}")
            if chip == target:
                _save_cache(bypath_of(tty), target)
                return tty, f"esptool: {chip}; pinned {bypath_of(tty)}"
    return None, f"no free native-USB {target} found"


def benchmark_nodes(verbose=True):
    """The ESP32 nodes of a benchmark run: one per board in BOARDS that passes
    the --node filter, with the tty it was found on. An absent board is kept,
    with tty None and its reason, so the run still prints its SKIP line."""
    nodes = []
    for target, board in BOARDS.items():
        label = board_label(target)
        if bh.node_allowed(label, board["arch"]):
            tty, how = detect_board(target, verbose=verbose)
            nodes.append({"label": label, "arch": board["arch"], "esp32": True,
                          "target": target, "tty": tty,
                          "skip_reason": None if tty else "board not detected: " + how})
    return nodes


def available_boards(verbose=True):
    """{target: tty} of every benchmarked board that is connected and free."""
    found = {}
    for target in BOARDS:
        tty, how = detect_board(target, verbose=verbose)
        if tty:
            found[target] = tty
        elif verbose:
            print(f"[esp32] {target}: {how}")
    return found


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
# max-players baked into the flash settings. The server PRE-ALLOCATES the whole
# ClientWithMapEventLoop pool at startup (EventLoopClientList ctor reserves
# max_players * sizeof(ClientWithMapEventLoop) ~= max_players*728 B CONTIGUOUS,
# plus per-client construction heap), so this is bounded by the board's ~290 KiB
# heap AFTER the flash-resident datapack is deserialized — NOT a soft cap. Too
# high -> operator new throws bad_alloc right after bind (the firmware's real RAM
# ceiling). MEASURED on a no-PSRAM ESP32-D0WD (~290 KiB heap, "test" maincode):
# 16 boots, 18 OOMs in ClientWithMap's ctor right after bind -> 16 is the boot
# ceiling (and it aligns with the fleet's 16-bot medium slice). Right-sizing lwIP
# (24 sockets, not 60) freed the heap that let it reach 16; over-sized lwIP capped
# it at ~14. Tunable for re-bisection on a different board/maincode; the value
# that boots IS the headline ceiling.
ESP32_MAXPLAYERS  = int(os.environ.get("CC_ESP32_MAXPLAYERS", "24"))
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
    """rsync the source datapack -> prefix staging. Heavy media (monster/tileset
    sprites, audio) is stripped because the ONLYBYMIRROR server only parses+hashes
    the tree, never serves images. EXCEPTION: skin/ PNGs are KEPT — the engine's
    skin enumerator (FacilityLibGeneral::skinIdList) only counts a skin folder
    that contains back/front/trainer.png, so stripping them yields an EMPTY skin
    list, which makes the forcedskin in player/start.xml unresolvable and the
    client can never create a character (it never reaches the map). The skin PNGs
    are tiny (~0.5 MiB) and are NOT baked into flash as pixels — only the skin
    NAMES go into datapack-cpp; the client downloads the actual pixels from the
    mirror, which is built from this same stage, so server-hash == mirror tree.
    Never writes the read-only source."""
    os.makedirs(STAGE_DATAPACK, exist_ok=True)
    args = ["rsync", "-a", "--delete",
            # Keep skin PNGs (skinIdList gate). --include must precede the broad
            # media --exclude; rsync applies the first matching rule.
            "--include=skin/", "--include=skin/**/",
            "--include=skin/**/*.png", "--include=skin/**/*.jpg",
            "--exclude=*.png", "--exclude=*.jpg", "--exclude=*.ogg",
            "--exclude=*.opus", "--exclude=*.wav", "--exclude=.git",
            "--exclude=.claude",
            DATAPACK_SRC.rstrip("/") + "/", STAGE_DATAPACK + "/"]
    p = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    return p.returncode == 0, p.stdout.decode(errors="replace")[-500:]


def stage1_build_dir(benchmark=False):
    return STAGE1_BUILD + ("-bench" if benchmark else "")


def datapack_cpp_dir(benchmark=False):
    """The emitted datapack-cpp of the production or the benchmark firmware:
    each must be emitted by a host server built like it (stage1_emit)."""
    return DATAPACK_CPP_DIR + ("-bench" if benchmark else "")


def stage1_emit(verbose=True, benchmark=False):
    """Build host server-cli (DATAPACK_CPP_EMIT) + run --save -> emit the
    human-readable datapack_cpp.{cpp,hpp} the firmware compiles into flash.
    Returns (ok, detail). Mirrors test/testingcompilationESP32.py stage1.
    `benchmark` emits for the CATCHCHALLENGER_BENCHMARK firmware: the host
    server must be built the same way, because what it serialises depends on
    it (the DDoS settings exist only with the filter, which the benchmark
    build drops) -- a mismatch makes the board misread its own flash data."""
    stage1_build = stage1_build_dir(benchmark)
    cpp_dir = datapack_cpp_dir(benchmark)
    def _say(m):
        if verbose:
            print("[esp32-stage1] " + m)
    ok, msg = stage_datapack_server()
    if not ok:
        return False, "datapack staging failed: " + msg
    os.makedirs(stage1_build, exist_ok=True)
    os.makedirs(cpp_dir, exist_ok=True)
    _say("cmake configure + build (EMIT + DB_INTERNAL_VARS + SELECT)")
    bh.drop_stale_cmake_cache(stage1_build, SERVER_CLI)
    cfg = ["cmake", "-S", SERVER_CLI, "-B", stage1_build, "-DCMAKE_BUILD_TYPE=Release",
           "-DCATCHCHALLENGER_SELECT=ON", "-DCATCHCHALLENGER_DB_INTERNAL_VARS=ON",
           "-DCATCHCHALLENGER_DATAPACK_CPP_EMIT=ON",
           "-DCATCHCHALLENGER_BENCHMARK=" + ("ON" if benchmark else "OFF")]
    p = subprocess.run(cfg, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    if p.returncode != 0:
        return False, "cmake configure failed:\n" + p.stdout.decode(errors="replace")[-1500:]
    p = subprocess.run(["cmake", "--build", stage1_build, "-j", str(os.cpu_count() or 1)],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=1800)
    if p.returncode != 0:
        return False, "build failed:\n" + p.stdout.decode(errors="replace")[-1500:]
    exe = os.path.join(stage1_build, "catchchallenger-server-cli")
    work = os.path.join(stage1_build, "save-run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    shutil.copy2(exe, os.path.join(work, "catchchallenger-server-cli"))
    shutil.copytree(STAGE_DATAPACK, os.path.join(work, "datapack"))
    with open(os.path.join(work, "server-properties.xml"), "w") as f:
        f.write('<?xml version="1.0"?>\n<configuration>\n'
                f'    <server-port value="{ESP32_SERVER_PORT}"/>\n'
                '    <server-ip value=""/>\n'
                '    <automatic_account_creation value="true"/>\n'
                # max-players (CC_ESP32_MAXPLAYERS, default 24 — the proven
                # stable ceiling on this no-PSRAM board). The server PRE-ALLOCATES
                # the full ClientWithMapEventLoop pool at startup, so this is
                # bounded by the board's heap after the datapack loads (too high
                # -> bad_alloc right after bind; ~34 is where booting fails, and
                # concurrent WiFi clients fit fewer still). Tunable for bisecting
                # the real RAM ceiling; the value that boots IS the headline.
                f'    <max-players value="{ESP32_MAXPLAYERS}"/>\n'
                f'    <broadcastName value="{ESP32_BROADCAST}"/>\n'
                # ONLYBYMIRROR (the ESP32 firmware profile) refuses to start with
                # an empty mirror: the fileless server can't send the datapack, so
                # clients must fetch it from this URL. The benchmark bot uses a
                # LOCAL datapack and never downloads, but the value must be
                # non-empty to pass settings validation.
                f'    <httpDatapackMirror value="{ESP32_MIRROR}"/>\n'
                # Compression MUST be off on the ESP32. The character-select packet
                # path (ClientHeavyLoadSelectCharFinal.cpp) allocates a
                # CATCHCHALLENGER_COMPRESSBUFFERSIZE (16 MiB) scratch via new[]
                # whenever compressionTypeServer != None. The ESP32 has only ~320 KiB
                # DRAM (largest free chunk ~111 KiB), so that allocation throws
                # bad_alloc → __cxa_allocate_exception → abort() the instant a client
                # selects its character (i.e. right at map entry). "none" bakes
                # CompressionType::None into flash so that branch is never taken; the
                # client speaks cleartext and reaches the map.
                '    <compression value="none"/>\n'
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
    shutil.copy2(cpp, os.path.join(cpp_dir, "datapack_cpp.cpp"))
    shutil.copy2(hpp, os.path.join(cpp_dir, "datapack_cpp.hpp"))
    return True, f"datapack_cpp.cpp ({os.path.getsize(cpp)//1024} KiB)"


def firmware_build_dir(target="esp32", low_power=False, benchmark=False):
    """One build dir per chip + variant; the plain ESP32 keeps build-fw."""
    if target == "esp32" and not low_power and not benchmark:
        return FW_BUILD
    suffix = ("-" + target if target != "esp32" or low_power or benchmark else "") \
             + ("-lowpower" if low_power else "") + ("-bench" if benchmark else "")
    return FW_BUILD + suffix


def write_overlay(config, target="esp32"):
    """Write the out-of-repo sdkconfig overlay (WiFi creds + static IP) with
    0600 perms. Secrets stay in the prefix, never in git. Each chip is its own
    board with its own address: STATIC_IP for the ESP32, <TARGET>_STATIC_IP
    (e.g. ESP32C6_STATIC_IP) for the others."""
    ip_key = "STATIC_IP" if target == "esp32" else target.upper() + "_STATIC_IP"
    need = ("WIFI_SSID", "WIFI_PASSWORD", ip_key, "STATIC_NETMASK", "STATIC_GATEWAY")
    for k in need:
        if k not in config:
            return False, f"cc-esp32.conf missing {k}"
    overlay = SDKCFG_OVERLAY if target == "esp32" else SDKCFG_OVERLAY + "-" + target
    old = os.umask(0o077)
    try:
        with open(overlay, "w") as f:
            f.write("# GENERATED out-of-repo overlay — WiFi creds + static IP. NEVER commit.\n")
            f.write('CONFIG_CATCHCHALLENGER_WIFI_SSID="%s"\n'      % config["WIFI_SSID"])
            f.write('CONFIG_CATCHCHALLENGER_WIFI_PASSWORD="%s"\n'  % config["WIFI_PASSWORD"])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_IP="%s"\n'      % config[ip_key])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_NETMASK="%s"\n' % config["STATIC_NETMASK"])
            f.write('CONFIG_CATCHCHALLENGER_STATIC_GATEWAY="%s"\n' % config["STATIC_GATEWAY"])
            # partitions.csv is right-sized (1.875 MiB factory) to fit a 2 MiB
            # flash, so the image is portable to cheap 2 MiB ESP32s as well as the
            # 4 MiB D0WD devkits here (a 2 MiB image flashes fine on a 4 MiB chip).
            f.write('CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y\n')
            f.write('CONFIG_ESPTOOLPY_FLASHSIZE="2MB"\n')
    finally:
        os.umask(old)
    return True, overlay


def build_firmware(config, verbose=True, target="esp32", low_power=False,
                   benchmark=False):
    """idf.py set-target <target> + build, with the out-of-repo overlay + the
    stage1 datapack-cpp. `low_power` adds sdkconfig.lowpower (CPU at 20 MHz
    while idle). `benchmark` builds the CATCHCHALLENGER_BENCHMARK image
    (event-loop probe on the console, no DDoS filter) in its own build dir.
    sdkconfig.defaults.<target> is picked up by ESP-IDF itself.
    Returns (ok, detail)."""
    ok, overlay = write_overlay(config, target)
    if not ok:
        return False, overlay
    defaults = [os.path.join(ESP_PROJECT, "sdkconfig.defaults")]
    if low_power:
        defaults.append(os.path.join(ESP_PROJECT, "sdkconfig.lowpower"))
    defaults.append(overlay)
    os.environ["SDKCONFIG_DEFAULTS"] = ";".join(defaults)
    build_dir = firmware_build_dir(target, low_power, benchmark)
    fw_bin = os.path.join(build_dir, os.path.basename(FW_BIN))
    shutil.rmtree(build_dir, ignore_errors=True)
    if verbose:
        print(f"[esp32-build] idf.py build ({target}{', low power' if low_power else ''})"
              " — single configure with all -D")
    # ALL -D at the first (only) configure: -DIDF_TARGET sets the chip and
    # -DCATCHCHALLENGER_DATAPACK_CPP_DIR must be cached before main/CMakeLists.txt
    # reads it (a separate set-target would cache the default dir first).
    # -DSDKCONFIG out-of-repo: idf.py otherwise writes the RESOLVED sdkconfig
    # (which contains the WiFi password merged from the overlay) into the project
    # dir = inside the git repo. Keep it in the build dir so no secret lands in
    # the tree (also satisfies "separate compilation artifacts").
    rc, out = _run_idf(["idf.py", "-C", ESP_PROJECT, "-B", build_dir,
                        "-DIDF_TARGET=" + target,
                        "-DSDKCONFIG=" + os.path.join(build_dir, "sdkconfig"),
                        "-DCATCHCHALLENGER_DATAPACK_CPP_DIR=" + datapack_cpp_dir(benchmark),
                        "-DCATCHCHALLENGER_BENCHMARK=" + ("ON" if benchmark else "OFF"), "build"],
                       timeout=1800)
    if rc != 0 or not os.path.isfile(fw_bin):
        return False, "build failed:\n" + out[-2000:]
    return True, f"{fw_bin} ({os.path.getsize(fw_bin)//1024} KiB)"


def built_target(build_dir):
    """CONFIG_IDF_TARGET of an ESP-IDF build dir, or None if never configured."""
    try:
        with open(os.path.join(build_dir, "sdkconfig")) as f:
            found = re.search(r'^CONFIG_IDF_TARGET="([^"]+)"', f.read(), re.M)
        return found.group(1) if found else None
    except OSError:
        return None


def flash(tty, verbose=True, target="esp32", low_power=False, benchmark=False):
    """idf.py -p <tty> flash of the server firmware build_firmware() made for
    the SAME target/variant. Refuses when that image is missing or was built
    for another chip, instead of pushing a stale or foreign one (the classic
    build-fw used to be flashed whatever was asked). Returns (ok, detail)."""
    build_dir = firmware_build_dir(target, low_power, benchmark)
    fw_bin = os.path.join(build_dir, os.path.basename(FW_BIN))
    if not os.path.isfile(fw_bin):
        return False, f"no firmware image in {build_dir}: build_firmware(target={target!r}) first"
    if built_target(build_dir) != target:
        return False, (f"{build_dir} was built for {built_target(build_dir)!r}, "
                       f"not {target!r}: refusing to flash it")
    if verbose:
        print(f"[esp32-flash] idf.py -p {tty} flash ({target}{', low power' if low_power else ''})")
    rc, out = _run_idf(["idf.py", "-C", ESP_PROJECT, "-B", build_dir,
                        "-DSDKCONFIG=" + os.path.join(build_dir, "sdkconfig"),
                        "-p", tty, "flash"], timeout=600)
    if rc != 0:
        return False, "flash failed:\n" + out[-2000:]
    return True, "flashed"


def read_serial_ip(tty, timeout=90, target="esp32"):
    """Reset the board and read serial until it reports its IP + binds. Returns
    (ip_or_None, bound_bool, text). A native-USB board was already reset by the
    flash and is only read."""
    if BOARDS[target]["native_usb"]:
        text = read_native(bypath_of(tty), min(timeout, 90), until="correctly bind")
    else:
        text = read_with_reset(tty, secs=min(timeout, 90), until="correctly bind")
    m = re.search(r"(?:DHCP got IP|static IPv4 configured:?)\s*(\d+\.\d+\.\d+\.\d+)", text)
    ip = m.group(1) if m else None
    bound = "correctly bind" in text
    return ip, bound, text


# --------------------------------------------------------------------------
# Generic ESP32 "benchmark firmware" build+flash+read pipeline. Used by the
# PURE-CPU on-device benchmarks (e.g. benchmarkmapmanager) whose firmware is a
# self-contained ESP-IDF project under benchmark/<name>/esp32/ that runs the
# harness and prints `BENCH ...` lines to UART. Unlike the SERVER firmware above
# (WiFi + datapack-cpp + stage1 emit), these need NO datapack, NO WiFi creds, NO
# overlay — just `idf.py set-target esp32 build` then flash + read serial. Kept
# here (not duplicated per benchmark) per benchmark/CLAUDE.md "Generate helper to
# put in common".

def build_benchmark_firmware(project_dir, build_dir, verbose=True, timeout=1800,
                             target="esp32"):
    """Build a self-contained ESP-IDF benchmark firmware (no WiFi/datapack).
    `project_dir` is benchmark/<name>/esp32/ (holds the top CMakeLists.txt);
    `build_dir` is an OUT-OF-TREE build dir under the prefix. Returns
    (ok, bin_path_or_detail). Self-skip upstream when toolchain_status() is
    False — this just runs idf.py.

    -DSDKCONFIG points the RESOLVED sdkconfig into the build dir, NOT the project
    dir: idf.py otherwise writes a 60 KiB generated `sdkconfig` next to the
    checked-in CMakeLists.txt (a compilation artifact in the source tree, which
    the root CLAUDE.md forbids). Keeping it in build_dir keeps the tree clean."""
    if verbose:
        print(f"[esp32-build] idf.py build ({target}) — {project_dir}")
    # A build dir configured for another chip cannot be retargeted in place.
    if built_target(build_dir) not in (None, target):
        return False, (f"{build_dir} is configured for {built_target(build_dir)!r}, "
                       f"not {target!r}: give each target its own build dir")
    # ESP-IDF reads sdkconfig.defaults(.<target>) only when the build dir has no
    # sdkconfig yet, so an edited default would silently never apply: drop the
    # generated one when a defaults file is newer, the build regenerates it.
    generated = os.path.join(build_dir, "sdkconfig")
    for defaults in (os.path.join(project_dir, "sdkconfig.defaults"),
                     os.path.join(project_dir, "sdkconfig.defaults." + target)):
        if os.path.isfile(generated) and os.path.isfile(defaults) \
                and os.path.getmtime(defaults) > os.path.getmtime(generated):
            print(f"[esp32-build] {defaults} changed: regenerating {generated}")
            os.remove(generated)
    rc, out = _run_idf(["idf.py", "-C", project_dir, "-B", build_dir,
                        "-DIDF_TARGET=" + target,
                        "-DSDKCONFIG=" + os.path.join(build_dir, "sdkconfig"),
                        "build"], timeout=timeout)
    if rc != 0:
        return False, "build failed:\n" + out[-2000:]
    # The .bin is named after the project() in the top CMakeLists.txt; find it.
    bins = glob.glob(os.path.join(build_dir, "*.bin"))
    # exclude the bootloader / partition-table images
    app = [b for b in bins if os.path.basename(b) not in
           ("bootloader.bin", "partition-table.bin", "partition_table.bin")]
    if not app:
        return False, "build reported success but no app .bin found in " + build_dir
    # pick the largest .bin (the app), defensively
    app.sort(key=lambda p: os.path.getsize(p), reverse=True)
    return True, app[0]


def flash_firmware(project_dir, build_dir, tty, verbose=True, timeout=600):
    """idf.py -p <tty> flash for a benchmark firmware built by
    build_benchmark_firmware. Returns (ok, detail). Same -DSDKCONFIG-in-build-dir
    rule as the build step so flashing doesn't regenerate a tree-side sdkconfig."""
    if verbose:
        print(f"[esp32-flash] idf.py -p {tty} flash — {project_dir}")
    rc, out = _run_idf(["idf.py", "-C", project_dir, "-B", build_dir,
                        "-DSDKCONFIG=" + os.path.join(build_dir, "sdkconfig"),
                        "-p", tty, "flash"], timeout=timeout)
    if rc != 0:
        return False, "flash failed:\n" + out[-2000:]
    return True, "flashed"


def benchmark_build_dir(build_dir, target):
    """One build dir per chip: the classic ESP32 keeps the historical name."""
    return build_dir if target == "esp32" else build_dir + "-" + target


def run_ondevice_benchmark(project_dir, build_dir, read_secs=30, serial=None,
                           verbose=True, target="esp32"):
    """Build -> flash -> reset -> read serial for an ON-DEVICE benchmark firmware.

    This is the whole device-side half of an on-device benchmark: an ESP-IDF
    image that runs the SAME harness the native fleet runs and prints the SAME
    `BENCH ...` lines to UART. Every benchmark*.py wanting on-device numbers
    calls this and then parses `text` with its own parse_bench_lines() -- the
    metric MAPPING is benchmark-specific, everything before it is not, so it
    lives here instead of being copied per benchmark (benchmark/CLAUDE.md:
    "put in common the code it will mostly be used by the majority").

    `build_dir` is the classic ESP32's; other targets get "<build_dir>-<target>".
    The read stops at the firmware's BENCH_END line (or after `read_secs`).

    Returns (status, detail, text):
      ("PASS", app_bin_path, serial_output)
      ("SKIP", reason, "")   -- toolchain absent, board absent, build or flash
                                failed

    NEVER raises and never returns FAIL: an unmeasured special target is
    'unknown', not a regression. The caller decides how loudly to report the
    reason (e.g. benchmark_helpers.print_node_error) and whether an empty
    parse is also a SKIP."""
    tc_ok, tc_detail = toolchain_status()
    if not tc_ok:
        return "SKIP", "esp-idf absent: " + tc_detail, ""
    # Detect the board BEFORE building so a missing board self-skips fast
    # instead of after a multi-minute compile.
    if serial:
        dev, how = serial, "override"
    else:
        dev, how = detect_board(target, verbose=verbose)
    if dev is None:
        return "SKIP", "board not detected: " + how, ""
    if verbose:
        print(f"[esp32] {target} serial {dev} ({how})")
    build_dir = benchmark_build_dir(build_dir, target)
    ok, detail = build_benchmark_firmware(project_dir, build_dir, verbose=verbose,
                                          target=target)
    if not ok:
        return "SKIP", "firmware build failed: " + detail, ""
    if verbose:
        try:
            size = os.path.getsize(detail)
        except OSError:
            size = -1
        print(f"[esp32] firmware built: {detail} ({size} bytes)")
    ok, fdetail = flash_firmware(project_dir, build_dir, dev, verbose=verbose)
    if not ok:
        return "SKIP", "flash failed: " + fdetail, ""
    # Reset + read; the firmware runs its sweep on boot and prints BENCH lines.
    # The board idles afterwards (no reboot), so over-reading is harmless while
    # under-reading would truncate the block.
    if verbose:
        print(f"[esp32] reading serial for up to {read_secs}s ...")
    if BOARDS[target]["native_usb"]:
        # flashing already hard-reset it; the firmware waits for this open
        return "PASS", detail, read_native(bypath_of(dev), read_secs, until="BENCH_END")
    return "PASS", detail, read_with_reset(dev, secs=read_secs, until="BENCH_END")


def esp32_platform_fields(target="esp32"):
    """Synthesized platform metadata for an ESP32 history JSON. The board has no
    shell, so history_recorder.PlatformRecord.collect() (which shells out) can't
    run -- fill the fields directly. Values are the fixed ESP32 spec; ram_total_mb
    is null (the schema is int MB; 520 KiB SRAM would record a misleading '1 MB'
    -- the precise figure lives in ram_type). Shared so every on-device ESP32
    benchmark reports identical platform fields (benchmark/CLAUDE.md: helper, not
    copy-paste)."""
    board = BOARDS[target]
    return {
        "arch":        board["arch"],
        "cpu_model":   board["cpu_model"],
        "cpu_cores":   board["cpu_cores"],
        "cpu_mhz":     board["cpu_mhz"],
        "cpu_flags":   None,
        "cpu_cache":   [],
        "ram_total_mb": None,
        "ram_type":    board["ram_type"],
        "disk_root":   None,
        "disk_kind":   "flash",
        "net_card":    board["net_card"],
        "net_link":    "serial",          # results read over the console
        "net_link_detail": board["console"],
        "wifi_ssid":   None,
        "wifi_standard": None,
        "wifi_band":   None,
        "wifi_link_mbps": None,
        "eth_link_mbps": None,
        "virt_kind":   "bare-metal",
        "virt_type":   "none",
        "arch_emulated": False,
        "host_arch":   None,
        "kernel":      None,              # ESP-IDF/FreeRTOS firmware, no OS kernel
        "libc":        "newlib",
        "compiler":    board["compiler"],
    }


def ensure_online(config=None, flash_if_needed=True, serial=None, verbose=True,
                  target="esp32", benchmark=False):
    """Make the ESP32 server reachable; return (host, detail) or (None, reason).
    Fast path: if host:port already answers, return it (no reflash). Otherwise,
    when the toolchain + creds are present, stage1 -> build -> flash -> wait, and
    return the static IP. Self-skips (None) when anything required is absent.
    `benchmark` puts the CATCHCHALLENGER_BENCHMARK image on the board (no DDoS
    filter, event-loop probe on the console) and always flashes it: from the
    network nothing tells which image a board that answers is running."""
    config = config or load_config()
    ip_key = "STATIC_IP" if target == "esp32" else target.upper() + "_STATIC_IP"
    host = config.get(ip_key) or (os.environ.get("CC_ESP32_HOST", "192.168.158.160")
                                  if target == "esp32" else None)
    if not host:
        return None, f"no address for the {target} board: add {ip_key} to {CONF_PATH}"
    # already up?
    if not benchmark:
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
        if target == "esp32":
            dev, how = detect_esp32_serial(override=config.get("SERIAL") or None, verbose=verbose)
        else:
            dev, how = detect_board(target, verbose=verbose)
        if dev is None:
            return None, "cannot flash: " + how
        if verbose:
            print(f"[esp32] target serial {dev} ({how})")
    ok, d = stage1_emit(verbose=verbose, benchmark=benchmark)
    if not ok:
        return None, "stage1 failed: " + d
    ok, d = build_firmware(config, verbose=verbose, target=target, benchmark=benchmark)
    if not ok:
        return None, "build failed: " + d
    ok, d = flash(dev, verbose=verbose, target=target, benchmark=benchmark)
    if not ok:
        return None, "flash failed: " + d
    if verbose:
        print("[esp32] flashed; reading serial for IP + bind ...")
    ip, bound, _txt = read_serial_ip(dev, timeout=90, target=target)
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
