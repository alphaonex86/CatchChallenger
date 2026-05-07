#include "CCGuiStats.hpp"

#ifdef CATCHCHALLENGER_GUI_STATS

namespace CatchChallenger {
namespace gui_stats {

std::atomic<uint64_t> players_connected{0};
std::atomic<uint64_t> players_logged_total{0};
std::atomic<uint64_t> player_latency_ms_avg{0};
std::atomic<uint64_t> player_latency_ms_min{0};
std::atomic<uint64_t> player_latency_ms_max{0};
std::atomic<uint64_t> db_query_total{0};
std::atomic<uint64_t> db_query_running{0};
std::atomic<uint64_t> db_query_last_duration_ns{0};
std::atomic<uint64_t> net_bytes_received{0};
std::atomic<uint64_t> net_bytes_sent{0};

namespace {
// Bounded ring of events.  256 entries × ~80 bytes each ≈ 20 KiB —
// negligible memory, plenty of headroom for the per-second tick the
// GUI uses to drain.  Drop-oldest on overflow keeps the producer fast
// (no allocation in the bump-and-overwrite path).
constexpr size_t EVENT_RING_CAPACITY = 256;
std::mutex events_mu;
std::deque<EventLine> events_ring;
}  // namespace

void push_event(const std::string &text,
                const std::string &player,
                bool bold)
{
    std::lock_guard<std::mutex> g(events_mu);
    if (events_ring.size() >= EVENT_RING_CAPACITY)
        events_ring.pop_front();
    events_ring.push_back(EventLine{text, player, bold});
}

std::deque<EventLine> drain_events(size_t max)
{
    std::deque<EventLine> out;
    std::lock_guard<std::mutex> g(events_mu);
    size_t take = events_ring.size();
    if (take > max)
        take = max;
    while (take > 0) {
        out.push_back(events_ring.front());
        events_ring.pop_front();
        --take;
    }
    return out;
}

}  // namespace gui_stats
}  // namespace CatchChallenger

#endif  // CATCHCHALLENGER_GUI_STATS
