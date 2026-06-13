# client/ — clients

Combine with root CLAUDE.md.

## Client server-selection CLI flags — pick exactly ONE

Both clients accept three mutually exclusive groups; passing two+ is rejected (clears all three with `std::cerr` warning).

* **From server list** — `--server NAME`. TCP or WebSocket per entry. Always available.
* **TCP direct** — `--host HOST --port PORT` (both required). Only when `CATCHCHALLENGER_NO_TCPSOCKET` not defined.
* **WebSocket direct** — `--url URL` (`ws://...`/`wss://...`). Only when `CATCHCHALLENGER_NO_WEBSOCKET` not defined.

When adding a client front-end: parse all three; only one may set state; reject+clear on conflict; document in `client/dev.md` and `--help`. No `--name` flag (earlier alias of `--server` removed).

## Testing — drive the client over QLocalServer, don't add in-client test code

Prefer NOT to change production/base client code to implement a test case. The client exposes a **QLocalServer automation channel** (`client/libqtcatchchallenger/LocalListener.*`) precisely so a test harness can drive it EXTERNALLY over a unix socket — inject keys/clicks, read player state/position/map, inventory, chat, trade, fight. Write new client tests by scripting that channel (see `testingmulti.py`), not by adding `--test-*` self-test modes/scaffolding inside MapControllerMP & co. Bug FIXES to production code are fine; TEST drivers belong outside the client binary.
