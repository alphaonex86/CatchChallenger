// tools/libbot/cli/CliApiClientNotifications.cpp — the Api_protocol
// notifications a connect/login benchmark bot has nothing to do with.
//
// Api_protocol declares ~90 pure virtuals so a GUI client can turn every
// server packet into a screen update. A benchmark bot only needs the
// connection state machine (CliApiClient.cpp); the rest exists purely to make
// the class concrete. They are kept in this separate translation unit so the
// state machine stays readable.
//
// The bodies are deliberately empty rather than "log and ignore": the payload
// is already fully parsed by the production Api_protocol_*.cpp code (which is
// the point of reusing it — a wire-format change is inherited for free), and
// printing hundreds of map-visibility events per second would itself dominate
// a benchmark. Each group notes what a real client does with it, so a future
// bot that needs the data knows where to hook.

#include "CliApiClient.hpp"
//Latency instrumentation: expands to nothing unless CATCHCHALLENGER_BENCHMARK
//is set, so the two hooks below cost a normal build nothing at all.
#include "CliLatency.hpp"

using namespace CatchChallenger;

//---- latency / server load -------------------------------------------------
// A real client shows these in its status bar. A latency benchmark would
// timestamp lastReplyTime() here.
void CliApiClient::lastReplyTime(const uint32_t &time) { (void)time; }
void CliApiClient::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression) { (void)gateway; (void)progression; }
void CliApiClient::number_of_player(const PLAYER_INDEX_FOR_CONNECTED &number,const PLAYER_INDEX_FOR_CONNECTED &max_players) { (void)number; (void)max_players; }
void CliApiClient::random_seeds(const std::string &data) { (void)data; }

//---- day/night + map events ----------------------------------------------
// A real client re-tints the map and re-evaluates event-conditioned tiles.
void CliApiClient::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events) { (void)events; }
void CliApiClient::newEvent(const uint8_t &event,const uint8_t &event_value) { (void)event; (void)event_value; }

//---- other players on the map --------------------------------------------
// A real client feeds these to MapControllerMP to draw the other players.
// A visibility benchmark would count them here.
void CliApiClient::insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const Direction &direction)
{
    (void)simplifiedIndex; (void)player; (void)mapId; (void)x; (void)y; (void)direction;
#ifdef CATCHCHALLENGER_BENCHMARK
    //"see another player update": the other bot timestamped its own map
    //placement, so this is the visibility latency of that join.
    if(CliLatency::instance()!=NULL)
        CliLatency::instance()->onOtherPlayerInserted(this,player.pseudo);
#endif
}
void CliApiClient::move_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const std::vector<std::pair<uint8_t,Direction> > &movement)
{ (void)simplifiedIndex; (void)movement; }
void CliApiClient::remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex) { (void)simplifiedIndex; }
void CliApiClient::reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const uint8_t &x,const uint8_t &y,const Direction &direction)
{ (void)simplifiedIndex; (void)x; (void)y; (void)direction; }
void CliApiClient::full_reinsert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE y,const Direction &direction)
{ (void)simplifiedIndex; (void)mapId; (void)x; (void)y; (void)direction; }
void CliApiClient::dropAllPlayerOnTheMap() {}

//---- own player teleport -------------------------------------------------
// NOT here: teleportTo() carries the move state machine (it must answer the
// server query AND re-sync x/y/map), so it lives in CliApiClient.cpp next to
// the spam engine.

//---- crafting / inventory / catch ---------------------------------------
// Api_protocol already applied the result to player_informations; a real
// client only refreshes the widgets here.
void CliApiClient::recipeUsed(const RecipeUsage &recipeUsage) { (void)recipeUsage; }
void CliApiClient::objectUsed(const ObjectUsage &objectUsage) { (void)objectUsage; }
void CliApiClient::monsterCatch(const bool &success) { (void)success; }
void CliApiClient::have_current_player_info(const Player_private_and_public_informations &informations) { (void)informations; }
void CliApiClient::have_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) { (void)items; }
void CliApiClient::add_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) { (void)items; }
void CliApiClient::remove_to_inventory(const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> &items) { (void)items; }

//---- chat ---------------------------------------------------------------
void CliApiClient::new_chat_text(const Chat_type &chat_type,const std::string &text,const std::string &pseudo,const Player_type &player_type)
{
    (void)chat_type; (void)text; (void)pseudo; (void)player_type;
#ifdef CATCHCHALLENGER_BENCHMARK
    //only the tagged probes are timed; any other chatter is ignored there.
    if(CliLatency::instance()!=NULL)
        CliLatency::instance()->onChat(this,text,pseudo);
#endif
}
void CliApiClient::new_system_text(const Chat_type &chat_type,const std::string &text) { (void)chat_type; (void)text; }

//---- datapack transfer over the protocol --------------------------------
// NEVER fired here: only Api_client_real (the Qt client) asks the server for
// datapack content. This bot shares the server's local datapack on disk, so
// no file list is ever requested and none of these can arrive. A client that
// downloads its datapack must write the files and re-checksum.
void CliApiClient::haveTheDatapack() {}
void CliApiClient::haveTheDatapackMainSub() {}
void CliApiClient::newFileBase(const std::string &fileName,const std::string &data) { (void)fileName; (void)data; }
void CliApiClient::newHttpFileBase(const std::string &url,const std::string &fileName) { (void)url; (void)fileName; }
void CliApiClient::removeFileBase(const std::string &fileName) { (void)fileName; }
void CliApiClient::datapackSizeBase(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) { (void)datapckFileNumber; (void)datapckFileSize; }
void CliApiClient::newFileMain(const std::string &fileName,const std::string &data) { (void)fileName; (void)data; }
void CliApiClient::newHttpFileMain(const std::string &url,const std::string &fileName) { (void)url; (void)fileName; }
void CliApiClient::removeFileMain(const std::string &fileName) { (void)fileName; }
void CliApiClient::datapackSizeMain(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) { (void)datapckFileNumber; (void)datapckFileSize; }
void CliApiClient::newFileSub(const std::string &fileName,const std::string &data) { (void)fileName; (void)data; }
void CliApiClient::newHttpFileSub(const std::string &url,const std::string &fileName) { (void)url; (void)fileName; }
void CliApiClient::removeFileSub(const std::string &fileName) { (void)fileName; }
void CliApiClient::datapackSizeSub(const DATAPACK_FILE_NUMBER &datapckFileNumber,const uint32_t &datapckFileSize) { (void)datapckFileNumber; (void)datapckFileSize; }

//---- shop / factory -----------------------------------------------------
// Replies to queries this bot never sends.
void CliApiClient::haveShopList(const std::vector<ItemToSellOrBuy> &items) { (void)items; }
void CliApiClient::haveBuyObject(const BuyStat &stat,const uint32_t &newPrice) { (void)stat; (void)newPrice; }
void CliApiClient::haveSellObject(const SoldStat &stat,const uint32_t &newPrice) { (void)stat; (void)newPrice; }
void CliApiClient::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{ (void)remainingProductionTime; (void)resources; (void)products; }
void CliApiClient::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice) { (void)stat; (void)newPrice; }
void CliApiClient::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice) { (void)stat; (void)newPrice; }

//---- trade --------------------------------------------------------------
// An incoming request is simply never answered: Api_protocol keeps it in
// tradeRequestId and the server times it out. A trade benchmark would call
// tradeRefused()/tradeAccepted() here.
void CliApiClient::tradeRequested(const std::string &pseudo,const uint8_t &skinInt) { (void)pseudo; (void)skinInt; }
void CliApiClient::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt) { (void)pseudo; (void)skinInt; }
void CliApiClient::tradeCanceledByOther() {}
void CliApiClient::tradeFinishedByOther() {}
void CliApiClient::tradeValidatedByTheServer() {}
void CliApiClient::tradeAddTradeCash(const uint64_t &cash) { (void)cash; }
void CliApiClient::tradeAddTradeObject(const CATCHCHALLENGER_TYPE_ITEM &item,const uint32_t &quantity) { (void)item; (void)quantity; }
void CliApiClient::tradeAddTradeMonster(const PlayerMonster &monster) { (void)monster; }

//---- pvp battle ---------------------------------------------------------
// A fight FREEZES the player server-side: Client::singleMove() answers every
// following move with "try move when is in fight", and errorOutput() kicks. So
// any sign of a fight must take the bot out of the move rotation instead of
// letting it walk into a disconnect. (A WILD fight is never announced — the
// client is meant to derive it from the shared random seeds — which is why
// CliApiClient::tileIsSafeForSpam() refuses monster tiles up front.)
void CliApiClient::battleRequested(const std::string &pseudo,const uint8_t &skinInt) { (void)pseudo; (void)skinInt; }
void CliApiClient::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    (void)pseudo; (void)skinId; (void)stat; (void)monsterPlace; (void)publicPlayerMonster;
    stopSpam("a pvp battle started");
}
void CliApiClient::battleCanceledByOther() {}
void CliApiClient::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    (void)attackReturn;
    stopSpam("a fight is in progress");
}

//---- clan + city capture ------------------------------------------------
void CliApiClient::clanActionSuccess(const uint32_t &clanId) { (void)clanId; }
void CliApiClient::clanActionFailed() {}
void CliApiClient::clanDissolved() {}
void CliApiClient::clanInformations(const std::string &name) { (void)name; }
void CliApiClient::clanInvite(const uint32_t &clanId,const std::string &name) { (void)clanId; (void)name; }
void CliApiClient::cityCapture(const uint32_t &remainingTime,const uint8_t &type) { (void)remainingTime; (void)type; }
void CliApiClient::captureCityYourAreNotLeader() {}
void CliApiClient::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone) { (void)zone; }
void CliApiClient::captureCityPreviousNotFinished() {}
void CliApiClient::captureCityStartBattle(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count)
{
    (void)player_count; (void)clan_count;
    stopSpam("a city-capture battle started");
}
void CliApiClient::captureCityStartBotFight(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count,const uint32_t &fightId)
{
    (void)player_count; (void)clan_count; (void)fightId;
    stopSpam("a city-capture bot fight started");
}
void CliApiClient::captureCityDelayedStart(const PLAYER_INDEX_FOR_CONNECTED &player_count,const uint16_t &clan_count) { (void)player_count; (void)clan_count; }
void CliApiClient::captureCityWin() {}
