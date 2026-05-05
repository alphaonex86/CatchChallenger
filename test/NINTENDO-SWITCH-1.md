# Porting CatchChallenger qtopengl client to Nintendo Switch (1st gen, NX)

## TL;DR

**Officially: no.** A signed, store-installable build of the qtopengl client
on a retail Switch is not possible without becoming a licensed Nintendo
developer (NDA-gated, hardware-locked SDK, signing keys held by Nintendo).
Qt's pre-built Switch port exists but is restricted to NDA holders.

**Unofficially (homebrew, "package ready to install" via Atmosphère CFW):**
yes, with a substantial port effort and several Qt sub-modules either
swapped out or stubbed because the homebrew toolchain (devkitPro / libnx /
mesa-nx) does not provide the same surface as a Linux distribution. You
end up with an `.nro` (Homebrew Loader) or `.nsp` (forwarder) that runs
on jailbroken consoles only — not signed by Nintendo, won't install on
stock firmware.

This document covers the homebrew route end-to-end, calls out what
**must change in the codebase**, and lists what is *not feasible at all*
(e.g. true Authenticode/Nintendo-signed retail packages without joining
Nintendo's developer program).

---

## 1. Two paths, pick one

| Path | Requirements | Output | Runs on retail? |
|------|--------------|--------|-----------------|
| **Official (NDA)** | Approved Nintendo dev account, NX-SDK, Qt-for-NX licence, devkit | `.nsp` signed by Nintendo's CA, eShop-distributable | Yes |
| **Homebrew (devkitPro)** | devkitPro pacman + libnx + Atmosphère CFW on the console | `.nro` for the hbmenu, optionally an `.nsp` forwarder | Only on jailbroken units; **never** on stock firmware |

Everything below is the homebrew path.

---

## 2. What's actually portable

The qtopengl client uses these Qt 6 modules (from `client/qtopengl/CMakeLists.txt`
and the `find_package(Qt6 COMPONENTS …)` calls):

* **Qt Core / Gui / Widgets** — portable in principle; Qt has a Switch
  platform plugin in the upstream tree but only NDA-distributed binaries
  exist. The community port at <https://github.com/Switch-Qt/qt> targets
  Qt 5; Qt 6 has no public Switch port at the time of writing.
* **Qt OpenGL / OpenGLWidgets** — Switch GL is OpenGL ES 3.2 via NVIDIA's
  closed driver, exposed to homebrew through libnx + mesa-nx (Nouveau /
  Tegra X1 Maxwell). Qt's `QOpenGLWidget` works on GLES with minor shader
  language tweaks. **Vulkan via mesa-nx is the better target on Switch**
  but qtopengl is GL-only — see §5.
* **Qt Network** — TCP/UDP via `bsd:s`/`bsd:u` libnx services. WebSockets
  add a TLS dependency (mbedtls is shipped by libnx).
* **Qt Multimedia (Opus playback)** — there is no Qt Multimedia backend
  for libnx's `audrv`. **Must be replaced** with a libnx audrv driver +
  the existing libopusfile decoder you already vendor in
  `client/libqtcatchchallenger/libopusfile/`. Or build with
  `CATCHCHALLENGER_NOAUDIO` (already a supported build flag) and skip it.
* **Qt SQL (SQLite for solo savegames)** — Qt's `QSQLITE` driver compiles
  cleanly against libnx if you bring the SQLite amalgamation. No system
  install of SQLite is needed; ship the amalgamation as part of the
  `.nro`.
* **Qt XML** — pure user-space, ports cleanly.
* **Qt Multimedia / WebSockets** — gate behind existing
  `CATCHCHALLENGER_NOAUDIO` / `CATCHCHALLENGER_NO_WEBSOCKET` flags for a
  v1 port; iterate later.

Vendored deps (all already in-tree, all portable):
`general/blake3`, `general/hps`, `general/libxxhash`, `general/libzstd`,
`client/libqtcatchchallenger/libtiled`, `libogg`, `libopus`, `libopusfile`.

---

## 3. Toolchain setup (host: Linux)

```bash
# devkitPro pacman bootstrap
wget https://apt.devkitpro.org/install-devkitpro-pacman
sudo bash install-devkitpro-pacman
# Switch (NX) package group
sudo dkp-pacman -S switch-dev libnx switch-cmake switch-mesa \
    switch-libdrm_nouveau switch-glm switch-zlib switch-mbedtls \
    switch-libogg switch-libopus switch-opusfile
```

* `switch-dev` provides `aarch64-none-elf-{gcc,g++,as,ld}` plus
  `nx-hbmenu`, `elf2nro`, `nacptool`, `build_pfs0`.
* `switch-cmake` ships a CMake toolchain file at
  `${DEVKITPRO}/cmake/Switch.cmake` that wires the cross compiler,
  sysroot, and linker scripts.
* **Qt**: there is **no `switch-qt6` upstream package**. You compile Qt
  yourself against the devkitPro sysroot or pull a community Qt 5 port
  and back-port the qtopengl client (one-liner Qt5/Qt6 compatibility is
  not the case for this codebase — `general/base/GeneralStructures.hpp`
  already uses Qt 6 idioms).

## 4. Building Qt 6 for Switch (out-of-band, multi-day task)

This is the biggest non-trivial piece of work. Qt's official build
system understands Switch only in the NDA branch. To replicate publicly:

1. Take Qt 6.x sources (matching Linux build version — currently 6.5.x
   per CLAUDE.md macOS osxcross note).
2. Apply community Switch patches from <https://github.com/Switch-Qt/qt>
   (Qt 5) and forward-port to Qt 6 — this is the largest engineering
   risk.
3. Cross-build the modules listed in §2 using
   `${DEVKITPRO}/cmake/Switch.cmake` + a custom `qmake.conf` /
   `qt6/cmake/QtBuild.cmake` patch declaring the `nx` platform.
4. Stub or disable Qt Multimedia and Qt WebSockets for v1.
5. Install Qt under `${DEVKITPRO}/portlibs/switch/qt6/`.

Expected effort: **2–4 person-weeks** for a working subset, plus the
`QPlatform*Integration` glue (`QSurfaceFormat`, `QPlatformWindow`,
`QPlatformBackingStore`) targeting libnx's framebuffer / `nvgfx`.

## 5. Renderer choice: stay on GL, or move to Vulkan?

The Switch's NVIDIA TX1 maps best to Vulkan. mesa-nx exposes `nvk`
(community Nouveau Vulkan driver) plus a GLES 3.2 frontend on top.

* Staying on **OpenGL** is the smallest delta to the existing client.
  Performance is acceptable for a 2D tile map renderer; GLES 3.2 is a
  superset of what `MapVisualiser` uses today.
* Moving to **Vulkan** via `QRhi` + `QtQuick`/`QOpenGLWidget`
  abstraction is a much larger refactor and is forbidden by your
  in-tree rule (`CLAUDE.md`: "forbid the Qt `quick` module"). Skip it.

## 6. Code changes you'll need (in this repo)

### 6.1 Build-system changes (CMakeLists.txt)

Add a Switch toolchain branch alongside the existing Linux/Windows/macOS
paths. The pattern follows `testingcompilationwindows.py` / MXE:

```cmake
# CMakeLists.txt (top level, near the find_package(Qt6 …) block)
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(CC_PLATFORM_NX TRUE)
    add_compile_definitions(CATCHCHALLENGER_NX
                            CATCHCHALLENGER_NOAUDIO
                            CATCHCHALLENGER_NO_WEBSOCKET)
    # libnx ships its own libc (newlib); disable a couple of POSIX
    # syscalls the codebase assumes always succeed.
    add_compile_definitions(CATCHCHALLENGER_NO_FORK
                            CATCHCHALLENGER_NO_MMAP_HUGE)
endif()
```

A new test driver in this directory would mirror
`testingcompilationwindows.py`:
`testingcompilationswitch.py` — probe `${DEVKITPRO}`, run cmake with
`-DCMAKE_TOOLCHAIN_FILE=${DEVKITPRO}/cmake/Switch.cmake`, build, then
package via `nacptool` + `elf2nro`. Self-skip when `${DEVKITPRO}` /
`switch-qt6` is missing — same self-skip pattern Windows / Mac use.

### 6.2 Networking

`server/epoll/EpollClient.hpp` is server-side and not built on Switch.
Client-side `Api_protocol` uses `QTcpSocket` only — no change needed.

### 6.3 Filesystem layout on the cartridge

The client expects the datapack at `<cwd>/datapack/internal/`. On the
Switch, the `.nro`'s `argv[0]` directory is `sdmc:/switch/<rom>/` when
launched from hbmenu. Stage the datapack there:

```
sdmc:/switch/catchchallenger/
├── catchchallenger.nro
├── catchchallenger.nacp
├── icon.jpg
└── datapack/
    └── internal/
        ├── ...
```

Solo savegames need `XDG_DATA_HOME` redirection to
`sdmc:/switch/catchchallenger/saves/`. Wire a one-time
`qputenv("XDG_DATA_HOME", …)` at process start in
`client/qtopengl/main.cpp` when `CATCHCHALLENGER_NX` is defined.

### 6.4 Audio

Either:
* compile with `CATCHCHALLENGER_NOAUDIO` (zero work, ships silent), or
* write a libnx audrv backend in `client/libqtcatchchallenger/audio/`
  that pumps decoded Opus PCM via `audrvVoiceStart`. Roughly 200 LoC of
  new code modelled on the existing `AudioGL` class.

### 6.5 Input

libnx surfaces controllers via `padInitializeAny` / `padUpdate`. Map:

| Switch input | qtopengl action |
|--------------|-----------------|
| LStick / DPad | movement |
| A | confirm / talk |
| B | cancel / back |
| X | inventory |
| Y | menu |
| L / R | switch tabs |
| Plus | pause / quit |
| Touch screen | direct map taps (already supported via `QTouchEvent`) |

Implement in a small `NxInput` class that posts synthesised
`QKeyEvent` / `QMouseEvent` to the focused widget. ~150 LoC.

### 6.6 Strict-aliasing / printf format warnings

devkitPro's gcc 13.x is strict on `-Werror=format`. Audit the
`%lu` / `%zu` / `printf("%llx", uint64_t)` calls in `general/base/`.
You're already on `-Wall` per `CMakeLists.txt`; switch warnings to
non-fatal for the Switch build only:

```cmake
if(CC_PLATFORM_NX)
    add_compile_options(-Wno-error=format -Wno-error=cast-align)
endif()
```

---

## 7. Packaging: producing a hbmenu-installable `.nro`

```bash
# After a successful cmake --build, you have:
#   build-switch/client/qtopengl/catchchallenger.elf

# 1) Strip + convert to NRO
${DEVKITPRO}/tools/bin/nacptool --create \
    "CatchChallenger" "CatchChallenger Team" "0.1.0" \
    catchchallenger.nacp

${DEVKITPRO}/tools/bin/elf2nro \
    catchchallenger.elf catchchallenger.nro \
    --icon=icon.jpg --nacp=catchchallenger.nacp \
    --romfsdir=romfs

# 2) Stage the datapack alongside it under sdmc:/switch/catchchallenger/
rsync -a /home/user/Desktop/CatchChallenger/CatchChallenger-datapack/ \
    output/switch/catchchallenger/datapack/internal/
mv catchchallenger.nro output/switch/catchchallenger/

# 3) Optional: zip for distribution to homebrew users
cd output && zip -r catchchallenger-switch-0.1.0.zip switch/
```

End-user install:
1. Copy the `switch/catchchallenger/` folder to the SD card under
   `/switch/`.
2. Boot the console into Atmosphère CFW.
3. Open hbmenu, scroll to CatchChallenger, press A.

---

## 8. Signing & forwarders ("show as installed game on the home menu")

A homebrew `.nro` is **not signed** by Nintendo and **cannot be
installed** as a regular tile. Two community workarounds exist:

### 8.1 Forwarder NSP (Atmosphère only)

Tools like `nro2nsp` / `forwarder-creator` build a small `.nsp` whose
only job is to chain-launch the `.nro` from `sdmc:/switch/...`. The
result appears on the home menu as a tile. Steps:

```bash
# Requires: hacBrewPack from https://github.com/The-4n/hacBrewPack,
# and a copy of "prod.keys" + "title.keys" dumped from the user's
# console (per-console — never redistributable).
hacBrewPack \
    --titleid 0100AAAACCCCDDDD \
    --titlename "CatchChallenger" \
    --titleauthor "CatchChallenger Team" \
    --titleversion 0 \
    --keyset prod.keys \
    --nspdir output \
    --nrodir nro_root
```

The output `.nsp` is **signed with the user's own console keys** —
specific to that one console. There is no path to a universally-signed
homebrew `.nsp`. Distributing a single signed `.nsp` to other users is
not feasible without committing key piracy.

### 8.2 Atmosphère emuMMC homebrew sigpatched mode

If the user's CFW boots in "patches mode" (sigpatches from the
TegraExplorer / sys-patch toolchain), unsigned `.nsp`s install fine.
You ship the unsigned `.nsp` and rely on the user's CFW to accept it.
Same caveat: the `.nsp` only ever installs on jailbroken consoles.

### 8.3 Authenticated retail signing — not feasible

Producing a `.nsp` signed by Nintendo's CA chain requires a Nintendo
NX devkit + signed RSA-PSS keys held by Nintendo's key-management
server. There is **no analogue to osslsigncode for the Switch**.

If the user actually wants this: apply to
<https://developer.nintendo.com/>, sign the NDA, get hardware. The
rest of this document becomes obsolete (the official path uses
internal Nintendo tooling and Qt's NDA-licensed Switch port).

---

## 9. Testing

A `testingcompilationswitch.py` mirroring the Windows / Mac patterns
gives the same self-skip-when-toolchain-absent behaviour. The test can:

1. Probe `${DEVKITPRO}` and `${DEVKITPRO}/portlibs/switch/qt6/`.
2. Run cmake with the Switch toolchain.
3. Build qtopengl.
4. Run `elf2nro` / `nacptool` / forwarder-build.
5. **Skip the runtime phase entirely** — the `.nro` cannot be executed
   on Linux; you'd need `Ryujinx` or `yuzu`/`suyu` running headless,
   and emulator startup + datapack mount + GL probe is far too flaky
   for CI. State this in the doc and the test:
   "Compile + package only — no runtime smoke run on Switch."

---

## 10. Effort estimate

| Phase | Effort | Risk |
|-------|--------|------|
| Toolchain setup (devkitPro + sysroot) | 1 day | low |
| Qt 6 cross-build for Switch | 2–4 weeks | high (no public port) |
| Codebase changes (cmake guards, audio stub, input, paths) | 3–5 days | medium |
| Packaging script + forwarder NSP build | 2 days | low |
| testingcompilationswitch.py | 1 day | low |
| QA on real hardware | 1+ week | medium (homebrew quirks) |
| **Total** | **~5–7 weeks** to a hbmenu-runnable `.nro` | |

Subtract Qt cross-build effort (the dominant cost) only by recruiting a
contributor from the Switch-Qt community.

---

## 11. Licensing cost (official path)

Public, operator-reported figures for the Nintendo Developer Portal as of
late 2025. Exact pricing is NDA-gated; treat the numbers below as ±50%
planning estimates and get a real quote from
<https://developer.nintendo.com/> once your account is approved.

| Item | Cost | Notes |
|------|------|-------|
| Nintendo Developer Portal account | **Free** | Approval 1–4 weeks; individuals OR companies accepted |
| Switch DevKit (SDEV) | **~$450–500** one-time | Sold at cost after approval; indie devs typically get one |
| Switch TestKit (EDEV) | **~$450** one-time | Optional but standard for QA |
| Lotcheck (per-title cert) | **~$2,000** per submission | Re-submissions free for first round, then paid |
| Engine licence (Qt) | **N/A publicly** | Qt has no public Switch product. Either license an NDA-only port from The Qt Company (case-by-case quote) or absorb the community-port effort yourself (see §4) |
| Annual / royalty | **0% annual; ~30% per-title** | Standard eShop revenue split |

**Realistic floor to ship one indie game on Switch via the official
path:** ~**$3,000** hardware + lotcheck. The dominant cost is *not*
licensing — it's the Qt-on-Switch port engineering effort (§4): without
a public Qt 6 Switch port, that's 2–4 person-weeks of someone who knows
Qt's `QPlatform*Integration` internals. Recruit a contributor from the
Switch-Qt community to compress that line item.

This is the cheapest "ship to a console" license currently available;
the Switch's indie-friendly pricing is the reason this `.md` exists at
all and the reason the `PLAYSTATION-5.md` recommendation explicitly
deprioritises PS5 in favour of Switch + Steam Deck.

## 12. Recommendation

1. **Don't pursue store-distributed retail Switch port** unless you join
   Nintendo's developer programme. Outside that, "signed by Nintendo"
   is unreachable.
2. For the homebrew route, gate the work behind whether the upstream
   Switch-Qt community ships a working Qt 6 port. If yes, the effort
   collapses to ~1 person-week. If no (current state), the Qt port
   itself is the project and the CatchChallenger code changes are the
   smallest line item.
3. Add `testingcompilationswitch.py` *only after* a `${DEVKITPRO}`
   workspace exists on the test box — same self-skip pattern Windows /
   Mac use, so it's invisible until the toolchain is staged.
