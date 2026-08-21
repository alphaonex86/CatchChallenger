// test/testingmapmanagement/main.cpp — driver for the testing
// harness. Sets up a stub Client/ClientList world (see Stubs.hpp),
// instantiates the real MapVisibilityAlgorithm (compiled with
// -DCATCHCHALLENGER_TESTING from server/base/MapManagement/), runs a
// fixed set of scenarios designed to cover every branch in min_CPU(),
// min_balanced() and MapServer::playerToFullInsert(), and checks the
// bytes pushed through Client::sendRawBlock() against a tiny
// Api_protocol mirror (parsing 0x6C/0x65/0x6B/0x66/0x69/0xE3 exactly
// the way client/libcatchchallenger/Api_protocol_message.cpp does it).
// The mirror feeds a MapControllerMP-shaped OtherPlayer dict; after
// every tick we diff that dict against the server's authoritative
// map_clients_id, ignoring players whose pingCountInProgress()>0 (the
// stale-data window).
//
// Deterministic by construction: no time, no rand, no clock, no
// network, no event loop. Output line `PASS scenario_name` / `FAIL
// scenario_name <detail>` is consumed by testingmapmanagement.py.

#include "Stubs.hpp"
#include "TestApiProtocol.hpp"
#include "../../server/base/MapManagement/MapVisibilityAlgorithm.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace CatchChallenger;

// ---- Test state -----------------------------------------------------

static int g_pass = 0;
static int g_fail = 0;

static void pass_line(const std::string &name)
{
    std::cout << "PASS " << name << std::endl;
    g_pass++;
}

static void fail_line(const std::string &name, const std::string &detail)
{
    std::cout << "FAIL " << name << " " << detail << std::endl;
    g_fail++;
}

// ---- Api_protocol mirror -------------------------------------------
//
// Replaced by TestApiProtocol (declared in TestApiProtocol.hpp) — the
// REAL production Api_protocol subclass. parseIncommingDataRaw() feeds
// captured server bytes into the production framing layer, which
// dispatches to Api_protocol_message.cpp::parseMessage() (the same
// 0x6B/0x66/0x69/0x6C/0x65 payload parsing the production client
// uses). When the protocol or any of these payload formats changes,
// the test inherits the new behaviour at the next build — no
// test-side update needed.
//
// MapControllerMpStub (now declared in TestApiProtocol.hpp) receives
// the parsed events through the virtual callbacks
// (insert_player / remove_player / reinsert_player /
// full_reinsert_player / dropAllPlayerOnTheMap) that
// TestApiProtocol forwards.

static bool dontSendPseudo()
{
    return CommonSettingsServer::commonSettingsServer.dontSendPseudo;
}


// Diff helper: compare an observer's view (focus client's
// MapControllerMpStub::otherPlayerList) against the server-side
// authoritative map_clients_id, excluding the focus slot itself.
// Returns "" when in agreement, else a human-readable diff string.
static std::string diffView(const MapControllerMpStub &mc,
                            const std::vector<PLAYER_INDEX_FOR_CONNECTED> &map_clients_id,
                            uint8_t focusSlot,
                            ClientList &cl)
{
    std::unordered_set<uint8_t> expectedSlots;
    for(size_t idx = 0; idx < map_clients_id.size() && idx < 255; idx++)
    {
        if(idx == focusSlot) continue;
        if(map_clients_id[idx] == PLAYER_INDEX_FOR_CONNECTED_MAX) continue;
        expectedSlots.insert(static_cast<uint8_t>(idx));
    }
    if(expectedSlots.size() != mc.otherPlayerList.size())
    {
        std::ostringstream oss;
        oss << "slot_count_mismatch expected=" << expectedSlots.size() << " got=" << mc.otherPlayerList.size();
        return oss.str();
    }
    for(uint8_t slot : expectedSlots)
    {
        auto it = mc.otherPlayerList.find(slot);
        if(it == mc.otherPlayerList.end())
        {
            std::ostringstream oss;
            oss << "missing_slot=" << static_cast<unsigned>(slot);
            return oss.str();
        }
        const Client &server = cl.at(map_clients_id[slot]);
        const OtherPlayerView &view = it->second;
        if(view.x != server.getX() || view.y != server.getY() || view.direction != server.getLastDirection())
        {
            std::ostringstream oss;
            oss << "pos_or_dir_mismatch slot=" << static_cast<unsigned>(slot)
                << " server=(" << static_cast<unsigned>(server.getX()) << "," << static_cast<unsigned>(server.getY()) << "," << static_cast<unsigned>(server.getLastDirection()) << ")"
                << " client=(" << static_cast<unsigned>(view.x) << "," << static_cast<unsigned>(view.y) << "," << static_cast<unsigned>(view.direction) << ")";
            return oss.str();
        }
        if(!dontSendPseudo() && view.info.pseudo != server.public_and_private_informations.public_informations.pseudo)
        {
            std::ostringstream oss;
            oss << "pseudo_mismatch slot=" << static_cast<unsigned>(slot) << " server=" << server.public_and_private_informations.public_informations.pseudo << " client=" << view.info.pseudo;
            return oss.str();
        }
    }
    return std::string();
}

// ---- Fixture --------------------------------------------------------
//
// Reusable bootstrap. Owns the ClientList + clients + the
// MapVisibilityAlgorithm-under-test. Note: MVA is a class with static
// per-process state (tempBigBufferForChanges / tempBigBufferForRemove
// / flat_map_list) — but that's fine, we only instantiate it via the
// public API exactly like production.

// Subclass exposing the protected `map_clients_id` so Fixture can
// snapshot the authoritative server slot list when diffing against
// the focus observer's view, and also lets scenario_send_helpers_guards
// drive send_reinsertAll[_WithFilter] past the production outer
// guards.
class HarnessMVA : public MapVisibilityAlgorithm
{
public:
    using MapVisibilityAlgorithm::send_reinsertAll;
    using MapVisibilityAlgorithm::send_reinsertAllWithFilter;
    using MapVisibilityAlgorithm::map_clients_id;
    using MapVisibilityAlgorithm::map_removed_index;
};

// Map slot (index into map_clients_id) currently holding this global id,
// -1 if absent. Needed once a scenario reuses a freed slot, because then
// the wire slot no longer equals the client's creation order.
static int slot_of_gid(const HarnessMVA &mva, PLAYER_INDEX_FOR_CONNECTED gid)
{
    size_t i = 0;
    while(i < mva.map_clients_id.size())
    {
        if(mva.map_clients_id[i] == gid)
            return static_cast<int>(i);
        i++;
    }
    return -1;
}

// n-th live slot strictly above slot 0 (slot 0 is the focus and never
// leaves), -1 when there are fewer than n+1 of them.
static int oracle_live_slot(const HarnessMVA &mva, unsigned int n)
{
    size_t i = 1;
    while(i < mva.map_clients_id.size())
    {
        if(mva.map_clients_id[i] != PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            if(n == 0)
                return static_cast<int>(i);
            n--;
        }
        i++;
    }
    return -1;
}

// ---- HARD RULE: min_balanced must never tell a client about ITSELF ----
//
// min_CPU broadcasts every player to everyone and relies on the CLIENT to
// drop its own entry, which it can do because min_CPU sends 0x6C carrying
// "you are slot N" (Api_protocol::playerExcludeIndex).
//
// min_balanced does NOT send 0x6C. playerExcludeIndex therefore stays at its
// 255 default for the whole session, so the client filters nothing: if the
// server ever emitted a recipient's own slot, that client would insert a
// second copy of itself standing on its own head, and every later 0x66 for
// that slot would move the ghost instead of being ignored. The server is
// the ONLY thing preventing it.
//
// So this walks every packet of every block and rejects any 0x6B / 0x69 /
// 0x66 entry carrying the recipient's own slot. It is wired into
// runMinNetwork(), so it holds for EVERY client in EVERY scenario, not just
// a dedicated one -- that is what makes it a rule rather than a test.
static std::string selfEntryViolation(const std::vector<char> &b, uint8_t ownSlot)
{
    size_t pos = 0;
    while(pos < b.size())
    {
        const uint8_t code = static_cast<uint8_t>(b[pos]);
        if(code == 0x65)        // drop all: code only
            pos += 1;
        else if(code == 0xE3)   // ping: code + query number
            pos += 2;
        else if(code == 0x6C)   // "you are slot N" -- carries self BY DESIGN
            pos += 2;
        else if(code == 0x69)   // remove: [code][size4][count1][slots...]
        {
            if(pos + 6 > b.size()) return "truncated_0x69";
            const uint8_t count = static_cast<uint8_t>(b[pos + 5]);
            size_t e = pos + 6;
            unsigned int i = 0;
            while(i < count)
            {
                if(e >= b.size()) return "truncated_0x69_entry";
                if(static_cast<uint8_t>(b[e]) == ownSlot) return "self_in_0x69_remove";
                e += 1;
                i++;
            }
            pos = e;
        }
        else if(code == 0x66)   // changes: [code][size4][count1][slot,x,y,dir]*
        {
            if(pos + 6 > b.size()) return "truncated_0x66";
            const uint8_t count = static_cast<uint8_t>(b[pos + 5]);
            size_t e = pos + 6;
            unsigned int i = 0;
            while(i < count)
            {
                if(e + 4 > b.size()) return "truncated_0x66_entry";
                if(static_cast<uint8_t>(b[e]) == ownSlot) return "self_in_0x66_change";
                e += 4;
                i++;
            }
            pos = e;
        }
        else if(code == 0x6B)   // insert: [code][size4][maps1][mapid2][count1][entries]
        {
            if(pos + 9 > b.size()) return "truncated_0x6B";
            if(static_cast<uint8_t>(b[pos + 5]) != 0x01) return "unexpected_map_count";
            const uint8_t count = static_cast<uint8_t>(b[pos + 8]);
            size_t e = pos + 9;
            unsigned int i = 0;
            while(i < count)
            {
                if(e >= b.size()) return "truncated_0x6B_entry";
                if(static_cast<uint8_t>(b[e]) == ownSlot) return "self_in_0x6B_insert";
                e += 1;                 // slot
                e += 3;                 // x, y, direction|type
                if(!dontSendPseudo())   // pseudo is length-prefixed when sent
                {
                    if(e >= b.size()) return "truncated_pseudo_len";
                    e += 1 + static_cast<uint8_t>(b[e]);
                }
                e += 1;                 // skin
                e += 2;                 // followed monster
                i++;
            }
            pos = e;
        }
        else
            return "unknown_packet_code";
    }
    return std::string();
}


class Fixture
{
public:
    ClientList cl;
    HarnessMVA mva;
    std::vector<ClientWithMap *> owned;
    // Persistent observer for the focus client. The brief calls for
    // exactly 1 Api_protocol instance / 1 player view, so we keep one
    // observer + one Api_protocol instance that accumulates state
    // across ticks. setFocus() can be called by scenarios that want a
    // different focus client.
    MapControllerMpStub observer;
    TestApiProtocol api;
    // focus_slot indexes map_clients_id (the wire slot); focus_owned
    // indexes owned[] (creation order). They are the same number only
    // while the map has never reused a freed slot -- insertOnMap pops
    // the LIFO free list, so after any leave+join the joiner's wire slot
    // is BELOW its creation order. Scenarios that provoke that must set
    // both (setFocus(slot,ownedIndex)).
    uint8_t focus_slot = 0;
    uint8_t focus_owned = 0;
    // Last sync-check result (filled by runMinCpu/runMinNetwork). "" =
    // observer matches server. "skip_ping" = focus has pingCountInProgress()>0
    // so the diff was intentionally skipped per the brief. Any other
    // value is a real failure for the caller to report.
    std::string last_sync_status;

    Fixture()
    {
        ClientList::list = &cl;
        api.map_controller = &observer;
        // ProtocolParsing's internal flags govern whether
        // parseIncommingDataRaw will accept dynamic-size packets
        // (0x6B/0x66/0x69 use 0xFE size markers). The bit is normally
        // set by ProtocolParsingInputOutput::write() before SENDING;
        // for our INPUT-driven test we set it explicitly.
        api.allowDynamicSizeForTest();
        GlobalServerData::serverSettings.mapVisibility.simple.max = 200;
        GlobalServerData::serverSettings.dontSendPlayerType = false;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo = false;
    }
    ~Fixture()
    {
        for(ClientWithMap *c : owned) delete c;
        owned.clear();
        cl.clear();
        ClientList::list = nullptr;
    }

    void setFocus(uint8_t slot) { focus_slot = slot; focus_owned = slot; }
    void setFocus(uint8_t slot, uint8_t ownedIndex) { focus_slot = slot; focus_owned = ownedIndex; }

    // Create one stub client. Returns its slot index in MVA's
    // map_clients_id (the same as its ClientList global id, since the
    // test never sparsely populates beyond the focus scenarios).
    uint8_t addClient(const std::string &pseudo, COORD_TYPE x, COORD_TYPE y,
                      Direction d, uint32_t playerId,
                      CATCHCHALLENGER_TYPE_MAPID mapIdx, uint8_t skin = 1,
                      CATCHCHALLENGER_TYPE_MONSTER monsterFollow = 0,
                      Player_type t = Player_type_normal)
    {
        ClientWithMap *c = new ClientWithMap();
        c->setX(x); c->setY(y); c->setDirection(d);
        c->setPlayerId(playerId);
        c->setMapIndex(mapIdx);
        c->public_and_private_informations.public_informations.pseudo = pseudo;
        c->public_and_private_informations.public_informations.type = t;
        c->public_and_private_informations.public_informations.skinId = skin;
        if(monsterFollow != 0)
        {
            PlayerMonster pm;
            pm.monster = monsterFollow;
            pm.level = 1; pm.hp = 1; pm.catched_with = 0; pm.gender = Gender_Unknown;
            c->public_and_private_informations.monsters.push_back(pm);
        }
        owned.push_back(c);
        PLAYER_INDEX_FOR_CONNECTED gid = cl.add(c);
        mva.insertOnMap(gid);
        return static_cast<uint8_t>(gid);
    }

    // Model a link whose round trip fits inside one tick: every client
    // answers the ping it was sent last tick, exactly as production does on
    // the 0xE3 reply. A scenario wanting a SLOW client raises that client's
    // counter with setPing(n), so it stays held back for n ticks despite
    // this. Set ackEachTick=false to model a link that never answers.
    bool ackEachTick = true;
    void ackAllPings()
    {
        for(ClientWithMap *c : owned) c->ackPing();
    }

    void runMinCpu(CATCHCHALLENGER_TYPE_MAPID mapId)
    {
        if(ackEachTick) ackAllPings();
        for(ClientWithMap *c : owned) c->sentBlocks.clear();
        mva.min_CPU(mapId);
        last_sync_status = syncCheckFocus();
    }

    // Enforce the "never tell a client about itself" rule over EVERY client
    // on the map, not just the focus observer. Returns "" when clean.
    // min_CPU is deliberately exempt (it broadcasts self and pairs that with
    // the 0x6C the client filters on) -- see selfEntryViolation().
    std::string checkNoSelfSend()
    {
        size_t gid = 0;
        while(gid < owned.size())
        {
            const int slot = slot_of_gid(mva, static_cast<PLAYER_INDEX_FOR_CONNECTED>(gid));
            // slots >= 255 are not expressible in the 8-bit wire index, so
            // the rule cannot be stated for them
            if(slot >= 0 && slot < 255 && owned[gid] != NULL)
            {
                size_t bi = 0;
                while(bi < owned[gid]->sentBlocks.size())
                {
                    const std::string v = selfEntryViolation(owned[gid]->sentBlocks[bi].bytes,
                                                             static_cast<uint8_t>(slot));
                    if(!v.empty())
                    {
                        std::ostringstream oss;
                        oss << "selfrule:" << v << " client=" << gid << " slot=" << slot;
                        return oss.str();
                    }
                    bi++;
                }
            }
            gid++;
        }
        return std::string();
    }

    // Pre-tick snapshot, taken AFTER the ACKs are delivered so it is exactly
    // the state min_balanced() will see. uint8_t rather than vector<bool> to
    // avoid the bitset specialisation.
    std::vector<uint8_t> preTickPing;
    std::vector<uint8_t> preTickPath2;
    void snapshotPreTick()
    {
        preTickPing.resize(owned.size());
        preTickPath2.resize(owned.size());
        size_t i = 0;
        while(i < owned.size())
        {
            preTickPing[i] = owned[i]->pingCountInProgress();
            preTickPath2[i] = (owned[i]->sendedMap == owned[i]->mapIndex) ? 1 : 0;
            i++;
        }
    }

    // ---- HARD RULE: no new map state to a client that has not replied ----
    //
    // If the previous ping is still outstanding when the tick timer runs,
    // the link has not drained what it was already given. Pushing more at it
    // buys nothing -- the bytes queue behind the undelivered ones and are
    // stale by the time they arrive -- and on a 2G / TOR uplink that is how
    // a client falls permanently behind. Such a recipient must receive
    // NOTHING and be served one coalesced delta when it answers.
    //
    // A client changing map (PATH 1) is exempt: its whole view is invalid
    // and the full reload has to go out regardless.
    std::string checkNoSendWhileUnacked()
    {
        size_t i = 0;
        while(i < owned.size())
        {
            if(i < preTickPing.size() && preTickPing[i] > 0 && preTickPath2[i] != 0
               && owned[i] != NULL && !owned[i]->sentBlocks.empty())
            {
                std::ostringstream oss;
                oss << "ackrule:sent_while_unacked client=" << i
                    << " ping=" << static_cast<unsigned>(preTickPing[i])
                    << " blocks=" << owned[i]->sentBlocks.size();
                return oss.str();
            }
            i++;
        }
        return std::string();
    }

    void runMinNetwork(CATCHCHALLENGER_TYPE_MAPID mapId)
    {
        if(ackEachTick) ackAllPings();
        for(ClientWithMap *c : owned) c->sentBlocks.clear();
        snapshotPreTick();
        mva.min_balanced(mapId);
        last_sync_status = syncCheckFocus();
        // A rule breach outranks whatever the sync diff said: report it
        // through the same channel so every existing scenario enforces both
        // rules without needing its own assertion.
        std::string rule = checkNoSelfSend();
        if(rule.empty())
            rule = checkNoSendWhileUnacked();
        if(!rule.empty())
            last_sync_status = rule;
    }

    // Feed the focus client's newly captured bytes through the REAL
    // production Api_protocol framing layer (parseIncommingDataRaw)
    // and the production payload parser
    // (Api_protocol_message.cpp::parseMessage and friends). The
    // production code calls map_controller-> insert / reinsert /
    // remove / dropAll as it parses the 0x6B / 0x66 / 0x69 / 0x6C /
    // 0x65 packets, so when production payload formats change the
    // observer's view automatically reflects the new behaviour.
    //
    // Returns:
    //   ""             — observer matches server (success)
    //   "skip_ping"    — focus has pingCountInProgress()>0 so per the
    //                    brief we intentionally don't compare (the
    //                    observer's state may legitimately lag).
    //   other string   — diff failure or parse error (real bug).
    std::string syncCheckFocus()
    {
        if(focus_owned >= owned.size())
            return std::string();
        ClientWithMap *fc = owned[focus_owned];
        // Re-target the test's Api_protocol at our (single) observer so
        // insert_player / reinsert_player / remove_player /
        // dropAllPlayerOnTheMap callbacks land here.
        api.map_controller = &observer;
        for(const Client::CapturedBlock &b : fc->sentBlocks)
        {
            uint32_t cursor = 0;
            while(cursor < b.bytes.size())
            {
                int8_t rc = api.parseIncommingDataRaw(b.bytes.data(),
                                                     static_cast<uint32_t>(b.bytes.size()),
                                                     cursor);
                if(rc != 1)
                {
                    std::ostringstream oss;
                    oss << "parse:rc=" << static_cast<int>(rc) << " cursor=" << cursor
                        << "/" << b.bytes.size();
                    return oss.str();
                }
            }
        }
        if(fc->pingCountInProgress() > 0)
            return std::string("skip_ping");
        return diffView(observer, mva.map_clients_id, focus_slot, cl);
    }
};

// Helper to drop into any scenario: after a tick wrapped via
// Fixture::runMinCpu / runMinNetwork the last_sync_status is set.
// "" => OK. "skip_ping" => skipped per brief. Anything else => FAIL.
static bool sync_ok(const Fixture &f)
{
    return f.last_sync_status.empty() || f.last_sync_status == "skip_ping";
}

// ---- Scenarios ------------------------------------------------------
//
// Each scenario: short name, tightly scoped. testingmapmanagement.py
// scans the binary's stdout for `PASS <name>` / `FAIL <name>` lines;
// every scenario below logs one (or more) of those.

static void scenario_min_cpu_first_tick_three_players()
{
    const char *name = "min_cpu_first_tick_three_players";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1, 1, 11);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1, 2, 12);
    f.addClient("carol", 7, 7, Direction_look_at_left,   102, 1, 3, 13);

    f.runMinCpu(1);
    if(!sync_ok(f)) { fail_line(name, "sync:" + f.last_sync_status); return; }

    // Every player should receive at least one CapturedBlock starting with 0x6C.
    for(uint8_t focus = 0; focus < 3; focus++)
    {
        ClientWithMap *fc = f.owned[focus];
        if(fc->sentBlocks.empty()) { fail_line(name, "focus_no_blocks"); return; }
        if(static_cast<uint8_t>(fc->sentBlocks.front().bytes.at(0)) != 0x6C) { fail_line(name, "first_byte_not_0x6C"); return; }
    }
    // focus's persistent observer must have received exactly one ping
    if(f.observer.pingsObserved.size() != 1) { fail_line(name, "expected_one_ping"); return; }
    pass_line(name);
}

static void scenario_min_cpu_second_tick_same_map()
{
    const char *name = "min_cpu_second_tick_same_map";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1, 1, 11);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1, 2, 12);
    f.runMinCpu(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // Second tick: same map -> should NOT emit 0x6C.
    f.runMinCpu(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t2:" + f.last_sync_status); return; }
    for(uint8_t focus = 0; focus < 2; focus++)
    {
        ClientWithMap *fc = f.owned[focus];
        if(fc->sentBlocks.empty()) { fail_line(name, "focus_no_blocks_tick2"); return; }
        uint8_t first = static_cast<uint8_t>(fc->sentBlocks.front().bytes.at(0));
        if(first != 0x65) { fail_line(name, std::string("expected_0x65_got_0x") + (first < 16 ? "0" : "") + std::to_string(first)); return; }
    }
    pass_line(name);
}

static void scenario_min_cpu_ping_in_progress_skips_ping()
{
    const char *name = "min_cpu_ping_in_progress_skips_ping";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1, 1, 11);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1, 2, 12);
    f.owned[0]->setPing(3); // simulate buffer saturated: 3 outstanding pings
    f.runMinCpu(1);
    // focus(alice) has ping inflight — diff must be skipped per the
    // brief.
    if(f.last_sync_status != "skip_ping") { fail_line(name, "expected_skip_ping_got:" + f.last_sync_status); return; }
    // alice's stream must not include a 0xE3 byte
    for(const auto &b : f.owned[0]->sentBlocks)
        for(char ch : b.bytes)
            if(static_cast<uint8_t>(ch) == 0xE3) { fail_line(name, "alice_had_ping"); return; }
    // bob (no ping inflight) must have a ping
    bool bobHasPing = false;
    for(const auto &b : f.owned[1]->sentBlocks)
        for(char ch : b.bytes)
            if(static_cast<uint8_t>(ch) == 0xE3) { bobHasPing = true; break; }
    if(!bobHasPing) { fail_line(name, "bob_missing_ping"); return; }
    pass_line(name);
}

static void scenario_min_cpu_one_player_returns_early()
{
    const char *name = "min_cpu_one_player_returns_early";
    Fixture f;
    f.addClient("solo", 5, 5, Direction_look_at_bottom, 100, 1, 1, 11);
    f.runMinCpu(1);
    if(!sync_ok(f)) { fail_line(name, "sync:" + f.last_sync_status); return; }
    if(!f.owned[0]->sentBlocks.empty()) { fail_line(name, "expected_no_send"); return; }
    pass_line(name);
}

static void scenario_min_cpu_skip_ge_max()
{
    const char *name = "min_cpu_skip_ge_max";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 2;
    f.addClient("a", 1, 1, Direction_look_at_bottom, 100, 1);
    f.addClient("b", 2, 2, Direction_look_at_bottom, 101, 1);
    f.addClient("c", 3, 3, Direction_look_at_bottom, 102, 1);
    f.runMinCpu(1);
    // server skipped — observer is empty AND server has 2 others.
    // diffView would (correctly) flag this as a desync. The brief
    // doesn't mandate sync when the server intentionally skipped a
    // broadcast (no bytes were sent). Skip the diff for this case.
    // 3 clients >= max(2) -> skip
    for(const auto *c : f.owned)
        if(!c->sentBlocks.empty()) { fail_line(name, "expected_no_send"); return; }
    pass_line(name);
}

static void scenario_min_balanced_first_tick_path1()
{
    const char *name = "min_balanced_first_tick_path1";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);

    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync:" + f.last_sync_status); return; }
    for(uint8_t focus = 0; focus < 2; focus++)
    {
        ClientWithMap *fc = f.owned[focus];
        if(fc->sentBlocks.empty()) { fail_line(name, "focus_no_blocks"); return; }
        // First byte should be 0x65 (drop all) — PATH1
        if(static_cast<uint8_t>(fc->sentBlocks.front().bytes.at(0)) != 0x65) { fail_line(name, "expected_0x65_first"); return; }
    }
    pass_line(name);
}

static void scenario_min_balanced_path2_movement()
{
    const char *name = "min_balanced_path2_movement";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    uint8_t s1 = f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);

    // tick 1: PATH1 -> populate sendedStatus + observer
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // Move bob: x++, dir changed
    f.owned[1]->setX(7);
    f.owned[1]->setDirection(Direction_move_at_right);

    // tick 2: PATH2 -> should emit only 0x66 + 0xE3 (no 0x65/0x6B)
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t2:" + f.last_sync_status); return; }
    ClientWithMap *alice = f.owned[0];
    if(alice->sentBlocks.empty()) { fail_line(name, "alice_no_blocks_tick2"); return; }
    bool found66 = false;
    for(const auto &b : alice->sentBlocks)
    {
        if(b.bytes.size() > 0 && static_cast<uint8_t>(b.bytes[0]) == 0x6B)
        {
            fail_line(name, "unexpected_0x6B_in_path2"); return;
        }
        if(b.bytes.size() > 0 && static_cast<uint8_t>(b.bytes[0]) == 0x66) found66 = true;
        if(b.bytes.size() > 0 && static_cast<uint8_t>(b.bytes[0]) == 0x69) found66 = false; // change first if there's no insert -> 0x66
    }
    // Persistent observer state: bob should now be at (7,*) with new dir.
    auto it = f.observer.otherPlayerList.find(static_cast<uint8_t>(s1));
    if(it == f.observer.otherPlayerList.end()) { fail_line(name, "bob_lost_from_view"); return; }
    if(it->second.x != 7 || it->second.direction != Direction_move_at_right) {
        std::ostringstream oss;
        oss << "bob_state_wrong x=" << static_cast<unsigned>(it->second.x) << " dir=" << static_cast<unsigned>(it->second.direction);
        fail_line(name, oss.str()); return;
    }
    if(!found66) { fail_line(name, "no_0x66_emitted"); return; }
    pass_line(name);
}

static void scenario_min_balanced_path2_no_change_no_send()
{
    const char *name = "min_balanced_path2_no_change_no_send";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);
    f.runMinNetwork(1); // PATH1
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    f.runMinNetwork(1); // PATH2, no diff
    if(!sync_ok(f)) { fail_line(name, "sync_t2:" + f.last_sync_status); return; }
    // both clients should have NO bytes
    for(const auto *c : f.owned)
        if(!c->sentBlocks.empty()) { fail_line(name, "expected_no_send"); return; }
    pass_line(name);
}

// Flow control: a recipient that has not answered the previous ping is
// HELD BACK — it receives nothing at all, not merely a ping-less diff.
// Putting more bytes on a link that has not drained the last ones is the
// thing min_balanced exists to avoid, and they would be stale on arrival.
static void scenario_min_balanced_ping_inflight_blocks_state()
{
    const char *name = "min_balanced_ping_inflight_blocks_state";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob", 6, 6, Direction_look_at_right, 101, 1);
    f.addClient("carol", 7, 7, Direction_look_at_right, 102, 1);
    // first tick: PATH1 — focus (alice = slot 0) has no ping in
    // flight, observer catches up, sync diff must succeed.
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    // alice's link has not answered: 5 outstanding pings keep her held back
    // for the next ticks even though the driver ACKs one round per tick.
    // bob keeps answering and must keep being served normally. carol is the
    // one who moves, so bob has something to receive that is not himself.
    f.owned[0]->setPing(5);
    f.owned[2]->setX(8); f.owned[2]->setY(8); // move carol
    f.runMinNetwork(1);
    if(f.last_sync_status != "skip_ping") {
        fail_line(name, "expected_skip_ping_got:" + f.last_sync_status); return;
    }
    // alice must have received NOTHING: not a diff, not a ping.
    if(!f.owned[0]->sentBlocks.empty()) { fail_line(name, "held_client_got_bytes"); return; }
    // bob keeps up and must still be served normally.
    if(f.owned[1]->sentBlocks.empty()) { fail_line(name, "healthy_client_starved"); return; }
    pass_line(name);
}

// The two hard rules, stressed together over a busy map: no client is ever
// told about itself, and no client with an unanswered ping is given new map
// state. Both are enforced by runMinNetwork() on EVERY tick for EVERY
// client; this scenario exists so a regression names itself in the test
// list instead of only surfacing as a side effect somewhere else.
static void scenario_min_balanced_hard_rules_under_mixed_lag()
{
    const char *name = "min_balanced_hard_rules_under_mixed_lag";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 1000;
    unsigned int i = 0;
    while(i < 12)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "p%u", i);
        f.addClient(buf, static_cast<COORD_TYPE>(1 + i),
                    static_cast<COORD_TYPE>(2 + i),
                    Direction_look_at_bottom, 400 + i, 1);
        i++;
    }
    f.runMinNetwork(1);   // PATH1 for everyone
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // Every third client is on a slow link and stays held for several ticks;
    // the rest keep answering. Movement, a join and a leave run underneath.
    unsigned int tick = 0;
    while(tick < 8)
    {
        unsigned int k = 0;
        while(k < f.owned.size())
        {
            if((k % 3) == 0)
                f.owned[k]->setPing(3);            // slow link
            if(((k + tick) % 2) == 0)
                f.owned[k]->setX(static_cast<COORD_TYPE>(1 + ((k + tick) % 40)));
            k++;
        }
        if(tick == 3)
        {
            const int victim = oracle_live_slot(f.mva, 2);
            if(victim > 0)
                f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(victim));
        }
        if(tick == 5)
            f.addClient("late", 9, 9, Direction_look_at_top, 999, 1);
        f.runMinNetwork(1);
        // sync_ok covers BOTH hard rules: runMinNetwork folds a breach of
        // either into last_sync_status.
        if(!sync_ok(f))
        {
            std::ostringstream oss;
            oss << "tick" << tick << ":" << f.last_sync_status;
            fail_line(name, oss.str()); return;
        }
        tick++;
    }
    pass_line(name);
}

// ...and when it finally answers, it gets ONE delta carrying everything it
// missed, not one delta per tick it was away. This is the byte win of the
// hold-back: K ticks of movement collapse into a single packet whose
// entries hold the FINAL position, and a slot that appeared and vanished
// while the client was away costs nothing at all.
static void scenario_min_balanced_coalesced_delta_on_ack()
{
    const char *name = "min_balanced_coalesced_delta_on_ack";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    uint8_t s1 = f.addClient("bob", 6, 6, Direction_look_at_right, 101, 1);
    f.addClient("carol", 7, 7, Direction_look_at_right, 102, 1);
    f.runMinNetwork(1);   // PATH1 for everyone
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // alice stops answering (enough outstanding pings to stay held for the
    // whole window below); bob walks for 4 ticks.
    f.owned[0]->setPing(10);
    unsigned int t = 0;
    while(t < 4)
    {
        f.owned[1]->setX(static_cast<COORD_TYPE>(10 + t));
        f.runMinNetwork(1);
        if(!f.owned[0]->sentBlocks.empty()) { fail_line(name, "held_client_got_bytes"); return; }
        t++;
    }
    // A player joins and leaves entirely within the held window: alice
    // never needs to hear about him at all.
    f.addClient("ghost", 9, 9, Direction_look_at_top, 103, 1);
    f.runMinNetwork(1);
    const int ghost_slot = slot_of_gid(f.mva, 3);
    if(ghost_slot < 0) { fail_line(name, "setup_ghost"); return; }
    f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(ghost_slot));
    f.runMinNetwork(1);
    if(!f.owned[0]->sentBlocks.empty()) { fail_line(name, "held_client_got_bytes_2"); return; }

    // alice answers: exactly one tick's worth of bytes, carrying bob's
    // FINAL position, and no mention of the ghost.
    f.owned[0]->setPing(0);
    f.runMinNetwork(1);
    if(f.owned[0]->sentBlocks.empty()) { fail_line(name, "no_catchup_delta"); return; }
    if(!sync_ok(f)) { fail_line(name, "sync_after_ack:" + f.last_sync_status); return; }
    std::unordered_map<uint8_t, OtherPlayerView>::const_iterator it =
        f.observer.otherPlayerList.find(static_cast<uint8_t>(s1));
    if(it == f.observer.otherPlayerList.end()) { fail_line(name, "bob_missing"); return; }
    if(it->second.x != 13) {
        std::ostringstream oss;
        oss << "bob_not_at_final_x got=" << static_cast<unsigned>(it->second.x);
        fail_line(name, oss.str()); return;
    }
    if(f.observer.otherPlayerList.find(static_cast<uint8_t>(ghost_slot))
       != f.observer.otherPlayerList.end())
    { fail_line(name, "ghost_leaked_to_held_client"); return; }
    pass_line(name);
}

static void scenario_playerToFullInsert_combinations()
{
    const char *name = "playerToFullInsert_combinations";

    // For each combination of (dontSendPlayerType, dontSendPseudo,
    // monsters) drive a single insert and verify the buffer layout
    // matches what Api_protocol_message.cpp expects for 0x6B player
    // entries. We don't invoke MVA — just the static method directly.
    static const struct
    {
        bool dontSendPlayerType;
        bool dontSendPseudo;
        bool withMonster;
        const char *tag;
    } cases[] = {
        {false, false, false, "type_pseudo_nomon"},
        {true,  false, false, "notype_pseudo_nomon"},
        {false, true,  false, "type_nopseudo_nomon"},
        {true,  true,  false, "notype_nopseudo_nomon"},
        {false, false, true,  "type_pseudo_mon"},
        {true,  true,  true,  "notype_nopseudo_mon"},
    };

    for(const auto &c : cases)
    {
        Fixture f;
        GlobalServerData::serverSettings.dontSendPlayerType = c.dontSendPlayerType;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo = c.dontSendPseudo;
        f.addClient("zoe", 3, 4, Direction_look_at_bottom, 200, 1, 9, c.withMonster ? 77 : 0, Player_type_gm);

        char buf[1024];
        unsigned int n = MapServer::playerToFullInsert(*f.owned[0], buf);
        // Hand-decode expected layout
        // [x][y][dir|type][optional pseudo_len + pseudo][skin][monsterId LE2]
        unsigned int idx = 0;
        if(static_cast<uint8_t>(buf[idx]) != 3) { fail_line(name, std::string(c.tag) + ":bad_x"); return; }
        idx++;
        if(static_cast<uint8_t>(buf[idx]) != 4) { fail_line(name, std::string(c.tag) + ":bad_y"); return; }
        idx++;
        uint8_t dt = static_cast<uint8_t>(buf[idx]);
        uint8_t dirInt = dt & 0x0F;
        uint8_t typeInt = dt & 0xF0;
        if(dirInt != Direction_look_at_bottom) { fail_line(name, std::string(c.tag) + ":bad_dir"); return; }
        if(c.dontSendPlayerType)
        {
            if(typeInt != Player_type_normal) { fail_line(name, std::string(c.tag) + ":bad_type_force_normal"); return; }
        }
        else
        {
            if(typeInt != Player_type_gm) { fail_line(name, std::string(c.tag) + ":bad_type_should_be_gm"); return; }
        }
        idx++;
        if(!c.dontSendPseudo)
        {
            uint8_t plen = static_cast<uint8_t>(buf[idx]); idx++;
            if(plen != 3) { fail_line(name, std::string(c.tag) + ":bad_plen"); return; }
            if(std::string(buf + idx, plen) != "zoe") { fail_line(name, std::string(c.tag) + ":bad_pseudo"); return; }
            idx += plen;
        }
        uint8_t skin = static_cast<uint8_t>(buf[idx]); idx++;
        if(skin != 9) { fail_line(name, std::string(c.tag) + ":bad_skin"); return; }
        uint16_t monsterLE;
        std::memcpy(&monsterLE, buf + idx, sizeof(monsterLE));
        uint16_t monster = le16toh(monsterLE);
        idx += 2;
        if(c.withMonster)
        {
            if(monster != 77) { fail_line(name, std::string(c.tag) + ":bad_monster"); return; }
        }
        else
        {
            if(monster != 0) { fail_line(name, std::string(c.tag) + ":monster_nonzero"); return; }
        }
        if(idx != n) { fail_line(name, std::string(c.tag) + ":wrong_total_n"); return; }
    }
    pass_line(name);
}

static void scenario_send_helpers_guards()
{
    const char *name = "send_helpers_guards";
    HarnessMVA hmva;
    char buf[8192];
    // clients_size<=1 guards
    if(hmva.send_reinsertAll(1, buf, 0) != 0) { fail_line(name, "reinsertAll_le1_should_return_0"); return; }
    if(hmva.send_reinsertAllWithFilter(1, buf, 0, 0) != 0) { fail_line(name, "filter_le1_should_return_0"); return; }
    // skipped_id>=255 -> delegates to send_reinsertAll
    Fixture f;
    f.addClient("a", 1, 1, Direction_look_at_bottom, 100, 1);
    f.addClient("b", 2, 2, Direction_look_at_bottom, 101, 1);
    // Manually drive send_filter_skip_ge255 on hmva using the
    // fixture's client list — hmva and mva are unrelated instances;
    // we just need a populated map_clients_id.
    HarnessMVA &hm = static_cast<HarnessMVA&>(f.mva);
    if(hm.send_reinsertAllWithFilter(1, buf, 2, 999) == 0) { fail_line(name, "filter_skip_ge255_should_emit"); return; }
    pass_line(name);
}

// 255 clients -> clamp branch in both min_CPU and min_balanced, plus
// count_ge254 (full insert clamped), plus general "large map" path.
// Observer can't fully sync — the algorithm only sends the first 254
// players, so slots 254/255 are absent from the observer. Diff is
// expected to fail; the scenario passes because the algorithm
// returned a packet that the observer's clamped view CAN follow.
static void scenario_clamp_and_count_ge254()
{
    const char *name = "clamp_and_count_ge254";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 1000;
    // Insert 256 clients so map_clients_id.size() > 254 -> clamp.
    for(unsigned int i = 0; i < 256; i++)
    {
        char buf[16]; std::snprintf(buf, sizeof(buf), "p%u", i);
        f.addClient(buf, static_cast<COORD_TYPE>(i % 100), static_cast<COORD_TYPE>((i+1) % 100),
                    Direction_look_at_bottom, 1000 + i, 1);
    }
    f.runMinCpu(1);
    f.runMinNetwork(1);
    pass_line(name);
}

// Empty slot in map_clients_id: insert 3, remove the middle one. Hits
// min_cpu_slot_empty + send_reinsertAll_loop_empty in min_CPU; and
// min_net_slot_empty + send_filter_loop_skip + min_net_path1_status_empty
// in min_balanced PATH1. min_balanced must run BEFORE min_CPU on the
// same fixture so PATH1 still fires (min_CPU sets sendedMap which
// would then force PATH2 on the next min_balanced tick).
//
// NOTE: a sparse middle (slot 0 valid, slot 1 MAX, slot 2 valid) is NOT
// artificial -- it is the ordinary state between a player leaving and the
// next one joining, and insertOnMap's LIFO refill can leave it standing
// (see scenario_min_balanced_path1_hole_keeps_live_top_slot). So this
// scenario asserts state equivalence too: carol at slot 2 must reach the
// observer even though slot 1 is a hole.
static void scenario_empty_slot_in_map()
{
    const char *name = "empty_slot_in_map";
    Fixture f;
    f.addClient("alice", 1, 1, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   2, 2, Direction_look_at_bottom, 101, 1);
    f.addClient("carol", 3, 3, Direction_look_at_bottom, 102, 1);
    f.mva.removeOnMap(1); // bob's slot now PLAYER_INDEX_FOR_CONNECTED_MAX
    f.runMinNetwork(1);   // PATH1 hits status_empty for slot 1
    if(!sync_ok(f)) { fail_line(name, "sync_min_balanced:" + f.last_sync_status); return; }
    // No sync check after min_CPU here: min_balanced already set sendedMap,
    // so min_CPU takes its "same map as last tick" branch and skips the
    // 0x6C that carries the recipient's own slot. min_CPU relies on the
    // client filtering itself via playerExcludeIndex, which without that
    // 0x6C is still 255 -- so the observer legitimately keeps its own
    // entry. That is an artifact of driving BOTH algorithms over one
    // fixture (production picks one for the server's lifetime); the call
    // is kept only for branch coverage of send_reinsertAll's empty slot.
    f.runMinCpu(1);       // hits min_cpu_slot_empty + send_reinsertAll_loop_empty
    pass_line(name);
}

// min_balanced skip_ge_max branch — same shape as min_CPU skip_ge_max.
static void scenario_min_balanced_skip_ge_max()
{
    const char *name = "min_balanced_skip_ge_max";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 2;
    f.addClient("a", 1, 1, Direction_look_at_bottom, 100, 1);
    f.addClient("b", 2, 2, Direction_look_at_bottom, 101, 1);
    f.addClient("c", 3, 3, Direction_look_at_bottom, 102, 1);
    f.runMinNetwork(1);
    for(const auto *c : f.owned)
        if(!c->sentBlocks.empty()) { fail_line(name, "expected_no_send"); return; }
    pass_line(name);
}

// PATH1 with ping in flight on focus client -> ping_skip branch
// inside PATH1 (different code path from PATH2's ping_skip).
static void scenario_min_balanced_path1_ping_skip()
{
    const char *name = "min_balanced_path1_ping_skip";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);
    f.owned[0]->setPing(3);
    f.runMinNetwork(1);
    if(f.last_sync_status != "skip_ping") { fail_line(name, "expected_skip_ping_got:" + f.last_sync_status); return; }
    // alice (focus 0) should NOT have a ping byte 0xE3 anywhere
    for(const auto &b : f.owned[0]->sentBlocks)
        for(char ch : b.bytes)
            if(static_cast<uint8_t>(ch) == 0xE3) { fail_line(name, "alice_unexpected_ping"); return; }
    pass_line(name);
}

// Removal across PATH2 ticks: hits within_empty + empty_remove + has_remove
// on first PATH2 after a removal; then empty_already on the next.
// After the remove + PATH2 emits 0x69, the observer's view must drop
// bob — strict sync check.
static void scenario_min_balanced_path2_removal_sequence()
{
    const char *name = "min_balanced_path2_removal_sequence";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);
    f.addClient("carol", 7, 7, Direction_look_at_right,  102, 1);
    f.runMinNetwork(1); // tick1 PATH1, populates sendedStatus
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    f.mva.removeOnMap(1); // bob leaves
    f.runMinNetwork(1); // tick2: within_empty + empty_remove + has_remove
    if(!sync_ok(f)) { fail_line(name, "sync_t2_remove:" + f.last_sync_status); return; }
    f.runMinNetwork(1); // tick3: bob still gone; empty_already
    if(!sync_ok(f)) { fail_line(name, "sync_t3:" + f.last_sync_status); return; }
    pass_line(name);
}

// ---- Byte-exactness oracle -----------------------------------------
//
// min_balanced exists to keep the WIRE small (ADSL / 2G / TOR home
// servers), so a change to it has to be judged on the exact bytes it
// emits, not merely on whether the client view converges. This folds the
// entire output byte stream of a deterministic mixed workload into one
// digest: build the binary before a change, note the line, build it after,
// compare. Any difference in packet order, header, count, entry layout or
// volume moves the digest.
static uint64_t fnv1a_fold(uint64_t h, const char *d, size_t n)
{
    size_t i = 0;
    while(i < n)
    {
        h ^= static_cast<unsigned char>(d[i]);
        h *= 1099511628211ULL;
        i++;
    }
    return h;
}

static Direction oracle_dir(uint32_t r)
{
    switch(r & 0x3)
    {
        case 0:  return Direction_look_at_top;
        case 1:  return Direction_look_at_right;
        case 2:  return Direction_look_at_bottom;
        default: return Direction_look_at_left;
    }
}

static void scenario_min_balanced_byte_oracle()
{
    const char *name = "min_balanced_byte_oracle";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 1000;
    uint32_t rng = 0x5EEDu;
    unsigned int i = 0;
    while(i < 24)
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "p%u", i);
        f.addClient(buf, static_cast<COORD_TYPE>(1 + i % 40),
                    static_cast<COORD_TYPE>(1 + (i * 7) % 40),
                    Direction_look_at_bottom, 2000 + i, 1);
        i++;
    }
    uint64_t digest = 1469598103934665603ULL;
    uint64_t total = 0;
    unsigned int tick = 0;
    while(tick < 40)
    {
        // ~40% of players change position/direction each tick.
        unsigned int k = 0;
        while(k < f.owned.size())
        {
            rng = rng * 1664525u + 1013904223u;
            if(((rng >> 16) % 100) < 40)
            {
                f.owned[k]->setX(static_cast<COORD_TYPE>(1 + ((rng >> 8) % 40)));
                f.owned[k]->setDirection(oracle_dir(rng));
            }
            k++;
        }
        // Every 7th tick two players leave and one joins: exercises remove,
        // LIFO slot reuse, a surviving hole, and a live slot above it.
        if((tick % 7) == 6)
        {
            const int a = oracle_live_slot(f.mva, 1);
            const int b = oracle_live_slot(f.mva, 3);
            if(a > 0 && b > 0 && a != b)
            {
                f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(a));
                f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(b));
                char buf[16];
                std::snprintf(buf, sizeof(buf), "j%u", tick);
                f.addClient(buf, static_cast<COORD_TYPE>(2 + (tick % 30)),
                            static_cast<COORD_TYPE>(3 + (tick % 20)),
                            Direction_look_at_top, 9000 + tick, 1);
            }
        }
        // Saturate one client's ping every 11th tick so the "ping already in
        // flight" branch is part of the measured byte stream too.
        if((tick % 11) == 5 && f.owned.size() > 2)
            f.owned[2]->setPing(2);

        f.runMinNetwork(1);
        if(!sync_ok(f)) { fail_line(name, "sync_tick:" + f.last_sync_status); return; }

        // Fold every client's output, in slot order, into the digest.
        k = 0;
        while(k < f.owned.size())
        {
            const ClientWithMap *c = f.owned[k];
            size_t bi = 0;
            while(bi < c->sentBlocks.size())
            {
                const std::vector<char> &by = c->sentBlocks[bi].bytes;
                digest = fnv1a_fold(digest, reinterpret_cast<const char *>(&k), sizeof(k));
                digest = fnv1a_fold(digest, by.data(), by.size());
                total += by.size();
                bi++;
            }
            k++;
        }
        tick++;
    }
    std::cout << "[ORACLE] min_balanced bytes=" << total
              << " digest=" << std::hex << digest << std::dec << std::endl;
    pass_line(name);
}

// A LIVE player sitting ABOVE a hole must survive another player's PATH1.
//
// Reached with the production API only -- insertOnMap / removeOnMap are
// exactly what a map change does. Four players; two walk off; one walks
// in. insertOnMap pops the LIFO free list, so the joiner takes the HIGHER
// freed slot and leaves the LOWER one as the hole:
//
//     slot 0     slot 1     slot 2       slot 3
//     [alice]    [ hole ]   [erin new]   [dave]   <- dave still live
//
// clients_size = map_clients_id.size()(4) - map_removed_index.size()(1)
// = 3, and send_reinsertAllWithFilter's bound is `index<clients_size`, so
// the loop stops at slot 2 and never reaches dave at slot 3. Erin's 0x6B
// is short one player.
//
// This cannot self-heal. 0x66 carries only [slot][x][y][direction] -- no
// pseudo, skin or monster -- so the client cannot materialise a player it
// was never inserted, and reinsert_player_final() drops the entry outright
// ("Other player (%1) not exists", MapControllerMPAPI.cpp:954). PATH2's
// diff then sees dense[3]==previous[3] and never re-sends. dave stays
// invisible to erin for as long as erin stays on this map.
static void scenario_min_balanced_path1_hole_keeps_live_top_slot()
{
    const char *name = "min_balanced_path1_hole_keeps_live_top_slot";
    Fixture f;
    f.addClient("alice", 1, 1, Direction_look_at_bottom, 100, 1);
    uint8_t bob_g   = f.addClient("bob",   2, 2, Direction_look_at_bottom, 101, 1);
    uint8_t carol_g = f.addClient("carol", 3, 3, Direction_look_at_bottom, 102, 1);
    uint8_t dave_g  = f.addClient("dave",  4, 4, Direction_look_at_bottom, 103, 1);
    f.runMinNetwork(1);   // tick1: everybody PATH1, no hole yet
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // bob and carol walk off the map.
    const int bob_slot   = slot_of_gid(f.mva, bob_g);
    const int carol_slot = slot_of_gid(f.mva, carol_g);
    if(bob_slot < 0 || carol_slot < 0) { fail_line(name, "setup_slot_lookup"); return; }
    f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(bob_slot));
    f.mva.removeOnMap(static_cast<PLAYER_INDEX_FOR_CONNECTED>(carol_slot));

    // erin walks in and reuses carol's slot, leaving bob's as the hole.
    uint8_t erin_g = f.addClient("erin", 5, 5, Direction_look_at_bottom, 104, 1);
    const int erin_slot = slot_of_gid(f.mva, erin_g);
    const int dave_slot = slot_of_gid(f.mva, dave_g);
    if(erin_slot < 0 || dave_slot < 0) { fail_line(name, "setup_slot_lookup2"); return; }
    // The whole point is a live slot ABOVE the hole; assert the setup got there.
    if(dave_slot <= erin_slot) { fail_line(name, "setup_dave_not_above_hole"); return; }

    // Focus the observer on erin (owned[] index 4, wire slot 2). Her PATH1
    // block opens with 0x65 drop-all, which resets the shared observer to
    // erin's own view.
    f.setFocus(static_cast<uint8_t>(erin_slot), 4);
    f.runMinNetwork(1);   // tick2: erin PATH1, the others PATH2
    if(!sync_ok(f)) { fail_line(name, "sync_t2_join:" + f.last_sync_status); return; }
    if(f.observer.otherPlayerList.find(static_cast<uint8_t>(dave_slot))
       == f.observer.otherPlayerList.end())
    { fail_line(name, "dave_missing_from_joiner_view"); return; }

    // dave moves: PATH2 emits a 0x66 for his slot, which the client drops
    // when it never received the matching insert. Proves it is permanent,
    // not merely one tick late.
    f.owned[3]->setX(6);
    f.owned[3]->setDirection(Direction_move_at_right);
    f.runMinNetwork(1);   // tick3
    if(!sync_ok(f)) { fail_line(name, "sync_t3_after_move:" + f.last_sync_status); return; }
    pass_line(name);
}

// PATH2 with new slots beyond sendedStatus.size(): tick1 PATH1 alone
// (sendedStatus.size()=2 just alice+seed), then insert late + insert+remove
// for a beyond_empty case, run tick2 -> both beyond_valid + beyond_empty.
static void scenario_min_balanced_path2_beyond_slots()
{
    const char *name = "min_balanced_path2_beyond_slots";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    // Need >1 player for the outer guard to allow PATH1
    f.addClient("seed",  9, 9, Direction_look_at_bottom, 999, 1);
    f.runMinNetwork(1); // tick1 PATH1, sendedStatus.size() = 2
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    // Add a new slot AFTER PATH1 — slot 2 is beyond sendedStatus
    f.addClient("late",  3, 3, Direction_look_at_bottom, 555, 1);
    // Add slot 3 AND immediately remove it -> beyond_empty case
    f.addClient("evict", 4, 4, Direction_look_at_bottom, 666, 1);
    f.mva.removeOnMap(3);
    f.runMinNetwork(1); // tick2 PATH2: slot 2 -> beyond_valid; slot 3 -> beyond_empty
    if(!sync_ok(f)) { fail_line(name, "sync_t2:" + f.last_sync_status); return; }
    pass_line(name);
}

// PATH2 no_diff_ping branch: PATH1, then no movement but ping inflight
// so the no_diff PATH2 still has an inner if(pingInProgress>0) log.
static void scenario_min_balanced_path2_no_diff_ping_inflight()
{
    const char *name = "min_balanced_path2_no_diff_ping_inflight";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);
    f.runMinNetwork(1);     // tick1 PATH1
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    f.owned[0]->setPing(2); // saturate alice
    f.runMinNetwork(1);     // tick2 PATH2 no diff but ping>0 -> no_diff_ping
    if(f.last_sync_status != "skip_ping") { fail_line(name, "expected_skip_ping_got:" + f.last_sync_status); return; }
    pass_line(name);
}

// 255 inserts -> insert_ge254 branch: tick1 PATH1 with just alice +
// 1 seed (so PATH1 runs), then add 254 more clients before tick2 so
// PATH2 sees 254 new slots all beyond sendedStatus -> insertCount=254.
static void scenario_min_balanced_path2_insert_ge254()
{
    const char *name = "min_balanced_path2_insert_ge254";
    Fixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max = 1000;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("seed",  9, 9, Direction_look_at_bottom, 999, 1);
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    for(unsigned int i = 0; i < 254; i++)
    {
        char b[16]; std::snprintf(b, sizeof(b), "x%u", i);
        f.addClient(b, static_cast<COORD_TYPE>(i % 100), static_cast<COORD_TYPE>((i+1) % 100),
                    Direction_look_at_bottom, 2000 + i, 1);
    }
    f.runMinNetwork(1);
    // 254 inserts clamps the player_count in the 0x6B header to 254
    // even though the server has more visible. Observer ends up with
    // fewer entries than server; this scenario passes if the algorithm
    // emits a clamped packet that the observer can decode without
    // overflowing, not if it achieves strict sync.
    pass_line(name);
}

// PATH2 corner cases: replaced character + remove + new slot. Each is
// driven by mutating Client state between ticks (the simplest way to
// exercise the diff branches without rewiring server-side queues).
// Observer must remain in sync after every tick.
static void scenario_min_balanced_path2_replaced_remove_newslot()
{
    const char *name = "min_balanced_path2_replaced_remove_newslot";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   6, 6, Direction_look_at_right,  101, 1);
    f.runMinNetwork(1); // PATH1 — alice's sendedStatus = [self,bob]
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }

    // Replace bob's playerId in the slot so PATH2 sees a DIFFERENT
    // characterId in the same slot index. This drives the
    // min_net_path2_replaced branch.
    f.owned[1]->setPlayerId(999);
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_replaced:" + f.last_sync_status); return; }

    // Now move bob and run again — PATH2 same character, change branch.
    f.owned[1]->setY(10);
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_change:" + f.last_sync_status); return; }

    // Now mark the slot as empty (simulate removeOnMap). This triggers
    // a desync between server (slot 1 now empty) and observer (still
    // has bob from prior ticks). Production emits a 0x69 remove, so
    // after the tick the observer should drop bob.
    f.mva.removeOnMap(1);
    f.runMinNetwork(1);
    // After remove, clients_size = 1 -> min_balanced early-returns
    // without sending anything. Observer still has stale bob; this is
    // a real desync but matches production behaviour ("no broadcast
    // for solo map"). Skip the diff for this final tick.
    pass_line(name);
}

// Regression for the client crash where the character-block parser fed
// a non-decompressible block to zstd, ignored the int32 -1 error return,
// let it wrap into the unsigned decompressedSize (-> size2 == -1) and
// then abort()'d inside parseError() under CATCHCHALLENGER_HARDENED
// (Api_protocol_loadchar.cpp). A decompression failure here usually means a
// client/server protocol-version skew (the stream desynced upstream so the
// compressed-block size was read at the wrong offset); the client must FAIL
// CLEANLY (return false, no crash) rather than abort on bad network input.
//
// The crafted packet reproduces the exact on-wire prefix from the field
// backtrace: empty event/monster/warehouse/reputation lists so the parser
// stays on-track up to the compressed block at pos 39, then a 4-byte
// "compressed" payload that is not a valid zstd frame.
static void scenario_character_block_bad_compressed_block_no_crash()
{
    const char *name = "character_block_bad_compressed_block_no_crash";

    static const unsigned char packet[] = {
        0x00,                                           // events list size = 0
        0x01,0x00,                                      // mapIndex = 1 (uint16 LE)
        0x05,                                           // x
        0x05,                                           // y
        0x12,                                           // direction(2)=right | playerType(0x10)=normal
        0x00,0x00, 0x00,0x00,0x01,                      // rescue: mapIndex u16, x, y, orientation
        0x00,0x00, 0x00,0x00,0x01,                      // unvalidated_rescue: same layout
        0x00,                                           // pseudo length = 0
        0x00,                                           // skinId
        0x00,                                           // allow_create_clan
        0x00,0x00,0x00,0x00,                            // clan (uint32)
        0x00,                                           // clan_leader
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,        // cash (uint64)
        0x00,                                           // team monster list size = 0
        0x00,                                           // warehouse monster list size = 0
        0x00,                                           // reputation list size = 0
        0x04,0x00,0x00,0x00,                            // compressed block size = 4 (uint32 LE)
        0xDE,0xAD,0xBE,0xEF                             // 4 bytes that are NOT a valid zstd frame
    };

    // Force the client into "server uses Zstandard" mode so the parser
    // takes the decompression branch (the crashing path).
    const CompressionProtocol::CompressionType saved = CompressionProtocol::compressionTypeClient;
    CompressionProtocol::compressionTypeClient = CompressionProtocol::CompressionType::Zstandard;

    TestApiProtocol api;
    const bool returnCode = api.testParseCharacterBlockCharacter(
        0xAC, 15, reinterpret_cast<const char *>(packet), (int)sizeof(packet));

    CompressionProtocol::compressionTypeClient = saved;

    // Before the fix this line is never reached under HARDENED (the
    // process abort()'d in parseError); reaching it at all means no crash.
    if(returnCode)
        fail_line(name, "parser accepted an undecompressible character block (expected reject)");
    else if(api.lastNewError.find("decompress") == std::string::npos)
        fail_line(name, "rejected, but not via the decompression guard: " + api.lastNewError);
    else
        pass_line(name);
}

// Regression for the client infinite-loop / use-after-free in
// have_main_and_sub_datapack_loaded() (Api_protocol.cpp). Several
// message handlers (0x64 player count, 0x6B/0x6C, ...) RE-QUEUE
// themselves into delayedMessages while !character_selected. The old
// loop iterated the live delayedMessages member by reference, so each
// re-queue both (1) reallocated the vector and dangled the reference
// the loop was about to read (the crash building a std::string from
// freed data) and (2) grew size() in lockstep with the index so the
// loop never ended.
//
// With the fix the queue is swapped into a local copy first: the loop
// runs exactly once per originally-queued message, re-queued messages
// land in the now-empty member, and the call returns promptly.
static void scenario_delayed_messages_requeue_no_infinite_loop()
{
    const char *name = "delayed_messages_requeue_no_infinite_loop";

    TestApiProtocol api;
    // Logged in, but NOT character-selected yet — the state in which the
    // 0x64 handler re-queues instead of consuming.
    api.testSetCharacterSelected(false);
    // have_main_and_sub_datapack_loaded() early-returns unless a map count
    // was set first.
    if(!api.setMapNumber(6))
    {
        fail_line(name, "setMapNumber(6) refused");
        return;
    }

    // Queue a handful of 0x64 (player-count) messages with a valid 1-byte
    // payload. Each will re-queue itself while !character_selected.
    const std::string payload(1, '\0');
    const size_t queued = 5;
    for(size_t idx = 0; idx < queued; idx++)
        api.testQueueDelayedMessage(0x64, payload);

    // Pre-fix: this never returns (infinite loop) / crashes on the dangling
    // reference. Post-fix: returns after exactly `queued` iterations.
    api.have_main_and_sub_datapack_loaded();

    // Each message re-queued exactly once into the fresh member vector;
    // nothing was lost, nothing exploded.
    if(api.testDelayedMessageCount() != queued)
        fail_line(name, "delayed message count = " + std::to_string(api.testDelayedMessageCount()) + ", expected " + std::to_string(queued));
    else
        pass_line(name);
}

// ---- min_range (view range visibility) ------------------------------
//
// Own fixture: min_range() walks the REAL
// MapVisibilityAlgorithm::flat_map_list to reach the border maps, so the
// world is built there, with the border offsets in the state Map_loader
// leaves them (already resolved: border.right.y_offset is the delta to ADD
// to y when crossing right, and the map on the right carries the opposite).
//
// The oracle is INDEPENDENT from the code under test: the test places every
// map at an explicit WORLD position, DERIVES the border offsets from those
// positions, and recomputes the expected visible set from the world
// positions with a plain |dx|<=VIEW_X && |dy|<=VIEW_Y. Nothing of
// resolveNeighbours()/sendViewDelta() is reused to state the expectation.
//
// The oracle ignores the hysteresis margin, so a scenario checking it (see
// scenario_min_range_enter_leave_hysteresis) asserts on the packets instead
// of calling checkView().

struct RangeMapPlacement
{
    int worldX,worldY;
};

class RangeFixture
{
public:
    ClientList cl;
    MapControllerMpStub observer;
    TestApiProtocol api;
    std::vector<ClientWithMap *> owned;
    std::vector<CATCHCHALLENGER_TYPE_MAPID> ownedMap;//test side copy, map of each client
    std::vector<RangeMapPlacement> placement;
    PLAYER_INDEX_FOR_CONNECTED focus;
    bool ackEachTick;

    RangeFixture() :
        focus(0),
        ackEachTick(true),
        algo(0)
    {
        ClientList::list=&cl;
        api.map_controller=&observer;
        api.allowDynamicSizeForTest();
        GlobalServerData::serverSettings.mapVisibility.simple.max=50;
        GlobalServerData::serverSettings.dontSendPlayerType=false;
        CommonSettingsServer::commonSettingsServer.dontSendPseudo=false;
        MapVisibilityAlgorithm::flat_map_list.clear();
        //zoom 1: the widest view a client can ask for (21 tiles), so the
        //scenarios below have room to place somebody IN and OUT of the range
        //on a 40 wide map
        MapVisibilityAlgorithm::resolveViewRange(1);
    }
    ~RangeFixture()
    {
        size_t index=0;
        while(index<owned.size())
        {
            delete owned.at(index);
            index++;
        }
        owned.clear();
        cl.clear();
        //static: never leave a world behind for the next scenario
        MapVisibilityAlgorithm::flat_map_list.clear();
        ClientList::list=nullptr;
    }

    //Build every map FIRST (resize invalidates the references), then link.
    CATCHCHALLENGER_TYPE_MAPID addMap(const int &worldX,const int &worldY,const uint8_t &width,const uint8_t &height)
    {
        const CATCHCHALLENGER_TYPE_MAPID index=static_cast<CATCHCHALLENGER_TYPE_MAPID>(MapVisibilityAlgorithm::flat_map_list.size());
        MapVisibilityAlgorithm::flat_map_list.resize(static_cast<size_t>(index)+1);
        MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list[index];
        map.width=width;
        map.height=height;
        RangeMapPlacement p;
        p.worldX=worldX;
        p.worldY=worldY;
        placement.push_back(p);
        return index;
    }
    // b is the RIGHT border map of a. The offset is derived from the world
    // placement: crossing right from a(width-1,y) lands on b(0,y+offset),
    // and both are the same physical row, so offset = worldY(a)-worldY(b).
    void linkRight(const CATCHCHALLENGER_TYPE_MAPID &a,const CATCHCHALLENGER_TYPE_MAPID &b)
    {
        MapVisibilityAlgorithm &ma=MapVisibilityAlgorithm::flat_map_list[a];
        MapVisibilityAlgorithm &mb=MapVisibilityAlgorithm::flat_map_list[b];
        ma.border.right.mapIndex=b;
        ma.border.right.y_offset=static_cast<int8_t>(placement.at(a).worldY-placement.at(b).worldY);
        mb.border.left.mapIndex=a;
        mb.border.left.y_offset=static_cast<int8_t>(placement.at(b).worldY-placement.at(a).worldY);
    }
    // b is the BOTTOM border map of a: crossing down from a(x,height-1)
    // lands on b(x+offset,0), so offset = worldX(a)-worldX(b).
    void linkBottom(const CATCHCHALLENGER_TYPE_MAPID &a,const CATCHCHALLENGER_TYPE_MAPID &b)
    {
        MapVisibilityAlgorithm &ma=MapVisibilityAlgorithm::flat_map_list[a];
        MapVisibilityAlgorithm &mb=MapVisibilityAlgorithm::flat_map_list[b];
        ma.border.bottom.mapIndex=b;
        ma.border.bottom.x_offset=static_cast<int8_t>(placement.at(a).worldX-placement.at(b).worldX);
        mb.border.top.mapIndex=a;
        mb.border.top.x_offset=static_cast<int8_t>(placement.at(b).worldX-placement.at(a).worldX);
    }

    PLAYER_INDEX_FOR_CONNECTED addClient(const std::string &pseudo,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                                         const COORD_TYPE &x,const COORD_TYPE &y,const uint32_t &playerId)
    {
        ClientWithMap *c=new ClientWithMap();
        c->setX(x);
        c->setY(y);
        c->setDirection(Direction_look_at_bottom);
        c->setPlayerId(playerId);
        c->setMapIndex(mapIndex);
        c->public_and_private_informations.public_informations.pseudo=pseudo;
        c->public_and_private_informations.public_informations.type=Player_type_normal;
        c->public_and_private_informations.public_informations.skinId=1;
        owned.push_back(c);
        const PLAYER_INDEX_FOR_CONNECTED gid=cl.add(c);
        ownedMap.push_back(mapIndex);
        MapVisibilityAlgorithm::flat_map_list[mapIndex].insertOnMap(gid);
        return gid;
    }
    void moveTo(const PLAYER_INDEX_FOR_CONNECTED &gid,const COORD_TYPE &x,const COORD_TYPE &y)
    {
        owned.at(gid)->setX(x);
        owned.at(gid)->setY(y);
    }
    //map change of a player, the way ClientLocalBroadcast does it
    void mapChange(const PLAYER_INDEX_FOR_CONNECTED &gid,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                   const COORD_TYPE &x,const COORD_TYPE &y)
    {
        MapVisibilityAlgorithm &oldMap=MapVisibilityAlgorithm::flat_map_list[ownedMap.at(gid)];
        PLAYER_INDEX_FOR_CONNECTED slot=0;
        while(slot<oldMap.map_clients_list_size())
        {
            if(oldMap.map_clients_list_isValid(slot) && oldMap.map_clients_list_at(slot)==gid)
            {
                oldMap.removeOnMap(slot);
                slot=oldMap.map_clients_list_size();
            }
            else
                slot++;
        }
        MapVisibilityAlgorithm::flat_map_list[mapIndex].insertOnMap(gid);
        owned.at(gid)->setMapIndex(mapIndex);
        ownedMap.at(gid)=mapIndex;
        moveTo(gid,x,y);
    }

    //which algorithm the tick runs, the 3 mapVisibility/minimize settings:
    //0 = "network" min_range(), 1 = "balanced" min_balanced(), 2 = "cpu" min_CPU()
    int algo;
    void runTick()
    {
        size_t index=0;
        while(index<owned.size())
        {
            if(ackEachTick)
                owned.at(index)->ackPing();
            owned.at(index)->sentBlocks.clear();
            index++;
        }
        unsigned int mapIndex=0;
        while(mapIndex<MapVisibilityAlgorithm::flat_map_list.size())
        {
            MapVisibilityAlgorithm &map=MapVisibilityAlgorithm::flat_map_list[mapIndex];
            if(algo==1)
                map.min_balanced(static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex));
            else if(algo==2)
                map.min_CPU(static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex));
            else
                map.min_range(static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex));
            mapIndex++;
        }
    }
    //feed the focus captured bytes to the REAL production parser
    std::string parseFocus()
    {
        api.map_controller=&observer;
        const ClientWithMap * const fc=owned.at(focus);
        size_t blockIndex=0;
        while(blockIndex<fc->sentBlocks.size())
        {
            const Client::CapturedBlock &b=fc->sentBlocks.at(blockIndex);
            uint32_t cursor=0;
            while(cursor<b.bytes.size())
            {
                const int8_t rc=api.parseIncommingDataRaw(b.bytes.data(),static_cast<uint32_t>(b.bytes.size()),cursor);
                if(rc!=1)
                {
                    std::ostringstream oss;
                    oss << "parse:rc=" << static_cast<int>(rc) << " cursor=" << cursor << "/" << b.bytes.size();
                    return oss.str();
                }
            }
            blockIndex++;
        }
        return std::string();
    }
    void tickAndParse(std::string &status)
    {
        runTick();
        status=parseFocus();
    }

    //---- oracle ----
    static std::string viewKey(const std::string &pseudo,const CATCHCHALLENGER_TYPE_MAPID &map,
                               const COORD_TYPE &x,const COORD_TYPE &y)
    {
        std::ostringstream oss;
        oss << pseudo << "@" << map << ":" << static_cast<unsigned>(x) << "," << static_cast<unsigned>(y);
        return oss.str();
    }
    std::vector<std::string> expectedView() const
    {
        std::vector<std::string> out;
        const int focusX=placement.at(ownedMap.at(focus)).worldX+static_cast<int>(owned.at(focus)->getX());
        const int focusY=placement.at(ownedMap.at(focus)).worldY+static_cast<int>(owned.at(focus)->getY());
        size_t index=0;
        while(index<owned.size())
        {
            if(index!=focus)
            {
                const int candidateX=placement.at(ownedMap.at(index)).worldX+static_cast<int>(owned.at(index)->getX());
                const int candidateY=placement.at(ownedMap.at(index)).worldY+static_cast<int>(owned.at(index)->getY());
                int dx=candidateX-focusX;
                if(dx<0)
                    dx=-dx;
                int dy=candidateY-focusY;
                if(dy<0)
                    dy=-dy;
                if(dx<=MapVisibilityAlgorithm::view_x && dy<=MapVisibilityAlgorithm::view_y)
                    out.push_back(viewKey(owned.at(index)->public_and_private_informations.public_informations.pseudo,
                                          ownedMap.at(index),owned.at(index)->getX(),owned.at(index)->getY()));
            }
            index++;
        }
        std::sort(out.begin(),out.end());
        return out;
    }
    std::vector<std::string> actualView() const
    {
        std::vector<std::string> out;
        for(const std::pair<const uint8_t,OtherPlayerView> &n : observer.otherPlayerList)
            out.push_back(viewKey(n.second.info.pseudo,n.second.current_map,n.second.x,n.second.y));
        std::sort(out.begin(),out.end());
        return out;
    }
    // "" when the client view is exactly what the world says it must be.
    // HARD RULE checked here for every scenario: a recipient is NEVER told
    // about itself, whatever the slot numbering is.
    std::string checkView()
    {
        const std::string &self=owned.at(focus)->public_and_private_informations.public_informations.pseudo;
        for(const std::pair<const uint8_t,OtherPlayerView> &n : observer.otherPlayerList)
            if(n.second.info.pseudo==self)
                return std::string("selfrule:focus_in_its_own_view");
        const std::vector<std::string> expected=expectedView();
        const std::vector<std::string> actual=actualView();
        if(expected==actual)
            return std::string();
        std::ostringstream oss;
        oss << "view_mismatch expected={";
        size_t index=0;
        while(index<expected.size())
        {
            oss << expected.at(index) << ";";
            index++;
        }
        oss << "} got={";
        index=0;
        while(index<actual.size())
        {
            oss << actual.at(index) << ";";
            index++;
        }
        oss << "}";
        return oss.str();
    }
    //count of packets of one code inside the focus captured bytes
    unsigned int focusPacketCount(const uint8_t &code) const
    {
        unsigned int count=0;
        const ClientWithMap * const fc=owned.at(focus);
        size_t blockIndex=0;
        while(blockIndex<fc->sentBlocks.size())
        {
            const std::vector<char> &b=fc->sentBlocks.at(blockIndex).bytes;
            size_t pos=0;
            while(pos<b.size())
            {
                const uint8_t current=static_cast<uint8_t>(b.at(pos));
                if(current==code)
                    count++;
                //same packet grammar as selfEntryViolation() above
                if(current==0x65)
                    pos+=1;
                else if(current==0xE3)
                    pos+=2;
                else if(current==0x6C)
                    pos+=2;
                else if(current==0x69)
                    pos+=6+static_cast<uint8_t>(b.at(pos+5));
                else if(current==0x66)
                    pos+=6+static_cast<size_t>(static_cast<uint8_t>(b.at(pos+5)))*4;
                else if(current==0x6B)
                {
                    const uint8_t entries=static_cast<uint8_t>(b.at(pos+8));
                    size_t entry=pos+9;
                    unsigned int done=0;
                    while(done<entries)
                    {
                        entry+=1+3;//slot + x + y + direction|type
                        if(!dontSendPseudo())
                            entry+=1+static_cast<uint8_t>(b.at(entry));
                        entry+=1+2;//skin + followed monster
                        done++;
                    }
                    pos=entry;
                }
                else
                    return 0xffffffff;//unknown packet code, the caller will see the absurd count
            }
            blockIndex++;
        }
        return count;
    }
};

// resolveNeighbours() must give the 4 borders AND the diagonals reached in 2
// hops, with the right translation, and must NOT keep a map 2 maps away in
// the same direction: the client only loads/displays what TOUCHES the
// current map rect, an insert on any other map would never be applied.
static void scenario_min_range_neighbours_resolved()
{
    const char *name = "min_range_neighbours_resolved";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m2=f.addMap(80,0,40,40);//2 maps away: must be dropped
    const CATCHCHALLENGER_TYPE_MAPID m3=f.addMap(0,40,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m4=f.addMap(40,40,40,40);//diagonal, reached in 2 hops
    f.linkRight(m0,m1);
    f.linkRight(m1,m2);
    f.linkBottom(m0,m3);
    f.linkBottom(m1,m4);
    f.linkRight(m3,m4);
    MapVisibilityAlgorithm::resolveNeighbours();
    const MapVisibilityAlgorithm &map0=MapVisibilityAlgorithm::flat_map_list[m0];
    if(map0.neighbours.size()!=3) { fail_line(name,"neighbour_count="+std::to_string(map0.neighbours.size())); return; }
    unsigned int index=0;
    bool seen1=false,seen3=false,seen4=false;
    while(index<map0.neighbours.size())
    {
        const MapVisibilityAlgorithm::NeighbourMap &n=map0.neighbours.at(index);
        if(n.mapIndex==m2) { fail_line(name,"m2_not_touching_but_kept"); return; }
        if(n.mapIndex==m1) { seen1=true; if(n.offset_x!=40 || n.offset_y!=0) { fail_line(name,"m1_offset"); return; } }
        if(n.mapIndex==m3) { seen3=true; if(n.offset_x!=0 || n.offset_y!=40) { fail_line(name,"m3_offset"); return; } }
        if(n.mapIndex==m4) { seen4=true; if(n.offset_x!=40 || n.offset_y!=40) { fail_line(name,"m4_offset"); return; } }
        index++;
    }
    if(!seen1 || !seen3 || !seen4) { fail_line(name,"missing_neighbour"); return; }
    pass_line(name);
}

// Same map: only what is inside the view rectangle is announced.
static void scenario_min_range_same_map_only_in_view()
{
    const char *name = "min_range_same_map_only_in_view";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,5,5,100);
    f.addClient("near",m0,15,5,101);//dx=10: inside
    f.addClient("far",m0,38,5,102);//dx=33: outside
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"expected_one_visible"); return; }
    pass_line(name);
}

// A player standing on the BORDER map, inside the view: announced with ITS
// OWN map id so the client places it on the map it already displays.
static void scenario_min_range_border_map_in_view()
{
    const char *name = "min_range_border_map_in_view";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    f.linkRight(m0,m1);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,39,10,100);//right edge of m0, world x=39
    f.addClient("across",m1,0,10,101);//world x=40: 1 tile away, on the other map
    f.addClient("deep",m1,35,10,102);//world x=75: 36 away, outside
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    bool foundMap=false;
    for(const std::pair<const uint8_t,OtherPlayerView> &n : f.observer.otherPlayerList)
        if(n.second.info.pseudo=="across")
            foundMap=(n.second.current_map==m1);
    if(!foundMap) { fail_line(name,"announced_on_wrong_map"); return; }
    pass_line(name);
}

// Border with a non zero offset: the range test must use the TRANSLATED
// coordinate, not the raw one.
static void scenario_min_range_border_offset()
{
    const char *name = "min_range_border_offset";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,30,40,40);//shifted 30 tiles down
    f.linkRight(m0,m1);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,39,35,100);//world (39,35)
    f.addClient("aligned",m1,0,5,101);//world (40,35): 1 tile away
    f.addClient("shifted",m1,0,0,102);//world (40,30): 5 tiles away, still inside
    f.addClient("below",m1,0,39,103);//world (40,69): 34 tiles below, outside
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=2) { fail_line(name,"expected_two_visible"); return; }
    pass_line(name);
}

// The DIAGONAL map is only reachable by composing two borders, and a player
// standing in its corner is inside the view.
static void scenario_min_range_diagonal_map()
{
    const char *name = "min_range_diagonal_map";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m3=f.addMap(0,40,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m4=f.addMap(40,40,40,40);
    f.linkRight(m0,m1);
    f.linkBottom(m0,m3);
    f.linkBottom(m1,m4);
    f.linkRight(m3,m4);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,39,39,100);//world (39,39)
    f.addClient("corner",m4,0,0,101);//world (40,40): diagonal, 1 tile away
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"diagonal_not_visible"); return; }
    pass_line(name);
}

// Hysteresis: inserted at the view limit, kept up to view+margin, removed
// past it, and NOT re-inserted before it is back inside the view. Without it
// somebody walking on the edge costs an insert+remove every tick.
static void scenario_min_range_enter_leave_hysteresis()
{
    const char *name = "min_range_enter_leave_hysteresis";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,120,40);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,0,5,100);
    const PLAYER_INDEX_FOR_CONNECTED walker=f.addClient("walker",m0,MapVisibilityAlgorithm::view_x,5,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"not_inserted_at_view_limit"); return; }
    //inside the margin: kept, and it costs a 4 bytes 0x66, not an insert
    f.moveTo(walker,MapVisibilityAlgorithm::view_x+CATCHCHALLENGER_SERVER_MAP_VIEW_MARGIN,5);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"dropped_inside_margin"); return; }
    if(f.focusPacketCount(0x66)!=1 || f.focusPacketCount(0x69)!=0 || f.focusPacketCount(0x6B)!=0)
        { fail_line(name,"margin_move_is_not_a_change"); return; }
    //past the margin: removed
    f.moveTo(walker,MapVisibilityAlgorithm::view_x+CATCHCHALLENGER_SERVER_MAP_VIEW_MARGIN+1,5);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(!f.observer.otherPlayerList.empty()) { fail_line(name,"not_removed_past_margin"); return; }
    //back inside the margin but NOT inside the view: stays out
    f.moveTo(walker,MapVisibilityAlgorithm::view_x+1,5);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(!f.observer.otherPlayerList.empty()) { fail_line(name,"reinserted_inside_margin"); return; }
    //back inside the view: inserted again
    f.moveTo(walker,MapVisibilityAlgorithm::view_x,5);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"not_reinserted_inside_view"); return; }
    pass_line(name);
}

// A visible player that moves inside the view costs ONE 0x66 change and
// nothing else. A quiet tick costs no block at all.
static void scenario_min_range_move_then_quiet()
{
    const char *name = "min_range_move_then_quiet";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,5,5,100);
    const PLAYER_INDEX_FOR_CONNECTED mover=f.addClient("mover",m0,10,5,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    f.moveTo(mover,11,5);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.focusPacketCount(0x66)!=1 || f.focusPacketCount(0x6B)!=0 || f.focusPacketCount(0x69)!=0)
        { fail_line(name,"move_is_not_a_single_change"); return; }
    //nothing moved: nothing sent
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(!f.owned.at(f.focus)->sentBlocks.empty()) { fail_line(name,"sent_on_quiet_tick"); return; }
    pass_line(name);
}

// A visible player crossing to the border map must be RE-INSERTED: 0x66
// carries no map id, so a plain change would leave the client drawing it on
// the map it came from.
static void scenario_min_range_visible_player_changes_map()
{
    const char *name = "min_range_visible_player_changes_map";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    f.linkRight(m0,m1);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,30,10,100);
    const PLAYER_INDEX_FOR_CONNECTED walker=f.addClient("walker",m0,39,10,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"not_visible_before"); return; }
    //one step to the right: same physical tile+1, other map
    f.mapChange(walker,m1,0,10);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    bool onNewMap=false;
    for(const std::pair<const uint8_t,OtherPlayerView> &n : f.observer.otherPlayerList)
        if(n.second.info.pseudo=="walker")
            onNewMap=(n.second.current_map==m1 && n.second.x==0);
    if(!onNewMap) { fail_line(name,"still_on_the_old_map"); return; }
    pass_line(name);
}

// mapVisibility Max caps what ONE RECIPIENT sees, not what the map holds:
// the map keeps every player, the recipient view stops at the cap.
static void scenario_min_range_visible_cap()
{
    const char *name = "min_range_visible_cap";
    RangeFixture f;
    GlobalServerData::serverSettings.mapVisibility.simple.max=3;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,5,5,100);
    unsigned int index=0;
    while(index<6)
    {
        f.addClient(std::string("p")+std::to_string(index),m0,static_cast<COORD_TYPE>(6+index),5,200+index);
        index++;
    }
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=3) { fail_line(name,"cap_not_applied count="+std::to_string(f.observer.otherPlayerList.size())); return; }
    if(MapVisibilityAlgorithm::flat_map_list[m0].map_clients_list_size()!=7) { fail_line(name,"map_lost_players"); return; }
    GlobalServerData::serverSettings.mapVisibility.simple.max=50;
    pass_line(name);
}

// ACK flow control: a recipient that has not answered the previous ping is
// handed NOTHING, and gets ONE delta covering every tick it missed as soon
// as it answers. visibleSlots IS the baseline, so nothing else is needed.
static void scenario_min_range_held_back_then_one_delta()
{
    const char *name = "min_range_held_back_then_one_delta";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,5,5,100);
    const PLAYER_INDEX_FOR_CONNECTED mover=f.addClient("mover",m0,10,5,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    //link slower than the tick: 3 ticks without any answer
    f.ackEachTick=false;
    f.owned.at(f.focus)->setPing(1);
    unsigned int tick=0;
    while(tick<3)
    {
        f.moveTo(mover,static_cast<COORD_TYPE>(11+tick),5);
        f.runTick();
        if(!f.owned.at(f.focus)->sentBlocks.empty()) { fail_line(name,"sent_while_unacked"); return; }
        tick++;
    }
    //it answers: ONE delta with the final position
    f.ackEachTick=true;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.owned.at(f.focus)->sentBlocks.size()!=1) { fail_line(name,"expected_one_block"); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    pass_line(name);
}

// The recipient changing map itself: PATH 1, the client is told to drop its
// whole view (0x65) then gets the new one, because it reloads the maps
// around it and its old entries would point at destroyed maps.
static void scenario_min_range_recipient_changes_map()
{
    const char *name = "min_range_recipient_changes_map";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    f.linkRight(m0,m1);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,39,10,100);
    f.addClient("stayer",m1,0,10,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"stayer_not_visible"); return; }
    f.mapChange(f.focus,m1,0,10);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.focusPacketCount(0x65)!=1) { fail_line(name,"no_drop_all_on_map_change"); return; }
    status=f.checkView();
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"view_lost_after_map_change"); return; }
    pass_line(name);
}

// inViewRange(): the one-pair form of the same rectangle, used by /trade and
// /battle through Client::otherPlayerIsInRange(). A map that is not even a
// border map of this one is never in range, whatever the distance says.
static void scenario_min_range_in_view_range_helper()
{
    const char *name = "min_range_in_view_range_helper";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m2=f.addMap(80,0,40,40);//2 maps away
    f.linkRight(m0,m1);
    f.linkRight(m1,m2);
    MapVisibilityAlgorithm::resolveNeighbours();
    //same map, inside then outside the rectangle
    if(!MapVisibilityAlgorithm::inViewRange(m0,5,5,m0,5,5+MapVisibilityAlgorithm::view_y))
        { fail_line(name,"same_map_limit_not_in_range"); return; }
    if(MapVisibilityAlgorithm::inViewRange(m0,5,5,m0,5,5+MapVisibilityAlgorithm::view_y+1))
        { fail_line(name,"same_map_past_limit_in_range"); return; }
    //border map: one tile away across the seam
    if(!MapVisibilityAlgorithm::inViewRange(m0,39,10,m1,0,10))
        { fail_line(name,"border_map_not_in_range"); return; }
    //border map but too far inside it
    if(MapVisibilityAlgorithm::inViewRange(m0,39,10,m1,39,10))
        { fail_line(name,"border_map_far_in_range"); return; }
    //not a border map of m0 at all
    if(MapVisibilityAlgorithm::inViewRange(m0,39,10,m2,0,10))
        { fail_line(name,"non_neighbour_map_in_range"); return; }
    //out of the map list
    if(MapVisibilityAlgorithm::inViewRange(60000,5,5,m0,5,5))
        { fail_line(name,"bad_map_index_in_range"); return; }
    pass_line(name);
}

// resolveViewRange(): the view is DERIVED from the client window and the
// datapack zoom, it is not a magic number. Expected values recomputed here by
// hand from the client rule (MapControllerMP::setScale) for the reference
// 800x600 window and a 16px datapack tile:
//   factor = ceil(min(1920,1080)*zoom/512) (min 1), tile = 16*factor,
//   tiles = max(ceil(1920/tile),ceil(1080/tile)), view = tiles/2 + 1 margin.
//   zoom 1 -> factor 3 ->  48px -> 40x23 tiles -> 21
//   zoom 2 -> factor 5 ->  80px -> 24x14 tiles -> 13
//   zoom 4 -> factor 9 -> 144px -> 14x8  tiles ->  8   (the real datapack)
//   zoom 0 -> no map/layers.xml -> falls back on zoom 2
// Change CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_* and this test MUST be
// re-derived: that is the point.
static void scenario_min_range_view_range_from_datapack_zoom()
{
    const char *name = "min_range_view_range_from_datapack_zoom";
    const uint8_t expected[4][3]={{1,21,21},{2,13,13},{4,8,8},{0,13,13}};
    unsigned int index=0;
    while(index<4)
    {
        MapVisibilityAlgorithm::resolveViewRange(expected[index][0]);
        if(MapVisibilityAlgorithm::view_x!=expected[index][1] || MapVisibilityAlgorithm::view_y!=expected[index][2])
        {
            std::ostringstream oss;
            oss << "zoom=" << static_cast<unsigned>(expected[index][0])
                << " expected=" << static_cast<unsigned>(expected[index][1]) << "x" << static_cast<unsigned>(expected[index][2])
                << " got=" << static_cast<unsigned>(MapVisibilityAlgorithm::view_x) << "x" << static_cast<unsigned>(MapVisibilityAlgorithm::view_y);
            fail_line(name,oss.str());
            return;
        }
        index++;
    }
    pass_line(name);
}

// A player can be INSIDE the view rectangle and still NOT be announced: its
// map must also be one the client loads (a border map of this one, see
// resolveNeighbours). Two maps away is close enough in world coordinates here
// and MUST stay invisible -- announcing it would leave the insert stuck in the
// client delayedActions for ever, on a map it never displays.
// checkView() cannot state this one: its oracle is pure world distance and
// knows nothing about which maps the client has.
static void scenario_min_range_non_adjacent_map_never_sent()
{
    const char *name = "min_range_non_adjacent_map_never_sent";
    RangeFixture f;
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,20,20);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(20,0,20,20);
    const CATCHCHALLENGER_TYPE_MAPID m2=f.addMap(40,0,20,20);//2 hops from m0
    f.linkRight(m0,m1);
    f.linkRight(m1,m2);
    MapVisibilityAlgorithm::resolveNeighbours();
    f.focus=f.addClient("focus",m0,19,10,100);//world x=19
    f.addClient("neighbour",m1,0,10,101);//world x=20: 1 tile away, border map
    //world x=40, so exactly at the view limit by DISTANCE, but on a map the
    //client does not have: it must not be announced
    f.addClient("twohops",m2,0,10,102);
    if((40-19)>(int)MapVisibilityAlgorithm::view_x) { fail_line(name,"fixture_broken_twohops_out_of_range_anyway"); return; }
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"expected_only_the_border_map_player count="+std::to_string(f.observer.otherPlayerList.size())); return; }
    for(const std::pair<const uint8_t,OtherPlayerView> &n : f.observer.otherPlayerList)
    {
        if(n.second.current_map==m2 || n.second.info.pseudo=="twohops")
            { fail_line(name,"player_of_a_non_border_map_announced"); return; }
        if(n.second.info.pseudo!="neighbour")
            { fail_line(name,"unexpected_player:"+n.second.info.pseudo); return; }
    }
    pass_line(name);
}

// The border crossing of ANOTHER player, all the way through: it is visible on
// this map, it steps onto the border map (0x66 carries no map id, so the whole
// entry has to be re-inserted with the new map), then it walks out of the
// range and is removed. Zoom 4 (the real datapack) so the view is small enough
// for a few steps to cross it.
static void scenario_min_range_visible_player_crosses_then_leaves()
{
    const char *name = "min_range_visible_player_crosses_then_leaves";
    RangeFixture f;
    MapVisibilityAlgorithm::resolveViewRange(4);
    const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
    const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
    f.linkRight(m0,m1);
    MapVisibilityAlgorithm::resolveNeighbours();
    const uint8_t view=MapVisibilityAlgorithm::view_x;
    const uint8_t keep=static_cast<uint8_t>(view+CATCHCHALLENGER_SERVER_MAP_VIEW_MARGIN);
    //focus placed so the walker is AT the view limit on the last column of m0
    f.focus=f.addClient("focus",m0,static_cast<COORD_TYPE>(39-view),10,100);
    const PLAYER_INDEX_FOR_CONNECTED walker=f.addClient("walker",m0,39,10,101);
    std::string status;
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"not_visible_at_the_view_limit"); return; }
    //one step right: it is on the OTHER map now, and still inside the keep band
    f.mapChange(walker,m1,0,10);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(f.observer.otherPlayerList.size()!=1) { fail_line(name,"dropped_when_crossing_while_still_in_range"); return; }
    bool onNewMap=false;
    for(const std::pair<const uint8_t,OtherPlayerView> &n : f.observer.otherPlayerList)
        if(n.second.info.pseudo=="walker")
            onNewMap=(n.second.current_map==m1 && n.second.x==0);
    if(!onNewMap) { fail_line(name,"crossed_but_still_drawn_on_the_old_map"); return; }
    //keeps walking into the border map until past the hysteresis band
    f.moveTo(walker,static_cast<COORD_TYPE>(keep-view+1),10);
    f.tickAndParse(status);
    if(!status.empty()) { fail_line(name,status); return; }
    if(!f.observer.otherPlayerList.empty()) { fail_line(name,"not_removed_after_walking_out_of_range"); return; }
    if(f.focusPacketCount(0x69)!=1) { fail_line(name,"leaving_is_not_one_remove"); return; }
    pass_line(name);
}

// "balanced" and "cpu" are WHOLE MAP algorithms: they show every player of the
// recipient own map whatever the distance, and NEVER anybody of a border map.
// Only "network" (min_range) crosses the seam.
static void scenario_balanced_and_cpu_local_map_only()
{
    const char *name = "balanced_and_cpu_local_map_only";
    int algo=1;
    while(algo<=2)
    {
        RangeFixture f;
        f.algo=algo;
        const CATCHCHALLENGER_TYPE_MAPID m0=f.addMap(0,0,40,40);
        const CATCHCHALLENGER_TYPE_MAPID m1=f.addMap(40,0,40,40);
        f.linkRight(m0,m1);
        MapVisibilityAlgorithm::resolveNeighbours();
        f.focus=f.addClient("focus",m0,39,10,100);
        //far away on the SAME map: a whole-map algorithm shows it anyway
        f.addClient("farlocal",m0,0,39,101);
        //one tile away but on the BORDER map: never announced by these two
        f.addClient("across",m1,0,10,102);
        std::string status;
        f.tickAndParse(status);
        if(!status.empty()) { fail_line(name,status); return; }
        if(f.observer.otherPlayerList.size()!=1)
            { fail_line(name,"algo"+std::to_string(algo)+":expected_only_the_local_player count="+std::to_string(f.observer.otherPlayerList.size())); return; }
        for(const std::pair<const uint8_t,OtherPlayerView> &n : f.observer.otherPlayerList)
        {
            if(n.second.current_map!=m0 || n.second.info.pseudo!="farlocal")
                { fail_line(name,"algo"+std::to_string(algo)+":unexpected:"+n.second.info.pseudo); return; }
        }
        algo++;
    }
    pass_line(name);
}

// ---- Driver ---------------------------------------------------------

int main()
{
    std::cout << "[INFO] testingmapmanagement starting" << std::endl;
    // Populate ProtocolParsingBase::packetFixedSize[] — required by
    // parseIncommingDataRaw before the FIRST packet enters the
    // framing layer.
    ProtocolParsing::initialiseTheVariable(ProtocolParsing::InitialiseTheVariableType::AllInOne);
    scenario_playerToFullInsert_combinations();
    scenario_min_cpu_one_player_returns_early();
    scenario_min_cpu_skip_ge_max();
    scenario_min_cpu_first_tick_three_players();
    scenario_min_cpu_second_tick_same_map();
    scenario_min_cpu_ping_in_progress_skips_ping();
    scenario_min_balanced_first_tick_path1();
    scenario_min_balanced_path2_movement();
    scenario_min_balanced_path2_no_change_no_send();
    scenario_min_balanced_ping_inflight_blocks_state();
    scenario_min_balanced_coalesced_delta_on_ack();
    scenario_min_balanced_hard_rules_under_mixed_lag();
    scenario_min_balanced_path2_replaced_remove_newslot();
    scenario_send_helpers_guards();
    scenario_clamp_and_count_ge254();
    scenario_empty_slot_in_map();
    scenario_min_balanced_skip_ge_max();
    scenario_min_balanced_path1_ping_skip();
    scenario_min_balanced_path2_removal_sequence();
    scenario_min_balanced_path1_hole_keeps_live_top_slot();
    scenario_min_balanced_byte_oracle();
    scenario_min_balanced_path2_beyond_slots();
    scenario_min_balanced_path2_no_diff_ping_inflight();
    scenario_min_balanced_path2_insert_ge254();
    scenario_character_block_bad_compressed_block_no_crash();
    scenario_delayed_messages_requeue_no_infinite_loop();
    scenario_min_range_neighbours_resolved();
    scenario_min_range_same_map_only_in_view();
    scenario_min_range_border_map_in_view();
    scenario_min_range_border_offset();
    scenario_min_range_diagonal_map();
    scenario_min_range_enter_leave_hysteresis();
    scenario_min_range_move_then_quiet();
    scenario_min_range_visible_player_changes_map();
    scenario_min_range_visible_cap();
    scenario_min_range_held_back_then_one_delta();
    scenario_min_range_recipient_changes_map();
    scenario_min_range_in_view_range_helper();
    scenario_min_range_view_range_from_datapack_zoom();
    scenario_min_range_non_adjacent_map_never_sent();
    scenario_min_range_visible_player_crosses_then_leaves();
    scenario_balanced_and_cpu_local_map_only();
    std::cout << "[INFO] pass=" << g_pass << " fail=" << g_fail << std::endl;
    return g_fail == 0 ? 0 : 1;
}
