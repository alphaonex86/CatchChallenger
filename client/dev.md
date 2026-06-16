# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem, copy with take care of use after free (path finding), less size (16Bits vs 32/64Bits)
* async the ressources loading to never freeze the interface
* async the network to never wait the internet and timeout if apply
* client side predicion and compute (like path finding) to scale better on server

# arguments, mostly to debug
* Server selection — pick **exactly one** of the following three options:
  * --server NAME: select an existing server entry from the server list (loaded from the internet feed or saved by the user previously). The entry can be either TCP or WebSocket.
  * --host HOST + --port PORT: TCP direct-connect using ad-hoc credentials. NOT added to the server list. Both required together. Available only when CATCHCHALLENGER_NO_TCPSOCKET is NOT defined.
  * --url URL: WebSocket direct-connect (`ws://...` or `wss://...`). NOT added to the server list. Available only when CATCHCHALLENGER_NO_WEBSOCKET is NOT defined.
  Combining any two (e.g. --server + --url) is rejected by the parser; pass exactly one.
* --autosolo: enter automatically to the solo game (creates a savegame if none; the test character spawns on the `test` maincode city at tile 15,26)
* --autosolo=DX,DY: like --autosolo, then once on the map simulate a click on the tile at player+(DX,DY) — the player walks to it, faces it and opens it (e.g. a Sign). Emits `[CLICKTEST] ...`. Does not quit on its own (combine with --closewhenonmapafter=N). TEST-ONLY scenario helper.
* --autologin: on login/pass page, automaticlly try login
* --character name: on character selection page, automaticly select character with this name
* --closewhenonmap: close 1s after the character is spawn on map
* --closewhenonmapafter=10: similar --closewhenonmap but when on map then change direction each 1s (if look botton, look top, if look left, look right, ...) and close after the number of second specified
* --dropsenddataafteronmap: dropany output comunicacion from client to server after the player spawn on map, used to explore datapack map, ignore zone monster colision (to not open fight with wild monster) and any fight
* --take-screenshot=PATH: (qtopengl) once on the map, write the rendered viewport to PATH and exit; pins srand(42) so random tile variants and animation offsets are reproducible (used by testingmap4client.py)
* --main-datapack-code=CODE: override the autosolo maincode under datapack/internal/map/main/, instead of picking the first directory alphabetically (used by testingmap4client.py to force the "test" fixture)
* --remote-control: open the QLocalServer remote-control / automation channel (see below). OFF by default — no socket is created without it.

# Remote-control / automation channel (--remote-control)

Lets an external process drive a running client and read its state over a unix
socket. **Both** clients support it identically (qtopengl and qtcpu800x600);
the implementation is shared, NOT duplicated:

* Socket + framing: `client/libqtcatchchallenger/LocalListener.{hpp,cpp}` —
  newline-terminated text commands in, newline-terminated replies out. Socket
  path: `<QDir::tempPath()>/CatchChallenger-Client-<n>-<uid>` (n = instance
  slot, usually 0; set a per-test `TMPDIR` to isolate it).
* Command dispatch + vocabulary: `MapControllerMP::remoteActionExecute()` in
  `client/libqtcatchchallenger/maprender/MapControllerMP.cpp` (the authoritative
  command list is the comment above `remoteAction()` in `MapControllerMP.hpp`).
* Wiring per client: qtopengl `client/qtopengl/main.cpp` + `ScreenTransition::setRemoteControl()`;
  qtcpu800x600 `client/qtcpu800x600/ultimate/main.cpp` + `MainWindow::wireRemoteControl()`.
  Both gate the socket on `CliClientOptions::remoteControl` (set from
  `--remote-control`; cpu mirrors it from `AutoArgs::remoteControl`).

Commands (one per line; replies are one line each unless noted):
* `KEY <Up|Down|Left|Right|Return|Enter|Escape|Esc|Space>` — synthesised key (arrows move, Enter acts/talks, Escape closes the dialog).
* `CLICKTILE <x> <y>` — click that tile on the CURRENT map → walk there / face+act / teleport (pathfinding).
* `CLICKPIXEL <px> <py>` — click that viewport pixel (lower level than CLICKTILE).
* `GOTO <mapIndex|mapBasename> <x> <y>` — pathfind to a tile on a given map (numeric index, or basename like `gym`/`gym.tmx`). Works for the current map and border-connected maps; door/teleporter-connected maps are NOT walk-reachable — click the door tile with CLICKTILE instead.
* `CLOSEDIALOG` — close the in-game dialog box (same as Escape).
* `GETSTATE` → `STATE map=<id> x=<x> y=<y> dir=<d>`.
* `GETDIALOG` → `DIALOG <text>` — the open dialog/sign text, empty when none. Read-only observation (used to assert a sign actually opened).
* `GETINVENTORY` → `INVENTORY <itemId>:<qty> ...`.
* `SENDCHAT <local|all|clan|pm|system|system_important> <text...>` / `GETCHAT`.
* Trade: `TRADEREQUEST <pseudo>` / `TRADEACCEPT` / `TRADEREFUSE` / `TRADEADDCASH <n>` / `TRADEADDITEM <id> <qty>` / `TRADEADDMONSTER <pos>` / `TRADEFINISH` / `TRADECANCEL` / `GETTRADE`.
* Fight: `FIGHTREQUEST <pseudo>` / `FIGHTACCEPT` / `FIGHTREFUSE` / `FIGHTSKILL <id>` / `FIGHTCHANGEMONSTER <pos>` / `FIGHTITEM <id> [monsterPos]` / `FIGHTESCAPE` / `GETFIGHT` → `... inFight=<0|1> count=<n>`.

Server-pushed events (chat, trade, fight) are buffered and drained by the
matching `GET*` command, so a controller polls them — this is how server→client
events reach the connected controller.

**Writing a test against the channel:** launch the client with `--remote-control`
(plus `--autosolo` for solo, or `--host/--port` for multiplayer) and an isolated
`TMPDIR`/`HOME`, wait for the `MapVisualiserPlayer::mapDisplayedSlot()` marker,
connect to `<TMPDIR>/CatchChallenger-Client-0-<uid>`, then send commands and
parse replies. Reference implementations live in `test/`:
`test/testingclient.py:run_qtopengl_sign_dialog_channel_test()` (solo, qtopengl)
and `test/testingmulti.py` (`_channel_connect`/`_channel_cmd` helpers + the
teleport-on-push and crash-regression channel runs, multiplayer). The map fixture
coordinates these tests click are catalogued in the datapack at
`map/main/test/TOTEST.md`.
