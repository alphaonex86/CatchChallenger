#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "../../general/base/CommonDatapack.hpp"

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.hpp"
#endif

#include <chrono>

using namespace CatchChallenger;

void Client::selectCharacter(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_character_by_id.empty())
    {
        errorOutput("selectCharacter() Query is empty, bug");
        return;
    }
    /*if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_last_connect.empty())
    {
        errorOutput("selectCharacter() Query db_query_update_character_last_connect is empty, bug");
        return;
    }
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_time_to_delete.empty())
    {
        errorOutput("selectCharacter() Query db_query_update_character_time_to_delete is empty, bug");
        return;
    }*/
    #endif
    SelectCharacterParam *selectCharacterParam=new SelectCharacterParam;
    selectCharacterParam->query_id=query_id;
    selectCharacterParam->characterId=characterId;
    stat=ClientStat::CharacterSelecting;
    //std::cout << "Client::selectCharacter(), Try select character " << characterId << " with account " << account_id << "..." << std::endl;

    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_character_by_id.asyncRead(this,&Client::selectCharacter_static,{std::to_string(characterId)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_character_by_id.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x02,GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_character_by_id.queryText()+": "+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterParam==NULL)
        abort();
    #endif
    selectCharacter_return(selectCharacterParam->query_id,selectCharacterParam->characterId);
    paramToPassToCallBack.pop();
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
    encyclopedia_monster(15),encyclopedia_item(16),achievements(17),blob_version(18),date(19)*/

    callbackRegistred.pop();
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    {
        std::cerr << "Try select character " << characterId << " but not found with account " << account_id << std::endl;
        std::cerr << "Via: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_character_by_id.queryText() << ", callback list size: " << paramToPassToCallBack.size() << std::endl;
        std::cerr << "Maybe connected on another db than login server: " <<  DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType()) << " at ";
        if(GlobalServerData::serverSettings.database_common.host=="localhost")
            std::cerr << "localhost";
        else
            std::cerr << GlobalServerData::serverSettings.database_common.host/* << ":" << GlobalServerData::serverSettings.database_common.port*/;
        std::cerr << " on " << GlobalServerData::serverSettings.database_common.db << std::endl;
        character_id=characterId;
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
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x03,"character_loaded already to true, stat: "+std::to_string(stat));
        return;
    }
    const uint32_t &account_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
    if(!ok)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x02,"Account for character: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not an id");
        return;
    }
    if(this->account_id!=account_id)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x02,"Character: "+std::to_string(characterId)+" is not owned by the account: "+std::to_string(this->account_id));
        return;
    }
    if(GlobalServerData::serverPrivateVariables.connected_players_id_list.find(characterId)!=GlobalServerData::serverPrivateVariables.connected_players_id_list.cend())
    {
        character_id=characterId;
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
        GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_set_character_time_to_delete_to_zero.asyncWrite({std::to_string(characterId)});

    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_character_last_connect.asyncWrite({
                    std::to_string(sFrom1970()),
                    std::to_string(characterId)
                    });

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
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"start from base selection");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.get_profileList().size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"selectCharacter_return() profile common and server don't match: "+
                                  std::to_string(CommonDatapack::commonDatapack.get_profileList().size())+
                                  "!="+
                                  std::to_string(GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
                                  );
        return;
    }
    #endif
    if(starter>=DictionaryLogin::dictionary_starter_database_to_internal.size())
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"starter "+std::to_string(starter)+
                                  " >= DictionaryLogin::dictionary_starter_database_to_internal.size() "+std::to_string(DictionaryLogin::dictionary_starter_database_to_internal.size()));
        return;
    }
    profileIndex=DictionaryLogin::dictionary_starter_database_to_internal.at(starter);
    if(profileIndex>=CommonDatapack::commonDatapack.get_profileList().size())
    {
        errorOutput("profile index: "+std::to_string(profileIndex)+
                    " out of range (profileList size: "+std::to_string(CommonDatapack::commonDatapack.get_profileList().size())+")");
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(characterId);
        #endif
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"profile index: "+std::to_string(profileIndex)+" profil not valid");
        return;
    }

    const uint8_t &blob_version=GlobalServerData::serverPrivateVariables.db_common->stringtouint8(GlobalServerData::serverPrivateVariables.db_common->value(18),&ok);
    if(!ok)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"Blob version not a number");
        return;
    }
    if(blob_version!=GlobalServerData::serverPrivateVariables.server_blobversion_datapack)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"Blob version incorrect");
        return;
    }

    //allow
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(10),&ok);
        const char * const data_raw=data.data();
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"allow not in hexa");
            return;
        }
        unsigned int pos=0;
        while(pos<data.size())
        {
            const uint8_t &allow=data_raw[pos];
            if(allow>=1 && allow<=1)
                public_and_private_informations.allow.insert(static_cast<ActionAllow>(allow));
            else
            {
                ok=false;
                normalOutput("allow id: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not reverse");
            }
            pos+=1;
        }
    }
    //encyclopedia_monster
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(15),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"encyclopedia_monster not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.encyclopedia_monster!=NULL)
        {
            delete public_and_private_informations.encyclopedia_monster;
            public_and_private_informations.encyclopedia_monster=NULL;
        }
        public_and_private_informations.encyclopedia_monster=(char *)malloc(CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
        memset(public_and_private_informations.encyclopedia_monster,0x00,CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.get_monstersMaxId()/8+1))
            memcpy(public_and_private_informations.encyclopedia_monster,data_raw,CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
        else
            memcpy(public_and_private_informations.encyclopedia_monster,data_raw,data.size());
    }
    //encyclopedia_item
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(16),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"encyclopedia_item not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.encyclopedia_item!=NULL)
        {
            delete public_and_private_informations.encyclopedia_item;
            public_and_private_informations.encyclopedia_item=NULL;
        }
        public_and_private_informations.encyclopedia_item=(char *)malloc(CommonDatapack::commonDatapack.get_items().itemMaxId/8+1);
        memset(public_and_private_informations.encyclopedia_item,0x00,CommonDatapack::commonDatapack.get_items().itemMaxId/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.get_items().itemMaxId/8+1))
            memcpy(public_and_private_informations.encyclopedia_item,data_raw,CommonDatapack::commonDatapack.get_items().itemMaxId/8+1);
        else
            memcpy(public_and_private_informations.encyclopedia_item,data_raw,data.size());
    }
    //item
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(11),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"item not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(2+4)!=0)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"item have wrong size");
            return;
        }
        uint32_t lastItemId=0;
        while(pos<data.size())
        {
            uint32_t item=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(data_raw+pos))+lastItemId;
            if(item>65535)
                item-=65536;
            lastItemId=static_cast<uint16_t>(item);
            pos+=2;
            if(CommonDatapack::commonDatapack.get_items().item.find(static_cast<uint16_t>(item))==CommonDatapack::commonDatapack.get_items().item.cend())
                normalOutput("Take care load unknown item: "+std::to_string(item));
            else
            {
                const uint16_t &item16=static_cast<uint16_t>(item);
                const uint32_t &quantity=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
                public_and_private_informations.encyclopedia_item[item/8]|=(1<<(7-item%8));
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(public_and_private_informations.items.find(item16)!=public_and_private_informations.items.cend())
                {
                    character_id=characterId;
                    characterSelectionIsWrong(query_id,0x04,"item duplicate");
                    return;
                }
                #endif
                public_and_private_informations.items[item16]=quantity;
            }
            pos+=4;
        }
    }
    //item_warehouse
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(12),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"item_warehouse not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(2+4)!=0)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"item warehouse have wrong size");
            return;
        }
        uint32_t lastItemId=0;
        while(pos<data.size())
        {
            uint32_t item=(uint32_t)le16toh(*reinterpret_cast<const uint16_t *>(data_raw+pos))+lastItemId;
            if(item>65535)
                item-=65536;
            lastItemId=static_cast<uint16_t>(item);
            pos+=2;
            if(CommonDatapack::commonDatapack.get_items().item.find(static_cast<uint16_t>(item))==
                    CommonDatapack::commonDatapack.get_items().item.cend())
                normalOutput("Take care load unknown item: "+std::to_string(item));
            else
            {
                const uint16_t &item16=static_cast<uint16_t>(item);
                const uint32_t &quantity=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
                public_and_private_informations.encyclopedia_item[item/8]|=(1<<(7-item%8));
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(public_and_private_informations.warehouse_items.find(item16)!=public_and_private_informations.warehouse_items.cend())
                {
                    character_id=characterId;
                    characterSelectionIsWrong(query_id,0x04,"item duplicate in ware house");
                    return;
                }
                #else
                (void)item16;
                #endif
                public_and_private_informations.warehouse_items[static_cast<uint16_t>(item)]=quantity;
            }
            pos+=4;
        }
    }
    //recipes
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(13),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"recipes not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        if(public_and_private_informations.recipes!=NULL)
        {
            delete public_and_private_informations.recipes;
            public_and_private_informations.recipes=NULL;
        }
        public_and_private_informations.recipes=(char *)malloc(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
        memset(public_and_private_informations.recipes,0x00,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
        if(data.size()>(uint16_t)(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1))
            memcpy(public_and_private_informations.recipes,data_raw,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
        else
            memcpy(public_and_private_informations.recipes,data_raw,data.size());
    }
    //reputations
    {
        const std::vector<char> &data=GlobalServerData::serverPrivateVariables.db_common->hexatoBinary(GlobalServerData::serverPrivateVariables.db_common->value(14),&ok);
        if(!ok)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"reputations not in hexa");
            return;
        }
        const char * const data_raw=data.data();
        unsigned int pos=0;
        if(data.size()%(4+1+1)!=0)
        {
            character_id=characterId;
            characterSelectionIsWrong(query_id,0x04,"reputations have wrong size");
            return;
        }
        uint16_t lastReputationId=0;
        while(pos<data.size())
        {
            PlayerReputation playerReputation;
            playerReputation.point=le32toh(*reinterpret_cast<const uint32_t *>(data_raw+pos));
            pos+=4;
            uint16_t typeReputation=(uint16_t)data_raw[pos]+lastReputationId;
            if(typeReputation>255)
                typeReputation-=256;
            lastReputationId=static_cast<uint8_t>(typeReputation);
            pos+=1;
            playerReputation.level=data_raw[pos];
            pos+=1;
            if(playerReputation.level<-100 || playerReputation.level>100)
            {
                normalOutput("reputation level is <100 or >100, skip: "+std::to_string(typeReputation));
                continue;
            }
            if(typeReputation>=DictionaryLogin::dictionary_reputation_database_to_internal.size())
            {
                normalOutput("The reputation: "+std::to_string(typeReputation)+" don't exist");
                continue;
            }
            if(DictionaryLogin::dictionary_reputation_database_to_internal.at(typeReputation)==-1)
            {
                normalOutput("The reputation: "+std::to_string(typeReputation)+" not resolved");
                continue;
            }
            const int &reputationInternalIdFull=DictionaryLogin::dictionary_reputation_database_to_internal.at(typeReputation);
            const uint8_t &reputationInternalId=static_cast<uint8_t>(reputationInternalIdFull);
            if(reputationInternalIdFull>=0)
            {
                if(playerReputation.level>=0)
                {
                    if((uint32_t)playerReputation.level>=CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_positive.size())
                    {
                        normalOutput("The reputation level "+std::to_string(typeReputation)+
                                     " is wrong because is out of range (reputation level: "+std::to_string(playerReputation.level)+
                                     " > max level: "+
                                     std::to_string(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_positive.size())+
                                     ")");
                        continue;
                    }
                }
                else
                {
                    if((uint32_t)(-playerReputation.level)>CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_negative.size())
                    {
                        normalOutput("The reputation level "+std::to_string(typeReputation)+
                                     " is wrong because is out of range (reputation level: "+std::to_string(playerReputation.level)+
                                     " < max level: "+std::to_string(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_negative.size())+")");
                        continue;
                    }
                }
                if(playerReputation.point>0)
                {
                    if(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_positive.size()==(uint32_t)(playerReputation.level+1))//start at level 0 in positive
                    {
                        normalOutput("The reputation level is already at max, drop point");
                        playerReputation.point=0;
                    }
                    if(playerReputation.point>=CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_positive.at(playerReputation.level+1))//start at level 0 in positive
                    {
                        normalOutput("The reputation point "+std::to_string(playerReputation.point)+
                                     " is greater than max "+std::to_string(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_positive.at(playerReputation.level)));
                        continue;
                    }
                }
                else if(playerReputation.point<0)
                {
                    if(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_negative.size()==(uint32_t)-playerReputation.level)//start at level -1 in negative
                    {
                        normalOutput("The reputation level is already at min, drop point");
                        playerReputation.point=0;
                    }
                    if(playerReputation.point<CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_negative.at(-playerReputation.level))//start at level -1 in negative
                    {
                        normalOutput("The reputation point "+std::to_string(playerReputation.point)+
                                     " is greater than max "+std::to_string(CommonDatapack::commonDatapack.get_reputation().at(reputationInternalId).reputation_negative.at(playerReputation.level)));
                        continue;
                    }
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(public_and_private_informations.reputation.find(reputationInternalId)!=public_and_private_informations.reputation.cend())
                {
                    normalOutput("The reputation duplicate");
                    continue;
                }
                #endif
                public_and_private_informations.reputation[reputationInternalId]=playerReputation;
            }
        }
    }
    //achievements(17) ignored for now

    const uint64_t &commonCharacterDate=GlobalServerData::serverPrivateVariables.db_common->stringtouint64(GlobalServerData::serverPrivateVariables.db_common->value(19),&ok);
    if(!ok)
    {
        character_id=characterId;
        characterSelectionIsWrong(query_id,0x04,"creation date wrong");
        return;
    }
    Client::selectCharacterServer(query_id,characterId,commonCharacterDate);
}
