#ifndef LATENCYRECORDER_H
#define LATENCYRECORDER_H

// HEADLESS: yes (runs offscreen, no widget interaction needed)
//
// Client-side latency instrumentation for the bot-actions benchmark build.
// The WHOLE file is compiled in only when CATCHCHALLENGER_BENCHMARK is set:
// with the define OFF this header expands to nothing, so a normal/dev build
// of bot-actions carries zero of this code and zero overhead. The shared
// client lib (client/libcatchchallenger, client/libqtcatchchallenger) is
// never touched -- we only connect to signals it already emits.
//
// All bots live in ONE process, so a single QElapsedTimer is a single
// monotonic clock shared by sender and receiver: timestamping an event on bot
// A and the matching event on bot B and subtracting gives the true
// server-relay + network propagation latency as the client perceives it.
//
// The latency driver issues its own deterministic, server-safe traffic
// (tagged local chat), throttled below the server anti-DDOS limits. It
// measures three client-perceived latencies:
//   chat -- our tagged local chat reaches ANOTHER bot (map relay latency =
//           "time to send a message to another player")
//   rtt  -- our tagged local chat echoes back to its OWN sender (round trip =
//           "query-with-reply latency"; the server echoes local chat to the
//           speaker)
//   join -- a bot reaches the map (QthaveCharacter) -> another already-present
//           bot sees it appear (Qtinsert_player) = "see another player update"
// Metrics are ns (lower is better), dumped on SIGINT as "BENCH <m>_lat_hist_<i>"
// log2 buckets (bucket i = [2^i, 2^(i+1)) ns -- identical to the server-side
// BENCH_LAT_BUCKETS so benchmark*.py reuses the same percentile math).

#ifdef CATCHCHALLENGER_BENCHMARK

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <csignal>

#include "../../client/libqtcatchchallenger/Api_client_real.hpp"

class LatencyRecorder;
// Set by main() when --latency is passed; read by MainWindow.
extern bool g_latencyMode;
// The single recorder instance (NULL unless latency mode).
extern LatencyRecorder *g_latencyRecorder;
// Measurement window in seconds (--latency-seconds, default 15). The bot times
// a FIXED window that starts only once every bot is on the map, so process
// startup (datapack load, N logins) is excluded from the measurement; then it
// dumps BENCH and exits on its own. 0 => no self-timer (wait for SIGINT).
extern int g_latencySeconds;
// Set by the SIGINT handler (async-signal-safe: a flag write only). A QTimer
// in MainWindow polls it, then dumps + exits from the event loop.
extern volatile std::sig_atomic_t g_latencySigint;

class LatencyRecorder : public QObject
{
    Q_OBJECT
public:
    enum Metric
    {
        Metric_chat = 0,
        Metric_join = 1,
        Metric_rtt  = 2,
        Metric_COUNT = 3
    };
    explicit LatencyRecorder(QObject *parent = NULL);

    // Connect one bot's signals to the recorder slots and enrol it for the
    // chat driver. Called once per bot from MainWindow::logged().
    void registerApi(CatchChallenger::Api_client_real *api);
    // Start the deterministic chat driver. Called once, when every bot is on
    // the map. (The clock runs from construction so join is timed too.) When
    // seconds>0 the recorder dumps BENCH + exits after that fixed window,
    // excluding all startup time from the measurement.
    void start(int seconds);
    // Emit "BENCH ..." lines on stdout. Called from the event loop (NOT the
    // signal handler), so std::cout is safe here.
    void dumpBench() const;

private slots:
    void onHaveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                         const COORD_TYPE &x, const COORD_TYPE &y,
                         const CatchChallenger::Direction &last_direction);
    void onInsertPlayer(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,
                        const CatchChallenger::Player_public_informations &player,
                        const SIMPLIFIED_PLAYER_ID_FOR_MAP &mapId,
                        const uint8_t &x, const uint8_t &y,
                        const CatchChallenger::Direction &direction);
    void onChat(const CatchChallenger::Chat_type &chat_type, const std::string &text,
                const std::string &pseudo, const CatchChallenger::Player_type &player_type);
    void doSend();
    void onMeasureDone();

private:
    int bucketOf(int64_t ns) const;
    void record(Metric m, int64_t deltaNs);
    void dumpOne(const char *prefix, Metric m) const;

    QElapsedTimer clock;
    QTimer sendTimer;
    QTimer finishTimer;
    std::vector<CatchChallenger::Api_client_real *> apis;
    uint64_t hist[Metric_COUNT][48];
    uint64_t count[Metric_COUNT];
    // per-bot earliest next chat send (ns), throttled below the server chat
    // anti-DDOS limit so probes never get the bot kicked.
    std::map<CatchChallenger::Api_protocol_Qt *, int64_t> nextChatNs;
    // pseudo -> monotonic ns when that bot received its OWN map placement
    // (QthaveCharacter) = join t0.
    std::map<std::string, int64_t> joinSendNs;
};

#endif // CATCHCHALLENGER_BENCHMARK

#endif // LATENCYRECORDER_H
