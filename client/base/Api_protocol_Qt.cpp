#include "Api_protocol_Qt.h"
#include <iostream>

using namespace CatchChallenger;

Api_protocol_Qt::Api_protocol_Qt()
{
}

void Api_protocol_Qt::newError(const std::string &error,const std::string &detailedError)
{
    emit QtnewError(error,detailedError);
}
void Api_protocol_Qt::message(const std::string &message)
{
    emit Qtmessage(message);
}
void Api_protocol_Qt::lastReplyTime(const uint32_t &time)
{
    emit QtlastReplyTime(time);
}

//protocol/connection info
void Api_protocol_Qt::disconnected(const std::string &reason)
{
    emit Qtdisconnected(reason);
}
void Api_protocol_Qt::notLogged(const std::string &reason)
{
    emit QtnotLogged(reason);
}
void Api_protocol_Qt::logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    emit Qtlogged(characterEntryList);
}
void Api_protocol_Qt::protocol_is_good()
{
    emit Qtprotocol_is_good();
}
void Api_protocol_Qt::connectedOnLoginServer()
{
    emit QtconnectedOnLoginServer();
}
void Api_protocol_Qt::connectingOnGameServer()
{
    emit QtconnectingOnGameServer();
}
void Api_protocol_Qt::connectedOnGameServer()
{
    emit QtconnectedOnGameServer();
}
void Api_protocol_Qt::haveDatapackMainSubCode()
{
    emit QthaveDatapackMainSubCode();
}
void Api_protocol_Qt::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    emit QtgatewayCacheUpdate(gateway,progression);
}

//general info
void Api_protocol_Qt::number_of_player(const uint16_t &number,const uint16_t &max_players)
{
    emit Qtnumber_of_player(number,max_players);
}
void Api_protocol_Qt::random_seeds(const std::string &data)
{
    emit Qtrandom_seeds(data);
}

//character
void Api_protocol_Qt::newCharacterId(const uint8_t &returnCode,const uint32_t &characterId)
{
    emit QtnewCharacterId(returnCode,characterId);
}
void Api_protocol_Qt::haveCharacter()
{
    emit QthaveCharacter();
}
//events
void Api_protocol_Qt::setEvents(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    emit QtsetEvents(events);
}
void Api_protocol_Qt::newEvent(const uint8_t &event,const uint8_t &event_value)
{
    emit QtnewEvent(event,event_value);
}

//map move
void Api_protocol_Qt::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtinsert_player(player,mapId,x,y,direction);
}
void Api_protocol_Qt::move_player(const uint16_t &id,const std::vector<std::pair<uint8_t,CatchChallenger::Direction> > &movement)
{
    emit Qtmove_player(id,movement);
}
void Api_protocol_Qt::remove_player(const uint16_t &id)
{
    emit Qtremove_player(id);
}
void Api_protocol_Qt::reinsert_player(const uint16_t &id,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit Qtreinsert_player(id,x,y,direction);
}
void Api_protocol_Qt::full_reinsert_player(const uint16_t &id,const uint32_t &mapId,const uint8_t &x,const uint8_t y,const CatchChallenger::Direction &direction)
{
    emit Qtfull_reinsert_player(id,mapId,x,y,direction);
}
void Api_protocol_Qt::dropAllPlayerOnTheMap()
{
    emit QtdropAllPlayerOnTheMap();
}
void Api_protocol_Qt::teleportTo(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    emit QtteleportTo(mapId,x,y,direction);
}

//plant
void Api_protocol_Qt::insert_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const uint8_t &plant_id,const uint16_t &seconds_to_mature)
{
    emit Qtinsert_plant(mapId,x,y,plant_id,seconds_to_mature);
}
void Api_protocol_Qt::remove_plant(const uint32_t &mapId,const uint8_t &x,const uint8_t &y)
{
    emit Qtremove_plant(mapId,x,y);
}
void Api_protocol_Qt::seed_planted(const bool &ok)
{
    emit Qtseed_planted(ok);
}
void Api_protocol_Qt::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    emit Qtplant_collected(stat);
}
//crafting
void Api_protocol_Qt::recipeUsed(const RecipeUsage &recipeUsage)
{
    emit QtrecipeUsed(recipeUsage);
}
//inventory
void Api_protocol_Qt::objectUsed(const ObjectUsage &objectUsage)
{
    emit QtobjectUsed(objectUsage);
}
void Api_protocol_Qt::monsterCatch(const bool &success)
{
    emit QtmonsterCatch(success);
}

//chat
void Api_protocol_Qt::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type)
{
    emit Qtnew_chat_text(chat_type,text,pseudo,player_type);
}
void Api_protocol_Qt::new_system_text(const CatchChallenger::Chat_type &chat_type,const std::string &text)
{
    emit Qtnew_system_text(chat_type,text);
}

//player info
void Api_protocol_Qt::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    emit Qthave_current_player_info(informations);
}
void Api_protocol_Qt::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const std::unordered_map<uint16_t,uint32_t> &warehouse_items)
{
    emit Qthave_inventory(items,warehouse_items);
}
void Api_protocol_Qt::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtadd_to_inventory(items);
}
void Api_protocol_Qt::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    emit Qtremove_to_inventory(items);
}

//datapack
void Api_protocol_Qt::haveTheDatapack()
{
    std::cerr << "Api_protocol_Qt::haveTheDatapack()" << std::endl;
    emit QthaveTheDatapack();
}
void Api_protocol_Qt::haveTheDatapackMainSub()
{
    emit QthaveTheDatapackMainSub();
}
//base
void Api_protocol_Qt::newFileBase(const std::string &fileName,const std::string &data)
{
    emit QtnewFileBase(fileName,data);
}
void Api_protocol_Qt::newHttpFileBase(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileBase(url,fileName);
}
void Api_protocol_Qt::removeFileBase(const std::string &fileName)
{
    emit QtremoveFileBase(fileName);
}
void Api_protocol_Qt::datapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeBase(datapckFileNumber,datapckFileSize);
}
//main
void Api_protocol_Qt::newFileMain(const std::string &fileName,const std::string &data)
{
    emit QtnewFileMain(fileName,data);
}
void Api_protocol_Qt::newHttpFileMain(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileMain(url,fileName);
}
void Api_protocol_Qt::removeFileMain(const std::string &fileName)
{
    emit QtremoveFileMain(fileName);
}
void Api_protocol_Qt::datapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeMain(datapckFileNumber,datapckFileSize);
}
//sub
void Api_protocol_Qt::newFileSub(const std::string &fileName,const std::string &data)
{
    emit QtnewFileSub(fileName,data);
}
void Api_protocol_Qt::newHttpFileSub(const std::string &url,const std::string &fileName)
{
    emit QtnewHttpFileSub(url,fileName);
}
void Api_protocol_Qt::removeFileSub(const std::string &fileName)
{
    emit QtremoveFileSub(fileName);
}
void Api_protocol_Qt::datapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    emit QtdatapackSizeSub(datapckFileNumber,datapckFileSize);
}

//shop
void Api_protocol_Qt::haveShopList(const std::vector<ItemToSellOrBuy> &items)
{
    emit QthaveShopList(items);
}
void Api_protocol_Qt::haveBuyObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyObject(stat,newPrice);
}
void Api_protocol_Qt::haveSellObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellObject(stat,newPrice);
}

//factory
void Api_protocol_Qt::haveFactoryList(const uint32_t &remainingProductionTime,const std::vector<ItemToSellOrBuy> &resources,const std::vector<ItemToSellOrBuy> &products)
{
    emit QthaveFactoryList(remainingProductionTime,resources,products);
}
void Api_protocol_Qt::haveBuyFactoryObject(const BuyStat &stat,const uint32_t &newPrice)
{
    emit QthaveBuyFactoryObject(stat,newPrice);
}
void Api_protocol_Qt::haveSellFactoryObject(const SoldStat &stat,const uint32_t &newPrice)
{
    emit QthaveSellFactoryObject(stat,newPrice);
}

//trade
void Api_protocol_Qt::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeRequested(pseudo,skinInt);
}
void Api_protocol_Qt::tradeAcceptedByOther(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QttradeAcceptedByOther(pseudo,skinInt);
}
void Api_protocol_Qt::tradeCanceledByOther()
{
    emit QttradeCanceledByOther();
}
void Api_protocol_Qt::tradeFinishedByOther()
{
    emit QttradeFinishedByOther();
}
void Api_protocol_Qt::tradeValidatedByTheServer()
{
    emit QttradeValidatedByTheServer();
}
void Api_protocol_Qt::tradeAddTradeCash(const uint64_t &cash)
{
    emit QttradeAddTradeCash(cash);
}
void Api_protocol_Qt::tradeAddTradeObject(const uint32_t &item,const uint32_t &quantity)
{
    emit QttradeAddTradeObject(item,quantity);
}
void Api_protocol_Qt::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    emit QttradeAddTradeMonster(monster);
}

//battle
void Api_protocol_Qt::battleRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    emit QtbattleRequested(pseudo,skinInt);
}
void Api_protocol_Qt::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    emit QtbattleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
}
void Api_protocol_Qt::battleCanceledByOther()
{
    emit QtbattleCanceledByOther();
}
void Api_protocol_Qt::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    emit QtsendBattleReturn(attackReturn);
}

//clan
void Api_protocol_Qt::clanActionSuccess(const uint32_t &clanId)
{
    emit QtclanActionSuccess(clanId);
}
void Api_protocol_Qt::clanActionFailed()
{
    emit QtclanActionFailed();
}
void Api_protocol_Qt::clanDissolved()
{
    emit QtclanDissolved();
}
void Api_protocol_Qt::clanInformations(const std::string &name)
{
    emit QtclanInformations(name);
}
void Api_protocol_Qt::clanInvite(const uint32_t &clanId,const std::string &name)
{
    emit QtclanInvite(clanId,name);
}
void Api_protocol_Qt::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
    emit QtcityCapture(remainingTime,type);
}

//city
void Api_protocol_Qt::captureCityYourAreNotLeader()
{
    emit QtcaptureCityYourAreNotLeader();
}
void Api_protocol_Qt::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    emit QtcaptureCityYourLeaderHaveStartInOtherCity(zone);
}
void Api_protocol_Qt::captureCityPreviousNotFinished()
{
    emit QtcaptureCityPreviousNotFinished();
}
void Api_protocol_Qt::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityStartBattle(player_count,clan_count);
}
void Api_protocol_Qt::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint32_t &fightId)
{
    emit QtcaptureCityStartBotFight(player_count,clan_count,fightId);
}
void Api_protocol_Qt::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    emit QtcaptureCityDelayedStart(player_count,clan_count);
}
void Api_protocol_Qt::captureCityWin()
{
    emit QtcaptureCityWin();
}

//market
void Api_protocol_Qt::marketList(const uint64_t &price,const std::vector<MarketObject> &marketObjectList,const std::vector<MarketMonster> &marketMonsterList,const std::vector<MarketObject> &marketOwnObjectList,const std::vector<MarketMonster> &marketOwnMonsterList)
{
    emit QtmarketList(price,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
}
void Api_protocol_Qt::marketBuy(const bool &success)
{
    emit QtmarketBuy(success);
}
void Api_protocol_Qt::marketBuyMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketBuyMonster(playerMonster);
}
void Api_protocol_Qt::marketPut(const bool &success)
{
    emit QtmarketPut(success);
}
void Api_protocol_Qt::marketGetCash(const uint64_t &cash)
{
    emit QtmarketGetCash(cash);
}
void Api_protocol_Qt::marketWithdrawCanceled()
{
    emit QtmarketWithdrawCanceled();
}
void Api_protocol_Qt::marketWithdrawObject(const uint32_t &objectId,const uint32_t &quantity)
{
    emit QtmarketWithdrawObject(objectId,quantity);
}
void Api_protocol_Qt::marketWithdrawMonster(const PlayerMonster &playerMonster)
{
    emit QtmarketWithdrawMonster(playerMonster);
}
