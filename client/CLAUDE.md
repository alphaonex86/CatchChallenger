# client/ — clients

Combine with root CLAUDE.md.

## Client server-selection CLI flags — pick exactly ONE

Both clients accept three mutually exclusive groups; passing two+ is rejected (clears all three with `std::cerr` warning).

* **From server list** — `--server NAME`. TCP or WebSocket per entry. Always available.
* **TCP direct** — `--host HOST --port PORT` (both required). Only when `CATCHCHALLENGER_NO_TCPSOCKET` not defined.
* **WebSocket direct** — `--url URL` (`ws://...`/`wss://...`). Only when `CATCHCHALLENGER_NO_WEBSOCKET` not defined.

When adding a client front-end: parse all three; only one may set state; reject+clear on conflict; document in `client/dev.md` and `--help`. No `--name` flag (earlier alias of `--server` removed).
