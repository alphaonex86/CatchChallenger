#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "../../general/base/cpp11addition.h"
#include "DatabaseFunction.h"
#include "SqlFunction.h"
#include "PreparedDBQuery.h"
#include "DictionaryLogin.h"
#include "DictionaryServer.h"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.h"
#endif

#include <chrono>

using namespace CatchChallenger;

void Client::characterSelectionIsWrong(const uint8_t &query_id,const uint8_t &returnCode,const std::string &debugMessage)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(1);
    posOutput+=4;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=returnCode;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    //send to server to stop the connection
    errorOutput(debugMessage);
}

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::selectCharacter(const uint8_t &query_id, const char * const token)
{
    const auto &time=sFrom1970();
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-LinkToMaster::maxLockAge))
        {
            if(memcmp(tokenAuth.token,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)==0)
            {
                delete tokenAuth.token;
                account_id=tokenAuth.accountIdRequester;/// \warning need take care of only write if character is selected
                selectCharacter(query_id,tokenAuth.characterId);
                tokenAuthList.erase(tokenAuthList.begin()+index);
                flags|=0x08;
                return;
            }
            index++;
        }
        else
        {
            LinkToMaster::linkToMaster->characterDisconnected(tokenAuth.characterId);
            tokenAuthList.erase(tokenAuthList.begin()+index);
        }
    }
    //if never found
    errorOutput("selectCharacter() Token never found to login, bug");
}

void Client::purgeTokenAuthList()
{
    const auto &time=sFrom1970();
    unsigned int index=0;
    while(index<tokenAuthList.size())
    {
        const TokenAuth &tokenAuth=tokenAuthList.at(index);
        if(tokenAuth.createTime>(time-LinkToMaster::maxLockAge))
            index++;
        else
        {
            LinkToMaster::linkToMaster->characterDisconnected(tokenAuth.characterId);
            tokenAuthList.erase(tokenAuthList.begin()+index);
        }
    }
}
#endif

void Client::selectCharacter(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_character_by_id.empty())
    {
        errorOutput("selectCharacter() Query is empty, bug");
        return;
    }
    /*if(PreparedDBQueryCommon::db_query_update_character_last_connect.empty())
    {
        errorOutput("selectCharacter() Query db_query_update_character_last_connect is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_time_to_delete.empty())
    {
        errorOutput("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug");
        return;
    }*/
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;
    stat=ClientStat::CharacterSelecting;

    std::string queryText=PreparedDBQueryCommon::db_query_character_by_id.compose(
                std::to_string(characterId)
                );
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::selectCharacter_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        characterSelectionIsWrong(query_id,0x02,queryText+": "+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack.push(selectCharacterParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("SelectCharacterParam");
        #endif
        callbackRegistred.push(callback);
    }
}

void Client::selectCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectCharacter_object();
}

void Client::selectCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacter_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::selectCharacter_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="SelectCharacterParam")
    {
        std::cerr << "is not SelectCharacterParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif

    /* account(0),pseudo(1),skin(2),type(3),clan(4),cash(5),
    warehouse_cash(6),clan_leader(7),time_to_delete(8),starter(9),
    allow(10),item(11),item_warehouse(12),recipes(13),reputations(14),
    encyclopedia_monster(15),encyclopedia_item(16),achievements(17),blob_version(18)*/

    callbackRegistred.pop();
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    {
        std::cerr << "Try select " << characterId << " but not found with account " << account_id << std::endl;
        characterSelectionIsWrong(query_id,0x02,"Result return query wrong");
        return;
    }

    bool ok;
    public_and_private_informations.clan=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
    if(!ok)
    {
        //normalOutput(std::stringLiteral("clan id is not an number, clan disabled"));
        public_and_private_informations.clan=0;//no clan
    }
    public_and_private_informations.clan_leader=(GlobalServerData::serverPrivateVariables.db_common->stringtobool(GlobalServerData::serverPrivateVariables.db_common->value(7),&ok)==1);
    if(!ok)
    {
        //normalOutput(std::stringLiteral("clan_leader id is not an number, clan_leader disabled"));
        public_and_private_informations.clan_leader=false;//no clan
    }

    if(stat!=ClientStat::CharacterSelecting)
    {
        characterSelectionIsWrong(query_id,0x03,"character_loaded already to true, stat: "+std::to_string(stat));
        return;
    }
    const uint32_t &account_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,"Account for character: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not an id");
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,"Character: "+std::to_string(characterId)+" is not owned by the account: "+std::to_string(this->account_id));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.find(characterId)!=GlobalServerData::serverPrivateVariables.connected_players_id_list.cend())
    {
        characterSelectionIsWrong(query_id,0x03,"Already logged");
        return;
    }

    public_and_private_informations.public_informations.pseudo=std::string(GlobalServerData::serverPrivateVariables.db_common->value(1));

    /*#ifndef SERVERBENCHMARK
    if(GlobalServerData::serverSettings.anonymous)
        normalOutput("Charater id is logged: "+std::to_string(characterId));
    else
        normalOutput("Charater is logged: "+GlobalServerData::serverPrivateVariables.db_common->value(1));
    #endif*/
    const uint32_t &time_to_delete=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(8),&ok);
    if(!ok || time_to_delete>0)
    {
        const std::string &queryText=PreparedDBQueryCommon::db_query_set_character_time_to_delete_to_zero.compose(
                    std::to_string(characterId)
                    );
        dbQueryWriteCommon(queryText);
    }


    {
        const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_last_connect.compose(
                    std::to_string(sFrom1970()),
                    std::to_string(characterId)
                    );
        dbQueryWriteCommon(queryText);
    }

    const uint32_t &skin_database_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
    if(!ok)
    {
        normalOutput("Skin is not a number");
        public_and_private_informations.public_informations.skinId=0;
    }
    else
    {
        if(skin_database_id<(uint32_t)DictionaryLogin::dictionary_skin_database_to_internal.size())
            public_and_private_informations.public_informations.skinId=DictionaryLogin::dictionary_skin_database_to_internal.at(skin_database_id);
        else
        {
            normalOutput("Skin not found, or out of the 255 first folder, default of the first by order alphabetic if have");
            public_and_private_informations.public_informations.skinId=0;
        }
    }
    const uint32_t &type=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
    if(!ok)
    {
        normalOutput("Player type is not a number, default to normal");
        public_and_private_informations.public_informations.type=Player_type_normal;
    }
    else
    {
        if(type<=3)
            public_and_private_informations.public_informations.type=static_cast<Player_type>((type+1)*16);
        else
        {
            normalOutput("Player wrong type value: "+std::to_string(type));
            public_and_private_informations.public_informations.type=Player_type_normal;
        }
    }
    public_and_private_informations.cash=GlobalServerData::serverPrivateVariables.db_common->stringtouint64(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
    if(!ok)
    {
        normalOutput("cash id is not an number, cash set to 0");
        public_and_private_informations.cash=0;
    }
    public_and_private_informations.warehouse_cash=GlobalServerData::serverPrivateVariables.db_common->stringtouint64(GlobalServerData::serverPrivateVariables.db_common->value(6),&ok);
    if(!ok)
    {
        normalOutput("warehouse cash id is not an number, warehouse cash set to 0");
        public_and_private_informations.warehouse_cash=0;
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;

    const uint8_t &starter=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(9),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"start from base selection");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        characterSelectionIsWrong(query_id,0x04,"profile common and server don't match");
        return;
    }
    #endif
    if(starter>=DictionaryLogin::dictionary_starter_database_to_internal.size())
    {
        characterSelectionIsWrong(query_id,0x04,"starter "+std::to_string(starter)+
                                  " >= DictionaryLogin::dictionary_starter_database_to_internal.size() "+std::to_string(DictionaryLogin::dictionary_starter_database_to_internal.size()));
        return;
    }
    profileIndex=DictionaryLogin::dictionary_starter_database_to_internal.at(starter);
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput("profile index: "+std::to_string(profileIndex)+
                    " out of range (profileList size: "+std::to_string(CommonDatapack::commonDatapack.profileList.size())+")");
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(characterId);
        #endif
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        characterSelectionIsWrong(query_id,0x04,"profile index: "+std::to_string(profileIndex)+" profil not valid");
        return;
    }

    const uint8_t &blob_version=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(18),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version not a number");
        return;
    }
    if(blob_version!=CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version incorrect");
        return;
    }

    //allow
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(10),&ok);
        const char * const data_raw=data.data();
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"allow not in hexa");
            return;
        }
        unsigned int pos=0;
        while(pos<data.size())
        {
            const uint8_t &allow=data_raw[pos];
            if(allow<1 || allow>1)
                public_and_private_informations.allow.insert(static_cast<ActionAllow>(allow));
            else
            {
                ok=false;
                normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not reverse");
            }
            pos+=1;
        }
    }
    //item
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(11),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"item not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(2+4)!=0)
        {
            characterSelectionIsWrong(query_id,0x04,"item have wrong size");
            return;
        }
        while(pos<data.size())
        {
            const uint16_t &item=le16toh(*reinterpret_cast<const uint16_t *>(data_raw+pos));
            pos+=2;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.items.item.find(item)==CommonDatapack::commonDatapack.items.item.cend())
                normalOutput("Take care load unknown item: "+std::to_string(item));
            #endif
            const uint32_t &quantity=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
            pos+=4;
            public_and_private_informations.items[item]=quantity;
        }
    }
    //item_warehouse
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(12),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"item_warehouse not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(2+4)!=0)
        {
            characterSelectionIsWrong(query_id,0x04,"item warehouse have wrong size");
            return;
        }
        while(pos<data.size())
        {
            const uint16_t &item=le16toh(*reinterpret_cast<const uint16_t *>(data_raw+pos));
            pos+=2;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.items.item.find(item)==CommonDatapack::commonDatapack.items.item.cend())
                normalOutput("Take care load unknown item: "+std::to_string(item));
            #endif
            const uint32_t &quantity=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
            pos+=4;
            public_and_private_informations.warehouse_items[item]=quantity;
        }
    }
    //recipes
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(13),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"recipes not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.recipes!=NULL)
        {
            delete public_and_private_informations.recipes;
            public_and_private_informations.recipes=NULL;
        }
        public_and_private_informations.recipes=(char *)malloc(CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
        memset(public_and_private_informations.recipes,0x00,CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1))
            memcpy(public_and_private_informations.recipes,data_raw,CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
        else
            memcpy(public_and_private_informations.recipes,data_raw,data.size());
    }
    //reputations
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(14),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"reputations not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(4+1+1)!=0)
        {
            characterSelectionIsWrong(query_id,0x04,"reputations have wrong size");
            return;
        }
        while(pos<data.size())
        {
            PlayerReputation playerReputation;
            playerReputation.point=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
            pos+=4;
            const uint8_t &type=data_raw[pos];
            pos+=1;
            playerReputation.level=data_raw[pos];
            pos+=1;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(playerReputation.level<-100 || playerReputation.level>100)
            {
                normalOutput("reputation level is <100 or >100, skip: "+std::to_string(type));
                continue;
            }
            if(type>=DictionaryLogin::dictionary_reputation_database_to_internal.size())
            {
                normalOutput("The reputation: "+std::to_string(type)+" don't exist");
                continue;
            }
            if(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)==-1)
            {
                normalOutput("The reputation: "+std::to_string(type)+" not resolved");
                continue;
            }
            if(playerReputation.level>=0)
            {
                if((uint32_t)playerReputation.level>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())
                {
                    normalOutput("The reputation level "+std::to_string(type)+
                                 " is wrong because is out of range (reputation level: "+std::to_string(playerReputation.level)+
                                 " > max level: "+
                                 std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())+
                                 ")");
                    continue;
                }
            }
            else
            {
                if((uint32_t)(-playerReputation.level)>CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())
                {
                    normalOutput("The reputation level "+std::to_string(type)+
                                 " is wrong because is out of range (reputation level: "+std::to_string(playerReputation.level)+
                                 " < max level: "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())+")");
                    continue;
                }
            }
            if(playerReputation.point>0)
            {
                if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size()==(uint32_t)(playerReputation.level+1))//start at level 0 in positive
                {
                    normalOutput("The reputation level is already at max, drop point");
                    playerReputation.point=0;
                }
                if(playerReputation.point>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(playerReputation.level+1))//start at level 0 in positive
                {
                    normalOutput("The reputation point "+std::to_string(playerReputation.point)+
                                 " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(playerReputation.level)));
                    continue;
                }
            }
            else if(playerReputation.point<0)
            {
                if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size()==(uint32_t)-playerReputation.level)//start at level -1 in negative
                {
                    normalOutput("The reputation level is already at min, drop point");
                    playerReputation.point=0;
                }
                if(playerReputation.point<CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(-playerReputation.level))//start at level -1 in negative
                {
                    normalOutput("The reputation point "+std::to_string(playerReputation.point)+
                                 " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(playerReputation.level)));
                    continue;
                }
            }
            #endif
            public_and_private_informations.reputation[type]=playerReputation;
        }
    }
    //encyclopedia_monster
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(15),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"encyclopedia_monster not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.encyclopedia_monster!=NULL)
        {
            delete public_and_private_informations.encyclopedia_monster;
            public_and_private_informations.encyclopedia_monster=NULL;
        }
        public_and_private_informations.encyclopedia_monster=(char *)malloc(CommonDatapack::commonDatapack.monstersMaxId/8+1);
        memset(public_and_private_informations.encyclopedia_monster,0x00,CommonDatapack::commonDatapack.monstersMaxId/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.monstersMaxId/8+1))
            memcpy(public_and_private_informations.encyclopedia_monster,data_raw,CommonDatapack::commonDatapack.monstersMaxId/8+1);
        else
            memcpy(public_and_private_informations.encyclopedia_monster,data_raw,data.size());
    }
    //encyclopedia_item
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(16),&ok);
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"encyclopedia_item not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.encyclopedia_item!=NULL)
        {
            delete public_and_private_informations.encyclopedia_item;
            public_and_private_informations.encyclopedia_item=NULL;
        }
        public_and_private_informations.encyclopedia_item=(char *)malloc(CommonDatapack::commonDatapack.items.itemMaxId/8+1);
        memset(public_and_private_informations.encyclopedia_item,0x00,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.items.itemMaxId/8+1))
            memcpy(public_and_private_informations.encyclopedia_item,data_raw,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
        else
            memcpy(public_and_private_informations.encyclopedia_item,data_raw,data.size());
    }

    //achievements(17) ignored for now

    Client::selectCharacterServer(query_id,characterId);
}





























void Client::selectCharacterServer(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_character_server_by_id.empty())
    {
        characterSelectionIsWrong(query_id,0x04,"selectCharacterServer() Query is empty, bug");
        return;
    }
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;

    const std::string &queryText=PreparedDBQueryServer::db_query_character_server_by_id.compose(
                std::to_string(characterId)
                );
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::selectCharacterServer_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        characterSelectionIsWrong(query_id,0x02,queryText+": "+GlobalServerData::serverPrivateVariables.db_server->errorMessage());
        delete selectCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack.push(selectCharacterParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("SelectCharacterParam");
        #endif
        callbackRegistred.push(callback);
    }
}

void Client::selectCharacterServer_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectCharacterServer_object();
    GlobalServerData::serverPrivateVariables.db_server->clear();
}

void Client::selectCharacterServer_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    SelectCharacterParam *selectCharacterParam=static_cast<SelectCharacterParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacterServer_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    delete selectCharacterParam;
    GlobalServerData::serverPrivateVariables.db_server->clear();
}

void Client::selectCharacterServer_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="SelectCharacterParam")
    {
        std::cerr << "is not SelectCharacterParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    /*map(0),x(1),y(2),orientation(3),
    rescue_map(4),rescue_x(5),rescue_y(6),rescue_orientation(7),
    unvalidated_rescue_map(8),unvalidated_rescue_x(9),unvalidated_rescue_y(10),unvalidated_rescue_orientation(11),
    market_cash(12),botfight_id(13),itemonmap(14),quest(15),blob_version(16),plants(17)*/
    callbackRegistred.pop();
    const auto &characterIdString=std::to_string(characterId);
    const auto &sFrom1970String=std::to_string(sFrom1970());
    if(!GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);

        dbQueryWriteServer(serverProfileInternal.preparedQueryAddCharacterForServer.compose(
                               characterIdString,
                               sFrom1970String
                               )
                           );
        const std::string &queryText=PreparedDBQueryCommon::db_query_update_character_last_connect.compose(
                    sFrom1970String,
                    characterIdString
                    );
        dbQueryWriteCommon(queryText);

        characterIsRightWithParsedRescue(query_id,
            characterId,
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //rescue
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation,
            //last unvalidated
            serverProfileInternal.map,
            serverProfileInternal.x,
            serverProfileInternal.y,
            serverProfileInternal.orientation
        );
        return;
    }

    bool ok;
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(simplifiedIdList.size()<=0)
        {
            characterSelectionIsWrong(query_id,0x04,"Not free id to login");
            return;
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }

    const uint8_t &blob_version=GlobalServerData::serverPrivateVariables.db_server->stringtouint8(GlobalServerData::serverPrivateVariables.db_server->value(16),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version not a number");
        return;
    }
    if(blob_version!=CATCHCHALLENGER_SERVER_DATABASE_SERVER_BLOBVERSION)
    {
        characterSelectionIsWrong(query_id,0x04,"Blob version incorrect");
        return;
    }

    market_cash=GlobalServerData::serverPrivateVariables.db_server->stringtouint64(GlobalServerData::serverPrivateVariables.db_server->value(12),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"Market cash wrong: "+GlobalServerData::serverPrivateVariables.db_server->value(12));
        return;
    }

    //botfight_id
    {
        const std::vector<char> &botfight_id=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(13),&ok);
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        const char * const raw_botfight_id=botfight_id.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"botfight_id: "+GlobalServerData::serverPrivateVariables.db_server->value(13)+" is not a hexa");
            return;
        }
        else
        {
            uint16_t index=0;
            while(index<=CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId && (index/8)<botfight_id.size())
            {
                const uint16_t &bitgetUp=index;
                if(
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        botfight_id
                        #else
                        raw_botfight_id
                        #endif
                        [bitgetUp/8] & (1<<(7-bitgetUp%8)))
                    public_and_private_informations.bot_already_beaten.insert(bitgetUp);
                index++;
            }
        }
    }
    //itemonmap
    {
        const std::vector<char> &itemonmap=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(14),&ok);
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        const char * const raw_itemonmap=itemonmap.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"itemonmap: "+GlobalServerData::serverPrivateVariables.db_server->value(14)+" is not a hexa");
            return;
        }
        else
        {
            public_and_private_informations.itemOnMap.reserve(itemonmap.size());
            uint32_t pos=0;
            while(pos<itemonmap.size())
            {
                const uint32_t &pointOnMapDatabaseId=
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                itemonmap
                #else
                raw_itemonmap
                #endif
                [pos];
                if(!ok)
                {
                    normalOutput("wrong value type for item on map, skip: "+std::to_string(pointOnMapDatabaseId));
                    continue;
                }
                if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
                {
                    normalOutput("item on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
                    continue;
                }
                if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).map==NULL)
                {
                    normalOutput("item on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
                    continue;
                }
                const uint8_t &indexOfItemOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).indexOfItemOnMap;
                if(indexOfItemOnMap==255)
                {
                    normalOutput("item on map is not into the map list (3), skip: "+std::to_string(pointOnMapDatabaseId));
                    continue;
                }
                if(indexOfItemOnMap>=Client::indexOfItemOnMap)
                {
                    normalOutput("item on map is not into the map list (4), skip: "+std::to_string(indexOfItemOnMap)+" on "+std::to_string(Client::indexOfItemOnMap));
                    continue;
                }
                public_and_private_informations.itemOnMap.insert(indexOfItemOnMap);
                ++pos;
            }
        }
    }
    //plants
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    {
        const std::vector<char> &plants=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(17),&ok);
        const char * const raw_plants=plants.data();
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"plants: "+GlobalServerData::serverPrivateVariables.db_server->value(17)+" is not a hexa");
            return;
        }
        else
        {
            if(plants.size()%(1+1+8)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"plants missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(17));
                return;
            }
            else
            {
                public_and_private_informations.plantOnMap.reserve(plants.size()/(1+1+8));
                PlayerPlant plant;
                uint32_t pos=0;
                while(pos<plants.size())
                {
                    const uint8_t &pointOnMap=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            plants
                            #else
                            raw_plants
                            #endif
                            [pos];
                    ++pos;

                    if(pointOnMap>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
                    {
                        normalOutput("dirt on map is not into the map list (1), skip: "+std::to_string(pointOnMap));
                        pos+=1+8;
                        continue;
                    }
                    if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMap).map==NULL)
                    {
                        normalOutput("dirt on map is not into the map list (2), skip: "+std::to_string(pointOnMap));
                        pos+=1+8;
                        continue;
                    }
                    const uint8_t &indexOfDirtOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMap).indexOfDirtOnMap;
                    if(indexOfDirtOnMap==255)
                    {
                        normalOutput("dirt on map is not into the map list (3), skip: "+std::to_string(pointOnMap));
                        pos+=1+8;
                        continue;
                    }

                    plant.plant=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            plants
                            #else
                            raw_plants
                            #endif
                            [pos];
                    ++pos;

                    if(CommonDatapack::commonDatapack.plants.find(plant.plant)==CommonDatapack::commonDatapack.plants.cend())
                    {
                        normalOutput("wrong value type for plant dirt on map, skip: "+std::to_string(plant.plant));
                        pos+=8;
                        continue;
                    }

                    uint64_t mature_at;
                    memcpy(&mature_at,
                            raw_plants
                            +pos,sizeof(uint64_t));
                    plant.mature_at=le64toh(mature_at)+CommonDatapack::commonDatapack.plants.at(plant.plant).fruits_seconds;
                    pos+=8;

                    public_and_private_informations.plantOnMap[pointOnMap]=plant;
                }
            }
        }
    }
    #endif
    //quest
    {
        const std::vector<char> &quests=GlobalServerData::serverPrivateVariables.db_server->hexatoBinary(GlobalServerData::serverPrivateVariables.db_server->value(15),&ok);
        #ifndef CATCHCHALLENGER_EXTRA_CHECK
        const char * const raw_quests=quests.data();
        #endif
        if(!ok)
        {
            characterSelectionIsWrong(query_id,0x04,"plants: "+GlobalServerData::serverPrivateVariables.db_server->value(15)+" is not a hexa");
            return;
        }
        else
        {
            if(quests.size()%(1+1+1)!=0)
            {
                characterSelectionIsWrong(query_id,0x04,"plants missing data: "+GlobalServerData::serverPrivateVariables.db_server->value(15));
                return;
            }
            else
            {
                public_and_private_informations.quests.reserve(quests.size()/(1+1+1));
                PlayerQuest playerQuest;
                uint32_t pos=0;
                while(pos<quests.size())
                {
                    uint8_t questId=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;
                    playerQuest.finish_one_time=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;
                    playerQuest.step=
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            quests
                            #else
                            raw_quests
                            #endif
                            [pos];
                    ++pos;

                    if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(questId)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
                    {
                        normalOutput("wrong value type for quest on map, skip: "+std::to_string(questId));
                        pos+=2;
                        continue;
                    }

                    const Quest &datapackQuest=CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId);
                    if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>datapackQuest.steps.size())
                    {
                        normalOutput("step out of quest range, skip: "+std::to_string(questId));
                        continue;
                    }
                    if(playerQuest.step<=0 && !playerQuest.finish_one_time)
                    {
                        normalOutput("can't be to step 0 if have never finish the quest, skip: "+std::to_string(questId));
                        continue;
                    }
                    public_and_private_informations.quests[questId]=playerQuest;
                    if(playerQuest.step>0)
                        addQuestStepDrop(questId,playerQuest.step);
                }
            }
        }
    }

    public_and_private_informations.public_informations.speed=CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    Orientation orientation;
    const uint32_t &orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(3),&ok);
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            orientation=static_cast<Orientation>(orientationInt);
        else
        {
            orientation=Orientation_bottom;
            normalOutput("Wrong orientation corrected with bottom");
        }
    }
    else
    {
        orientation=Orientation_bottom;
        normalOutput("Wrong orientation (not number) corrected with bottom: "+GlobalServerData::serverPrivateVariables.db_server->value(3));
    }
    //all is rights
    const uint32_t &map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"map_database_id is not a number");
        return;
    }
    if(map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        characterSelectionIsWrong(query_id,0x04,"map_database_id out of range");
        return;
    }
    CommonMap * const map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_database_id));
    if(map==NULL)
    {
        characterSelectionIsWrong(query_id,0x04,"map_database_id have not reverse: "+std::to_string(map_database_id)+", mostly due to start previously start with another mainDatapackCode");
        return;
    }
    const uint8_t &x=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"x coord is not a number");
        return;
    }
    const uint8_t &y=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x04,"y coord is not a number");
        return;
    }
    if(x>=map->width)
    {
        characterSelectionIsWrong(query_id,0x04,"x to out of map: "+std::to_string(x)+" >= "+std::to_string(map->width)+" ("+map->map_file+")");
        return;
    }
    if(y>=map->height)
    {
        characterSelectionIsWrong(query_id,0x04,"y to out of map: "+std::to_string(y)+" >= "+std::to_string(map->height)+" ("+map->map_file+")");
        return;
    }
    characterIsRightWithRescue(query_id,
        characterId,
        map,
        x,
        y,
        orientation,
            GlobalServerData::serverPrivateVariables.db_server->value(4),
            GlobalServerData::serverPrivateVariables.db_server->value(5),
            GlobalServerData::serverPrivateVariables.db_server->value(6),
            GlobalServerData::serverPrivateVariables.db_server->value(7),
            GlobalServerData::serverPrivateVariables.db_server->value(8),
            GlobalServerData::serverPrivateVariables.db_server->value(9),
            GlobalServerData::serverPrivateVariables.db_server->value(10),
            GlobalServerData::serverPrivateVariables.db_server->value(11)
    );
}



































void Client::characterIsRightWithRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                  const std::string &rescue_map, const std::string &rescue_x, const std::string &rescue_y, const std::string &rescue_orientation,
                  const std::string &unvalidated_rescue_map, const std::string &unvalidated_rescue_x, const std::string &unvalidated_rescue_y, const std::string &unvalidated_rescue_orientation
                                             )
{
    bool ok;
    const uint32_t &rescue_map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_map,&ok);
    if(!ok)
    {
        normalOutput("rescue_map_database_id is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput("rescue_map_database_id out of range");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id)==NULL)
    {
        normalOutput("rescue_map_database_id have not reverse");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_database_id);
    if(rescue_map_final==NULL)
    {
        normalOutput("rescue map not resolved");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_x=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_x,&ok);
    if(!ok)
    {
        normalOutput("rescue x coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &rescue_new_y=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_y,&ok);
    if(!ok)
    {
        normalOutput("rescue y coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_x>=rescue_map_final->width)
    {
        normalOutput("rescue x to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(rescue_new_y>=rescue_map_final->height)
    {
        normalOutput("rescue y to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint32_t &orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(rescue_orientation,&ok);
    Orientation rescue_new_orientation;
    if(ok)
    {
        if(orientationInt>=1 && orientationInt<=4)
            rescue_new_orientation=static_cast<Orientation>(orientationInt);
        else
        {
            rescue_new_orientation=Orientation_bottom;
            normalOutput("Wrong rescue orientation corrected with bottom");
        }
    }
    else
    {
        rescue_new_orientation=Orientation_bottom;
        normalOutput("Wrong rescue orientation (not number) corrected with bottom");
    }





    const uint32_t &unvalidated_rescue_map_database_id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_map,&ok);
    if(!ok)
    {
        normalOutput("unvalidated_rescue_map_database_id is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_map_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        normalOutput("unvalidated_rescue_map_database_id out of range");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id)==NULL)
    {
        normalOutput("unvalidated_rescue_map_database_id have not reverse");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    CommonMap *unvalidated_rescue_map_final=DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_database_id);
    if(unvalidated_rescue_map_final==NULL)
    {
        normalOutput("unvalidated rescue map not resolved");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_x=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_x,&ok);
    if(!ok)
    {
        normalOutput("unvalidated rescue x coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    const uint8_t &unvalidated_rescue_new_y=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_y,&ok);
    if(!ok)
    {
        normalOutput("unvalidated rescue y coord is not a number");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_x>=unvalidated_rescue_map_final->width)
    {
        normalOutput("unvalidated rescue x to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    if(unvalidated_rescue_new_y>=unvalidated_rescue_map_final->height)
    {
        normalOutput("unvalidated rescue y to out of map");
        characterIsRight(query_id,characterId,map,x,y,orientation);
        return;
    }
    Orientation unvalidated_rescue_new_orientation;
    const uint32_t &unvalidated_orientationInt=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(unvalidated_rescue_orientation,&ok);
    if(ok)
    {
        if(unvalidated_orientationInt>=1 && unvalidated_orientationInt<=4)
            unvalidated_rescue_new_orientation=static_cast<Orientation>(unvalidated_orientationInt);
        else
        {
            unvalidated_rescue_new_orientation=Orientation_bottom;
            normalOutput("Wrong unvalidated orientation corrected with bottom");
        }
    }
    else
    {
        unvalidated_rescue_new_orientation=Orientation_bottom;
        normalOutput("Wrong unvalidated orientation (not number) corrected with bottom");
    }





    characterIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,
                                 rescue_map_final,rescue_new_x,rescue_new_y,rescue_new_orientation,
                                 unvalidated_rescue_map_final,unvalidated_rescue_new_x,unvalidated_rescue_new_y,unvalidated_rescue_new_orientation
            );
}

void Client::characterIsRight(const uint8_t &query_id,uint32_t characterId, CommonMap *map, const uint8_t &x, const uint8_t &y, const Orientation &orientation)
{
    characterIsRightWithParsedRescue(query_id,characterId,map,x,y,orientation,map,x,y,orientation,map,x,y,orientation);
}

void Client::characterIsRightWithParsedRescue(const uint8_t &query_id, uint32_t characterId, CommonMap* map, const /*COORD_TYPE*/ uint8_t &x, const /*COORD_TYPE*/ uint8_t &y, const Orientation &orientation,
                  CommonMap* rescue_map, const /*COORD_TYPE*/ uint8_t &rescue_x, const /*COORD_TYPE*/ uint8_t &rescue_y, const Orientation &rescue_orientation,
                  CommonMap *unvalidated_rescue_map, const uint8_t &unvalidated_rescue_x, const uint8_t &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                  )
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_clan.empty())
    {
        errorOutput("loginIsRightWithParsedRescue() Query is empty, bug");
        return;
    }
    #endif
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(simplifiedIdList.empty())
        {
            errorOutput("Client::characterIsRightWithParsedRescue(): simplifiedIdList is empty, no more id");
            return;
        }
        public_and_private_informations.public_informations.simplifiedId = simplifiedIdList.front();
        simplifiedIdList.erase(simplifiedIdList.begin());
        break;
        case MapVisibilityAlgorithmSelection_None:
        public_and_private_informations.public_informations.simplifiedId = 0;
        break;
    }
    //load the variables
    character_id=characterId;
    stat=ClientStat::CharacterSelecting;
    GlobalServerData::serverPrivateVariables.connected_players_id_list.insert(characterId);
    connectedSince=sFrom1970();
    this->map=map;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif
    this->x=x;
    this->y=y;
    this->last_direction=static_cast<Direction>(orientation);
    this->rescue.map=rescue_map;
    this->rescue.x=rescue_x;
    this->rescue.y=rescue_y;
    this->rescue.orientation=rescue_orientation;
    this->unvalidated_rescue.map=unvalidated_rescue_map;
    this->unvalidated_rescue.x=unvalidated_rescue_x;
    this->unvalidated_rescue.y=unvalidated_rescue_y;
    this->unvalidated_rescue.orientation=unvalidated_rescue_orientation;
    selectCharacterQueryId.push_back(query_id);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(public_and_private_informations.clan!=0)
    {
        if(GlobalServerData::serverPrivateVariables.clanList.find(public_and_private_informations.clan)!=GlobalServerData::serverPrivateVariables.clanList.cend())
        {
            GlobalServerData::serverPrivateVariables.clanList[public_and_private_informations.clan]->players.push_back(this);
            loadMonsters();
            return;
        }
        else
        {
            normalOutput("First client of the clan: "+std::to_string(public_and_private_informations.clan)+", get the info");
            //do the query
            const std::string &queryText=PreparedDBQueryCommon::db_query_clan.compose(
                        std::to_string(public_and_private_informations.clan)
                        );
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::selectClan_static);
            if(callback==NULL)
            {
                std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
                loadMonsters();
                return;
            }
            else
                callbackRegistred.push(callback);
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            std::string mapToDebug=this->map->map_file;
            mapToDebug+=this->map->map_file;
        }
        #endif
        loadMonsters();
        return;
    }
}

/*void Client::loadItemOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_itemOnMap.empty())
    {
        errorOutput("loadBotAlreadyBeaten() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_itemOnMap;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadItemOnMap_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        loadPlantOnMap();
        #else
        characterIsRightFinalStep();
        #endif
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadItemOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadItemOnMap_return();
}

void Client::loadItemOnMap_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &pointOnMapDatabaseId=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for item on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput("item on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput("item on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        const uint8_t &indexOfItemOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).indexOfItemOnMap;
        if(indexOfItemOnMap==255)
        {
            normalOutput("item on map is not into the map list (3), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfItemOnMap>=Client::indexOfItemOnMap)
        {
            normalOutput("item on map is not into the map list (4), skip: "+std::to_string(indexOfItemOnMap)+" on "+std::to_string(Client::indexOfItemOnMap));
            continue;
        }
        public_and_private_informations.itemOnMap.insert(indexOfItemOnMap);
    }
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    loadPlantOnMap();
    #else
    characterIsRightFinalStep();
    #endif
}*/


/*#ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
void Client::loadPlantOnMap()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_plant.empty())
    {
        errorOutput("loadBotAlreadyBeaten() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_plant;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadPlantOnMap_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        characterIsRightFinalStep();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadPlantOnMap_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlantOnMap_return();
}

void Client::loadPlantOnMap_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        const uint16_t &pointOnMapDatabaseId=GlobalServerData::serverPrivateVariables.db_server->stringtouint16(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for dirt on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(pointOnMapDatabaseId>=DictionaryServer::dictionary_pointOnMap_database_to_internal.size())
        {
            normalOutput("dirt on map is not into the map list (1), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).map==NULL)
        {
            normalOutput("dirt on map is not into the map list (2), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        const uint8_t &indexOfDirtOnMap=DictionaryServer::dictionary_pointOnMap_database_to_internal.at(pointOnMapDatabaseId).indexOfDirtOnMap;
        if(indexOfDirtOnMap==255)
        {
            normalOutput("dirt on map is not into the map list (3), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(indexOfDirtOnMap>=Client::indexOfDirtOnMap)
        {
            normalOutput("dirt on map is not into the map list (4), skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }

        const uint32_t &plant=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(1),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for dirt plant id on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        if(CommonDatapack::commonDatapack.plants.find(plant)==CommonDatapack::commonDatapack.plants.cend())
        {
            normalOutput("wrong value type for plant dirt on map, skip: "+std::to_string(plant));
            continue;
        }
        const uint32_t &plant_timestamps=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok);
        if(!ok)
        {
            normalOutput("wrong value type for plant timestamps on map, skip: "+std::to_string(pointOnMapDatabaseId));
            continue;
        }
        {
            PlayerPlant playerPlant;
            playerPlant.plant=plant;
            playerPlant.mature_at=plant_timestamps+CommonDatapack::commonDatapack.plants.at(plant).fruits_seconds;
            public_and_private_informations.plantOnMap[indexOfDirtOnMap]=playerPlant;
        }
    }
    characterIsRightFinalStep();
}
#endif*/



void Client::characterIsRightFinalStep()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    stat=ClientStat::CharacterSelected;

    const uint8_t &query_id=selectCharacterQueryId.front();
    selectCharacterQueryId.erase(selectCharacterQueryId.begin());

    //send the network reply
    removeFromQueryReceived(query_id);

    uint32_t posOutput=0;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,Client::characterIsRightFinalStepHeader,Client::characterIsRightFinalStepHeaderSize);
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
    posOutput+=Client::characterIsRightFinalStepHeaderSize;

    /// \todo optimise and cache this block
    //send the event
    {
        std::vector<std::pair<uint8_t,uint8_t> > events;
        unsigned int index=0;
        while(index<GlobalServerData::serverPrivateVariables.events.size())
        {
            const uint8_t &value=GlobalServerData::serverPrivateVariables.events.at(index);
            if(value!=0)
                events.push_back(std::pair<uint8_t,uint8_t>(index,value));
            index++;
        }
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=events.size();
        posOutput+=1;
        while(index<events.size())
        {
            const std::pair<uint8_t,uint8_t> &event=events.at(index);
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=event.first;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=event.second;
            posOutput+=1;
            index++;
        }
    }

    //temporary character id
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.public_informations.simplifiedId;
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.public_informations.simplifiedId);
        posOutput+=2;
    }

    //pseudo
    {
        const std::string &text=public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.allow.size();
    posOutput+=1;
    {
        auto i=public_and_private_informations.allow.begin();
        while(i!=public_and_private_informations.allow.cend())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=*i;
            posOutput+=1;
            ++i;
        }
    }

    //clan related
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(public_and_private_informations.clan);
    posOutput+=4;
    if(public_and_private_informations.clan_leader)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;

    {
        const uint64_t cash=htole64(public_and_private_informations.cash);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash,8);
        posOutput+=8;
    }
    {
        const uint64_t cash=htole64(public_and_private_informations.warehouse_cash);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash,8);
        posOutput+=8;
    }


    //temporary variable
    uint32_t index;
    uint32_t size;

    //send monster
    index=0;
    size=public_and_private_informations.playerMonster.size();
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=size;
    posOutput+=1;
    while(index<size)
    {
        posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.playerMonster.at(index));
        index++;
    }
    index=0;
    size=public_and_private_informations.warehouse_playerMonster.size();
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=size;
    posOutput+=1;
    while(index<size)
    {
        posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.warehouse_playerMonster.at(index));
        index++;
    }

    /// \todo force to 255 max
    //send reputation
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.reputation.size();
        posOutput+=1;
        auto i=public_and_private_informations.reputation.begin();
        while(i!=public_and_private_informations.reputation.cend())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->first;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->second.level;
            posOutput+=1;
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(i->second.point);
            posOutput+=4;
            ++i;
        }
    }
    /// \todo make the buffer overflow control here or above
    {
        char *buffer;
        if(ProtocolParsingBase::compressionTypeServer==CompressionType::None)
            buffer=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
        else
            buffer=ProtocolParsingBase::tempBigBufferForCompressedOutput;
        uint32_t posOutputTemp=0;
        //recipes
        if(public_and_private_informations.recipes!=NULL)
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.recipes,CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
            posOutputTemp+=CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }
        //encyclopedia_monster
        if(public_and_private_informations.encyclopedia_monster!=NULL)
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(CommonDatapack::commonDatapack.monstersMaxId/8+1);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.encyclopedia_monster,CommonDatapack::commonDatapack.monstersMaxId/8+1);
            posOutputTemp+=CommonDatapack::commonDatapack.monstersMaxId/8+1;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }
        //encyclopedia_item
        if(public_and_private_informations.encyclopedia_item!=NULL)
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(CommonDatapack::commonDatapack.items.itemMaxId/8+1);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.encyclopedia_item,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
            posOutputTemp+=CommonDatapack::commonDatapack.items.itemMaxId/8+1;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }
        //achievements
        buffer[posOutputTemp]=0;
        posOutputTemp++;

        if(ProtocolParsingBase::compressionTypeServer==CompressionType::None)
            posOutput=posOutputTemp;
        else
        {
            //compress
            const int32_t &compressedSize=computeCompression(buffer,ProtocolParsingBase::tempBigBufferForOutput+posOutput+4,posOutputTemp,sizeof(ProtocolParsingBase::tempBigBufferForOutput)-posOutput-1-4,ProtocolParsingBase::compressionTypeServer);
            if(compressedSize<0)
            {
                errorOutput("Error to compress the data");
                return;
            }
            //copy
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(compressedSize);
            posOutput+=4;
            posOutput+=compressedSize;
        }
    }

    //------------------------------------------- End of common part, start of server specific part ----------------------------------

    /// \todo force to 255 max
    //send quest
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.quests.size());
        posOutput+=2;
        auto j=public_and_private_informations.quests.begin();
        while(j!=public_and_private_informations.quests.cend())
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(j->first);
            posOutput+=2;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=j->second.step;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=j->second.finish_one_time;
            posOutput+=1;
            ++j;
        }
    }

    //send bot_already_beaten
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.bot_already_beaten.size());
        posOutput+=2;
        auto k=public_and_private_informations.bot_already_beaten.begin();
        while(k!=public_and_private_informations.bot_already_beaten.cend())
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(*k);
            posOutput+=2;
            ++k;
        }
    }

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.itemOnMap.size();
    posOutput+=1;
    {
        auto i=public_and_private_informations.itemOnMap.begin();
        while (i!=public_and_private_informations.itemOnMap.cend())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=*i;
            posOutput+=1;
            ++i;
        }
    }

    //send plant on map
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    const auto &time=sFrom1970();
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=public_and_private_informations.plantOnMap.size();
    posOutput+=1;
    auto i=public_and_private_informations.plantOnMap.begin();
    while(i!=public_and_private_informations.plantOnMap.cend())
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->first;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->second.plant;
        posOutput+=1;
        /// \todo Can essaylly int 16 ovbertflow
        if(time<i->second.mature_at)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(i->second.mature_at-time);
        else
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
        posOutput+=2;
        ++i;
    }
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    if(!sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput))
        return;

    if(this->map==NULL)
        return;

    if(!sendInventory())
        return;
    updateCanDoFight();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    //send signals into the server
    #ifndef SERVERBENCHMARK
    if(this->map==NULL)
        return;
    normalOutput("Logged: "+public_and_private_informations.public_informations.pseudo+
                 " on the map: "+map->map_file+
                 " ("+std::to_string(x)+","+std::to_string(y)+")");
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("load the normal player id: "+std::to_string(character_id)+", simplified id: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_player_is_connected(public_and_private_informations);
    #endif
    GlobalServerData::serverPrivateVariables.connected_players++;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        GlobalServerData::serverPrivateVariables.player_updater.addConnectedPlayer();
    playerByPseudo[public_and_private_informations.public_informations.pseudo]=this;
    clientBroadCastList.push_back(this);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(this->map==NULL)
            return;
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(this->map==NULL)
        return;
    put_on_the_map(
                map,//map pointer
        x,
        y,
        static_cast<Orientation>(last_direction)
    );

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(this->map==NULL)
            return;
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif
}

void Client::selectClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->selectClan_return();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::selectClan_return()
{
    callbackRegistred.pop();
    //parse the result
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        bool ok;
        uint64_t cash=GlobalServerData::serverPrivateVariables.db_common->stringtouint64(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            cash=0;
            normalOutput("Warning: clan linked: have wrong cash value, then reseted to 0");
        }
        haveClanInfo(public_and_private_informations.clan,GlobalServerData::serverPrivateVariables.db_common->value(0),cash);
    }
    else
    {
        public_and_private_informations.clan=0;
        normalOutput("Warning: clan linked: %1 is not found into db");
    }
    loadMonsters();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::loginIsWrong(const uint8_t &query_id, const uint8_t &returnCode, const std::string &debugMessage)
{
    //network send
    *(Client::loginIsWrongBuffer+1)=query_id;
    *(Client::loginIsWrongBuffer+1+1+4)=returnCode;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    removeFromQueryReceived(query_id);
    #endif
    inputQueryNumberToPacketCode[query_id]=0;
    internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));

    //send to server to stop the connection
    errorOutput(debugMessage);
}
#endif

/*void Client::loadPlayerAllow()
{
    if(!PreparedDBQueryCommon::db_query_select_allow.empty())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_select_allow;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadPlayerAllow_static);
        if(callback==NULL)
        {
            std::cerr << "Sql error for: " << PreparedDBQueryCommon::db_query_select_allow << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            loadItems();
            return;
        }
        else
            callbackRegistred.push(callback);
    }
}

void Client::loadPlayerAllow_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadPlayerAllow_return();
}

void Client::loadPlayerAllow_return()
{
    callbackRegistred.pop();
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &allowCode=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(ok)
        {
            if(allowCode<(uint32_t)DictionaryLogin::dictionary_allow_database_to_internal.size())
            {
                const ActionAllow &allow=DictionaryLogin::dictionary_allow_database_to_internal.at(allowCode);
                if(allow!=ActionAllow_Nothing)
                    public_and_private_informations.allow.insert(allow);
                else
                {
                    ok=false;
                    normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not reverse");
                }
            }
            else
            {
                ok=false;
                normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" out of reverse list");
            }
        }
        else
            normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not a number");
    }
    loadItems();
}*/
