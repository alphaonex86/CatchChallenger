#include "Client.h"
#include "PreparedDBQuery.h"
#include "GlobalServerData.h"
#include "SqlFunction.h"
#include "StaticText.h"

using namespace CatchChallenger;

void Client::clanAction(const uint8_t &query_id,const uint8_t &action,const std::string &text)
{
    //send the network reply
    //removeFromQueryReceived(query_id);->Some time need query the database before reply, to check duplicate name
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    switch(action)
    {
        //create
        case 0x01:
        {
            if(public_and_private_informations.clan>0)
            {
                errorOutput("You are already in clan");
                return;
            }
            if(text.size()==0)
            {
                errorOutput("You can't create clan with empty name");
                return;
            }
            if(public_and_private_informations.allow.find(ActionAllow_Clan)==public_and_private_informations.allow.cend())
            {
                errorOutput("You have not the right to create clan");
                return;
            }
            ClanActionParam *clanActionParam=new ClanActionParam();
            clanActionParam->query_id=query_id;
            clanActionParam->action=action;
            clanActionParam->text=text;

            const std::string &queryText=PreparedDBQueryCommon::db_query_select_clan_by_name.compose(
                        SqlFunction::quoteSqlVariable(text)
                        );
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::addClan_static);
            if(callback==NULL)
            {
                std::cerr << "Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

                removeFromQueryReceived(query_id);
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                posOutput+=1;
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                delete clanActionParam;
                return;
            }
            else
            {
                paramToPassToCallBack.push(clanActionParam);
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType.push("ClanActionParam");
                #endif
                callbackRegistred.push(callback);
            }
            return;
        }
        break;
        //leave
        case 0x02:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(public_and_private_informations.clan_leader)
            {
                errorOutput("You can't leave if you are the leader");
                return;
            }
            removeFromClan();
            clanChangeWithoutDb(public_and_private_informations.clan);
            //send the network reply
            removeFromQueryReceived(query_id);
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            //update the db
            const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_clan_to_reset.compose(
                        std::to_string(character_id)
                        );
            dbQueryWriteCommon(queryText);
        }
        break;
        /// \todo: where is the code is resolv when the clan is dissolved but player not connected?
        //dissolve
        case 0x03:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to dissolve the clan");
                return;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            if(clan->captureCityInProgress.size()>0)
            {
                errorOutput("You can't disolv the clan if is in city capture");
                return;
            }
            #endif
            const std::vector<Client *> &players=clanList.at(public_and_private_informations.clan)->players;
            //send the network reply
            removeFromQueryReceived(query_id);
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            //update the db
            unsigned int index=0;
            while(index<players.size())
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_clan_to_reset.compose(
                            std::to_string(players.at(index)->getPlayerId())
                            );
                dbQueryWriteCommon(queryText);
                index++;
            }
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_delete_clan.compose(
                            std::to_string(public_and_private_informations.clan)
                            );
                dbQueryWriteCommon(queryText);
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            {
                const std::string &queryText=PreparedDBQueryServer::db_query_delete_city.compose(
                            clan->capturedCity
                            );
                dbQueryWriteServer(queryText);
            }
            #endif
            //update the object
            clanList.erase(public_and_private_informations.clan);
            #ifndef EPOLLCATCHCHALLENGERSERVER
            GlobalServerData::serverPrivateVariables.cityStatusListReverse.erase(clan->clanId);
            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
            #endif
            delete clan;
            index=0;
            while(index<players.size())
            {
                if(players.at(index)==this)
                {
                    public_and_private_informations.clan=0;
                    clan=NULL;
                    clanChangeWithoutDb(public_and_private_informations.clan);//to send to another thread the clan change, 0 to remove
                }
                else
                    players.at(index)->dissolvedClan();
                index++;
            }
        }
        break;
        //invite
        case 0x04:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to invite into the clan");
                return;
            }
            bool haveAClan=true;
            if(playerByPseudo.find(text)!=playerByPseudo.cend())
                if(!playerByPseudo.at(text)->haveAClan())
                    haveAClan=false;
            bool isFound=playerByPseudo.find(text)!=playerByPseudo.cend();
            //send the network reply
            removeFromQueryReceived(query_id);
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            if(isFound && !haveAClan)
            {
                if(playerByPseudo.at(text)->inviteToClan(public_and_private_informations.clan))
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                else
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            }
            else
            {
                if(!isFound)
                    normalOutput("Clan invite: Player "+text+" not found, is connected?");
                if(haveAClan)
                    normalOutput("Clan invite: Player "+text+" is already into a clan");
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            }
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        //eject
        case 0x05:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput("You have not a clan");
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput("You are not a leader to invite into the clan");
                return;
            }
            if(public_and_private_informations.public_informations.pseudo==text)
            {
                errorOutput("You can't eject your self");
                return;
            }
            bool isIntoTheClan=false;
            if(playerByPseudo.find(text)!=playerByPseudo.cend())
                if(playerByPseudo.at(text)->getClanId()==public_and_private_informations.clan)
                    isIntoTheClan=true;
            bool isFound=playerByPseudo.find(text)!=playerByPseudo.cend();
            //send the network reply
            removeFromQueryReceived(query_id);
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
            if(isFound && isIntoTheClan)
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            else
            {
                if(!isFound)
                    normalOutput("Clan invite: Player "+text+" not found, is connected?");
                if(!isIntoTheClan)
                    normalOutput("Clan invite: Player "+text+" is not into your clan");
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
            }
            posOutput+=1;
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

            if(!isFound)
            {
                const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_clan_to_reset.compose(
                            std::to_string(character_id)
                            );
                dbQueryWriteCommon(queryText);
                return;
            }
            else if(isIntoTheClan)
                playerByPseudo[text]->ejectToClan();
        }
        break;
        default:
            errorOutput("Action on the clan not found");
        return;
    }
}

void Client::addClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addClan_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::addClan_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.size()==0)
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    ClanActionParam *clanActionParam=static_cast<ClanActionParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    addClan_return(clanActionParam->query_id,clanActionParam->action,clanActionParam->text);
    delete clanActionParam;
}

void Client::addClan_return(const uint8_t &query_id,const uint8_t &,const std::string &text)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="ClanActionParam")
    {
        std::cerr << "is not ClanActionParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();

    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;

    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    bool ok;
    const uint32_t clanId=getClanId(&ok);
    if(!ok)
    {
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    public_and_private_informations.clan=clanId;
    createMemoryClan();
    clan->name=text;
    public_and_private_informations.clan_leader=true;
    //send the network reply
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(clanId);
    posOutput+=4;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    //add into db
    const std::string &queryText=PreparedDBQueryCommon::db_query_insert_clan.compose(
                std::to_string(clanId),
                SqlFunction::quoteSqlVariable(text),
                std::to_string(sFrom1970())
                );
    dbQueryWriteCommon(queryText);
    insertIntoAClan(clanId);
}

uint32_t Client::getPlayerId() const
{
    if(stat==ClientStat::CharacterSelected)
        return character_id;
    return 0;
}

void Client::haveClanInfo(const uint32_t &clanId,const std::string &clanName,const uint64_t &cash)
{
    normalOutput("First client of the clan: "+clanName+", clanId: "+std::to_string(clanId)+" to connect");
    createMemoryClan();
    clanList[clanId]->name=clanName;
    clanList[clanId]->cash=cash;
}

void Client::sendClanInfo()
{
    if(public_and_private_informations.clan==0)
        return;
    if(clan==NULL)
        return;
    normalOutput("Send the clan info: "+clan->name+", clanId: "+std::to_string(public_and_private_informations.clan)+", get the info");

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x62;
    posOutput+=1+4;

    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clan->name.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,clan->name.data(),clan->name.size());
        posOutput+=clan->name.size();
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::dissolvedClan()
{
    public_and_private_informations.clan=0;
    clan=NULL;

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x61;
    posOutput+=1+4;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=0;//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    clanChangeWithoutDb(public_and_private_informations.clan);
}

bool Client::inviteToClan(const uint32_t &clanId)
{
    if(inviteToClanList.size()>0)
        return false;
    if(clan==NULL)
        return false;
    inviteToClanList.push_back(clanId);

    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x63;
    posOutput+=1+4;

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(clanId);
    posOutput+=4;
    {
        const std::string &text=clan->name;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    return true;
}

void Client::clanInvite(const bool &accept)
{
    if(!accept)
    {
        normalOutput("You have refused the clan invitation");
        inviteToClanList.erase(inviteToClanList.begin());
        return;
    }
    normalOutput("You have accepted the clan invitation");
    if(inviteToClanList.size()==0)
    {
        errorOutput("Can't responde to clan invite, because no in suspend");
        return;
    }
    public_and_private_informations.clan_leader=false;
    public_and_private_informations.clan=inviteToClanList.front();
    createMemoryClan();
    insertIntoAClan(inviteToClanList.front());
    inviteToClanList.erase(inviteToClanList.begin());
}

uint32_t Client::clanId() const
{
    return public_and_private_informations.clan;
}

void Client::insertIntoAClan(const uint32_t &clanId)
{
    //add into db
    std::string clan_leader;
    if(GlobalServerData::serverPrivateVariables.db_common->databaseType()!=DatabaseBase::DatabaseType::PostgreSQL)
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=StaticText::text_1;
        else
            clan_leader=StaticText::text_0;
    }
    else
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=StaticText::text_true;
        else
            clan_leader=StaticText::text_false;
    }
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_clan_and_leader.compose(
                std::to_string(clanId),
                clan_leader,
                std::to_string(character_id)
                );
    dbQueryWriteCommon(queryText);
    sendClanInfo();
    clanChangeWithoutDb(public_and_private_informations.clan);
}

void Client::ejectToClan()
{
    dissolvedClan();
    const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_clan_to_reset.compose(
                std::to_string(character_id)
                );
    dbQueryWriteCommon(queryText);
}

uint32_t Client::getClanId() const
{
    return public_and_private_informations.clan;
}

bool Client::haveAClan() const
{
    return public_and_private_informations.clan>0;
}
