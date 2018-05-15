#include "Api_client_real.h"

using namespace CatchChallenger;

void Api_client_real::newError(const std::string &error,const std::string &detailedError)
{
    emit QtnewError(error,detailedError);
}
void Api_client_real::message(const std::string &message)
{
    emit QtnewError(message);
}
void Api_client_real::lastReplyTime(const uint32_t &time)
{
    emit QtlastReplyTime(time);
}

//protocol/connection info
void Api_client_real::disconnected(const std::string &reason)
{
    emit Qtdisconnected(reason);
}
void Api_client_real::notLogged(const std::string &reason)
{
    emit QtnotLogged(reason);
}
void Api_client_real::logged(const std::vector<ServerFromPoolForDisplay *> &serverOrdenedList,const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    emit Qtlogged(serverOrdenedList,characterEntryList);
}
void Api_client_real::protocol_is_good()
{
    emit Qtprotocol_is_good();
}
void Api_client_real::connectedOnLoginServer()
{
    emit QtconnectedOnLoginServer();
}
void Api_client_real::connectingOnGameServer()
{
    emit QtconnectingOnGameServer();
}
void Api_client_real::connectedOnGameServer()
{
    emit QtconnectedOnGameServer();
}
void Api_client_real::haveDatapackMainSubCode()
{
    emit QthaveDatapackMainSubCode();
}
void Api_client_real::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    emit QtgatewayCacheUpdate(gateway,progression);
}

//general info
void Api_client_real::number_of_player(const uint16_t &number,const uint16_t &max_players)
{
    emit Qtnumber_of_player(number,max_players);
}
void Api_client_real::random_seeds(const std::string &data)
{
    emit Qtrandom_seeds(data);
}

//character
void Api_client_real::newCharacterId(const uint8_t &returnCode,const uint32_t &characterId)
{
    emit QtnewCharacterId(returnCode,characterId);
}
void Api_client_real::haveCharacter()
{
    emit QthaveCharacter();
}
//events
void Api_client_real::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    emit QtsetEvents(events);
}
void Api_client_real::newEvent(const uint8_t &event,const uint8_t &event_value)
{
    emit QtnewEvent(event,event_value);
}

//map move
void Api_client_real::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtinsert_player(player,mapId,x,y,direction);
}
void Api_client_real::move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement)
{
    emit Qtmove_player(id,movement);
}
void Api_client_real::remove_player(const uint16_t &id)
{
    emit Qtremove_player(id);
}
void Api_client_real::reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtreinsert_player(id,x,y,direction);
}
void Api_client_real::full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction)
{
    emit Qtfull_reinsert_player(id,mapId,x,y,direction);
}
void Api_client_real::dropAllPlayerOnTheMap()
{
    emit QtdropAllPlayerOnTheMap();
}
void Api_client_real::teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit QtteleportTo(mapId,x,y,direction);
}

//plant
void Api_client_real::insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature)
{
    emit Qtinsert_plant(mapId,x,y,plant_id,seconds_to_mature);
}
void Api_client_real::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    emit Qtremove_plant(mapId,x,y);
}
void Api_client_real::seed_planted(const bool &ok)
{
    emit Qtseed_planted(ok);
}
void Api_client_real::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    emit Qtplant_collected(stat);
}
//crafting
void Api_client_real::recipeUsed(const RecipeUsage &recipeUsage)
{
    emit QtrecipeUsed(recipeUsage);
}
//inventory
void Api_client_real::objectUsed(const ObjectUsage &objectUsage)
{
    emit QtobjectUsed(objectUsage);
}
void Api_client_real::monsterCatch(const bool &success)
{
    emit QtmonsterCatch(success);
}

//chat
void Api_client_real::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type)
{
    emit Qtnew_chat_text(chat_type,text,pseudo,player_type);
}
void Api_client_real::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    emit Qtnew_system_text(chat_type,text);
}

//player info
void Api_client_real::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    emit Qthave_current_player_info(informations);
}
void Api_client_real::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items)
{
    emit Qthave_inventory(items,warehouse_items);
}
void Api_client_real::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtadd_to_inventory(items);
}
void Api_client_real::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtremove_to_inventory(items);
}

//datapack
void Api_client_real::haveTheDatapack()
{
    emit QthaveTheDatapack();
}
void Api_client_real::haveTheDatapackMainSub()
{
    emit QthaveTheDatapackMainSub();
}
//base
void Api_client_real::newFileBase(const std::string &fileName,const std::string &data)
{
    emit QtnewFileBase(fileName,data);
}
void Api_client_real::newHttpFileBase(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileBase(url,fileName);
}
void Api_client_real::removeFileBase(const std::string &fileName)
{
    emit QtremoveFileBase(fileName);
}
void Api_client_real::datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeBase(datapckFileNumber,datapckFileSize);
}
//main
void Api_client_real::newFileMain(const std::string &fileName,const std::string &data)
{
    emit QtnewFileMain(fileName,data);
}
void Api_client_real::newHttpFileMain(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileMain(url,fileName);
}
void Api_client_real::removeFileMain(const std::string &fileName)
{
    emit QtremoveFileMain(fileName);
}
void Api_client_real::datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeMain(datapckFileNumber,datapckFileSize);
}
//sub
void Api_client_real::newFileSub(const std::string &fileName,const std::string &data)
{
    emit QtnewFileSub(fileName,data);
}
void Api_client_real::newHttpFileSub(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileSub(url,fileName);
}
void Api_client_real::removeFileSub(const std::string &fileName)
{
    emit QtremoveFileSub(fileName);
}
void Api_client_real::datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeSub(datapckFileNumber,datapckFileSize);
}

//shop
void Api_client_real::haveShopList(const std::vector<ItemToSellOrBuy> &items)
{
    emit QthaveShopList(items);
}
void Api_client_real::haveBuyObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyObject(stat,newPrice);
}
void Api_client_real::haveSellObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellObject(stat,newPrice);
}

//factory
void Api_client_real::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{
    emit QthaveFactoryList(remainingProductionTime,resources,products);
}
void Api_client_real::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyFactoryObject(stat,newPrice);
}
void Api_client_real::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellFactoryObject(stat,newPrice);
}

//trade
void Api_client_real::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeRequested(pseudo,skinInt);
}
void Api_client_real::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeAcceptedByOther(pseudo,skinInt);
}
void Api_client_real::tradeCanceledByOther()
{
    emit QttradeCanceledByOther();
}
void Api_client_real::tradeFinishedByOther()
{
    emit QttradeFinishedByOther();
}
void Api_client_real::tradeValidatedByTheServer()
{
    emit QttradeValidatedByTheServer();
}
void Api_client_real::tradeAddTradeCash(const uint64_t &cash)
{
    emit QttradeAddTradeCash(cash);
}
void Api_client_real::tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity)
{
    emit QttradeAddTradeObject(item,quantity);
}
void Api_client_real::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
tradeAddTradeMonster(monster);
}

//battle
void Api_client_real::battleRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QtbattleRequested(pseudo,skinInt);
}
void Api_client_real::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    emit QtbattleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
}
void Api_client_real::battleCanceledByOther()
{
    emit QtbattleCanceledByOther();
}
void Api_client_real::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    emit QtsendBattleReturn(attackReturn);
}

//clan
void Api_client_real::clanActionSuccess(const uint32_t &clanId)
{
    emit QtclanActionSuccess(clanId);
}
void Api_client_real::clanActionFailed()
{
    emit QtclanActionFailed();
}
void Api_client_real::clanDissolved()
{
    emit QtclanDissolved();
}
void Api_client_real::clanInformations(const std::string &name)
{
    emit QtclanInformations(name);
}
void Api_client_real::clanInvite(const uint32_t &clanId,const std::string &name)
{
    emit QtclanInvite(clanId,name);
}
void Api_client_real::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
cityCapture(remainingTime,type);
}

//city
void Api_client_real::captureCityYourAreNotLeader()
{
    emit QtcaptureCityYourAreNotLeader();
}
void Api_client_real::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    emit QtcaptureCityYourLeaderHaveStartInOtherCity(zone);
}
void Api_client_real::captureCityPreviousNotFinished()
{
    emit QtcaptureCityPreviousNotFinished();
}
void Api_client_real::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityStartBattle(player_count,clan_count);
}
void Api_client_real::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId)
{
    emit QtcaptureCityStartBotFight(player_count,clan_count,fightId);
}
void Api_client_real::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityDelayedStart(player_count,clan_count);
}
void Api_client_real::captureCityWin()
{
    emit QtcaptureCityWin();
}

//market
void Api_client_real::marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList)
{
    emit QtmarketList(price,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
}
void Api_client_real::marketBuy(const bool &success)
{
    emit QtmarketBuy(success);
}
void Api_client_real::marketBuyMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketBuyMonster(playerMonster);
}
void Api_client_real::marketPut(const bool &success)
{
    emit QtmarketPut(success);
}
void Api_client_real::marketGetCash(const uint64_t &cash)
{
    emit QtmarketGetCash(cash);
}
void Api_client_real::marketWithdrawCanceled()
{
    emit QtmarketWithdrawCanceled();
}
void Api_client_real::marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity)
{
    emit QtmarketWithdrawObject(objectId,quantity);
}
void Api_client_real::marketWithdrawMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketWithdrawMonster(playerMonster);
}
