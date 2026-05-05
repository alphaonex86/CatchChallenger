# Porting CatchChallenger qtopengl client to PlayStation 5

## TL;DR

**Officially: practical only as a licensed Sony developer.** A PSN-distributable
build (`.pkg` signed by Sony's CA, runs on every retail console) requires
joining the PartnerNet developer programme: NDA-gated, hardware-locked
DevKit / TestKit, Sony-issued signing keys, and explicit certification.
Outside that programme, you cannot produce a "package ready to install"
on a stock retail console.

**Unofficially (homebrew): much narrower than on Switch.** PS5 jailbreaks
exist for a small range of firmware versions (≤ 4.51 via the
WebKit / IPV6 / Mast1c0re chains, ≤ 5.50 via the Byepervisor / FontFace
exploits) — single-shot ROP chains, not a persistent CFW. A jailbroken
PS5 can run unsigned ELFs via `etaHEN` / `goldhen-ps5`, but:

* Each boot needs the exploit chain re-run.
* The userland sandbox is more restrictive than Switch homebrew —
  no direct GPU access path that's documented and stable.
* No upstream Qt port exists, public OR private — there is no
  community equivalent of Switch-Qt. Apart from a few homebrew
  emulators (RPCS5 forks, ScummVM stubs), application-class GUI code
  has not been ported.
* Any artefact you build is **not signed by Sony** and **never installs
  on stock firmware**. There is no tile-on-home-menu story analogous
  to Switch's forwarder NSPs.

This document covers the homebrew route to the extent it's possible and
the official route at a high level. Realistic recommendation up front:
**this port is not currently feasible** without joining Sony's developer
programme. Read on for the full rationale.

---

## 1. Two paths, neither comparable to the Switch case

| Path | Requirements | Output | Runs on retail? |
|------|--------------|--------|-----------------|
| **Official (SIE PartnerNet)** | Approved Sony developer account, DevKit / TestKit hardware, ProDG toolchain, Sony-issued signing keys, certification (TRC) | `.pkg` signed by Sony's CA, PSN-distributable | Yes |
| **Homebrew (etaHEN / goldhen-ps5)** | One of the WebKit / Mast1c0re / Byepervisor exploit chains for the operator's specific firmware version (mostly stuck ≤ 4.51 / 5.50) | Unsigned ELF or `.pkg` loaded by a homebrew loader | No (only on the same firmware that the exploit targets, with the chain re-run on every boot) |

The Switch homebrew story (signed forwarders via console-keys,
Atmosphère CFW persistent across boots, mature `.nro` toolchain) **does
not have a PS5 analogue**. The closest is the OG-PS4 scene which runs
fake `.pkg` files via `goldhen` — that pipeline does not yet exist for
PS5 in the same form.

---

## 2. What the qtopengl client uses today

From `client/qtopengl/CMakeLists.txt` and the `find_package(Qt6 COMPONENTS …)`
calls:

* **Qt Core / Gui / Widgets** — pure user-space, would port if a Qt
  port existed; **none does**, public or private.
* **Qt OpenGL / OpenGLWidgets** — PS5 GPU is RDNA 2 (AMD); Sony's
  graphics API is **GNM / GNMX** (low-level, NDA only) plus a higher-
  level wrapper (PSSL shaders). Mesa-PS5 / Vulkan-on-PS5 do not exist
  publicly. There is no GLES bring-up community equivalent to
  Switch's mesa-nx.
* **Qt Network** — TCP/UDP via libkernel sockets; doable in homebrew
  loaders that expose `socket()` shims.
* **Qt Multimedia (Opus)** — no ndspu / Audio3D backend exists in any
  public Qt port. Replace with the vendored `libopusfile` + a
  thin libsce-aut wrapper, or build with the existing
  `CATCHCHALLENGER_NOAUDIO` flag.
* **Qt SQL (SQLite for solo savegames)** — would port if you bring
  the SQLite amalgamation; no system SQLite on PS5.
* **Qt XML** — pure user-space, fine.

---

## 3. Official path (SIE / PartnerNet)

**This is the only path that produces a Sony-signed retail-installable
`.pkg`.** Steps Sony documents only behind NDA; outline:

1. Apply at <https://partners.playstation.net/> — corporate / studio
   identity required, individual indie path is via initiatives like
   `PlayStation®Talents` (region-specific) or via a publisher.
2. Sign the SIE NDA, receive the developer toolchain (ProDG, PSSL
   compiler, GNMX SDK).
3. Receive DevKit / TestKit hardware (returnable; Sony bills lease /
   non-disclosure deposit).
4. Port the codebase against the SIE C runtime (newlib-ish; some
   POSIX shims missing).
5. Use Sony's **internal** Qt port if Sony's licensing allows it
   (Qt is licensed separately to PS5 ISVs by The Qt Company under
   "Qt for PlayStation"; a per-title fee plus Qt's own commercial
   licence). This is a non-trivial cost line item.
6. Pass Sony's TRC (Technical Requirements Checklist) — covers system
   software interactions, suspend/resume, save data corruption
   recovery, accessibility, region/parental controls. ~3–6 months QA
   pass on a small title.
7. Submit master to QA, ship via PSN store.

Estimated cost gate: tens of thousands of EUR up front (Qt licence +
DevKit + NDA legal review), plus ongoing per-title certification fees.
Does not bypass Sony's signing infrastructure.

---

## 4. Homebrew path (best-effort, console-specific, not distributable)

### 4.1 Target firmware

PS5 jailbreaks are firmware-locked. As of writing, public chains exist
for:

* **FW ≤ 4.51** — WebKit + `Mast1c0re` (PS2-emulator side-channel) +
  kernel exploit; runs `etaHEN` payloads.
* **FW ≤ 5.50** — `Byepervisor` (BPF / hypervisor escape).
* **FW 6.x – 11.x** — no public chain at the time of writing.

The operator's console must be running a target FW *and not have
auto-updated past it*. Most retail PS5s are well beyond 5.50; the
homebrew window is small and shrinking.

### 4.2 Toolchain

`Open Orbis SDK` for PS4 has a partial PS5 fork at
<https://github.com/OpenOrbis/OpenOrbis-PS5-Toolchain> built on Clang
+ a custom libc + PRX loader. It targets:

* C/C++ user-space binaries for the PS5 user PID
* No GPU access (no GNMX, no Mesa, no GLES) — the homebrew scene
  currently runs **CPU-side** demos and ports of console emulators
  that use software rendering or limited Vulkan via undocumented
  driver paths.
* Networking via libkernel `sceNetSocket` is documented.

**This is not enough to run qtopengl.** The OpenGL / GLES / Vulkan
surface qtopengl needs is not exposed to homebrew. A software renderer
fallback (CPU-rasterise the tile map into a framebuffer) is the only
near-term path, and that's a complete rewrite of `MapVisualiser` —
much larger than a port.

### 4.3 Audio / input

Same status as graphics: documented homebrew coverage for audio is
partial, controller input via the libkernel HID path is workable but
also undocumented for PS5 specifically (most public examples come
from PS4 work).

### 4.4 Packaging

A homebrew "package" on PS5 means a `fpkg` / `.pkg` file repacked
without Sony's signing — installable only on a jailbroken console
with `etaHEN` running. The build pipeline is:

```bash
# OpenOrbis-PS5-Toolchain provides these wrappers:
${OO_PS5_TOOLCHAIN}/bin/clang++ \
    --target=x86_64-pc-freebsd12-elf \
    -fPIC -fPIE \
    -isysroot ${OO_PS5_TOOLCHAIN}/sysroot \
    main.cpp -o eboot.elf

${OO_PS5_TOOLCHAIN}/bin/create-fself eboot.elf eboot.bin
${OO_PS5_TOOLCHAIN}/bin/create-pkg \
    --content-id=UP0001-CCC00001_00-CATCHCHALLENGE \
    --output=catchchallenger.pkg \
    --root=pkg-root/
```

The `.pkg` is **not** signed by Sony; it has the homebrew loader's
trust anchor, which only `etaHEN` honours. There is no TSA / RFC3161
analogue, no Authenticode equivalent, no `osslsigncode` analogue. You
can't produce a Sony-signed homebrew `.pkg`.

---

## 5. Code changes you'd need (in this repo)

Same shape as the Nintendo Switch port, but blocked on Qt-on-PS5 not
existing publicly. If a Qt port appeared upstream tomorrow, the changes
would be:

```cmake
# CMakeLists.txt, top level
if(CMAKE_SYSTEM_NAME STREQUAL "PS5")
    set(CC_PLATFORM_PS5 TRUE)
    add_compile_definitions(CATCHCHALLENGER_PS5
                            CATCHCHALLENGER_NOAUDIO
                            CATCHCHALLENGER_NO_WEBSOCKET)
endif()
```

A `testingcompilationps5.py` mirroring the Windows / Mac / Switch
patterns would self-skip when `${OO_PS5_TOOLCHAIN}` is missing — same
self-skip approach as the cross-compile scripts already in this dir.

The other code changes (savegame path redirection, controller input,
audio backend) are identical in shape to the Switch port and gated by
`CATCHCHALLENGER_PS5`.

---

## 6. Signing & retail packaging — not feasible without Sony

Like the Switch case but stricter:

| Want | Possible? |
|------|-----------|
| Tile on home menu, retail PS5, store install | **No** without Sony developer account + full TRC pass |
| Tile on home menu via "homebrew forwarder" (Switch-style) | **No** equivalent on PS5 — no persistent CFW, no tile install path |
| Run-from-payload via `etaHEN` on jailbroken PS5 (FW ≤ 5.50) | **Yes**, but not redistributable as "install on any console" |

There is no `osslsigncode` / `wix` / `hacBrewPack` analogue for PS5.
`fpkg` repacking does not produce a Sony-trusted artefact. The official
key infrastructure is a Sony asset.

---

## 7. Effort estimate

| Phase | Effort | Risk |
|-------|--------|------|
| Apply to PartnerNet, sign NDA, receive DevKit | 3–6 months calendar (gating step) | extreme |
| Qt for PS5 licence acquisition (commercial) | weeks of legal + 5-figure EUR cost | high |
| Port qtopengl against Sony's Qt + GNMX | 4–8 weeks | medium |
| TRC certification | 8–16 weeks | medium |
| **Official total** | **~9–14 months** + budget | |
| **Homebrew total** | not feasible without GPU stack — multi-year research project | extreme |

---

## 8. Licensing cost (official path)

Public, operator-reported figures for SIE PartnerNet as of late 2025.
Exact pricing is NDA-gated and region-specific (SIEE / SIEA / SIEJ each
quote separately). Treat the numbers below as ±50% planning estimates;
get a real quote from <https://partners.playstation.net/> once you have
an NDA in place.

| Item | Cost | Notes |
|------|------|-------|
| PartnerNet account | **Free, but gated** | Studios approved fastest; individuals usually routed to a publisher or regional indie programme (PlayStation Talents, etc.). Months of calendar time before approval |
| DevKit (DevKit-A "Tunatuna") | **~$2,500–3,500** one-time | Sold at near-cost; non-transferable; must be returned if account closes |
| TestKit (TestKit-A) | **~$1,500–2,000** one-time | Used for retail-disc-equivalent testing |
| **Qt for PlayStation** licence (The Qt Company) | **5-figure EUR / year**, project-dependent | Separate from Qt's open-source LGPL — closed platforms require the commercial licence. Quote-only; public range is **€15,000–€50,000/year** for a single SKU |
| TRC (Technical Requirements Checklist) cert | **~€5,000–€10,000** per submission | Variable by region |
| Revenue share | **~30%** to Sony | Standard PSN store split |
| Legal / NDA review | **A few €k** in lawyer time on day one | One-time |

**Realistic floor to ship one indie game on PS5 via the official path:**
**~€30,000–€80,000 up front** (DevKit + Qt-for-PS5 first-year +
first TRC pass + legal), before a line of porting code is written.
Plus ongoing per-year Qt-for-PS5 licence renewal + per-title cert
fees on every title or major version.

For comparison, the equivalent Switch ship cost (see
`NINTENDO-SWITCH-1.md` §11) is ~$3,000 — **PS5 licensing alone is
roughly an order of magnitude more expensive** than Switch licensing,
and that's *before* the Qt-for-PS5 line item which has no Switch
analogue. This is why the recommendation in §9 below treats PS5 as a
publisher-fronted decision, not an indie one.

## 9. Recommendation

1. **Do not pursue the homebrew route.** The PS5 jailbreak scene does
   not currently expose GPU APIs; running qtopengl is blocked at the
   GLES/Vulkan layer, not at the source-code layer. Even a successful
   build would not be redistributable.
2. **Do not pursue an "officially signed homebrew" PS5 package.** No
   such artefact exists; Sony does not sign third-party content
   outside their developer programme.
3. **If retail PSN distribution genuinely matters**, treat this as a
   commercial-publishing decision: budget the licensing, the DevKit,
   the Qt-for-PS5 fee, and the certification timeline. The
   CatchChallenger source code is fine to port; the cost is
   organisational, not technical.
4. **In the interim**, prioritise the platforms where homebrew
   distribution actually works:
   * Nintendo Switch homebrew (see `NINTENDO-SWITCH-1.md`) — Qt 6
     port still missing publicly but the scene is mature enough to
     justify the engineering investment.
   * Steam Deck (Linux native) — works **today** with the existing
     `client/qtopengl/` build, no port effort beyond choosing a
     packaging format (Flatpak / AppImage). This is the closest
     "console-like" platform that doesn't require an NDA.
