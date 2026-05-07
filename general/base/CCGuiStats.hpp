#ifndef CATCHCHALLENGER_GUI_STATS_H
#define CATCHCHALLENGER_GUI_STATS_H

// Live-stats hookup for the Qt admin GUI (server-gui).  ALL of this
// header is gated behind CATCHCHALLENGER_GUI_STATS so the headless
// server / epoll server / login server compile with zero overhead and
// zero Qt dependency.  server-gui's CMakeLists defines the macro on
// the catchchallenger-server-gui target only.
//
// The point of having this in general/base/ — outside server/qt/ — is
// that the COUNTERS get bumped from the engine's own non-Qt code
// (ClientList, DB layer, broadcast loop) without those modules taking
// a Qt dependency.  Only server/qt/gui/MainWindow.cpp READS the
// counters, and that's allowed to use Qt freely.
//
// Two flavours of state exposed:
//   - simple atomic counters (player count, query counters, latency)
//   - bounded MPSC queues for human-readable event lines
//     (textEditAnalytics) and captured stdout/stderr lines
//     (textEditConsole)
//
// Everything is plain <atomic> + <mutex> + std::deque so the rest of
// the codebase can include this header without dragging Qt in.

#ifdef CATCHCHALLENGER_GUI_STATS

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace CatchChallenger {
namespace gui_stats {

// ── connected-player counters ────────────────────────────────────────
// Bumped from server/base/Client.cpp on accept/disconnect.  The Qt GUI
// reads this once per timer tick — no need for a callback.
extern std::atomic<uint64_t> players_connected;
// Cumulative login count since process start.  Useful for spotting
// churn (connected can plateau while logged-in keeps climbing).
extern std::atomic<uint64_t> players_logged_total;

// ── latency (ms) — populated by the engine when it measures a tick or
// ── per-client RTT.  GUI shows the rolling avg.
extern std::atomic<uint64_t> player_latency_ms_avg;
extern std::atomic<uint64_t> player_latency_ms_min;
extern std::atomic<uint64_t> player_latency_ms_max;

// ── DB activity — total queries observed and most-recent latency.
// QtDatabase already tracks per-connection stats; this surface is the
// non-Qt facing copy so the headless / epoll DB layer can also feed
// the same counters.  GUI reads delta since last tick.
extern std::atomic<uint64_t> db_query_total;
extern std::atomic<uint64_t> db_query_running;
extern std::atomic<uint64_t> db_query_last_duration_ns;

// ── network counters (already tracked in server/qt/QtClient for the
// Qt server; epoll mirrors them here when CATCHCHALLENGER_GUI_STATS is
// on).  Bytes are cumulative; the GUI converts to per-tick deltas.
extern std::atomic<uint64_t> net_bytes_received;
extern std::atomic<uint64_t> net_bytes_sent;

// ── analytics queue (textEditAnalytics) ──────────────────────────────
// Engine code calls push_event() whenever something interesting
// happens (player joined, caught a monster, traded, etc.).  The Qt GUI
// drains the queue and renders each entry as
//
//     [HH:MM:SS]  <player>  <text>
//
// — the time-format conversion lives in MainWindow.cpp because it's
// Qt-only (QDateTime), but the timestamp itself is a plain time_t
// captured here so engine code stays Qt-free.
//
// Bounded ring; oldest entries are dropped on overflow so a runaway
// producer can't OOM the GUI process.  push_event() is safe from any
// thread.
struct EventLine {
    std::string text;       // event description, e.g. "caught a Pikachu"
    std::string player;     // player pseudo, may be empty for system events
    bool        bold;       // bold = "highlight this in the GUI"
    // No timestamp: the [HH:MM:SS] prefix is generated GUI-side in
    // server/qt/gui/MainWindow.cpp at drain time.  Non-Qt producer
    // code stays time-agnostic so it can't drift if the engine ever
    // batches events.
};

void push_event(const std::string &text,
                const std::string &player = std::string(),
                bool bold = false);

// Drain up to `max` events; returns them in submission order.
std::deque<EventLine> drain_events(size_t max = 64);

}  // namespace gui_stats
}  // namespace CatchChallenger

#endif  // CATCHCHALLENGER_GUI_STATS

#endif  // CATCHCHALLENGER_GUI_STATS_H
