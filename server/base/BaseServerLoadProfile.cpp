#include "BaseServer.h"
#include "GlobalServerData.h"
#include "DictionaryLogin.h"
#include "PreparedDBQuery.h"

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
    /*if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileList.size())
    {
        std::cout << "profile common and server don't match" << std::endl;
        return;
    }*/
    if(CommonDatapack::commonDatapack.profileList.size()!=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
    {
        std::cout << "profile common and server don't match" << std::endl;
        return;
    }
    #endif
    {
        unsigned int index=0;
        while(index<CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
        {
            stringreplaceOne(CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList[index].mapString,CACHEDSTRING_dottmx,"");
            index++;
        }
    }
    GlobalServerData::serverPrivateVariables.serverProfileInternalList.clear();

    GlobalServerData::serverPrivateVariables.serverProfileInternalList.resize(CommonDatapack::commonDatapack.profileList.size());
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    const DatabaseBase::DatabaseType &type=GlobalServerData::serverPrivateVariables.db_common->databaseType();
    #endif

    unsigned int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const ServerSpecProfile &serverProfile=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.at(index);
        ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(index);
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
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);

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
            char item_raw[(2+4)*item_list.size()];
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
            item=binarytoHexa(item_raw,sizeof(item_raw));

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
            encyclopedia_item=binarytoHexa(bitlist,sizeof(bitlist));
        }
        std::string reputations;
        if(!profile.reputations.empty())
        {
            uint32_t lastReputationId=0;
            auto reputations_list=profile.reputations;
            std::sort(reputations_list.begin(),reputations_list.end(),[](const Profile::Reputation &a, const Profile::Reputation &b) {
                return a.reputationDatabaseId<b.reputationDatabaseId;
            });
            uint32_t pos=0;
            char reputation_raw[(1+4+1)*reputations_list.size()];
            unsigned int index=0;
            while(index<reputations_list.size())
            {
                const auto &reputation=reputations_list.at(index);
                *reinterpret_cast<uint32_t *>(reputation_raw+pos)=htole32(reputation.point);
                pos+=4;
                reputation_raw[pos]=reputation.reputationDatabaseId-lastReputationId;
                pos+=1;
                lastReputationId=reputation.reputationDatabaseId;
                reputation_raw[pos]=reputation.level;
                pos+=1;
                index++;
            }
            reputations=binarytoHexa(reputation_raw,sizeof(reputation_raw));
        }

        //assume here all is the same type
        {
            unsigned int monsterGroupIndex=0;
            serverProfileInternal.monster_insert.resize(profile.monstergroup.size());
            while(monsterGroupIndex<profile.monstergroup.size())
            {
                const auto &monsters=profile.monstergroup.at(monsterGroupIndex);
                std::vector<uint16_t> monsterForEncyclopedia;
                std::vector<StringWithReplacement> &monsterGroupQuery=serverProfileInternal.monster_insert[monsterGroupIndex];
                unsigned int monsterIndex=0;
                while(monsterIndex<monsters.size())
                {
                    const auto &monster=monsters.at(monsterIndex);
                    const auto &monsterDatapack=CommonDatapack::commonDatapack.monsters.at(monster.id);
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
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose(
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
                                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                    );
                            monsterGroupQuery.push_back(queryText);
                        }
                        else
                        {
                            const std::string &queryText=PreparedDBQueryCommon::db_query_insert_monster.compose(
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
                                    binarytoHexa(raw_skill,sizeof(raw_skill)),
                                    binarytoHexa(raw_skill_endurance,sizeof(raw_skill_endurance))
                                    );
                            monsterGroupQuery.push_back(queryText);
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
                serverProfileInternal.monster_encyclopedia_insert.push_back(binarytoHexa(bitlist,sizeof(bitlist)));

                monsterGroupIndex++;
            }
        }
        switch(type)
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.character_insert=std::string("INSERT INTO `character`("
                        "`id`,`account`,`pseudo`,`skin`,`type`,`clan`,`cash`,`date`,`warehouse_cash`,`clan_leader`,"
                        "`time_to_delete`,`played_time`,`last_connect`,`starter`,`item`,`reputations`,`encyclopedia_monster`,`encyclopedia_item`"
                        ",`blob_version`) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,0,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",UNHEX('"+item+"'),UNHEX('"+reputations+"'),UNHEX('%6'),UNHEX('"+encyclopedia_item+"'),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.character_insert=std::string("INSERT INTO character("
                        "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                        "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                        ",blob_version) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,0,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",'"+item+"','"+reputations+"','%6','"+encyclopedia_item+"',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.character_insert=std::string("INSERT INTO character("
                        "id,account,pseudo,skin,type,clan,cash,date,warehouse_cash,clan_leader,"
                        "time_to_delete,played_time,last_connect,starter,item,reputations,encyclopedia_monster,encyclopedia_item"
                        ",blob_version) VALUES(%1,%2,'%3',%4,0,0,"+
                        std::to_string(profile.cash)+",%5,0,FALSE,"
                        "0,0,0,"+
                        std::to_string(DictionaryLogin::dictionary_starter_internal_to_database.at(index)/*starter*/)+",'\\x"+item+"','\\x"+reputations+"','\\x%6','\\x"+encyclopedia_item+"',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+");");
            break;
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
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,plants`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,plants,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'\\x','\\x','\\x',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');");
            break;
        }
        #else
        switch(GlobalServerData::serverPrivateVariables.db_server->databaseType())
        {
            default:
            case DatabaseBase::DatabaseType::Mysql:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO `character_forserver`(`character`,`map`,`x`,`y`,`orientation`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`date`,`market_cash`,`botfight_id`,`itemonmap`,`blob_version`,`quest`) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,UNHEX(''),UNHEX(''),"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",UNHEX(''));");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'','',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'');");
            break;
            case DatabaseBase::DatabaseType::PostgreSQL:
                serverProfileInternal.preparedQueryAddCharacterForServer=std::string("INSERT INTO character_forserver(character,map,x,y,orientation,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,date,market_cash,botfight_id,itemonmap,blob_version,quest) VALUES("
                "%1,"+mapQuery+","+mapQuery+","+mapQuery+",%2,0,'\\x','\\x',"+std::to_string(GlobalServerData::serverPrivateVariables.server_blobversion_datapack)+",'\\x');");
            break;
        }
        #endif
        serverProfileInternal.valid=true;

        index++;
    }

    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.empty())
    {
        std::cerr << "No profile loaded, seam unable to work without it" << std::endl;
        abort();
    }
    std::cout << GlobalServerData::serverPrivateVariables.serverProfileInternalList.size() << " profile loaded" << std::endl;
}