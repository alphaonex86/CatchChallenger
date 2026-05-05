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
};

#endif // CLICLIENTOPTIONS_HPP
