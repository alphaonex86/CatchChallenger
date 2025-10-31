#include "../Client.hpp"
#include "../ClientList.hpp"
#include "../PreparedDBQuery.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../GlobalServerData.hpp"
#include <string.h>
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

bool Client::captureCityInProgress()
{
    if(public_and_private_informations.clan==0)
        return false;
    if(clanList.find(public_and_private_informations.clan)==clanList.cend())
        return false;
    const Clan &clan=clanList.at(public_and_private_informations.clan);
    if(clan.captureCityInProgress==ZONE_TYPE_MAX)
        return false;
    //search in capture not validated
    if(captureCity.find(clan.captureCityInProgress)!=captureCity.end())
        if(vectorindexOf(captureCity[clan.captureCityInProgress],index_connected_player)>=0)
            return true;
    //search in capture validated
    if(captureCityValidatedList.find(clan.captureCityInProgress)!=captureCityValidatedList.end())
    {
        const CaptureCityValidated &captureCityValidated=captureCityValidatedList.at(clan.captureCityInProgress);
        //check if this player is into the capture city with the other player of the team
        if(vectorindexOf(captureCityValidated.players,index_connected_player)>=0)
            return true;
    }
    return false;
}

void Client::waitingForCityCaputre(const bool &cancel)
{
    if(index_connected_player==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    if(mapIndex>=65535)
        return;
    if(public_and_private_informations.clan==0)
    {
        errorOutput("Try capture city when is not in clan");
        return;
    }
    if(clanList.find(public_and_private_informations.clan)==clanList.cend())
    {
        errorOutput("Try capture city when clan not found");
        return;
    }
    Clan &clan=clanList[public_and_private_informations.clan];
    if(!cancel)
    {
        if(captureCityInProgress())
        {
            errorOutput("Try capture city when is already into that's");
            return;
        }
        if(isInFight())
        {
            errorOutput("Try capture city when is in fight");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("ask zonecapture at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
        #endif
        COORD_TYPE new_x=0,new_y=0;
        const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
        if(new_map==nullptr)
        {
            errorOutput("Can't move at this direction from "+std::to_string(mapIndex)+" ("+std::to_string(x)+","+std::to_string(y)+")");
            return;
        }
        const std::pair<uint8_t,uint8_t> pos(x,y);
        //check if is zonecapture
        if(new_map->zoneCapture.find(pos)==new_map->zoneCapture.cend())
        {
            errorOutput("no zonecapture point in this direction");
            return;
        }
        //send the zone capture
        const ZONE_TYPE &zoneId=new_map->zone;
        if(!public_and_private_informations.clan_leader)
        {
            if(clan.captureCityInProgress==ZONE_TYPE_MAX)
            {
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1);//set the dynamic size

                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                posOutput+=1;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return;
            }
        }
        else
        {
            if(clan.captureCityInProgress==ZONE_TYPE_MAX)
                clan.captureCityInProgress=zoneId;
        }
        if(clan.captureCityInProgress!=zoneId)
        {
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
            posOutput+=1+4;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            posOutput+=1;

            {
                //const std::string &text=clan.captureCityInProgress;
                const std::string text;/// \todo no change to uint16_t to not change the protocol, change when protocol change
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            return;
        }
        std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> &playerListInCapture=captureCity[zoneId];
        if(vectorcontainsAtLeastOne(playerListInCapture,index_connected_player))
        {
            errorOutput("already in capture city");
            return;
        }
        playerListInCapture.push_back(index_connected_player);
        setInCityCapture(true);
    }
    else
    {
        if(clan.captureCityInProgress==ZONE_TYPE_MAX)
        {
            errorOutput("your clan is not in capture city");
            return;
        }
        if(!vectorremoveOne(captureCity[clan.captureCityInProgress],index_connected_player))
        {
            errorOutput("not in capture city");
            return;
        }
        leaveTheCityCapture();
    }
}

void Client::leaveTheCityCapture()
{
    if(public_and_private_informations.clan==0)
        return;
    if(clanList.find(public_and_private_informations.clan)==clanList.cend())
        return;
    Clan &clan=clanList[public_and_private_informations.clan];
    if(clan.captureCityInProgress==ZONE_TYPE_MAX)
        return;
    if(vectorremoveOne(captureCity[clan.captureCityInProgress],index_connected_player))
    {
        const std::vector<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> playerListInCaptures=captureCity.at(clan.captureCityInProgress);
        //drop all the capture because no body clam it
        if(playerListInCaptures.size()==0)
        {
            captureCity.erase(clan.captureCityInProgress);
            clan.captureCityInProgress=ZONE_TYPE_MAX;
        }
        else
        {
            //drop the clan capture in no other player of the same clan is into it
            unsigned int index=0;
            while(index<playerListInCaptures.size())
            {
                if(ClientList::list->empty(playerListInCaptures.at(index)) && ClientList::list->at(playerListInCaptures.at(index)).clanId()==clanId())
                    break;
                index++;
            }
            if(index==playerListInCaptures.size())
                clan.captureCityInProgress=ZONE_TYPE_MAX;
        }
    }
    setInCityCapture(false);
    otherCityPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
}

void Client::startTheCityCapture()
{
    auto i=captureCity.begin();
    while(i!=captureCity.cend())
    {
        //the city is not free to capture
        if(captureCityValidatedList.find(i->first)!=captureCityValidatedList.cend())
        {
            unsigned int index=0;
            while(index<i->second.size())
            {
                const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &clientIndex=i->second.at(index);
                if(ClientList::list->empty(clientIndex))
                    ClientList::list->rw(clientIndex).previousCityCaptureNotFinished();
                index++;
            }
        }
        //the city is ready to be captured
        else
        {
            CaptureCityValidated tempCaptureCityValidated;
            if(GlobalServerData::serverPrivateVariables.cityStatusList.size()>i->first)
            {
                //init with empty data
                CityStatus e;
                e.clan=0;
                GlobalServerData::serverPrivateVariables.cityStatusList[i->first]=e;
            }
            if(GlobalServerData::serverPrivateVariables.cityStatusList.at(i->first).clan==0)
            {
                if(GlobalServerData::serverPrivateVariables.zoneIdToMapList.size()>i->first)
                {
                    const std::vector<CATCHCHALLENGER_TYPE_MAPID/*mapId*/> &mapsList=GlobalServerData::serverPrivateVariables.zoneIdToMapList.at(i->first);
                    unsigned int index=0;
                    while(index<mapsList.size())
                    {
                        const CATCHCHALLENGER_TYPE_MAPID &mapId=mapsList.at(index);
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(mapId>=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())
                        {
                            std::cerr << "Client::startTheCityCapture() mapId is > map list size" << std::endl;
                            abort();
                        }
                        #endif
                        const Map_server_MapVisibility_Simple_StoreOnSender &map=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapId);
                        std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> temp;
                        for (const std::pair<const uint8_t/*npc id*/,BotFight>& n : map.botFights)
                        {
                            temp.first=mapId;
                            temp.second=n.first;
                            tempCaptureCityValidated.bots.push_back(temp);
                        }
                        index++;
                    }
                }
            }
            tempCaptureCityValidated.players=i->second;
            unsigned int index=0;
            unsigned int sub_index=0;
            //do the clan count
            //why 16Bits? because is 16 fight, is too much to capture the city
            SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED player_count=static_cast<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED>(tempCaptureCityValidated.players.size()+tempCaptureCityValidated.bots.size());
            int clan_count=0;
            if(tempCaptureCityValidated.bots.size()>0)
                clan_count++;

            while(index<captureCityValidatedList.size())
            {
                Client &client=ClientList::list->rw(index);
                if(tempCaptureCityValidated.players.size()>0)
                {
                    index=0;
                    while(index<tempCaptureCityValidated.players.size())
                    {
                        const uint32_t &clanId=client.clanId();
                        if(tempCaptureCityValidated.clanSize.find(clanId)!=tempCaptureCityValidated.clanSize.cend())
                            tempCaptureCityValidated.clanSize[clanId]++;
                        else
                            tempCaptureCityValidated.clanSize[clanId]=1;
                        index++;
                    }
                    /// \todo take care about clan_count overflow
                    clan_count+=tempCaptureCityValidated.clanSize.size();
                }
                //do the PvP
                index=0;
                while(index<tempCaptureCityValidated.players.size())
                {
                    sub_index=index+1;
                    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &playersub_Index=tempCaptureCityValidated.players.at(sub_index);
                    if(ClientList::list->empty(playersub_Index))
                    {
                        Client &sub_client=ClientList::list->rw(playersub_Index);
                        while(sub_index<tempCaptureCityValidated.players.size())
                        {
                            if(client.clanId()!=sub_client.clanId())
                            {
                                client.otherCityPlayerBattle=tempCaptureCityValidated.players.at(sub_index);
                                sub_client.otherCityPlayerBattle=tempCaptureCityValidated.players.at(index);
                                client.battleFakeAccepted(sub_client);
                                tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.at(index));
                                client.cityCaptureBattle(player_count,static_cast<uint16_t>(clan_count));
                                tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.at(sub_index));
                                sub_client.cityCaptureBattle(player_count,static_cast<uint16_t>(clan_count));
                                tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin()+index);
                                index--;
                                tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin()+sub_index-1);
                                break;
                            }
                            sub_index++;
                        }
                    }
                    index++;
                }
                //bot the bot fight
                while(tempCaptureCityValidated.players.size()>0 && tempCaptureCityValidated.bots.size()>0)
                {
                    tempCaptureCityValidated.playersInFight.push_back(tempCaptureCityValidated.players.front());
                    client.cityCaptureBotFight(player_count,static_cast<uint16_t>(clan_count),tempCaptureCityValidated.bots.front());
                    tempCaptureCityValidated.botsInFight.push_back(tempCaptureCityValidated.bots.front());
                    ClientList::list->rw(tempCaptureCityValidated.players.front()).botFightStart(tempCaptureCityValidated.bots.front());
                    tempCaptureCityValidated.players.erase(tempCaptureCityValidated.players.begin());
                    tempCaptureCityValidated.bots.erase(tempCaptureCityValidated.bots.begin());
                }
                //send the wait to the rest
                cityCaptureSendInWait(tempCaptureCityValidated,player_count,static_cast<uint16_t>(clan_count));

                captureCityValidatedList[i->first]=tempCaptureCityValidated;
            }
        }
        ++i;
    }
    captureCity.clear();
    GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
}

void Client::cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated, const uint16_t &number_of_player, const uint16_t &number_of_clan)
{
    unsigned int index=0;
    while(index<captureCityValidated.players.size())
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &playerIndex=captureCityValidated.players.at(index);
        if(ClientList::list->empty(playerIndex))
        {
            Client &client=ClientList::list->rw(playerIndex);
            client.cityCaptureInWait(number_of_player,number_of_clan);
            //captureCityValidated.playersInFight.back()->cityCaptureInWait(number_of_player,number_of_clan);
        }
        index++;
    }
}

uint16_t Client::cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated)
{
    return static_cast<uint16_t>(captureCityValidated.bots.size()+captureCityValidated.botsInFight.size()+captureCityValidated.players.size()+captureCityValidated.playersInFight.size());
}

uint16_t Client::cityCaptureClanCount(const CaptureCityValidated &captureCityValidated)
{
    if(captureCityValidated.bots.size()==0 && captureCityValidated.botsInFight.size()==0)
        return static_cast<uint16_t>(captureCityValidated.clanSize.size());
    else
        return static_cast<uint16_t>(captureCityValidated.clanSize.size())+1;
}

void Client::cityCaptureBattle(const uint16_t &number_of_player,const uint16_t &number_of_clan)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+2+2);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x04;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_player);
    posOutput+=2;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_clan);
    posOutput+=2;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::cityCaptureBotFight(const uint16_t &number_of_player, const uint16_t &number_of_clan, const std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> &fight)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+2+2+4);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x04;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_player);
    posOutput+=2;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_clan);
    posOutput+=2;
    *reinterpret_cast<CATCHCHALLENGER_TYPE_MAPID *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(fight.first);
    posOutput+=2;
    *reinterpret_cast<uint8_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=fight.second;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::cityCaptureInWait(const uint16_t &number_of_player,const uint16_t &number_of_clan)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+2+2);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x05;
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_player);
    posOutput+=2;
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(number_of_clan);
    posOutput+=2;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::cityCaptureWin()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x06;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::previousCityCaptureNotFinished()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x5E;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

//fight == 0 if is in battle
void Client::fightOrBattleFinish(const bool &win, const std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/> &fight)
{
    if(public_and_private_informations.clan==0)
        return;
    if(clanList.find(public_and_private_informations.clan)==clanList.cend())
        return;
    Clan &clan=clanList[public_and_private_informations.clan];
    if(clan.captureCityInProgress!=ZONE_TYPE_MAX && captureCityValidatedList.find(clan.captureCityInProgress)!=captureCityValidatedList.cend())
    {
        CaptureCityValidated &captureCityValidated=captureCityValidatedList[clan.captureCityInProgress];
        //check if this player is into the capture city with the other player of the team
        if(vectorcontainsAtLeastOne(captureCityValidated.playersInFight,index_connected_player))
        {
            if(win)
            {
                if(fight.second!=0)
                    vectorremoveOne(captureCityValidated.botsInFight,fight);
                else
                {
                    if(otherCityPlayerBattle!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
                    {
                        vectorremoveOne(captureCityValidated.playersInFight,otherCityPlayerBattle);
                        otherCityPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
                    }
                }
                uint16_t player_count=cityCapturePlayerCount(captureCityValidated);
                uint16_t clan_count=cityCaptureClanCount(captureCityValidated);
                bool newFightFound=false;
                unsigned int index=0;
                while(index<captureCityValidated.players.size())
                {
                    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED player_index=captureCityValidated.players.at(index);
                    Client &client=ClientList::list->rw(player_index);
                    if(clanId()!=client.clanId())
                    {
                        battleFakeAccepted(client);
                        captureCityValidated.playersInFight.push_back(player_index);
                        client.cityCaptureBattle(player_count,clan_count);
                        cityCaptureBattle(player_count,clan_count);
                        captureCityValidated.players.erase(captureCityValidated.players.begin()+index);
                        newFightFound=true;
                        break;
                    }
                    index++;
                }
                if(!newFightFound && captureCityValidated.bots.size()>0)
                {
                    cityCaptureBotFight(player_count,clan_count,captureCityValidated.bots.front());
                    captureCityValidated.botsInFight.push_back(captureCityValidated.bots.front());
                    botFightStart(captureCityValidated.bots.front());
                    captureCityValidated.bots.erase(captureCityValidated.bots.begin());
                    newFightFound=true;
                }
                if(!newFightFound)
                {
                    vectorremoveOne(captureCityValidated.playersInFight,index_connected_player);
                    captureCityValidated.players.push_back(index_connected_player);
                    otherCityPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
                }
            }
            else
            {
                if(fight.second!=0)
                {
                    vectorremoveOne(captureCityValidated.botsInFight,fight);
                    vectorremoveOne(captureCityValidated.bots,fight);
                }
                else
                {
                    vectorremoveOne(captureCityValidated.playersInFight,index_connected_player);
                    otherCityPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
                }
                captureCityValidated.clanSize[clanId()]--;
                if(captureCityValidated.clanSize.at(clanId())==0)
                    captureCityValidated.clanSize.erase(clanId());
            }
            uint16_t player_count=cityCapturePlayerCount(captureCityValidated);
            uint16_t clan_count=cityCaptureClanCount(captureCityValidated);
            //city capture
            if(captureCityValidated.bots.size()==0 && captureCityValidated.botsInFight.size()==0 && captureCityValidated.playersInFight.size()==0)
            {
                if(clan.capturedCity==clan.captureCityInProgress)
                    clan.captureCityInProgress=ZONE_TYPE_MAX;
                else
                {
                    if(GlobalServerData::serverPrivateVariables.cityStatusList.size()>clan.capturedCity)
                    {
                        GlobalServerData::serverPrivateVariables.cityStatusListReverse.erase(clan.clanId);
                        GlobalServerData::serverPrivateVariables.cityStatusList[clan.capturedCity].clan=0;
                    }
                    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_delete_city.asyncWrite({
                    CommonDatapackServerSpec::commonDatapackServerSpec.get_idToZone().at(clan.capturedCity)
                    });
                    #elif CATCHCHALLENGER_DB_BLACKHOLE
                    #elif CATCHCHALLENGER_DB_FILE
                    #else
                    #error Define what do here
                    #endif
                    if(GlobalServerData::serverPrivateVariables.cityStatusList.size()>clan.captureCityInProgress)
                        GlobalServerData::serverPrivateVariables.cityStatusList[clan.captureCityInProgress].clan=0;

                    if(GlobalServerData::serverPrivateVariables.cityStatusList.at(clan.captureCityInProgress).clan!=0)
                    {
                        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_update_city_clan.asyncWrite({
                                    clan.captureCityInProgress,
                                    std::to_string(clan.clanId)
                                    });
                        #elif CATCHCHALLENGER_DB_BLACKHOLE
                        #elif CATCHCHALLENGER_DB_FILE
                        #else
                        #error Define what do here
                        #endif
                    }
                    else
                    {
                        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                        GlobalServerData::serverPrivateVariables.preparedDBQueryServer.db_query_insert_city.asyncWrite({
                                    std::to_string(clan.clanId),
                                    CommonDatapackServerSpec::commonDatapackServerSpec.get_idToZone().at(clan.captureCityInProgress)
                                    });
                        #elif CATCHCHALLENGER_DB_BLACKHOLE
                        #elif CATCHCHALLENGER_DB_FILE
                        #else
                        #error Define what do here
                        #endif
                    }
                    GlobalServerData::serverPrivateVariables.cityStatusListReverse[clan.clanId]=clan.captureCityInProgress;
                    GlobalServerData::serverPrivateVariables.cityStatusList[clan.captureCityInProgress].clan=clan.clanId;
                    clan.capturedCity=clan.captureCityInProgress;
                    clan.captureCityInProgress=ZONE_TYPE_MAX;
                    unsigned int index=0;
                    while(index<captureCityValidated.players.size())
                    {
                        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED player_index=captureCityValidated.players.back();
                        Client &client=ClientList::list->rw(player_index);
                        client.cityCaptureWin();
                        index++;
                    }
                }
            }
            else
                cityCaptureSendInWait(captureCityValidated,player_count,clan_count);
            return;
        }
    }
}
