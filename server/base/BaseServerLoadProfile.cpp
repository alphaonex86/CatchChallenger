#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "PreparedDBQuery.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/fight/CommonFightEngine.hpp"

using namespace CatchChallenger;

/**
 * into the BaseServerLogin
 * */
void BaseServer::preload_profile()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    std::cout << DictionaryLogin::dictionary_skin_database_to_internal.size() << " SQL skin dictionary" << std::endl;
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.get_profileList().size()!=CommonDatapackServerSpec::commonDatapackServerSpec.get_serverProfileList().size())
    {
        std::cout << "preload_profile() profile common and server don't match, maybe the main code ou sub code don't have start.xml valid" << std::endl;
        abort();
        return;
    }
    #endif
    {
        unsigned int emptyMap=0;
        unsigned int index=0;
        std::vector<ServerSpecProfile> &l=CommonDatapackServerSpec::commonDatapackServerSpec.get_serverProfileList_rw();
        while(index<l.size())
        {
            ServerSpecProfile &serverSpecProfile=l[index];
            stringreplaceOne(serverSpecProfile.mapString,".tmx","");
            if(serverSpecProfile.mapString.empty())
                emptyMap++;
            index++;
        }
        if(emptyMap>=l.size())
        {
            std::cerr << "serverProfileList have no map, maybe the main code ou sub code don't have start.xml valid" << std::endl;
            abort();
        }
    }
    GlobalServerData::serverPrivateVariables.serverProfileInternalList.clear();

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    const DatabaseBase::DatabaseType &databaseType=GlobalServerData::serverPrivateVariables.db_common->databaseType();
    CatchChallenger::DatabaseBase * const database=GlobalServerData::serverPrivateVariables.db_common;
    const uint8_t &common_blobversion_datapack=GlobalServerData::serverPrivateVariables.common_blobversion_datapack;
    #endif

    //if reallocate query, los pointer, it's why reserved/allocted before all
    GlobalServerData::serverPrivateVariables.serverProfileInternalList.resize(CommonDatapack::commonDatapack.get_profileList().size());
    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.get_profileList().size())
    {
        const ServerSpecProfile &serverProfile=CommonDatapackServerSpec::commonDatapackServerSpec.get_serverProfileList().at(index);
        ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[index];
        serverProfileInternal.map=nullptr;
        serverProfileInternal.orientation=Orientation_none;
        serverProfileInternal.valid=false;
        serverProfileInternal.x=0;
        serverProfileInternal.y=0;
        if(GlobalServerData::serverPrivateVariables.map_list.find(serverProfile.mapString)==GlobalServerData::serverPrivateVariables.map_list.cend())
        {
            std::cerr << "Into the starter the map \"" << serverProfile.mapString << "\" is not found, fix it (abort)" << std::endl;
            abort();
        }
        serverProfileInternal.map=
                static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.at(serverProfile.mapString));
        if(serverProfileInternal.map==NULL)
        {
            std::cerr << "Into the starter the map \"" << serverProfile.mapString << "\" is not resolved, fix it (abort)" << std::endl;
            abort();
        }
        serverProfileInternal.x=serverProfile.x;
        serverProfileInternal.y=serverProfile.y;
        serverProfileInternal.orientation=serverProfile.orientation;

        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(index);

        std::string encyclopedia_item,item;
        if(!profile.items.empty())
        {
            uint32_t lastItemId=0;
            auto item_list=profile.items;
            std::sort(item_list.begin(),item_list.end(),[](const Profile::Item &a, const Profile::Item &b) {
                return a.id<b.id;
            });
            auto max=item_list.at(item_list.size()-1).id;
            uint32_t pos=0;
            char item_raw[static_cast<uint8_t>(2+4)*item_list.size()];
            unsigned int index=0;
            while(index<item_list.size())
            {
                const auto &item=item_list.at(index);
                *reinterpret_cast<uint16_t *>(item_raw+pos)=htole16(item.id-lastItemId);
                pos+=2;
                lastItemId=item.id;
                *reinterpret_cast<uint32_t *>(item_raw+pos)=htole32(item.quantity);
                pos+=4;
                index++;
            }
            item=binarytoHexa(item_raw,static_cast<uint32_t>(sizeof(item_raw)));

            const size_t size=max/8+1;
            char bitlist[size];
            memset(bitlist,0,size);
            index=0;
            while(index<item_list.size())
            {
                const auto &item=profile.items.at(index);
                uint16_t bittoUp=item.id;
                bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                index++;
            }
            encyclopedia_item=binarytoHexa(bitlist,static_cast<uint32_t>(sizeof(bitlist)));
        }

        std::string reputations;
        if(!profile.reputations.empty())
        {
            struct ReputationTemp
            {
                uint8_t reputationDatabaseId;//datapack order, can can need the dicionary to db resolv
                int8_t level;
                int32_t point;
            };
            std::vector<ReputationTemp> reputationList;

            {
                unsigned int index=0;
                while(index<profile.reputations.size())
                {
                    ReputationTemp temp;
                    const Profile::Reputation &source=profile.reputations.at(index);
                    temp.level=source.level;
                    temp.point=source.point;
                    if(source.internalIndex>=CommonDatapack::commonDatapack.get_reputation().size())
                    {
                        std::cerr << "profile index out of range for profile preparation. internal error" << std::endl;
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        abort();
                        #else
                        break;
                        #endif
                    }
                    temp.reputationDatabaseId=static_cast<uint8_t>(CommonDatapack::commonDatapack.get_reputation().at(source.internalIndex).reverse_database_id);
                    reputationList.push_back(temp);
                    index++;
                }
            }
            uint32_t lastReputationId=0;
            auto reputations_list=reputationList;
            std::sort(reputations_list.begin(),reputations_list.end(),[](const ReputationTemp &a, const ReputationTemp &b) {
                return a.reputationDatabaseId<b.reputationDatabaseId;
            });
            uint32_t pos=0;
            char reputation_raw[static_cast<uint8_t>(1+4+1)*reputations_list.size()];
            unsigned int index=0;
            while(index<reputations_list.size())
            {
                const auto &reputation=reputations_list.at(index);
                *reinterpret_cast<uint32_t *>(reputation_raw+pos)=htole32(reputation.point);
                pos+=4;
                reputation_raw[pos]=static_cast<uint8_t>(reputation.reputationDatabaseId-lastReputationId);
                pos+=1;
                lastReputationId=reputation.reputationDatabaseId;
                reputation_raw[pos]=reputation.level;
                pos+=1;
                index++;
            }
            reputations=binarytoHexa(reputation_raw,static_cast<uint32_t>(sizeof(reputation_raw)));
        }

        ServerProfileInternal::PreparedStatementForCreation &preparedStatementForCreation=serverProfileInternal.preparedStatementForCreationByCommon;
        ServerProfileInternal::PreparedStatementForCreationType &preparedStatementForCreationType=preparedStatementForCreation.type;
        //assume here all is the same type
        {
            if(preparedStatementForCreationType.monsterGroup.size()!=profile.monstergroup.size())
                preparedStatementForCreationType.monsterGroup.resize(profile.monstergroup.size());

            unsigned int monsterGroupIndex=0;
            while(monsterGroupIndex<profile.monstergroup.size())
            {
                const auto &monsters=profile.monstergroup.at(monsterGroupIndex);
                ServerProfileInternal::PreparedStatementForCreationMonsterGroup &preparedStatementForCreationMonsterGroup=preparedStatementForCreationType.monsterGroup[monsterGroupIndex];
                if(preparedStatementForCreationMonsterGroup.monster_insert.size()!=monsters.size())
                    preparedStatementForCreationMonsterGroup.monster_insert.resize(monsters.size());
                std::vector<uint16_t> monsterForEncyclopedia;
                unsigned int monsterIndex=0;
                while(monsterIndex<monsters.size())
                {
                    const auto &monster=monsters.at(monsterIndex);
                    const auto &monsterDatapack=CommonDatapack::commonDatapack.get_monsters().at(monster.id);
                    const Monster::Stat &monsterStat=CommonFightEngineBase::getStat(monsterDatapack,monster.level);
                    std::vector<CatchChallenger::PlayerMonster::PlayerSkill> skills_list=CommonFightEngineBase::generateWildSkill(monsterDatapack,monster.level);
                    if(skills_list.empty())
                    {
                        std::cerr << "skills_list.empty() for some profile" << std::endl;
                        abort();
                    }
                    uint32_t lastSkillId=0;
                    std::sort(skills_list.begin(),skills_list.end(),[](const CatchChallenger::PlayerMonster::PlayerSkill &a, const CatchChallenger::PlayerMonster::PlayerSkill &b) {
                        return a.skill<b.skill;
                    });
                    char raw_skill[(2+1)*skills_list.size()],raw_skill_endurance[1*skills_list.size()];
                    //the skills
                    unsigned int sub_index=0;
                    while(sub_index<skills_list.size())
                    {
                        const auto &skill=skills_list.at(sub_index);
                        *reinterpret_cast<uint16_t *>(raw_skill+sub_index*(2+1))=htole16(skill.skill-lastSkillId);
                        lastSkillId=skill.skill;
                        raw_skill[sub_index*(2+1)+2]=skill.level;
                        raw_skill_endurance[sub_index]=skill.endurance;
                        sub_index++;
                    }
                    //dynamic part
                    {
                        //id,character,place,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position,skills,skills_endurance
                        if(monsterDatapack.ratio_gender!=-1)
                        {
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose({
                                    "%1",
                                    "%2",
                                    "1",
                                    std::to_string(monsterStat.hp),
                                    std::to_string(monster.id),
                                    std::to_string(monster.level),
                                    std::to_string(monster.captured_with),
                                    "%3",
                                    "%4",
                                    std::to_string(monsterIndex+1),
                                    binarytoHexa(raw_skill,static_cast<uint32_t>(sizeof(raw_skill))),
                                    binarytoHexa(raw_skill_endurance,static_cast<uint32_t>(sizeof(raw_skill_endurance)))
                                    });
                            preparedStatementForCreationMonsterGroup.monster_insert[monsterIndex]=PreparedStatementUnit(queryText,database);
                        }
                        else
                        {
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose({
                                    "%1",
                                    "%2",
                                    "1",
                                    std::to_string(monsterStat.hp),
                                    std::to_string(monster.id),
                                    std::to_string(monster.level),
                                    std::to_string(monster.captured_with),
                                    "3",
                                    "%3",
                                    std::to_string(monsterIndex+1),
                                    binarytoHexa(raw_skill,static_cast<uint32_t>(sizeof(raw_skill))),
                                    binarytoHexa(raw_skill_endurance,static_cast<uint32_t>(sizeof(raw_skill_endurance)))
                                    });
                            preparedStatementForCreationMonsterGroup.monster_insert[monsterIndex]=PreparedStatementUnit(queryText,database);
                        }
                    }
                    monsterForEncyclopedia.push_back(monster.id);
                    monsterIndex++;
                }
                //do the encyclopedia monster
                const auto &result=std::max_element(monsterForEncyclopedia.begin(),monsterForEncyclopedia.end());
                const size_t size=*result/8+1;
                char bitlist[size];
                memset(bitlist,0,size);
                monsterIndex=0;
                while(monsterIndex<monsterForEncyclopedia.size())
                {
                    uint16_t bittoUp=monsterForEncyclopedia.at(monsterIndex);
                    bitlist[bittoUp/8]|=(1<<(7-bittoUp%8));
                    monsterIndex++;
                }
                switch(databaseType)
                {
                    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
                    case DatabaseBase::DatabaseType::Mysql:
                        preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO `character`("
                                "`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,"
                                "`time_to_delete`,`played_time`,`last_connect`,`starter`,`item`,`reputations`,`encyclopedia_monster`,`encyclopedia_item`"
                                ",`blob_version`) VALUES(%1,%2,"+
                                                                                        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                        std::string("%3")+
                                                                                        #else
                                                                                        std::string("'%3'")+
                                                                                        #endif
                                                                                        ",%4,0,0,"+
                                std::to_string(profile.cash)+",%5,0,0,"
                                "0,0,0,"+
                                std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+
                                ",UNHEX('"+item+"'),UNHEX('"+reputations+"'),UNHEX('"+binarytoHexa(bitlist,static_cast<uint32_t>(sizeof(bitlist)))+
                                "'),UNHEX('"+encyclopedia_item+"'),"+std::to_string(common_blobversion_datapack)+");"),database);
                    break;
                    #endif
                    #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
                    case DatabaseBase::DatabaseType::SQLite:
                        preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(std::string("INSERT INTO character("
                                "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                ",blob_version) VALUES(%1,%2,"+
                                                                                        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                        std::string("%3")+
                                                                                        #else
                                                                                        std::string("'%3'")+
                                                                                        #endif
                                                                                        ",%4,0,0,"+
                                std::to_string(profile.cash)+",%5,0,0,"
                                "0,0,0,"+
                                std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",'"+item+
                                "','"+reputations+"','"+binarytoHexa(bitlist,static_cast<uint32_t>(sizeof(bitlist)))+"','"+encyclopedia_item+"',"+
                                std::to_string(common_blobversion_datapack)+");"),database);
                    break;
                    #endif
                    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
                    case DatabaseBase::DatabaseType::PostgreSQL:
                        preparedStatementForCreationMonsterGroup.character_insert=PreparedStatementUnit(
                                    std::string("INSERT INTO character("
                                "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                                "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                                ",blob_version) VALUES(%1,%2,"+
                                                                                        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                                                                                        std::string("%3")+
                                                                                        #else
                                                                                        std::string("'%3'")+
                                                                                        #endif
                                                                                        ",%4,0,0,"+
                                std::to_string(profile.cash)+",%5,0,FALSE,"
                                "0,0,0,"+
                                std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+
                                ",'\\x"+item+"','\\x"+reputations+"','\\x"+binarytoHexa(bitlist,static_cast<uint32_t>(sizeof(bitlist)))+
                                "','\\x"+encyclopedia_item+"',"+std::to_string(common_blobversion_datapack)+");")
                                                                                                        ,database);
                    break;
                    #endif
                    default:
                    abort();
                    break;
                }

                monsterGroupIndex++;
            }
        }
        #endif
        const std::string &mapQuery=std::to_string(serverProfileInternal.map->reverse_db_id)+
                ","+
                std::to_string(serverProfileInternal.x)+
                ","+
                std::to_string(serverProfileInternal.y)+
                ","+
                std::to_string(Orientation_bottom);
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,plants`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));"),GlobalServerData::serverPrivateVariables.db_server);
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');"),GlobalServerData::serverPrivateVariables.db_server);
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','','\\x',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');"),GlobalServerData::serverPrivateVariables.db_server);
            break;
        }
        #else
        switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
        {
            default:
            abort();
            break;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));"),GlobalServerData::serverPrivateVariables.db_server);
            break;
            #endif
            #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');"),GlobalServerData::serverPrivateVariables.db_server);
            break;
            #endif
            #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=PreparedStatementUnit(std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');"),GlobalServerData::serverPrivateVariables.db_server);
            break;
            #endif
        }
        #endif
        serverProfileInternal.valid=true;

        index++;
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(index!=CommonDatapack::commonDatapack.get_profileList().size())
        {
            std::cerr << "index!=CommonDatapack::commonDatapack.get_profileList().size() corrupted (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.size()!=CommonDatapack::commonDatapack.get_profileList().size())
        {
            std::cerr << "GlobalServerData::serverPrivateVariables.serverProfileInternalList.size()!=CommonDatapack::commonDatapack.get_profileList().size() corrupted (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        unsigned int profileIndex=0;
        while(profileIndex<GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
        {
            unsigned int monsterGroupId=0;
            while(monsterGroupId<GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.size())
            {
                if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
                {
                    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.at(monsterGroupId).character_insert.empty())
                    {
                        std::cerr << "GlobalServerData::serverPrivateVariables.serverProfileInternalList corrupted (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                        return;
                    }
                    unsigned int indexMonsterQueryIndex=0;
                    while(indexMonsterQueryIndex<GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.at(monsterGroupId).monster_insert.size())
                    {
                        if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.at(monsterGroupId).monster_insert.at(indexMonsterQueryIndex).empty())
                        {
                            std::cerr << "GlobalServerData::serverPrivateVariables.serverProfileInternalList corrupted (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                            abort();
                            return;
                        }
                        indexMonsterQueryIndex++;
                    }
                }
                monsterGroupId++;
            }
            profileIndex++;
        }
        #endif
    }
    #endif

    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.empty())
    {
        std::cerr << "No profile loaded, seam unable to work without it" << std::endl;
        abort();
    }
    std::cout << GlobalServerData::serverPrivateVariables.serverProfileInternalList.size() << " profile loaded" << std::endl;
}
