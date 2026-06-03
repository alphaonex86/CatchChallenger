# Running the CatchChallenger server on MS-DOS / FreeDOS

`catchchallenger-server-cli` (the all-in-one game server) cross-compiles to a
real 32-bit MS-DOS program with the **DJGPP** GCC toolchain and the **Watt-32**
TCP/IP stack. The build is driven by `test/testingcompilationmsdos.py`; this
file explains how to actually *run* the produced `.exe` — under an emulator for
CI/dev, and on a real DOS machine.

> The binary is a DJGPP **go32 / COFF** executable: a small real-mode MZ stub
> jumps into 32-bit protected mode through a **DPMI host** (CWSDPMI.EXE). It is
> NOT a plain 16-bit `.exe` — `command.com`/dosbox alone cannot run it without
> a DPMI provider, and it needs a packet driver for networking.

The Release build is ~8 MiB on disk (stripped ~5 MiB). If you ever see a
~150 MiB `.exe`, it was built `Debug` (`-g`) by mistake — rebuild with
`-DCMAKE_BUILD_TYPE=Release` (see `testingcompilationmsdos.py`).

---

## 1. Why QEMU and not DOSBox

`testingcompilationmsdos.py` boots the server under **`qemu-system-i386`**, not
DOSBox. The reason is networking, not CPU:

* The server needs a **packet driver** (NE2000 class) so Watt-32 can do TCP.
* DOSBox's NE2000 emulation bridges to the host via **libpcap / a TAP device**,
  which needs `CAP_NET_ADMIN` (root). Our CI host runs unprivileged, so that
  path is unavailable.
* QEMU's **user-mode networking (slirp)** needs no privileges at all and can
  **forward a host TCP port straight into the guest** with `hostfwd`. The guest
  runs a normal NE2000 ISA card; slirp NATs it to the outside world and exposes
  the server's listener on `127.0.0.1` of the host.

DOSBox is fine for graphical DOS games; for a *headless TCP server you connect
to from the host*, QEMU+slirp is the simple, privilege-free choice.

---

## 2. What you need (the toolchain/runtime prefix)

Everything lives OUTSIDE the repo under `$CC_MSDOS_PREFIX`
(default `/mnt/data/perso/progs/catchchallenger-msdos`):

```
catchchallenger-msdos/
├── djgpp/           DJGPP cross GCC (i586-pc-msdosdjgpp-g++, binutils, strip)
├── watt32/          Watt-32 TCP/IP static lib (lib/libwatt.a)
├── cwsdpmi/bin/CWSDPMI.EXE    DPMI host (required to enter protected mode)
├── hostlib/         libfl.so.2 stub so binutils ar/ranlib run on the host
├── msdos-env.sh     sources the cross env (PATH, GCC_EXEC_PREFIX, DJDIR, …)
└── qemu/            runtime artifacts for the emulator boot:
    ├── fd-boot.img  FreeDOS 1.3 boot floppy (1.44 MB)
    ├── NE2000.COM   Crynwr NE2000 packet driver v11.4.3
    ├── DOSLFN.COM   long-filename TSR (DJGPP needs LFN: informations.xml, …)
    ├── WATTCP.CFG   Watt-32 network config (slirp: 10.0.2.15 / gw 10.0.2.2)
    └── README.txt   provenance of each artifact
```

Host tools needed for the emulator run: **`qemu-system-i386`** and **`mtools`**
(`mcopy`, `mformat`, `mpartition`) to build the FAT16 data disk. KVM
(`/dev/kvm`) is used when present (~70 s boot), otherwise plain TCG.

### Creating the prefix and the env (one-time)

The prefix is operator-built and kept out of the repo; the harness never
installs it. To recreate it from scratch:

```bash
export CC_MSDOS_PREFIX=/mnt/data/perso/progs/catchchallenger-msdos
mkdir -p "$CC_MSDOS_PREFIX"

# 1) DJGPP cross GCC (GCC 12.2, target i586-pc-msdosdjgpp). Easiest via the
#    prebuilt build-djgpp tarballs; unpack so you get $PREFIX/djgpp/bin/... and
#    $PREFIX/djgpp/setenv. (https://github.com/andrewwutw/build-djgpp)
#    -> $CC_MSDOS_PREFIX/djgpp/

# 2) Watt-32 TCP/IP, built WITH the DJGPP toolchain to get lib/libwatt.a:
#    git clone https://github.com/gvanem/Watt-32  (or watt-32.net release)
#    source $CC_MSDOS_PREFIX/djgpp/setenv ; export DJGPP_PREFIX=i586-pc-msdosdjgpp
#    cd Watt-32/src && make -f djgpp.mak           # produces ../lib/libwatt.a
#    -> $CC_MSDOS_PREFIX/watt32/ (inc/ + lib/libwatt.a)

# 3) CWSDPMI DPMI host (https://sandmann.dotster.com/cwsdpmi/):
#    -> $CC_MSDOS_PREFIX/cwsdpmi/bin/CWSDPMI.EXE

# 4) hostlib stub: DJGPP's binutils ar/ranlib link libfl (flex runtime); on a
#    host without it, drop a libfl.so.2 stub here and add it to LD_LIBRARY_PATH.
#    -> $CC_MSDOS_PREFIX/hostlib/libfl.so.2

# 5) QEMU runtime artifacts (see $CC_MSDOS_PREFIX/qemu/README.txt for sources):
#    FreeDOS 1.3 boot floppy, Crynwr NE2000.COM, DOSLFN.COM, WATTCP.CFG.
#    -> $CC_MSDOS_PREFIX/qemu/
```

zlib is built from source by the server build itself — no host package needed.

Then write the env helper once and `source` it before every build:

```bash
cat > "$CC_MSDOS_PREFIX/msdos-env.sh" <<'EOF'
# source this to get the CatchChallenger MS-DOS (DJGPP + Watt-32) cross env
source "/mnt/data/perso/progs/catchchallenger-msdos/djgpp/setenv"
export DJGPP_PREFIX=i586-pc-msdosdjgpp
export DJGPP_ROOT="/mnt/data/perso/progs/catchchallenger-msdos/djgpp"
export WATT_ROOT="/mnt/data/perso/progs/catchchallenger-msdos/watt32"
export LD_LIBRARY_PATH="/mnt/data/perso/progs/catchchallenger-msdos/hostlib:$LD_LIBRARY_PATH"
EOF
source "$CC_MSDOS_PREFIX/msdos-env.sh"
```

`djgpp/setenv` puts the cross compilers on `PATH` and sets `GCC_EXEC_PREFIX` /
`DJDIR` so `cc1plus` is found; `DJGPP_ROOT` / `WATT_ROOT` are what the CMake
toolchain file reads (it falls back to the standard prefix when they're unset).
The whole prefix is **read-only** at test time — `testingcompilationmsdos.py`
self-skips cleanly if it (or `i586-pc-msdosdjgpp-g++` / `libwatt.a`) is absent.

---

## 3. The easy path — let the test script do it

```bash
cd test
python3 testingcompilationmsdos.py
```

Phase 1 cross-compiles the server. Phase 2 (best effort, auto-skips if QEMU /
mtools / the `qemu/` artifacts are missing) builds the disks, boots the server
under QEMU, waits for `correctly bind:`, then points the native
`qtcpu800x600` client at it and checks it reaches the map. Skips cleanly (not a
failure) when the toolchain or QEMU runtime artifacts are absent.

The rest of this document is the **manual** recipe — useful for debugging the
DOS side by hand or for understanding what the script automates.

### Run it by hand with a visible console — `server.sh`

`$CC_MSDOS_PREFIX/server.sh` does everything in §4 for you (stage datapack →
build the disks → boot QEMU) but with a **console you can watch and type into**.

```bash
/mnt/data/perso/progs/catchchallenger-msdos/server.sh                 # curses console
/mnt/data/perso/progs/catchchallenger-msdos/server.sh --display gtk   # a window
/mnt/data/perso/progs/catchchallenger-msdos/server.sh --headless      # CI behaviour
```

`--display MODE` picks what you see / type into:

| mode | what you get |
|---|---|
| `curses` *(default)* | the DOS text screen **in your terminal** — no X needed, works over SSH |
| `gtk` / `sdl` | a graphical QEMU window (needs an X/Wayland `DISPLAY`) |
| `vnc[=:N]` | a VNC server on `127.0.0.1:590N` — attach any VNC viewer |
| `none` (`--headless`) | no console: logs to `server.log`, waits for `correctly bind:`, then idles |

In every non-headless mode the server runs **on the visible console**, and when
you **quit the server you drop to a `C:\>` DOS prompt** where you can type DOS
commands — `CCSRV` to restart it, `DIR` to look around, `FDAPM POWEROFF` to shut
the VM down (or, in a `gtk`/`sdl` window, just close it). Other flags: `--port`,
`--exe`, `--datapack`, `--disk-mb`, `--keep`; `server.sh --help` lists them all.

The rest of §4 is the same thing spelled out by hand.

---

## 4. Manual boot under QEMU

### 4a. Build the server

```bash
source /mnt/data/perso/progs/catchchallenger-msdos/msdos-env.sh   # cross env
PREFIX=/mnt/data/perso/progs/catchchallenger-msdos
SRC=../server/cli
BUILD=/tmp/cc-msdos

cmake -S "$SRC" -B "$BUILD" \
  -DCMAKE_TOOLCHAIN_FILE="$SRC/cmake/toolchain-djgpp-msdos.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCATCHCHALLENGER_SELECT=ON \
  -DCATCHCHALLENGER_DB_FILE=ON
cmake --build "$BUILD" -j"$(nproc)"

# optional: shrink for the data disk / real floppy
$PREFIX/djgpp/bin/i586-pc-msdosdjgpp-strip "$BUILD/catchchallenger-server-cli.exe"
```

### 4b. Make a FAT16 data disk (the guest's `C:`)

DOS needs a partition table to assign a drive letter, and VFAT long-filename
entries so DJGPP can open `informations.xml` & friends.

```bash
WORK=/tmp/cc-msdos/run; mkdir -p "$WORK"; cd "$WORK"
export MTOOLSRC="$WORK/mtools.conf"
printf 'drive c:\n  file="%s/data.img"\n  partition=1\n  mformat_only\n' "$WORK" > "$MTOOLSRC"

truncate -s 96M data.img            # server + datapack + file_db writes
mpartition -I c:                    # blank partition table
mpartition -c -T 0x06 c:            # one FAT16 (0x06) partition
mpartition -a c:                    # mark active
mformat -F c:                       # format FAT16

# 8.3 name for the server; the boot batch runs CCSRV.EXE
cp /tmp/cc-msdos/catchchallenger-server-cli.exe CCSRV.EXE

# minimal server-properties.xml
cat > server-properties.xml <<'EOF'
<?xml version="1.0"?>
<configuration>
    <server-port value="42000"/>
    <server-ip value=""/>
    <automatic_account_creation value="true"/>
</configuration>
EOF

# copy drivers + config + server to C:\
mcopy -o CCSRV.EXE "$PREFIX"/cwsdpmi/bin/CWSDPMI.EXE \
      "$PREFIX"/qemu/NE2000.COM "$PREFIX"/qemu/DOSLFN.COM \
      "$PREFIX"/qemu/WATTCP.CFG server-properties.xml c:
# copy the datapack tree (omit audio/docs/.git to save space)
mcopy -s -Q -o /path/to/datapack c:
```

### 4c. Make the boot floppy run the server

Copy the FreeDOS boot floppy and overwrite `FDAUTO.BAT` (its `FDCONFIG.SYS`
chains to `A:\FDAUTO.BAT`). The batch loads the LFN TSR, loads the NE2000
packet driver at software interrupt `0x60` (IRQ 3, I/O 0x300), runs the server
with stdout redirected to **COM1** (so the host can watch it via the serial
file), then powers off:

```bash
cp "$PREFIX"/qemu/fd-boot.img boot.img
cat > FDAUTO.BAT <<'EOF'
@echo off
SET PATH=A:\FREEDOS\BIN;C:\
SET WATTCP.CFG=C:\
SET LFN=Y
C:
DOSLFN.COM
NE2000.COM 0x60 3 0x300
CCSRV.EXE > COM1
A:\FREEDOS\BIN\FDAPM.COM poweroff
EOF
# inject it into the floppy image (CRLF line endings matter for DOS .BAT)
unix2dos FDAUTO.BAT 2>/dev/null || true
mcopy -i boot.img -o FDAUTO.BAT ::FDAUTO.BAT
```

`NE2000.COM 0x60 3 0x300` ⇒ packet-driver vector `INT 0x60`, IRQ `3`,
I/O base `0x300` — these must match the QEMU `ne2k_isa` device below.

**Interactive variant (watch + type commands).** The batch above is the
*headless* form: it pipes the server to `COM1` and powers off on exit. To run
the server **on the visible console** and land at a `C:\>` prompt when it
quits, drop the `> COM1` redirect and the `FDAPM … poweroff` line:

```bash
cat > FDAUTO.BAT <<'EOF'
@echo off
SET PATH=A:\FREEDOS\BIN;C:\
SET WATTCP.CFG=C:\
SET LFN=Y
C:
DOSLFN.COM
NE2000.COM 0x60 3 0x300
CCSRV.EXE
EOF
unix2dos FDAUTO.BAT 2>/dev/null || true
mcopy -i boot.img -o FDAUTO.BAT ::FDAUTO.BAT
```

When `CCSRV.EXE` exits, the batch ends and FreeDOS leaves you at `C:\>` — type
`CCSRV` to relaunch, `DIR`, etc. Pair this with a visible QEMU display (4d).

### 4d. Boot it

```bash
qemu-system-i386 \
  $( [ -e /dev/kvm ] && echo -accel kvm ) \
  -m 128 -boot a \
  -drive file=boot.img,format=raw,if=floppy \
  -drive file=data.img,format=raw,if=ide \
  -netdev user,id=n0,hostfwd=tcp::42000-:42000 \
  -device ne2k_isa,netdev=n0,iobase=0x300,irq=3 \
  -display none \
  -serial file:server.log \
  -no-reboot
```

* `-netdev user,…,hostfwd=tcp::42000-:42000` — slirp NAT; host
  `127.0.0.1:42000` → guest `:42000` (the Watt-32 listener).
* `-device ne2k_isa,iobase=0x300,irq=3` — the emulated NIC the packet driver
  binds to (same I/O/IRQ as `NE2000.COM`).
* `-serial file:server.log` — the server's stdout (it printed to `COM1`); watch
  it with `tail -f server.log`. Success line is `correctly bind:`.
* `-display none` / `-no-reboot` — headless; halt instead of rebooting on
  poweroff.

**Want to SEE it and type into it?** Swap `-display none` for a console
backend (and use the *interactive* `FDAUTO.BAT` from 4c so output goes to the
screen, not `COM1`):

| replace `-display none` with | result |
|---|---|
| `-display curses` | the DOS text screen **in your terminal** — no X, SSH-friendly |
| `-display gtk` (or `sdl`) | a graphical QEMU window (needs `DISPLAY`) |
| `-display none -vnc 127.0.0.1:0` | a VNC server on `127.0.0.1:5900` — attach a viewer |

Run QEMU in the **foreground** for these (drop the trailing `&` / don't poll a
log). Quit cleanly with `FDAPM POWEROFF` at the DOS prompt, by closing the
`gtk`/`sdl` window, or — in `curses` — via the QEMU monitor (`Alt+2`, type
`quit`). `server.sh` (above) wraps all of this; this is the by-hand form.

### 4e. Connect a client (from the host)

Once `server.log` shows `correctly bind:`:

```bash
QT_QPA_PLATFORM=offscreen \
client/.../catchchallenger \
  --host 127.0.0.1 --port 42000 \
  --autologin --character PlayerCPU --closewhenonmap
```

The client connects through the slirp port-forward exactly as if the DOS box
were a remote server, syncs the datapack, and renders the map.

---

## 5. Running on REAL hardware (MS-DOS 6.22 / FreeDOS 1.x)

The same `CCSRV.EXE` runs on a physical 486/Pentium-class PC. Three things must
be provided that QEMU faked for us: a **DPMI host**, a **packet driver for your
real NIC**, and a **correct WATTCP.CFG** for your real LAN.

1. **DPMI host (mandatory).** DJGPP programs need DPMI to enter 32-bit mode.
   * Plain MS-DOS 6.22 / FreeDOS at a bare prompt: copy **`CWSDPMI.EXE`** next
     to `CCSRV.EXE` (or anywhere on `PATH`). go32 auto-loads it.
   * If you boot with EMM386/JEMM386 providing DPMI, or run inside a Windows
     9x DOS box, CWSDPMI is not needed (a DPMI server is already present).

2. **XMS / extended memory.** Load an XMS manager so go32 can use RAM above
   1 MB. In `CONFIG.SYS`: `DEVICE=C:\DOS\HIMEM.SYS` (MS-DOS) or
   `DEVICE=C:\FDOS\BIN\HIMEMX.EXE` (FreeDOS). A few MB of RAM is enough for a
   small player count; the engine itself targets i486-class hardware.

3. **Packet driver for your NIC (mandatory for networking).** Watt-32 talks to
   a Crynwr-style **packet driver**, not the card directly.
   * NE2000-compatible ISA card → the bundled `NE2000.COM <vector> <irq> <io>`,
     e.g. `NE2000.COM 0x60 3 0x300` (match your card's jumpers/PnP settings).
   * Any other card → get that model's packet driver (Crynwr collection or the
     vendor's `.COM`). Real Ethernet/Wi-Fi NICs each need their own.
   * Load the driver **before** the server; note the software-interrupt vector
     you gave it (commonly `0x60`).

4. **WATTCP.CFG for your LAN.** Edit it for your real network and point
   `SET WATTCP.CFG=` at its directory (or keep it beside `CCSRV.EXE`):
   ```
   my_ip      = dhcp                 # or a static address, e.g. 192.168.1.50
   netmask    = 255.255.255.0        # ignored when my_ip = dhcp
   gateway    = 192.168.1.1
   nameserver = 192.168.1.1
   pkt.vector = 0x00                 # 0 = auto-scan 0x60..0x80 for the driver
   ```
   `my_ip = dhcp` makes Watt-32 ask your router for an address — easiest on a
   real LAN. Clients then connect to that machine's IP on port 42000.

5. **Long filenames.** The datapack uses names longer than 8.3
   (`informations.xml`, …). Load an LFN TSR (`DOSLFN.COM`) and `SET LFN=Y`
   before running the server, or DJGPP will fail to open those files.

6. **Copy the payload.** Put on the DOS disk: `CCSRV.EXE`, `CWSDPMI.EXE`, your
   packet driver, `DOSLFN.COM`, `WATTCP.CFG`, `server-properties.xml`, and the
   whole `datapack\` tree. Leave room for `file_db` writes.

Typical real-hardware `AUTOEXEC.BAT` tail:
```
SET WATTCP.CFG=C:\CC
SET LFN=Y
C:
CD \CC
DOSLFN.COM
NE2000.COM 0x60 3 0x300
CCSRV.EXE
```

---

## 6. MS-DOS / FreeDOS quick help

For anyone who hasn't driven a DOS prompt recently.

### Booting FreeDOS
* From the install CD/USB choose **"Boot from System Harddisk"** to reach a
  bare `C:\>` once installed, or boot the **floppy/live** image to get a prompt
  without installing.
* FreeDOS config files are `FDCONFIG.SYS` (= `CONFIG.SYS`) and `FDAUTO.BAT`
  (= `AUTOEXEC.BAT`). MS-DOS uses `CONFIG.SYS` + `AUTOEXEC.BAT`.

### Everyday commands (both DOSes)
| Task | Command |
|---|---|
| List files | `DIR`  (`DIR /P` page, `DIR /W` wide) |
| Change dir | `CD \PATH`  ·  up one: `CD ..` |
| Switch drive | `C:` , `A:` , `D:` … (letter + colon) |
| Make / remove dir | `MD name` / `RD name` |
| Copy / move / delete | `COPY a b` · `MOVE a b` · `DEL file` |
| Rename | `REN old new` |
| Show a text file | `TYPE file.txt` (`| MORE` to page) |
| Edit a text file | `EDIT file.txt` (full-screen editor; Alt opens menus) |
| Clear screen | `CLS` |
| Set env var | `SET NAME=value` (view all: `SET`) |
| Memory map | `MEM /C` (MS-DOS) · `MEM` (FreeDOS) |
| Reboot / power off | `Ctrl+Alt+Del` · FreeDOS: `FDAPM poweroff` |

### Disk setup (only when preparing a fresh disk)
* `FDISK` — create/activate a partition (reboot after). FreeDOS: `FDISK /?`.
* `FORMAT C: /S` — format and copy the system files so the disk boots.
* `SYS C:` — make an already-formatted disk bootable.

### Notes & gotchas
* Filenames are **8.3** by default (`NAME.EXT`, ≤8 + ≤3). Long names need an
  LFN driver (`DOSLFN.COM`) — already required for the datapack here.
* Paths use **backslashes** (`C:\CC\datapack`).
* `.BAT` files must have **CRLF** line endings (`unix2dos`) or DOS misreads them.
* Drivers (`DEVICE=…`) load from `CONFIG.SYS`/`FDCONFIG.SYS`; TSRs and programs
  run from `AUTOEXEC.BAT`/`FDAUTO.BAT`.
* `Ctrl+C` / `Ctrl+Break` interrupts a running program.

### When networking doesn't come up
* `NE2000.COM` prints an error → wrong IRQ/IO; match the card (or the QEMU
  `ne2k_isa` device). `0x60 3 0x300` are vector/IRQ/IObase respectively.
* Server starts but no client can connect → check `WATTCP.CFG` (`my_ip`,
  `gateway`), that the packet driver loaded (it prints its vector), and that
  `server-properties.xml` `server-port` matches the client's `--port`.
* Under QEMU, connect to **host** `127.0.0.1:42000` (the `hostfwd`), not to the
  guest's `10.0.2.15`.
* The bind success message to look for is `correctly bind:`.

---

## 7. References
* Build/run automation: `test/testingcompilationmsdos.py`
* CMake toolchain: `server/cli/cmake/toolchain-djgpp-msdos.cmake`
* Runtime artifact provenance: `$CC_MSDOS_PREFIX/qemu/README.txt`
* DJGPP: <https://www.delorie.com/djgpp/> · Watt-32: <https://www.watt-32.net/>
* CWSDPMI: <https://sandmann.dotster.com/cwsdpmi/> · FreeDOS: <https://www.freedos.org/>
