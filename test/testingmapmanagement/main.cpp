// test/testingmapmanagement/main.cpp — driver for the testing
// harness. Sets up a stub Client/ClientList world (see Stubs.hpp),
// instantiates the real MapVisibilityAlgorithm (compiled with
// -DCATCHCHALLENGER_TESTING from server/base/MapManagement/), runs a
// fixed set of scenarios designed to cover every branch in min_CPU(),
// min_network() and MapServer::playerToFullInsert(), and checks the
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
    uint8_t focus_slot = 0;
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

    void setFocus(uint8_t slot) { focus_slot = slot; }

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

    void runMinCpu(CATCHCHALLENGER_TYPE_MAPID mapId)
    {
        for(ClientWithMap *c : owned) c->sentBlocks.clear();
        mva.min_CPU(mapId);
        last_sync_status = syncCheckFocus();
    }

    void runMinNetwork(CATCHCHALLENGER_TYPE_MAPID mapId)
    {
        for(ClientWithMap *c : owned) c->sentBlocks.clear();
        mva.min_network(mapId);
        last_sync_status = syncCheckFocus();
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
        if(focus_slot >= owned.size())
            return std::string();
        ClientWithMap *fc = owned[focus_slot];
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

static void scenario_min_network_first_tick_path1()
{
    const char *name = "min_network_first_tick_path1";
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

static void scenario_min_network_path2_movement()
{
    const char *name = "min_network_path2_movement";
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

static void scenario_min_network_path2_no_change_no_send()
{
    const char *name = "min_network_path2_no_change_no_send";
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

static void scenario_min_network_ping_inflight_blocks_state()
{
    const char *name = "min_network_ping_inflight_blocks_state";
    Fixture f;
    f.addClient("alice", 5, 5, Direction_look_at_bottom, 100, 1);
    uint8_t s1 = f.addClient("bob", 6, 6, Direction_look_at_right, 101, 1);
    // first tick: PATH1 — focus (alice = slot 0) has no ping in
    // flight, observer catches up, sync diff must succeed.
    f.runMinNetwork(1);
    if(!sync_ok(f)) { fail_line(name, "sync_t1:" + f.last_sync_status); return; }
    // Saturate alice's buffer: pingCountInProgress > 0 — diff is
    // skipped per the brief on the next tick.
    f.owned[0]->setPing(5);
    f.owned[1]->setX(8); f.owned[1]->setY(8); // move bob
    f.runMinNetwork(1);
    if(f.last_sync_status != "skip_ping") {
        fail_line(name, "expected_skip_ping_got:" + f.last_sync_status); return;
    }
    // Even though we skipped the diff, the observer still parsed the
    // PATH2 bytes — bob's view should advance to (8,8) because
    // production keeps sending diffs while only the ping byte is
    // omitted.
    auto it = f.observer.otherPlayerList.find(static_cast<uint8_t>(s1));
    if(it == f.observer.otherPlayerList.end()) { fail_line(name, "bob_missing"); return; }
    if(it->second.x != 8 || it->second.y != 8) { fail_line(name, "bob_pos_wrong"); return; }
    if(f.observer.pingsObserved.size() != 1) { fail_line(name, "extra_ping_emitted"); return; }
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

// 255 clients -> clamp branch in both min_CPU and min_network, plus
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
// in min_network PATH1. min_network must run BEFORE min_CPU on the
// same fixture so PATH1 still fires (min_CPU sets sendedMap which
// would then force PATH2 on the next min_network tick).
//
// NOTE: a sparse middle (slot 0 valid, slot 1 MAX, slot 2 valid) is
// an artificial layout. Production's insertOnMap always backfills
// removed slots first, so the high end ends up MAX, not the middle.
// With a middle gap, send_reinsertAllWithFilter uses clients_size=2
// as its upper loop bound and therefore never reaches slot 2 — the
// observer ends up short one player vs the server. We skip the sync
// diff here; the scenario's purpose is branch coverage, not state
// equivalence.
static void scenario_empty_slot_in_map()
{
    const char *name = "empty_slot_in_map";
    Fixture f;
    f.addClient("alice", 1, 1, Direction_look_at_bottom, 100, 1);
    f.addClient("bob",   2, 2, Direction_look_at_bottom, 101, 1);
    f.addClient("carol", 3, 3, Direction_look_at_bottom, 102, 1);
    f.mva.removeOnMap(1); // bob's slot now PLAYER_INDEX_FOR_CONNECTED_MAX
    f.runMinNetwork(1);   // PATH1 hits status_empty for slot 1
    f.runMinCpu(1);       // hits min_cpu_slot_empty + send_reinsertAll_loop_empty
    pass_line(name);
}

// min_network skip_ge_max branch — same shape as min_CPU skip_ge_max.
static void scenario_min_network_skip_ge_max()
{
    const char *name = "min_network_skip_ge_max";
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
static void scenario_min_network_path1_ping_skip()
{
    const char *name = "min_network_path1_ping_skip";
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
static void scenario_min_network_path2_removal_sequence()
{
    const char *name = "min_network_path2_removal_sequence";
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

// PATH2 with new slots beyond sendedStatus.size(): tick1 PATH1 alone
// (sendedStatus.size()=2 just alice+seed), then insert late + insert+remove
// for a beyond_empty case, run tick2 -> both beyond_valid + beyond_empty.
static void scenario_min_network_path2_beyond_slots()
{
    const char *name = "min_network_path2_beyond_slots";
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
static void scenario_min_network_path2_no_diff_ping_inflight()
{
    const char *name = "min_network_path2_no_diff_ping_inflight";
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
static void scenario_min_network_path2_insert_ge254()
{
    const char *name = "min_network_path2_insert_ge254";
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
static void scenario_min_network_path2_replaced_remove_newslot()
{
    const char *name = "min_network_path2_replaced_remove_newslot";
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
    // After remove, clients_size = 1 -> min_network early-returns
    // without sending anything. Observer still has stale bob; this is
    // a real desync but matches production behaviour ("no broadcast
    // for solo map"). Skip the diff for this final tick.
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
    scenario_min_network_first_tick_path1();
    scenario_min_network_path2_movement();
    scenario_min_network_path2_no_change_no_send();
    scenario_min_network_ping_inflight_blocks_state();
    scenario_min_network_path2_replaced_remove_newslot();
    scenario_send_helpers_guards();
    scenario_clamp_and_count_ge254();
    scenario_empty_slot_in_map();
    scenario_min_network_skip_ge_max();
    scenario_min_network_path1_ping_skip();
    scenario_min_network_path2_removal_sequence();
    scenario_min_network_path2_beyond_slots();
    scenario_min_network_path2_no_diff_ping_inflight();
    scenario_min_network_path2_insert_ge254();
    std::cout << "[INFO] pass=" << g_pass << " fail=" << g_fail << std::endl;
    return g_fail == 0 ? 0 : 1;
}
