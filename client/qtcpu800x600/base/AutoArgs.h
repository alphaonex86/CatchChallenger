#ifndef AUTOARGS_H
#define AUTOARGS_H

#include <QString>
#include <cstdint>

class AutoArgs
{
public:
    // --server NAME: select an existing server-list entry by name (TCP or WS).
    static QString server;
    // --host + --port: TCP direct connect. Meaningful only when
    // CATCHCHALLENGER_NO_TCPSOCKET is NOT defined.
    static QString host;
    static uint16_t port;
    // --url: WebSocket direct connect. Meaningful only when
    // CATCHCHALLENGER_NO_WEBSOCKET is NOT defined.
    static QString url;
    static bool autologin;
    static QString character;
    static bool closeWhenOnMap;
    static int closeWhenOnMapAfter;
    static bool dropSendDataAfterOnMap;
    static bool autosolo;
    // --autosolo=DX,DY: once on the map, click the tile at player+(DX,DY).
    static bool autosoloClick;
    static int autosoloClickDx,autosoloClickDy;
    // TEST-ONLY: --test-clicksign / --test-dialogoverflow. Mirrored into the
    // shared CliClientOptions so the map controller / dialog self-tests run.
    static bool clickSignTest;
    static bool clickDoorTest;
    static bool keyboardTest;
    static bool dialogOverflowTest;
    static QString mainDatapackCodeOverride;
    // --take-screenshot=PATH: render the first frame after the map
    // is reached (or, on a no-autosolo run, 2 s after launch — the
    // title screen) into the named PNG and exit. Used by
    // testingcompilationwindows.py's wine screenshot regression to
    // diff against a blessed reference; pinning srand(42) when the
    // flag is set keeps random tile-variant selections deterministic.
    static QString takeScreenshotPath;
    // --remote-control: open the QLocalServer automation channel (mirrored into
    // CliClientOptions::remoteControl). OFF by default — the socket is created
    // only with this flag. See client/dev.md.
    static bool remoteControl;
    // Note: qtopengl has a --fixed flag that freezes its animated
    // CCBackground. qtcpu800x600's UI is static — no parallax, no
    // cloud/grass timers — so the equivalent flag is intentionally
    // NOT mirrored here; the AutoArgs parser would reject an
    // unknown flag and the harness has to know which client it's
    // talking to (testingcompilationwindows.py drops --fixed from
    // the qtcpu800x600 invocation for this reason).

    static void parse(int &argc, char *argv[]);
    static void printHelp(const char *progName);
};

#endif // AUTOARGS_H
