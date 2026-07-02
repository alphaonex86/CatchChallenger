# client/ — clients

Combine with root CLAUDE.md.

## Datapack is checksummed — two distinct things: the datapack vs its binary cache

Keep two separate things straight:

1. **The datapack** — the raw datapack files
   (`~/.local/share/CatchChallenger/client*/datapack/argument-<host>-<port>/`).
   It is CHECKSUMMED. On connect the client sends its cached datapack hash; if it
   matches the server's, the datapack is already in sync and is NOT re-downloaded
   (only a mismatch triggers a full, or HTTP-mirror partial, download).
2. **The preprocessed binary cache** — ONE file, the parsed/preprocessed binary
   form of the datapack. It speeds up loading by NOT opening the many datapack
   files and NOT re-converting text -> the usable internal binary format on every
   start. It is REGENERATED ONLY when the datapack changed (checksum mismatch);
   an unchanged datapack reuses the existing binary cache.

So the checksum is on the DATAPACK and it drives BOTH: whether to re-download,
and whether to regen the binary cache. ALWAYS account for this: code touching
datapack sync must keep the checksum coherent (a stale/empty hash forces a
needless full re-download + binary-cache regen; a wrong "match" ships an
out-of-date datapack). A test that wants a fresh download/regen must clear the
datapack cache AND the persisted settings — otherwise a previous run's / another
test's hash (or mirror URL) leaks in.

### HTTP-mirror download strategy (when a mirror is advertised)

The mirror path favours ONE compressed archive (compression across ALL files
beats per-file compression) and only falls back to individual files:

* **Empty cache (full)** — download ONE `tar.zst` of ALL files (whole datapack,
  compressed across every file for best size), then unpack.
* **Partial (some changed / new / removed)** — FIRST try the delta archive keyed
  by the client's CURRENT hash (`.tar.zst` from that hash to the server's): it
  carries only the changed/new files, still compressed across all of them, plus a
  file LIST of what to REMOVE. If that current-hash delta is missing / fails to
  download, FALL BACK to downloading each changed/new file INDIVIDUALLY over HTTP
  (and still apply the removal list).
* **Full sync (hash matches)** — download NOTHING.

(Internal-protocol mode instead streams files inline over the game socket —
`0x75` size header, then `0x76` raw / `0x77` zstd file-content packets — with no
mirror; used when no mirror is advertised.)

## Client server-selection CLI flags — pick exactly ONE

Both clients accept three mutually exclusive groups; passing two+ is rejected (clears all three with `std::cerr` warning).

* **From server list** — `--server NAME`. TCP or WebSocket per entry. Always available.
* **TCP direct** — `--host HOST --port PORT` (both required). Only when `CATCHCHALLENGER_NO_TCPSOCKET` not defined.
* **WebSocket direct** — `--url URL` (`ws://...`/`wss://...`). Only when `CATCHCHALLENGER_NO_WEBSOCKET` not defined.

When adding a client front-end: parse all three; only one may set state; reject+clear on conflict; document in `client/dev.md` and `--help`. No `--name` flag (earlier alias of `--server` removed).

## PathFinding is a LEAST-DIRECTION-CHANGE planner, not a conventional shortest-path

`libqtcatchchallenger/maprender/PathFinding.cpp` (`internalSearchPath`) is a
NON-conventional pathfinder: it minimizes the number of DIRECTION CHANGES
(context switches / turns), NOT the travelled distance. That is why it keeps,
per cell, FOUR run-length-compressed path variants — `pathToGo.left/right/top/
bottom` = the best path that REACHES that cell ENDING in that direction — so
continuing straight just extends the last run (`.back().second++`, no turn)
while turning starts a new run. Don't "simplify" it into a standard A*/BFS
shortest-path; the four-direction bookkeeping is the whole point.

## Testing — drive the client over QLocalServer, don't add in-client test code

Prefer NOT to change production/base client code to implement a test case. The client exposes a **QLocalServer automation channel** (`client/libqtcatchchallenger/LocalListener.*`) precisely so a test harness can drive it EXTERNALLY over a unix socket — inject keys/clicks, read player state/position/map, inventory, chat, trade, fight. Write new client tests by scripting that channel (see `testingmulti.py`), not by adding `--test-*` self-test modes/scaffolding inside MapControllerMP & co. Bug FIXES to production code are fine; TEST drivers belong outside the client binary.
