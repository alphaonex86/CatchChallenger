# CatchChallenger server — ESP32 (ESP-IDF) project

ESP-IDF project that runs the all-in-one game server on an ESP32 with **no
filesystem**: the datapack and the server settings are compiled into flash
(`.rodata`, read in place — "datapack-cpp"), and account/character state uses the
in-memory DB backend. WiFi (STA) + DHCP bring-up is in `main/app_main.cpp`; the
server then binds over lwIP and (if a `<broadcastName>` was configured at stage1)
periodically emits the LAN-announce UDP broadcast so normal clients auto-discover
it.

**This is the cross-compile project; the full how-to (prefix layout, emulator
that mirrors your real ESP32, flashing real hardware, FreeRTOS notes) is in
[`../../../test/ESP32.md`](../../../test/ESP32.md).** Quick build:

```bash
. "$IDF_PATH/export.sh"
# stage1 first: generates /mnt/data/perso/tmpfs/datapack-cpp/datapack_cpp.{cpp,hpp}
#   (see test/ESP32.md §3 / test/testingcompilationESP32.py)
idf.py set-target esp32        # or esp32s3 / esp32c3 — match your board
idf.py menuconfig              # set CatchChallenger server -> WiFi SSID/password
idf.py -DCATCHCHALLENGER_DATAPACK_CPP_DIR=/mnt/data/perso/tmpfs/datapack-cpp build
idf.py -p /dev/ttyUSB0 flash monitor
```

`main/CMakeLists.txt` carries the **integration point**: the server source set
(mirrors the host CLI build minus Qt/SQL/XML/other-mains). `test/ESP32.md` lists
the build defines and how the host stage1/stage2 build verifies the exact same
code path without hardware.
