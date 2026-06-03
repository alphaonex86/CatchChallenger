# Running the CatchChallenger server on an ESP32

`catchchallenger-server-cli` (the all-in-one game server) runs on an **ESP32**
microcontroller with **no filesystem at all**: the whole datapack *and* the
server settings are compiled into the executable as human-readable C++ const
data that lives in **flash** (`.rodata`, read in place — execute/access in place,
"XIP", the "DAX-like" trick), and account/character state uses the in-memory
database backend (`CATCHCHALLENGER_DB_INTERNAL_VARS`). The server connects to
your WiFi (STA + DHCP) and announces itself on the LAN exactly like the desktop
"open to LAN" server, so the normal client auto-discovers it.

The build is driven by `test/testingcompilationESP32.py`; this file explains how
to *build and run* it — on the host (which fully verifies the no-filesystem code
path with no hardware), under an emulator that mirrors your real board, and on a
real ESP32.

> The ESP32 has very little RAM (a plain ESP32 has ~520 KB SRAM). That is why
> the datapack is kept in **flash** and read in place rather than parsed into
> RAM at boot, and why there is **no filesystem** (SPIFFS/littlefs) — every byte
> the engine needs is either a flash const (datapack + settings) or a small
> heap object built from it.

---

## 1. The idea — two stages, datapack-cpp

The desktop/server datapack is XML + binary files on disk. An ESP32 has neither
the RAM to parse it nor a filesystem to hold it. So we split the work in two:

* **stage1 (host)** — build `catchchallenger-server-cli` with
  `-DCATCHCHALLENGER_DATAPACK_CPP_EMIT=ON`, then run `--save` on the real
  datapack (maincode **`test`**, from
  `/home/user/Desktop/CatchChallenger/CatchChallenger-datapack/`). The server
  parses the datapack as usual, then **re-emits the whole cache — datapack
  *and* settings — as human-readable C++ const arrays** into
  `/mnt/data/perso/tmpfs/datapack-cpp/datapack_cpp.{cpp,hpp}`.

  The generated C++ is decoded, readable values (not a hex blob), e.g.

  ```c
  static const uint64_t _dcpp_u64[]={ 4,6,1,0,1,10, ... ,42498, ... };   // ← the listen port
  static const char * const _dcpp_str[]={ "", "CatchChallenger ESP32", "test", ... };
  static const unsigned char _dcpp_raw[]={ ... };   // collision maps, ids (bulk numeric)
  ```

  The server datapack is almost entirely numbers — it carries **no monster
  names, descriptions, …** (those are client-side display data), so the arrays
  are compact and dominated by typed numbers, with only a handful of strings
  (the broadcast name, the maincode, a few type/profile labels).

* **stage2 (ESP32, or host proof)** — build the server with
  `-DCATCHCHALLENGER_DATAPACK_CPP=ON -DCATCHCHALLENGER_DATAPACK_CPP_DIR=<that dir>`
  plus `-DCATCHCHALLENGER_NOXML=ON -DCATCHCHALLENGER_DB_INTERNAL_VARS=ON
  -DCATCHCHALLENGER_SELECT=ON`. The generated `.cpp` is compiled in; at boot the
  server reads the datapack + settings **straight from those flash arrays, in
  place** — no file is opened, nothing is copied into RAM beyond the small
  runtime containers, and there is no byte-deserialization step.

How it works internally (`server/base/DatapackCppBuffer.hpp`): the engine already
serializes the datapack via per-type `serialize()/parse()` methods. stage1 drives
them with a `CppEmitBuffer` that records each scalar's *decoded* value into the
typed arrays above; stage2 drives the same `parse()` methods with a
`CppReadBuffer` that pops those values back. Reusing the existing serializers
means the datapack-cpp path never drifts from the normal one. Both are entirely
`#ifdef`-gated (`CATCHCHALLENGER_DATAPACK_CPP_EMIT` / `CATCHCHALLENGER_DATAPACK_CPP`)
so every other build is byte-for-byte unchanged.

---

## 2. What you need (the toolchain/emulator prefix)

Everything ESP-specific lives **outside the repo** under `$CC_ESP32_PREFIX`
(default `/mnt/data/perso/progs/catchchallenger-ESP32`), mirroring the MXE and
MS-DOS prefixes. The harness never installs it; it self-skips the ESP-IDF and
emulator phases when absent.

```
catchchallenger-ESP32/
├── esp-idf/            Espressif ESP-IDF (export.sh sets IDF_PATH + the xtensa
│                       toolchain on PATH). `git clone -b v5.x --recursive
│                       https://github.com/espressif/esp-idf` then ./install.sh
├── qemu/               Espressif's qemu fork build (qemu-system-xtensa with the
│                       esp32 machine) + any flash image / efuse you dump from
│                       your real board so the emulator MIRRORS it (see §5)
└── README.txt          provenance of each piece
```

The repo carries the ESP-IDF *project* (the integration code) at
`server/cli/esp32/`:

```
server/cli/esp32/
├── CMakeLists.txt          idf project (passes CATCHCHALLENGER_DATAPACK_CPP_DIR)
├── sdkconfig.defaults      stack/lwIP/partition/PSRAM defaults
├── partitions.csv          NVS + one big app partition (no data/FS partition)
└── main/
    ├── CMakeLists.txt       the server source set + the no-FS profile defines
    ├── Kconfig.projbuild     WiFi SSID/password (menuconfig)
    └── app_main.cpp          wifi STA + DHCP, then runs the server loop
```

---

## 3. The easy path — let the test script do it

```bash
cd test
python3 testingcompilationESP32.py                       # host stages + emulator
python3 testingcompilationESP32.py --realhardware        # + flash the board (autodetect port)
python3 testingcompilationESP32.py --realhardware --serial /dev/ttyACM0   # pin the port
```

* **stage1** + **stage2** are **pure host builds** and always run — they fully
  exercise the new no-filesystem datapack-cpp + settings-in-flash +
  LAN-announce code without any ESP hardware or toolchain. The script asserts
  the stage2 server, launched in a directory containing *only the binary*,
  loads the datapack + settings from flash, binds, and arms the LAN announce.
* the **esp-idf** build phase self-skips (logs, not a failure) when
  `$CC_ESP32_PREFIX/esp-idf` is absent; otherwise it `idf.py build`s the project.
* the **qemu** phase boots the firmware under `qemu-system-xtensa`, waits for
  `correctly bind:`, then connects `qtcpu800x600 --host 127.0.0.1 --port 42498
  --autologin`; self-skips when qemu / the firmware image is absent.
* **`--realhardware`** flashes the board via `idf.py flash`, reads its serial
  for the DHCP IP + `correctly bind:`, then connects `qtcpu800x600 --host
  <board-ip> --port 42498 --autologin`; self-skips unless the flag is passed AND
  a board is present. The port is **autodetected** (the ESP32's USB serial among
  `/dev/ttyUSB*`+`/dev/ttyACM*`, ranked by ESP32 USB-serial vendor id —
  CP210x `10c4`, CH34x `1a86`, FTDI `0403`, Espressif native-USB `303a` — and
  confirmed with `esptool chip_id`); pin it with `--serial <dev>`.

> The connect phases assert the client reaches **protocol-good / login**, not
> the map: the fileless server serves no datapack files, so a fresh client
> connects + logs in but cannot finish datapack sync (it needs the matching
> datapack already, or an HTTP mirror). That is the expected embedded model.

The rest of this document is the **manual** recipe + the hardware/emulator how-to.

---

## 4. Manual build

### 4a. stage1 — generate the flash datapack-cpp (host)

```bash
cd /home/user/Desktop/CatchChallenger/git
SRC=server/cli
B1=/tmp/cc-esp32-stage1
cmake -S "$SRC" -B "$B1" -DCMAKE_BUILD_TYPE=Release \
  -DCATCHCHALLENGER_SELECT=ON \
  -DCATCHCHALLENGER_DB_INTERNAL_VARS=ON \
  -DCATCHCHALLENGER_DATAPACK_CPP_EMIT=ON
cmake --build "$B1" -j"$(nproc)"

# run --save in a scratch dir with the datapack + a server-properties.xml that
# pins the ESP32 profile (10 clients, min-CPU visibility, the "test" maincode,
# and a non-empty <broadcastName> to turn the LAN announce on).
W=/tmp/cc-esp32-save; mkdir -p "$W"; cd "$W"
cp "$B1/catchchallenger-server-cli" .
ln -sfn /path/to/CatchChallenger-datapack datapack    # maincode test
cat > server-properties.xml <<'EOF'
<?xml version="1.0"?>
<configuration>
    <server-port value="42498"/>
    <server-ip value=""/>
    <automatic_account_creation value="true"/>
    <max-players value="10"/>
    <broadcastName value="CatchChallenger ESP32"/>
    <content><mainDatapackCode value="test"/></content>
    <mapVisibility><minimize value="cpu"/></mapVisibility>
</configuration>
EOF
./catchchallenger-server-cli --save          # emits datapack_cpp.{cpp,hpp}

mkdir -p /mnt/data/perso/tmpfs/datapack-cpp
cp datapack_cpp.cpp datapack_cpp.hpp /mnt/data/perso/tmpfs/datapack-cpp/
```

`<broadcastName>` empty (the default) = LAN announce **disabled**; any non-empty
name enables it and is what LAN clients show in their server list. `max-players`
is the client cap (10) and `<mapVisibility><minimize>cpu</minimize>` selects the
**min-CPU** visibility algorithm (the right trade for a microcontroller). All of
this is baked into the flash cache, so the ESP32 needs no `server-properties.xml`.

### 4b. host proof (no ESP-IDF needed)

Verify the no-filesystem build works on the host before touching hardware:

```bash
B2=/tmp/cc-esp32-stage2
cmake -S server/cli -B "$B2" -DCMAKE_BUILD_TYPE=Release \
  -DCATCHCHALLENGER_SELECT=ON \
  -DCATCHCHALLENGER_DB_INTERNAL_VARS=ON \
  -DCATCHCHALLENGER_NOXML=ON \
  -DCATCHCHALLENGER_DATAPACK_CPP=ON \
  -DCATCHCHALLENGER_DATAPACK_CPP_DIR=/mnt/data/perso/tmpfs/datapack-cpp
cmake --build "$B2" -j"$(nproc)"

mkdir -p /tmp/cc-esp32-run && cp "$B2/catchchallenger-server-cli" /tmp/cc-esp32-run/
cd /tmp/cc-esp32-run && ./catchchallenger-server-cli      # NO datapack/, NO xml, NO database/
```

Expected log (datapack + settings come from flash):

```
DB_INTERNAL_VARS: in-memory database (no disk, no file syscall)
commonDatapack size: 15091B
Loaded the common datapack into 0ms
map size: 5651B
correctly bind: ... port: 42498 ...
LAN announce enabled: "CatchChallenger ESP32" -> 255.255.255.255:42490 every tick
Waiting connection on port 42498
```

This is exactly the code path the ESP32 firmware runs, minus the xtensa CPU and
lwIP — the only thing the real build adds is the ESP-IDF wifi/DHCP bring-up.

### 4c. stage2 — build the firmware (ESP-IDF)

```bash
. "$CC_ESP32_PREFIX/esp-idf/export.sh"
cd server/cli/esp32
idf.py set-target esp32                 # or esp32s3 (Xtensa) — match your board.
                                        # NB esp32c3/c6 are RISC-V, not Xtensa:
                                        # emulate them with qemu-system-riscv32.
idf.py menuconfig                       # CatchChallenger server -> WiFi SSID/password
idf.py -DCATCHCHALLENGER_DATAPACK_CPP_DIR=/mnt/data/perso/tmpfs/datapack-cpp build
```

`main/CMakeLists.txt` compiles the same server sources as the host CLI build
(minus Qt / SQL / XML / the other server mains) with the no-filesystem profile
defines, and renames the CLI `main()` to `catchchallenger_cli_main()` so
`app_main()` (wifi/DHCP) can call it. It is the **integration point**: if a
source moves, adjust the globs there; the host stage1/stage2 build is the
source of truth for which files must compile.

---

## 5. The emulator — mirror your real ESP32

Use **`qemu-system-xtensa`** (Espressif's fork; the upstream xtensa target does
not emulate the ESP32 peripherals). Build it once into `$CC_ESP32_PREFIX/qemu`
(see Espressif's `esp-toolchain`/`qemu` docs) or `idf.py qemu`.

To make the emulator behave like *your* board, dump these from the real ESP32
and feed them to qemu / record them here so the emulated machine matches:

```bash
# with the board on /dev/ttyUSB0 and the esp-idf env sourced:
esptool.py --port /dev/ttyUSB0 chip_id            # chip + revision
esptool.py --port /dev/ttyUSB0 flash_id           # flash size + manufacturer
esptool.py --port /dev/ttyUSB0 read_mac           # base MAC
esptool.py --port /dev/ttyUSB0 read_flash 0 0x400000 flash.bin   # full image (4 MiB)
espefuse.py --port /dev/ttyUSB0 summary           # efuses (PSRAM? rev? security?)
```

Then boot the same image under qemu (4 MiB flash, ESP32 machine):

```bash
qemu-system-xtensa -nographic -machine esp32 -m 4M \
  -drive file=$CC_ESP32_PREFIX/qemu/flash.bin,if=mtd,format=raw \
  -nic user,model=open_eth,hostfwd=tcp::42498-:42498
```

`hostfwd=tcp::42498-:42498` forwards the host's `127.0.0.1:42498` into the guest
(like the MS-DOS slirp setup), so you can point a client at it. `idf.py qemu`
(or `idf.py qemu monitor`) wraps all of this once the project is built.

> **Mirror the real hardware:** record your board's chip revision, flash size,
> PSRAM presence, and CPU clock in §8 below, and set the matching qemu `-machine`
> / `-m` / PSRAM options + the same `sdkconfig` so the emulator's RAM/flash
> limits match the device — otherwise something that fits in the emulator may
> not fit on the board.

---

## 6. Running on a real ESP32

```bash
. "$CC_ESP32_PREFIX/esp-idf/export.sh"
cd server/cli/esp32
idf.py menuconfig            # set your WiFi SSID + password
idf.py -DCATCHCHALLENGER_DATAPACK_CPP_DIR=/mnt/data/perso/tmpfs/datapack-cpp \
       -p /dev/ttyUSB0 flash monitor
```

1. **WiFi + DHCP.** `app_main.cpp` joins the configured SSID in STA mode and
   waits for a DHCP lease, then starts the server. The serial monitor prints
   `DHCP got IP <addr>` and then `correctly bind: ... port: 42498`.
2. **LAN announce.** Because `<broadcastName>` is set, the server broadcasts the
   same datagram the desktop "open to LAN" server sends (UDP `42490`,
   `255.255.255.255`), so any CatchChallenger client on the same LAN lists
   "CatchChallenger ESP32" automatically. Clients connect to the board's IP on
   TCP `42498`.
3. **Clients + the datapack.** The ESP32 holds **no datapack files**, so it does
   not serve the datapack over the wire. A connecting client must already have
   the matching datapack (same `test` maincode → same hashes, so no download),
   or fetch it from an HTTP mirror (`<httpDatapackMirror>`). This is the
   embedded model: the device is the game server, not a datapack CDN.
4. **Max 10 clients**, **min-CPU** visibility — baked into the flash settings.
5. **Reboot = fresh state.** `CATCHCHALLENGER_DB_INTERNAL_VARS` keeps
   accounts/characters in RAM only; they are lost on power-cycle. For
   persistence, add an NVS/flash write-back hook (out of scope here).

Flash sizing: the `test` maincode datapack + the server fit in a 2.75 MiB app
partition on a 4 MiB-flash ESP32 (`partitions.csv`). A larger datapack needs a
larger flash + a larger factory partition.

---

## 7. FreeRTOS notes

ESP-IDF is FreeRTOS-based. A few consequences, all already handled in the port:

* **No threads / one task.** The whole server runs in the `app_main` FreeRTOS
  task (single-threaded EventLoop). `sdkconfig.defaults` bumps
  `CONFIG_ESP_MAIN_TASK_STACK_SIZE` to 32 KiB because `main()` holds the
  `epoll_event[512]` + I/O buffers on its stack.
* **No `timerfd`, no `sendfile`.** lwIP has neither. The EventLoop's `select(2)`
  backend gets a timer registry driven off `select()`'s timeout (the same path
  MS-DOS uses), gated on `CC_TARGET_ESP32`; file-send falls back to a
  read+`send()` loop. The periodic LAN-announce timer rides this mechanism.
* **No signals.** The `SIGPIPE`/benchmark signal handlers compile out; lwIP
  `send()` reports `EPIPE` via the return value.
* **lwIP sockets.** `bind`/`listen`/`accept`/`recv`/`send`/`select` are the
  POSIX BSD API; `CONFIG_LWIP_MAX_SOCKETS=16` covers the listener + UDP announce
  + 10 clients. `CONFIG_LWIP_SO_REUSE` + IPv6 are on for the dual bind.
* **RAM.** The bulk datapack stays in flash; only the runtime containers live in
  RAM. A plain 520 KB-SRAM ESP32 fits the small `test` maincode; enable PSRAM
  (`CONFIG_SPIRAM`) on a WROVER/-S3 for headroom or a bigger datapack.

---

## 8. Real-hardware characteristics (fill in for your board)

> Dump these from your ESP32 (§5) and record them here so the emulator and the
> `sdkconfig` match the device.

| property | value (fill in) |
|---|---|
| Module / dev board | _e.g. ESP32-WROVER-E / ESP32-S3-DevKitC_ |
| Chip + revision (`esptool chip_id`) | |
| CPU cores / clock | |
| SRAM | _e.g. 520 KB_ |
| PSRAM (`espefuse summary`) | _e.g. 8 MB QSPI / none_ |
| Flash size (`esptool flash_id`) | _e.g. 4 MB_ |
| Base MAC (`esptool read_mac`) | |
| `idf.py set-target` | _esp32 / esp32s3 / esp32c3_ |
| Free heap at boot (`esp_get_free_heap_size`) | |
| Datapack-cpp flash size (`datapack_cpp.cpp`) | _~120 KiB for `test`_ |
| Max clients reached | |

---

## 9. Build defines reference

| define | set by | meaning |
|---|---|---|
| `CATCHCHALLENGER_DATAPACK_CPP_EMIT` | stage1 host build | `--save` also re-emits the cache as human-readable C++ (`datapack_cpp.{cpp,hpp}`). #ifdef-gated; zero cost otherwise. |
| `CATCHCHALLENGER_DATAPACK_CPP` | stage2 / ESP32 | compile the generated `.cpp` in; load datapack+settings in place from flash, no FS. |
| `CATCHCHALLENGER_DATAPACK_CPP_DIR` | both | dir holding `datapack_cpp.{cpp,hpp}`. |
| `CATCHCHALLENGER_DB_INTERNAL_VARS` | ESP32 | in-memory account/character DB (no file syscall). Implies `CATCHCHALLENGER_DB_FILE`. |
| `CATCHCHALLENGER_NOXML` | ESP32 | no XML parser; the datapack comes only from the flash cache. |
| `CATCHCHALLENGER_SELECT` | ESP32 | `select(2)` IO backend over lwIP (no epoll/io_uring). |
| `CC_TARGET_ESP32` | `main/CMakeLists.txt` | ESP32 build: no `timerfd`/`sendfile`, select-timeout timer registry. |
| `<broadcastName>` (server-properties.xml) | stage1 | non-empty = LAN announce on, with that name; baked into the flash cache. |

---

## 10. References
* Build/run automation: `test/testingcompilationESP32.py`
* ESP-IDF project: `server/cli/esp32/` (+ `server/cli/esp32/README.md`)
* datapack-cpp buffers: `server/base/DatapackCppBuffer.hpp`
* LAN announce timer: `server/cli/timer/TimerBroadcastAnnounce.{hpp,cpp}`
  (mirrors `server/qt/QtServer.cpp::sendBroadcastServer` / client
  `client/libqtcatchchallenger/LanBroadcastWatcher.cpp`, UDP 42490)
* ESP-IDF: <https://docs.espressif.com/projects/esp-idf/>
* qemu-xtensa (ESP32): <https://github.com/espressif/qemu>
