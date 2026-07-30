#ifndef CATCHCHALLENGER_CLI_CLILATENCY_H
#define CATCHCHALLENGER_CLI_CLILATENCY_H

// HEADLESS: yes (POSIX sockets + select(), no display, no Qt)
//
// Client-side latency instrumentation for the Qt-FREE benchmark client
// (tools/bot-bench). The WHOLE file is compiled in only when
// CATCHCHALLENGER_BENCHMARK is set: with the define OFF this header expands to
// nothing, so a normal build of bot-bench carries none of this code and none of
// its cost. The shared client lib (client/libcatchchallenger) is never touched
// -- we only hook the notifications CliApiClient already overrides.
//
// It is the Qt-free twin of tools/bot-actions/LatencyRecorder.{h,cpp} and emits
// the SAME "BENCH <metric>_lat_hist_<i>=<count>" contract, so
// benchmark_helpers.parse_client_bench_stdout() reads either one unchanged.
// The reason it exists: bot-actions links Qt6 Widgets, so it can only run on
// the orchestrating host, which forces the measuring client OFF the hardware
// under test. This one runs next to the server on every board of the fleet.
//
// All bots live in ONE process, so a single CLOCK_MONOTONIC reading is a clock
// shared by sender and receiver: timestamping an event on bot A and the
// matching event on bot B and subtracting gives the true server-relay +
// propagation latency as the client perceives it.
//
// Three client-perceived latencies, ns, lower-is-better, dumped as log2
// buckets (bucket i = [2^i, 2^(i+1)) ns -- identical to the server-side
// BENCH_LAT_BUCKETS, so benchmark*.py reuses the same percentile math):
//   chat -- our tagged local chat reaches ANOTHER bot (map relay latency =
//           "time to send a message to another player")
//   rtt  -- our tagged local chat echoes back to its OWN sender (round trip =
//           "query-with-reply latency"; the server echoes local chat to the
//           speaker)
//   join -- a bot reaches the map (haveCharacter) -> another already-present
//           bot sees it appear (insert_player) = "see another player update"
// chat and join need at least TWO bots on the same map; rtt works with one.

#ifdef CATCHCHALLENGER_BENCHMARK

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace CatchChallenger {

class CliApiClient;

class CliLatency
{
public:
    enum Metric : uint8_t
    {
        Metric_chat = 0,
        Metric_join = 1,
        Metric_rtt  = 2,
        Metric_COUNT = 3
    };
    CliLatency();
    ~CliLatency();

    /// \brief the single recorder, or NULL when --latency was not passed.
    /// The hooks in CliApiClient test this, which is what keeps a normal
    /// benchmark run (--spam, plain connect) free of any latency work.
    static CliLatency *instance();
    static void setInstance(CliLatency *recorder);

    /// \brief enrol one bot for the chat driver. Call before it logs in: the
    /// clock already runs, so its join is timed too.
    void registerClient(CliApiClient *client);

    //---- hooks called from CliApiClient's notification overrides --------
    /// \brief this bot reached the map = t0 of its own join visibility
    void onOwnMapPlacement(CliApiClient *client);
    /// \brief `pseudo` appeared in this bot's view
    void onOtherPlayerInserted(CliApiClient *client,const std::string &pseudo);
    /// \brief a chat line arrived at this bot; only our tagged probes count
    void onChat(CliApiClient *client,const std::string &text,const std::string &pseudo);

    /// \brief send one chat probe per bot whose cooldown elapsed. Called from
    /// the event loop, never from inside the parser.
    void sendDueProbes();
    /// \brief emit the "BENCH ..." lines on stdout. Called from the event
    /// loop after the measurement window, so std::cout is safe here.
    void dumpBench() const;
    /// \brief monotonic ns since this recorder was constructed
    int64_t nowNs() const;
private:
    int bucketOf(const int64_t &ns) const;
    void record(const Metric &metric,const int64_t &deltaNs);
    void dumpOne(const char * const prefix,const Metric &metric) const;

    /// \brief CLOCK_MONOTONIC reading at construction; every timestamp is
    /// relative to it so the values embedded in a chat probe stay small.
    int64_t startNs;
    std::vector<CliApiClient *> clients;
    uint64_t hist[Metric_COUNT][48];
    uint64_t count[Metric_COUNT];
    /// \brief per-bot earliest next probe (ns). A cooldown, not a tick rate:
    /// it keeps the traffic under the server chat anti-flood limit so a probe
    /// can never be the reason a bot is kicked.
    std::map<CliApiClient *,int64_t> nextChatNs;
    /// \brief pseudo -> ns when that bot received its OWN map placement = join t0
    std::map<std::string,int64_t> joinSendNs;
};

}

#endif // CATCHCHALLENGER_BENCHMARK

#endif // CATCHCHALLENGER_CLI_CLILATENCY_H
