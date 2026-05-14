// test/testingmapmanagement/TestApiProtocol.hpp — minimal but COMPLETE
// concrete subclass of CatchChallenger::Api_protocol that the test
// uses to drive the SAME parseMessage()/parseQuery()/parseReplyData()
// code paths the production client runs. The point per the user's
// brief: when production protocol parsing changes (e.g. the 0x6B
// player_entry layout gains a new field), the test inherits the new
// behaviour automatically with NO test-side edit. The test never
// duplicates the wire-format parsing logic.
//
// Strategy:
//   1. Implement every pure virtual of Api_protocol +
//      ProtocolParsingInputOutput + MoveOnTheMap with the minimum
//      body that satisfies the abstract base. Most are {} no-ops;
//      the map-broadcast callbacks (insert_player, remove_player,
//      reinsert_player, full_reinsert_player, dropAllPlayerOnTheMap)
//      forward to MapControllerMpStub so the harness can diff the
//      observer's view against the server's authoritative state.
//   2. Expose protected parseMessage/parseQuery/parseReplyData via
//      public wrappers so the test driver can hand each captured
//      packet directly to production parsing logic.
//   3. Set is_logged + character_selected at construction so
//      parseMessage doesn't push to the delayedMessages queue — the
//      test isn't a real game-session, the focus client is logged in
//      from tick 0.

#ifndef CATCHCHALLENGER_TESTINGMAPMANAGEMENT_TESTAPIPROTOCOL_H
#define CATCHCHALLENGER_TESTINGMAPMANAGEMENT_TESTAPIPROTOCOL_H

// ClientStructures.hpp (pulled by Api_protocol.hpp) references
// tinyxml2::XMLElement in a member type. With CATCHCHALLENGER_NOXML
// the customtinyxml2.hpp shim that would have brought in tinyxml2.h
// is skipped, so we forward-declare the type so just naming the
// pointer compiles.
namespace tinyxml2 { class XMLElement; }

#include "../../client/libcatchchallenger/Api_protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace CatchChallenger {

// Mirrors MapControllerMP::OtherPlayer just for the fields the test
// asserts on. Owned by MapControllerMpStub in main.cpp.
struct OtherPlayerView
{
    COORD_TYPE x;
    COORD_TYPE y;
    Direction direction;
    Player_public_informations info;
    CATCHCHALLENGER_TYPE_MAPID current_map;
};

class MapControllerMpStub
{
public:
    std::unordered_map<uint8_t, OtherPlayerView> otherPlayerList;
    uint8_t playerExcludeIndex = 255;
    std::vector<uint8_t> pingsObserved;
    std::vector<std::string> events;

    void insert(uint8_t simplifiedIdx, const Player_public_informations &info,
                CATCHCHALLENGER_TYPE_MAPID mapId, COORD_TYPE x, COORD_TYPE y,
                Direction d);
    void reinsert(uint8_t simplifiedIdx, COORD_TYPE x, COORD_TYPE y, Direction d);
    void remove(uint8_t simplifiedIdx);
    void dropAll();
    void setExclude(uint8_t idx);
    void onPing(uint8_t queryNumber);
};

class TestApiProtocol : public Api_protocol
{
public:
    TestApiProtocol();
    ~TestApiProtocol();

    // Test hook: directs the production parseMessage / parseQuery /
    // parseReplyData paths at the test's MapControllerMpStub.
    MapControllerMpStub *map_controller;

    // Public wrappers so the test driver can feed a parsed framing
    // packet directly into production payload parsing.
    bool testParseMessage(uint8_t packetCode, const char *data, unsigned int size);
    bool testParseQuery(uint8_t packetCode, uint8_t queryNumber, const char *data, unsigned int size);
    bool testParseReplyData(uint8_t packetCode, uint8_t queryNumber, const char *data, unsigned int size);

    // Production Api_protocol::parseQuery handles 0xE3 (ping) by
    // immediately calling postReplyData with no other side effects —
    // there's no virtual callback for ping arrival. We override here
    // so the test driver can observe pings via
    // MapControllerMpStub::pingsObserved.
    bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,
                    const char * const data, const unsigned int &size) override;

    // Production parseMessage early-returns to delayedMessages while
    // !character_selected; this flips both flags so the focus client
    // is "logged in + character selected" from the test's POV.
    void markCharacterSelected();

    // Set the protected ProtocolParsingBase::flags 0x08 bit so
    // parseIncommingDataRaw will accept dynamic-size (0xFE) packets.
    // Normally set on the OUTPUT path by ProtocolParsingInputOutput::write
    // before each send; on our INPUT-driven test path we set it once
    // and leave it. parseIncommingDataRaw doesn't clear it between
    // packets.
    void allowDynamicSizeForTest();

    // ----- Pure-virtual implementations (Api_protocol) -----
    bool haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botFightId) const override;
    void tryDisconnect() override;
    void readForFirstHeader() override;
    void defineMaxPlayers(const uint16_t &maxPlayers) override;
    void newError(const std::string &error,const std::string &detailedError) override;
    void message(const std::string &message) override;
    void lastReplyTime(const uint32_t &time) override;
    void disconnected(const std::string &reason) override;
    void notLogged(const std::string &reason) override;
    void logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList) override;
    void protocol_is_good() override;
    void connectedOnLoginServer() override;
    void connectingOnGameServer() override;
    void connectedOnGameServer() override;
    void haveDatapackMainSubCode() override;
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression) override;
    void number_of_player(const PLAYER_INDEX_FOR_CONNECTED &number,const PLAYER_INDEX_FOR_CONNECTED &max_players) override;
    void random_seeds(const std::string &data) override;
    void newCharacterId(const uint8_t &returnCode,const uint32_t &characterId) override;
    void haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &last_direction) override;
    void setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) override;
    void newEvent(const uint8_t &event,const uint8_t &event_value) override;
    void insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &direction) override;
    void move_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const std::vector<std::pair<uint8_t,Direction> > &movement) override;
    void remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex) override;
    void reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const uint8_t &x,const uint8_t &y,const Direction &direction) override;
    void full_reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE y,const Direction &direction) override;
    void dropAllPlayerOnTheMap() override;
    void teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &direction) override;
    void recipeUsed(const RecipeUsage &recipeUsage) override;
    void objectUsed(const ObjectUsage &objectUsage) override;
    void monsterCatch(const bool &success) override;
    void new_chat_text(const Chat_type &chat_type,const std::string &text,const std::string &pseudo,const Player_type &player_type) override;
    void new_system_text(const Chat_type &chat_type,const std::string &text) override;
    void have_current_player_info(const Player_private_and_public_informations &informations) override;
    void have_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void add_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void remove_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) override;
    void haveTheDatapack() override;
    void haveTheDatapackMainSub() override;
    void newFileBase(const std::string &fileName,const std::string &data) override;
    void newHttpFileBase(const std::string &url,const std::string &fileName) override;
    void removeFileBase(const std::string &fileName) override;
    void datapackSizeBase(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void newFileMain(const std::string &fileName,const std::string &data) override;
    void newHttpFileMain(const std::string &url,const std::string &fileName) override;
    void removeFileMain(const std::string &fileName) override;
    void datapackSizeMain(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void newFileSub(const std::string &fileName,const std::string &data) override;
    void newHttpFileSub(const std::string &url,const std::string &fileName) override;
    void removeFileSub(const std::string &fileName) override;
    void datapackSizeSub(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) override;
    void haveShopList(const std::vector<ItemToSellOrBuy> &items) override;
    void haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellObject(const SoldStat &stat,const uint32_t &newPrice) override;
    void haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products) override;
    void haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) override;
    void haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) override;
    void tradeRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) override;
    void tradeCanceledByOther() override;
    void tradeFinishedByOther() override;
    void tradeValidatedByTheServer() override;
    void tradeAddTradeCash(const uint64_t &cash) override;
    void tradeAddTradeObject(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity) override;
    void tradeAddTradeMonster(const PlayerMonster &monster) override;
    void battleRequested(const std::string &pseudo,const uint8_t &skinInt) override;
    void battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster) override;
    void battleCanceledByOther() override;
    void sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn) override;
    void clanActionSuccess(const uint32_t &clanId) override;
    void clanActionFailed() override;
    void clanDissolved() override;
    void clanInformations(const std::string &name) override;
    void clanInvite(const uint32_t &clanId,const std::string &name) override;
    void cityCapture(const uint32_t &remainingTime,const uint8_t &type) override;
    void captureCityYourAreNotLeader() override;
    void captureCityYourLeaderHaveStartInOtherCity(const std::string &zone) override;
    void captureCityPreviousNotFinished() override;
    void captureCityStartBattle(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count) override;
    void captureCityStartBotFight(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count,const uint32_t &fightId) override;
    void captureCityDelayedStart(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count) override;
    void captureCityWin() override;
    std::string getLanguage() const override;

    // ----- Pure virtuals from ProtocolParsing(InputOutput) -----
    void reset() override;
    ssize_t readFromSocket(char *data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    void registerOutputQuery(const uint8_t &queryNumber, const uint8_t &packetCode) override;
    // moveClientFastPath is server-only (gated by
    // CATCHCHALLENGER_CLASS_ALLINONESERVER + CATCHCHALLENGER_SERVER) —
    // the client-side test never defines CATCHCHALLENGER_SERVER so it
    // isn't a pure virtual here.
    bool disconnectClient() override;
    CompressionProtocol::CompressionType getCompressType() const override;
    void closeSocket() override;
    void errorParsingLayer(const std::string &error) override;
    void messageParsingLayer(const std::string &message) const override;

    // ----- Pure virtuals from MoveOnTheMap -----
    void send_player_move_internal(const uint8_t &moved_unit,const Direction &the_new_direction) override;
};

} // namespace CatchChallenger

#endif // CATCHCHALLENGER_TESTINGMAPMANAGEMENT_TESTAPIPROTOCOL_H
