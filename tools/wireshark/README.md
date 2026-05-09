# CatchChallenger Wireshark dissector

`catchchallenger.lua` is a Wireshark Lua plugin that decodes the
CatchChallenger TCP wire format described in
`general/base/ProtocolParsing.hpp` and `ProtocolParsingGeneral.cpp`.

## Install

Copy (or symlink — Wireshark plugins live outside the repo) the file into
your Wireshark Lua plugin directory:

* Linux:  `~/.local/lib/wireshark/plugins/` or `~/.config/wireshark/plugins/`
* macOS:  `~/.config/wireshark/plugins/`
* Windows: `%APPDATA%\Wireshark\plugins\`

Or run ad-hoc against a capture:

    wireshark -X lua_script:catchchallenger.lua capture.pcapng
    tshark   -X lua_script:catchchallenger.lua -r capture.pcapng

## Configure

Default TCP ports `9999` (codebase default) and `61917` (test harness) are
registered automatically. Change via *Edit → Preferences → Protocols →
CatchChallenger → TCP ports*, or apply `Decode As… → CatchChallenger` to
any other port.

## What it shows

For each PDU on the wire:

* `code` — packet code byte (hex).
* `name` — human label when known (Move, Server list, Login query, …).
* `qid` — query number byte, present for queries (code ≥ 0x80) and for
  replies (codes `0x01` client→server, `0x7F` server→client).
* `size` — payload length. Pulled from the fixed-size table when the
  protocol pins it, or from the inline `uint32 LE` size field when the
  packet is variable-length.
* `data` — raw payload bytes.
* `reply_to` — for reply packets, the original query's packet code,
  resolved by tracking outstanding queries per TCP conversation.

## Known limitations

* **No payload decompression.** CatchChallenger compresses bulk packets
  with Zstandard once compression is negotiated. Wireshark Lua can't
  link against libzstd, so payloads stay opaque. To inspect them, run
  the server with `<compression>none</compression>` in
  `server-properties.xml` while debugging.
* **Partial captures.** Reply size lookup needs the original query in the
  same capture. If you started capturing mid-session, replies show up as
  *unknown size — partial capture?* and dissection of that TCP segment
  stops there (the parser can't realign without knowing how many bytes
  the reply consumes).
* **Reply size for `0xB0/0xBF/0xC0/0xC1`** depends on
  `CATCHCHALLENGER_SERVER_MAXIDBLOCK`. The dissector treats them as
  dynamic to stay safe across non-default builds.
* **`packetFixedSize[0x64]`** is 1 byte when the server is configured
  for ≤255 players, 2 otherwise. The dissector assumes the 2-byte form
  (the codebase default).
* **WebSocket transport** is not handled here; only raw TCP.
