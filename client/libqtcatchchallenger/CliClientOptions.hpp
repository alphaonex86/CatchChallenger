#ifndef CLICLIENTOPTIONS_HPP
#define CLICLIENTOPTIONS_HPP

#include <QString>
#include <cstdint>

// Client-only command-line options shared between the qtopengl and
// (eventually) other Qt client front-ends. Do NOT use from server code.
class CliClientOptions
{
public:
    // Auto-select a server entry from the multiplayer list by its name.
    // Empty = no auto-selection; user picks manually in the UI.
    static QString serverName;

    // Direct-connect host (IP or DNS). Used together with `port` to bypass
    // the server list entirely. Empty = use the server list / serverName.
    // TCP-only: meaningful only when CATCHCHALLENGER_NO_TCPSOCKET is NOT
    // defined. The WebSocket counterpart is `name` + `url`.
    static QString host;

    // Direct-connect port. 0 = no direct connect (fall back to serverName).
    // TCP-only: see `host` above.
    static uint16_t port;

    // WebSocket direct-connect URL (`ws://...` or `wss://...`). Used to
    // bypass the server list entirely, like `host`+`port` does for TCP.
    // Empty = no direct connect. Meaningful only when
    // CATCHCHALLENGER_NO_WEBSOCKET is NOT defined.
    static QString url;

    // After the server is reached and credentials are filled, automatically
    // press the login button instead of waiting for the user.
    static bool autologin;

    // Auto-select a character on the character-selection screen by pseudo,
    // and pre-fill the new-character form with this name. Empty = manual.
    static QString characterName;

    // Quit the client ~1 second after the first map finishes loading.
    // Used by smoke tests to verify the login -> map flow without keeping
    // the process alive.
    static bool closeWhenOnMap;

    // Once on the map, toggle movement direction every second and quit
    // after this many seconds. 0 = disabled. Used by movement / soak tests.
    static int closeWhenOnMapAfter;

    // Once on the map, drop all outgoing network traffic. Lets tests verify
    // server-side timeout / disconnection handling without closing the socket
    // cleanly from the client side.
    static bool dropSendDataAfterOnMap;

    // Auto-launch a single-player solo game. If no playable savegame exists,
    // create one. A 10s watchdog dumps the character / map state and exits
    // if the solo session fails to reach the map.
    static bool autosolo;

    // Once on the map, save a PNG screenshot of the rendered viewport to this
    // path, then exit. Empty = disabled. Implies a fixed RNG seed so random
    // tile variants (lava etc.) and animation offsets are reproducible across
    // runs — used by testingmap4client.py for visual-regression diffing.
    static QString takeScreenshotPath;

    // Override the auto-selected maincode under datapack/internal/map/main/.
    // Empty = autosolo picks the first directory alphabetically. Used by
    // testingmap4client.py to force the "test" maincode fixture.
    static QString mainDatapackCodeOverride;

    // TEST-ONLY: --autosolo=DX,DY — once on the map, simulate a click on the tile
    // at the player's position + (DX,DY). Lets you build a predefined scenario:
    // pick an offset landing on a Sign/NPC to watch the player walk up to it, turn
    // to FACE it and open it. Emits "[CLICKTEST] ..." markers; does NOT quit
    // (combine with --closewhenonmapafter=N to auto-exit).
    static bool autosoloClick;
    static int autosoloClickDx,autosoloClickDy;

    // TEST-ONLY: once on the map, find the nearest Sign/NPC, simulate a click
    // on it and verify the player walks up to it, turns to FACE it and opens it
    // like Enter was pressed. Emits "[SIGNTEST] ..." markers (PASS/FAIL) and
    // quits. Drives the click-to-sign behaviour test for both clients.
    static bool clickSignTest;

    // TEST-ONLY: once on the map, push a very long, server-style text into the
    // Sign/NPC dialog and verify it never spills out of the widget — it must
    // word-wrap within the width, stay inside the (window-bounded) dialog and
    // show a vertical scrollbar when taller than the dialog. Emits
    // "[OVERFLOWTEST] ..." markers (PASS/FAIL) and quits.
    static bool dialogOverflowTest;

    // Freeze the animated CCBackground (clouds drifting, grass swaying,
    // tree-front / tree-back parallax cycles). The background is still
    // drawn but every per-frame timer is stopped, so repeated runs of
    // --take-screenshot=PATH produce byte-identical RGBA buffers (modulo
    // PNG-compressor non-determinism). Used by testingcompilation*.py to
    // diff against reference screenshots without flapping on a different
    // grass / cloud animation frame each run.
    static bool fixedBackground;
};

#endif // CLICLIENTOPTIONS_HPP
