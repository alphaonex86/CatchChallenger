// test/testingmapmanagement/Stubs.cpp — out-of-line definitions for the
// stubs declared in Stubs.hpp. CommonDatapack / CommonDatapackServerSpec
// / MoveOnTheMap definitions all come from catchchallenger_general
// (linked by CMakeLists.txt), so no stubs needed here.

#include "Stubs.hpp"

namespace CatchChallenger {

// ProtocolParsingBase::tempBigBufferForOutput is defined by
// production ProtocolParsingGeneral.cpp (linked via catchchallenger_
// general_minimal); no test-side definition needed.

ClientList *ClientList::list = nullptr;

_ServerSettings GlobalServerData::serverSettings{};

Client::Client() :
    mapIndex(0),
    pingQueryCounter(0),
    player_id_(0),
    x_(0),
    y_(0),
    direction_(Direction_look_at_bottom),
    ping_in_progress_(0)
{
    public_and_private_informations.public_informations.pseudo.clear();
    public_and_private_informations.public_informations.type = Player_type_normal;
    public_and_private_informations.public_informations.skinId = 0;
}

bool Client::sendRawBlock(const char * const data, const unsigned int &size)
{
    CapturedBlock b;
    b.bytes.assign(data, data + size);
    sentBlocks.push_back(std::move(b));
    return true;
}

// 0xE3 ping packet shape used in production: code(1) + query_num(1).
// sendPing returns total bytes written. We bump pingQueryCounter on
// every send so each ping carries a fresh, deterministic id.
//
// The id MUST stay inside the protocol's query-number space: the real
// parser rejects anything above CATCHCHALLENGER_MAXPROTOCOLQUERY-1
// (ProtocolParsingBase::parseQueryNumber). A free-running counter walked
// out of that range after 16 pings, which no scenario was long enough to
// reach until the byte oracle ran 40 ticks.
size_t Client::sendPing(char * output)
{
    // Production increments pingInProgress on send (Client.cpp:894) and
    // decrements it on the client's 0xE3 reply. min_network uses that count
    // as its flow-control signal, so the stub MUST increment too or the
    // held-back path is never exercised.
    if(ping_in_progress_ < 255)
        ping_in_progress_++;
    output[0] = static_cast<char>(0xE3);
    output[1] = static_cast<char>(pingQueryCounter % CATCHCHALLENGER_MAXPROTOCOLQUERY);
    pingQueryCounter = static_cast<uint16_t>((pingQueryCounter + 1) % CATCHCHALLENGER_MAXPROTOCOLQUERY);
    return 2;
}

ClientWithMap::ClientWithMap() :
    Client(),
    sendedMap(0xffff)
{
}

// MapServer::MapServer() is defined out-of-line by the production
// MapServer.cpp we compile into this binary.

} // namespace CatchChallenger

// CommonSettingsServer::commonSettingsServer is defined by production
// CommonSettingsServer.cpp; no test-side definition needed.
