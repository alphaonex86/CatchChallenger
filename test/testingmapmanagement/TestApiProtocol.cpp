#include "TestApiProtocol.hpp"

#include <iostream>
#include <sstream>

using namespace CatchChallenger;

// ---- MapControllerMpStub -------------------------------------------

void MapControllerMpStub::insert(uint8_t simplifiedIdx, const Player_public_informations &info,
                                  CATCHCHALLENGER_TYPE_MAPID mapId, COORD_TYPE x, COORD_TYPE y,
                                  Direction d)
{
    OtherPlayerView v;
    v.info = info;
    v.x = x;
    v.y = y;
    v.direction = d;
    v.current_map = mapId;
    otherPlayerList[simplifiedIdx] = v;
    std::ostringstream oss;
    oss << "insert:" << static_cast<unsigned>(simplifiedIdx) << "@"
        << static_cast<unsigned>(x) << "," << static_cast<unsigned>(y) << ":" << info.pseudo;
    events.push_back(oss.str());
}

void MapControllerMpStub::reinsert(uint8_t simplifiedIdx, COORD_TYPE x, COORD_TYPE y, Direction d)
{
    auto it = otherPlayerList.find(simplifiedIdx);
    if(it != otherPlayerList.end())
    {
        it->second.x = x;
        it->second.y = y;
        it->second.direction = d;
    }
    std::ostringstream oss;
    oss << "reinsert:" << static_cast<unsigned>(simplifiedIdx) << "@"
        << static_cast<unsigned>(x) << "," << static_cast<unsigned>(y);
    events.push_back(oss.str());
}

void MapControllerMpStub::remove(uint8_t simplifiedIdx)
{
    otherPlayerList.erase(simplifiedIdx);
    std::ostringstream oss;
    oss << "remove:" << static_cast<unsigned>(simplifiedIdx);
    events.push_back(oss.str());
}

void MapControllerMpStub::dropAll()
{
    otherPlayerList.clear();
    events.push_back("dropAll");
}

void MapControllerMpStub::setExclude(uint8_t idx)
{
    playerExcludeIndex = idx;
    std::ostringstream oss;
    oss << "exclude:" << static_cast<unsigned>(idx);
    events.push_back(oss.str());
}

void MapControllerMpStub::onPing(uint8_t queryNumber)
{
    pingsObserved.push_back(queryNumber);
    std::ostringstream oss;
    oss << "ping:" << static_cast<unsigned>(queryNumber);
    events.push_back(oss.str());
}

// ---- TestApiProtocol -----------------------------------------------

TestApiProtocol::TestApiProtocol() :
    Api_protocol(),
    map_controller(nullptr),
    lastNewError()
{
    // parseMessage() pushes to delayedMessages while !character_selected;
    // mark "logged in + character selected" so map packets parse
    // immediately. These are protected members of Api_protocol; safe to
    // poke from the subclass.
    is_logged = true;
    character_selected = true;
}

TestApiProtocol::~TestApiProtocol() {}

void TestApiProtocol::markCharacterSelected()
{
    is_logged = true;
    character_selected = true;
}

void TestApiProtocol::allowDynamicSizeForTest()
{
    flags |= 0x08;
}

bool TestApiProtocol::testParseMessage(uint8_t packetCode, const char *data, unsigned int size)
{
    return parseMessage(packetCode, data, size);
}

bool TestApiProtocol::testParseQuery(uint8_t packetCode, uint8_t queryNumber, const char *data, unsigned int size)
{
    return parseQuery(packetCode, queryNumber, data, size);
}

bool TestApiProtocol::testParseReplyData(uint8_t packetCode, uint8_t queryNumber, const char *data, unsigned int size)
{
    return parseReplyData(packetCode, queryNumber, data, size);
}

bool TestApiProtocol::testParseCharacterBlockCharacter(uint8_t packetCode, uint8_t queryNumber, const char *data, int size)
{
    return parseCharacterBlockCharacter(packetCode, queryNumber, data, size);
}

void TestApiProtocol::testSetCharacterSelected(bool v)
{
    is_logged = true;
    character_selected = v;
}

void TestApiProtocol::testQueueDelayedMessage(uint8_t packetCode, const std::string &data)
{
    DelayedMessage m;
    m.packetCode = packetCode;
    m.data = data;
    delayedMessages.push_back(m);
}

size_t TestApiProtocol::testDelayedMessageCount() const
{
    return delayedMessages.size();
}

bool TestApiProtocol::parseQuery(const uint8_t &packetCode, const uint8_t &queryNumber,
                                  const char * const data, const unsigned int &size)
{
    if(packetCode == 0xE3 && map_controller)
        map_controller->onPing(queryNumber);
    return Api_protocol::parseQuery(packetCode, queryNumber, data, size);
}

// ---- Map callbacks: forward to MapControllerMpStub ------------------

void TestApiProtocol::insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,
                                    const Player_public_informations &player,
                                    const CATCHCHALLENGER_TYPE_MAPID &mapId,
                                    const COORD_TYPE &x, const COORD_TYPE &y,
                                    const Direction &direction)
{
    if(map_controller) map_controller->insert(simplifiedIndex, player, mapId, x, y, direction);
}

void TestApiProtocol::move_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &,
                                  const std::vector<std::pair<uint8_t,Direction> > &)
{
    // MVA never emits 0x68 — no body needed.
}

void TestApiProtocol::remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex)
{
    if(map_controller) map_controller->remove(simplifiedIndex);
}

void TestApiProtocol::reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,
                                      const uint8_t &x, const uint8_t &y,
                                      const Direction &direction)
{
    if(map_controller) map_controller->reinsert(simplifiedIndex, x, y, direction);
}

void TestApiProtocol::full_reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &,
                                           const CATCHCHALLENGER_TYPE_MAPID &,
                                           const COORD_TYPE &, const COORD_TYPE,
                                           const Direction &)
{
    // MVA never emits 0x67 — no body needed.
}

void TestApiProtocol::dropAllPlayerOnTheMap()
{
    if(map_controller) map_controller->dropAll();
}

// 0x6C exclude — the protected production parseMessage assigns to
// playerExcludeIndex directly on `this`; we mirror it into the
// observer too so subsequent inserts can correctly filter self.
//
// NOTE: there's no virtual for 0x6C exclude — production code writes
// directly to the protected member. The test driver reads
// this->playerExcludeIndex after parseMessage(0x6C, ...) and forwards
// it to map_controller in main.cpp.

// ---- No-op pure virtuals -------------------------------------------

bool TestApiProtocol::haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &, const CATCHCHALLENGER_TYPE_BOTID &) const { return false; }
void TestApiProtocol::tryDisconnect() {}
void TestApiProtocol::readForFirstHeader() {}
void TestApiProtocol::defineMaxPlayers(const uint16_t &) {}
void TestApiProtocol::newError(const std::string &error, const std::string &detailedError) { lastNewError = error + " / " + detailedError; std::cerr << "[api] newError: " << error << " / " << detailedError << std::endl; }
void TestApiProtocol::message(const std::string &) {}
void TestApiProtocol::lastReplyTime(const uint32_t &) {}
void TestApiProtocol::disconnected(const std::string &) {}
void TestApiProtocol::notLogged(const std::string &) {}
void TestApiProtocol::logged(const std::vector<std::vector<CharacterEntry> > &) {}
void TestApiProtocol::protocol_is_good() {}
void TestApiProtocol::connectedOnLoginServer() {}
void TestApiProtocol::connectingOnGameServer() {}
void TestApiProtocol::connectedOnGameServer() {}
void TestApiProtocol::haveDatapackMainSubCode() {}
void TestApiProtocol::gatewayCacheUpdate(const uint8_t, const uint8_t) {}
void TestApiProtocol::number_of_player(const PLAYER_INDEX_FOR_CONNECTED &, const PLAYER_INDEX_FOR_CONNECTED &) {}
void TestApiProtocol::random_seeds(const std::string &) {}
void TestApiProtocol::newCharacterId(const uint8_t &, const uint32_t &) {}
void TestApiProtocol::haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &, const COORD_TYPE &, const COORD_TYPE &, const Direction &) {}
void TestApiProtocol::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &) {}
void TestApiProtocol::newEvent(const uint8_t &, const uint8_t &) {}
void TestApiProtocol::teleportTo(const CATCHCHALLENGER_TYPE_MAPID &, const COORD_TYPE &, const COORD_TYPE &, const Direction &) {}
void TestApiProtocol::recipeUsed(const RecipeUsage &) {}
void TestApiProtocol::objectUsed(const ObjectUsage &) {}
void TestApiProtocol::monsterCatch(const bool &) {}
void TestApiProtocol::new_chat_text(const Chat_type &, const std::string &, const std::string &, const Player_type &) {}
void TestApiProtocol::new_system_text(const Chat_type &, const std::string &) {}
void TestApiProtocol::have_current_player_info(const Player_private_and_public_informations &) {}
void TestApiProtocol::have_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &) {}
void TestApiProtocol::add_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &) {}
void TestApiProtocol::remove_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &) {}
void TestApiProtocol::haveTheDatapack() {}
void TestApiProtocol::haveTheDatapackMainSub() {}
void TestApiProtocol::newFileBase(const std::string &, const std::string &) {}
void TestApiProtocol::newHttpFileBase(const std::string &, const std::string &) {}
void TestApiProtocol::removeFileBase(const std::string &) {}
void TestApiProtocol::datapackSizeBase(const DATAPACK_FILE_NUMBER &, const uint32_t &) {}
void TestApiProtocol::newFileMain(const std::string &, const std::string &) {}
void TestApiProtocol::newHttpFileMain(const std::string &, const std::string &) {}
void TestApiProtocol::removeFileMain(const std::string &) {}
void TestApiProtocol::datapackSizeMain(const DATAPACK_FILE_NUMBER &, const uint32_t &) {}
void TestApiProtocol::newFileSub(const std::string &, const std::string &) {}
void TestApiProtocol::newHttpFileSub(const std::string &, const std::string &) {}
void TestApiProtocol::removeFileSub(const std::string &) {}
void TestApiProtocol::datapackSizeSub(const DATAPACK_FILE_NUMBER &, const uint32_t &) {}
void TestApiProtocol::haveShopList(const std::vector<ItemToSellOrBuy> &) {}
void TestApiProtocol::haveBuyObject(const BuyStat &, const uint32_t &) {}
void TestApiProtocol::haveSellObject(const SoldStat &, const uint32_t &) {}
void TestApiProtocol::haveFactoryList(const uint32_t &, const std::vector<ItemToSellOrBuy> &, const std::vector<ItemToSellOrBuy> &) {}
void TestApiProtocol::haveBuyFactoryObject(const BuyStat &, const uint32_t &) {}
void TestApiProtocol::haveSellFactoryObject(const SoldStat &, const uint32_t &) {}
void TestApiProtocol::tradeRequested(const std::string &, const uint8_t &) {}
void TestApiProtocol::tradeAcceptedByOther(const std::string &, const uint8_t &) {}
void TestApiProtocol::tradeCanceledByOther() {}
void TestApiProtocol::tradeFinishedByOther() {}
void TestApiProtocol::tradeValidatedByTheServer() {}
void TestApiProtocol::tradeAddTradeCash(const uint64_t &) {}
void TestApiProtocol::tradeAddTradeObject(const CATCHCHALLENGER_TYPE_ITEM &, const uint32_t &) {}
void TestApiProtocol::tradeAddTradeMonster(const PlayerMonster &) {}
void TestApiProtocol::battleRequested(const std::string &, const uint8_t &) {}
void TestApiProtocol::battleAcceptedByOther(const std::string &, const uint8_t &, const std::vector<uint8_t> &, const uint8_t &, const PublicPlayerMonster &) {}
void TestApiProtocol::battleCanceledByOther() {}
void TestApiProtocol::sendBattleReturn(const std::vector<Skill::AttackReturn> &) {}
void TestApiProtocol::clanActionSuccess(const uint32_t &) {}
void TestApiProtocol::clanActionFailed() {}
void TestApiProtocol::clanDissolved() {}
void TestApiProtocol::clanInformations(const std::string &) {}
void TestApiProtocol::clanInvite(const uint32_t &, const std::string &) {}
void TestApiProtocol::cityCapture(const uint32_t &, const uint8_t &) {}
void TestApiProtocol::captureCityYourAreNotLeader() {}
void TestApiProtocol::captureCityYourLeaderHaveStartInOtherCity(const std::string &) {}
void TestApiProtocol::captureCityPreviousNotFinished() {}
void TestApiProtocol::captureCityStartBattle(const PLAYER_INDEX_FOR_CONNECTED &, const uint16_t &) {}
void TestApiProtocol::captureCityStartBotFight(const PLAYER_INDEX_FOR_CONNECTED &, const uint16_t &, const uint32_t &) {}
void TestApiProtocol::captureCityDelayedStart(const PLAYER_INDEX_FOR_CONNECTED &, const uint16_t &) {}
void TestApiProtocol::captureCityWin() {}
std::string TestApiProtocol::getLanguage() const { return "en"; }

// ---- ProtocolParsing(InputOutput) ---

void TestApiProtocol::reset() {}
ssize_t TestApiProtocol::readFromSocket(char *, const size_t &) { return 0; }
ssize_t TestApiProtocol::writeToSocket(const char * const, const size_t &size) { return static_cast<ssize_t>(size); }
void TestApiProtocol::registerOutputQuery(const uint8_t &, const uint8_t &) {}
bool TestApiProtocol::disconnectClient() { return true; }
CompressionProtocol::CompressionType TestApiProtocol::getCompressType() const { return CompressionProtocol::None; }
void TestApiProtocol::closeSocket() {}
void TestApiProtocol::errorParsingLayer(const std::string &error) { std::cerr << "[api] errorParsingLayer: " << error << std::endl; }
void TestApiProtocol::messageParsingLayer(const std::string &) const {}

// ---- MoveOnTheMap ---

void TestApiProtocol::send_player_move_internal(const uint8_t &, const Direction &) {}
