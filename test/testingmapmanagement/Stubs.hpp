// test/testingmapmanagement/Stubs.hpp — pulled in BEFORE the server-side
// includes of MapVisibilityAlgorithm.cpp + MapServer.cpp when those files
// are compiled with -DCATCHCHALLENGER_TESTING. We pre-define the include
// guards of the heavy server headers so the subsequent `#include
// "../Client.hpp"` / `"../ClientList.hpp"` / `"../GlobalServerData.hpp"`
// / `"ClientWithMap.hpp"` / `"VariableServer.hpp"` / `"MapServer.hpp"`
// / `"CommonSettingsServer.hpp"` chains become no-ops. The
// substitute classes live in this single header so the test never pulls
// in the server stack (no network, no event loop, no DB, no datapack
// parser). Zero syscalls, zero IPC.
//
// All branch traces in the production .cpp are gated on the same
// CATCHCHALLENGER_TESTING define; production builds (which never set
// the flag) strip everything below as well as the trace prints.
#ifndef CATCHCHALLENGER_TESTINGMAPMANAGEMENT_STUBS_H
#define CATCHCHALLENGER_TESTINGMAPMANAGEMENT_STUBS_H

// Pre-set guards so the server-side headers that the test never wants
// expand to no-op. These are the ones MVA.cpp / MapServer.cpp #include
// but whose definitions would drag in the full server stack
// (BaseServer, ClientNetworkRead/Write, fight engine, DB, datapack,
// epoll/io_uring, …).
#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H 1
#define CATCHCHALLENGER_ClientWithMap_H 1
#define CATCHCHALLENGER_GLOBALSERVERDATA_H 1
#define CATCHCHALLENGER_CLIENT_H 1
#define CATCHCHALLENGER_CLIENTLIST_H 1
#define CATCHCHALLENGER_VARIABLESERVER_H 1
// MapBasicMove + ClientMapManagement headers are pulled in transitively
// by some of the above; the guards aren't shared so the test never
// includes them.

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <endian.h>

// Bring in the canonical types/enums + the REAL production protocol
// parsing base + common-settings server. We compile-link
// catchchallenger_general_minimal in this test, so those classes
// already have their authoritative definitions; using them here keeps
// ODR clean.
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
// The REAL packed sendedStatus slot type (one uint32_t per slot) shared
// with MapVisibilityAlgorithm::tempDenseBuffer — dependency-free header,
// so the stub ClientWithMap uses the production type instead of keeping
// a drift-prone mirror.
#include "../../server/base/MapManagement/DensePlayerState.hpp"

// CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE is used by stub MapServer's
// localChatDrop[] member; the same constant lives in VariableServer.hpp
// but we never include that. Define it locally — value doesn't matter
// for the algorithms under test.
#ifndef CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE
#define CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE 30
#endif

namespace CatchChallenger {

class Client;
class ClientWithMap;

// MapServer — base class of MapVisibilityAlgorithm; provides the
// map_clients_id / map_removed_index storage MVA reads, plus
// playerToFullInsert (defined out-of-line by the production
// MapServer.cpp we compile here). All members used by MVA / production
// MapServer.cpp::playerToFullInsert are present.
class MapServer
{
public:
    MapServer();
    static unsigned int playerToFullInsert(const Client &player, char * const bufferForOutput);
    //Match production MapServer.hpp — defined in production MapServer.cpp.
    void assertXYInRange(const uint8_t x,const uint8_t y,const char *who) const;

    uint8_t localChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE];
    uint8_t localChatDropTotalCache;
    uint8_t localChatDropNewValue;
    CATCHCHALLENGER_TYPE_MAPID id_db;
    uint8_t zone;

    // Map dimensions used by the CATCHCHALLENGER_TESTING x,y range check
    // in production MapServer.cpp / MapVisibilityAlgorithm.cpp. In
    // production these live on CommonMap; the stub holds them directly.
    // Initialised by MapServer::MapServer() under -DCATCHCHALLENGER_TESTING.
    uint8_t width;
    uint8_t height;

    // Minimal Map_Border surrogate matching production layout for the
    // sake of the constructor body in MapServer.cpp (which initialises
    // border.* fields). The constructor sets values we never read.
    struct _MapBorderSide
    {
        int8_t x_offset;
        int8_t y_offset;
        CATCHCHALLENGER_TYPE_MAPID mapIndex;
    };
    struct _MapBorder
    {
        _MapBorderSide top, bottom, left, right;
    } border;

    std::vector<PLAYER_INDEX_FOR_CONNECTED> map_clients_id;
    std::vector<PLAYER_INDEX_FOR_CONNECTED> map_removed_index;
    std::vector<PLAYER_INDEX_FOR_CONNECTED> map_removed_index_greater_than_254;

    // Definitions live in the production MapServer.cpp (compiled into
    // the test under -DCATCHCHALLENGER_TESTING); we only declare them
    // here so the stub MapServer ABI matches.
    PLAYER_INDEX_FOR_CONNECTED insertOnMap(const PLAYER_INDEX_FOR_CONNECTED &index_global);
    void removeOnMap(const PLAYER_INDEX_FOR_CONNECTED &index_map);
    void doDDOSLocalChat();
    PLAYER_INDEX_FOR_CONNECTED map_clients_list_size() const;
    bool map_clients_list_isValid(const PLAYER_INDEX_FOR_CONNECTED &index) const;
    const PLAYER_INDEX_FOR_CONNECTED &map_clients_list_at(const PLAYER_INDEX_FOR_CONNECTED &index) const;
};

// Client stub — only the surface referenced by MVA.cpp + production
// playerToFullInsert is exposed. Every output produced by min_CPU /
// min_network is captured into `sentPackets` (one entry per
// sendRawBlock) so the test driver can re-parse it via the same packet
// grammar Api_protocol uses on the client side.
class Client
{
public:
    Client();

    // Read-only accessors used by MVA + playerToFullInsert
    std::string getPseudo() const { return public_and_private_informations.public_informations.pseudo; }
    uint32_t getPlayerId() const { return player_id_; }
    COORD_TYPE getX() const { return x_; }
    COORD_TYPE getY() const { return y_; }
    Direction getLastDirection() const { return direction_; }
    uint8_t pingCountInProgress() const { return ping_in_progress_; }

    // Mutators used by the test driver (NOT by MVA / playerToFullInsert)
    void setX(COORD_TYPE v) { x_ = v; }
    void setY(COORD_TYPE v) { y_ = v; }
    void setDirection(Direction d) { direction_ = d; }
    void setPlayerId(uint32_t id) { player_id_ = id; }
    void setMapIndex(CATCHCHALLENGER_TYPE_MAPID m) { mapIndex = m; }
    void setPing(uint8_t p) { ping_in_progress_ = p; }

    // Production-style write surface: MVA emits packets through these.
    bool sendRawBlock(const char * const data, const unsigned int &size);
    size_t sendPing(char * output);

    // Same fields the production Client exposes to MVA / MapServer (only
    // those used by the visibility algorithm).
    Player_private_and_public_informations public_and_private_informations;
    CATCHCHALLENGER_TYPE_MAPID mapIndex;

    // Captured packets, per-client. Cleared by the test driver between
    // ticks so each call to min_CPU / min_network can be diffed.
    struct CapturedBlock { std::vector<char> bytes; };
    std::vector<CapturedBlock> sentBlocks;

    // ping bookkeeping (the test driver bumps ping_in_progress_ to
    // simulate buffer saturation; production has this in
    // ProtocolParsing*, here it's a plain counter the test owns).
    uint16_t pingQueryCounter;

private:
    uint32_t player_id_;
    COORD_TYPE x_, y_;
    Direction direction_;
    uint8_t ping_in_progress_;
};

class ClientWithMap : public Client
{
public:
    //same packed one-uint32_t-per-slot type as production sendedStatus
    //(see DensePlayerState.hpp include above)
    std::vector<DensePlayerState> sendedStatus;
    CATCHCHALLENGER_TYPE_MAPID sendedMap;

    ClientWithMap();
};

// ClientList — production uses a singleton pointer ClientList::list;
// at(id) / rw(id) / rwWithMap(id) / isNull(id) are the surface MVA
// requires. The stub keeps the same shape.
class ClientList
{
public:
    static ClientList *list;
    Client &at(PLAYER_INDEX_FOR_CONNECTED id) { return *clients_.at(id); }
    Client &rw(PLAYER_INDEX_FOR_CONNECTED id) { return *clients_.at(id); }
    ClientWithMap &rwWithMap(PLAYER_INDEX_FOR_CONNECTED id) { return *clients_.at(id); }
    bool isNull(PLAYER_INDEX_FOR_CONNECTED id) { return id >= clients_.size() || clients_.at(id) == nullptr; }

    // Test-only helpers used by main.cpp to wire up the stub world.
    PLAYER_INDEX_FOR_CONNECTED add(ClientWithMap *c) { clients_.push_back(c); return static_cast<PLAYER_INDEX_FOR_CONNECTED>(clients_.size() - 1); }
    void clear() { clients_.clear(); }

private:
    std::vector<ClientWithMap *> clients_;
};

// GlobalServerData — only mapVisibility.simple.max and dontSendPlayerType
// are read by MVA/playerToFullInsert. Stub exposes them via the same
// nested struct path.
struct _MapVisibilitySimple { unsigned int max; };
struct _MapVisibility { _MapVisibilitySimple simple; };
struct _ServerSettings { _MapVisibility mapVisibility; bool dontSendPlayerType; };

class GlobalServerData
{
public:
    static _ServerSettings serverSettings;
};

} // namespace CatchChallenger

#endif // CATCHCHALLENGER_TESTINGMAPMANAGEMENT_STUBS_H
