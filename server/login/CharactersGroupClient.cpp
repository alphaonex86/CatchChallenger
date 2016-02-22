#include "CharactersGroupForLogin.h"
#include "EpollServerLoginSlave.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/SqlFunction.h"
#include "../base/PreparedDBQuery.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

void CharactersGroupForLogin::character_list(EpollClientLoginSlave * const client,const uint32_t &account_id)
{
    std::string queryText=PreparedDBQueryCommon::db_query_characters;
    stringreplaceOne(queryText,"%1",std::to_string(account_id));
    stringreplaceOne(queryText,"%2",std::to_string(CommonSettingsCommon::commonSettingsCommon.max_character*2));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::character_list_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn.push_back(client);
}

void CharactersGroupForLogin::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->character_list_object();
}

void CharactersGroupForLogin::character_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.front();
    clientQueryForReadReturn.erase(clientQueryForReadReturn.begin());

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));//performance
    int tempRawDataSize=0x01;

    const auto &current_time=sFrom1970();
    bool ok;
    uint8_t validCharaterCount=0;
    while(databaseBaseCommon->next() && validCharaterCount<CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        unsigned int character_id=DatabaseFunction::stringtouint32(databaseBaseCommon->value(0),&ok);
        if(ok)
        {
            //delete
            uint32_t time_to_delete=DatabaseFunction::stringtouint32(databaseBaseCommon->value(3),&ok);
            if(!ok)
            {
                std::cerr << "time_to_delete is not number: " << databaseBaseCommon->value(3) << " for " << character_id << " fixed by 0" << std::endl;
                time_to_delete=0;
            }
            if(time_to_delete==0 || current_time<time_to_delete)
            {
                //send the char, not delete

                validCharaterCount++;

                //Character id
                {
                    *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(character_id);
                    tempRawDataSize+=sizeof(uint32_t);
                }

                //can't be empty or have wrong or too large utf8 data
                //pseudo
                {
                    const uint8_t &newSize=FacilityLibGeneral::toUTF8WithHeader(databaseBaseCommon->value(1),tempRawData+tempRawDataSize);
                    if(newSize==0)
                    {
                        std::cerr << "can't be empty or have wrong or too large utf8 data: " << databaseBaseCommon->value(1) << " by hide this char" << std::endl;
                        tempRawDataSize-=sizeof(uint32_t);
                        validCharaterCount--;
                        continue;
                    }
                    tempRawDataSize+=newSize;
                }

                //skin
                {
                    const uint32_t databaseSkinId=DatabaseFunction::stringtouint32(databaseBaseCommon->value(2),&ok);
                    if(!ok)//if not number
                    {
                        std::cerr << "character return skin is not number: " << databaseBaseCommon->value(2) << " for " << character_id << " fixed by 0" << std::endl;
                        tempRawData[tempRawDataSize]=0;
                        ok=true;
                    }
                    else
                    {
                        if(databaseSkinId>=(uint32_t)DictionaryLogin::dictionary_skin_database_to_internal.size())//out of range
                        {
                            std::cerr << "character return skin out of range: " << databaseBaseCommon->value(2) << " for " << character_id << " fixed by 0" << std::endl;
                            tempRawData[tempRawDataSize]=0;
                            ok=true;
                        }
                        //else all is good
                        else
                            tempRawData[tempRawDataSize]=DictionaryLogin::dictionary_skin_database_to_internal.at(databaseSkinId);
                    }
                    tempRawDataSize+=1;
                }

                //delete register
                {
                    unsigned int delete_time_left=0;
                    if(time_to_delete==0)
                        delete_time_left=0;
                    else
                        delete_time_left=time_to_delete-current_time;
                    *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(delete_time_left);
                    tempRawDataSize+=sizeof(uint32_t);
                }

                //played_time
                {
                    unsigned int played_time=DatabaseFunction::stringtouint32(databaseBaseCommon->value(4),&ok);
                    if(!ok)
                    {
                        std::cerr << "played_time is not number: " << databaseBaseCommon->value(4) << " for " << character_id << " fixed by 0" << std::endl;
                        played_time=0;
                    }
                    *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(played_time);
                    tempRawDataSize+=sizeof(uint32_t);
                }

                //last_connect
                {
                    unsigned int last_connect=DatabaseFunction::stringtouint32(databaseBaseCommon->value(5),&ok);
                    if(!ok)
                    {
                        std::cerr << "last_connect is not number: " << databaseBaseCommon->value(5) << " for " << character_id << " fixed by 0" << std::endl;
                        last_connect=current_time;
                    }
                    *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                    tempRawDataSize+=sizeof(uint32_t);
                }
            }
            else
                deleteCharacterNow(character_id);
        }
        else
            std::cerr << "Server id is not number: " << databaseBaseCommon->value(0) << " for " << character_id << " fixed by 0" << std::endl;
    }
    tempRawData[0]=validCharaterCount;

    client->character_list_return(this->index,tempRawData,tempRawDataSize);
    //delete tempRawData;//delete later to order the list, see EpollClientLoginSlave::server_list_return(), QMapIterator<uint8_t,CharacterListForReply> i(characterTempListForReply);, delete i.value().rawData;

    //get server list
    server_list(client,client->account_id);
}

void CharactersGroupForLogin::server_list(EpollClientLoginSlave * const client,const uint32_t &account_id)
{
    std::string queryText=PreparedDBQueryCommon::db_query_select_server_time;
    stringreplaceOne(queryText,"%1",std::to_string(account_id));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::server_list_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        client->askLogin_cancel();
        return;
    }
    else
        clientQueryForReadReturn.push_back(client);
}

void CharactersGroupForLogin::server_list_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->server_list_object();
}

void CharactersGroupForLogin::server_list_object()
{
    EpollClientLoginSlave * const client=clientQueryForReadReturn.front();
    clientQueryForReadReturn.erase(clientQueryForReadReturn.begin());

    char * const tempRawData=new char[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x00;

    const auto &current_time=sFrom1970();
    bool ok;
    uint8_t validServerCount=0;
    while(databaseBaseCommon->next() && validServerCount<CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        const unsigned int server_id=DatabaseFunction::stringtouint32(databaseBaseCommon->value(0),&ok);
        if(ok)
        {
            //global over the server group
            if(servers.find(server_id)!=servers.cend())
            {
                //server index
                tempRawData[tempRawDataSize]=servers.at(server_id).indexOnFlatList;
                tempRawDataSize+=1;

                //played_time
                unsigned int played_time=DatabaseFunction::stringtouint32(databaseBaseCommon->value(1),&ok);
                if(!ok)
                {
                    std::cerr << "played_time is not number: " << databaseBaseCommon->value(1) << " fixed by 0" << std::endl;
                    played_time=0;
                }
                *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(uint32_t);

                //last_connect
                unsigned int last_connect=DatabaseFunction::stringtouint32(databaseBaseCommon->value(2),&ok);
                if(!ok)
                {
                    std::cerr << "last_connect is not number: " << databaseBaseCommon->value(2) << " fixed by 0" << std::endl;
                    last_connect=current_time;
                }
                *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(last_connect);
                tempRawDataSize+=sizeof(uint32_t);

                validServerCount++;
            }
        }
        else
            std::cerr << "Character id is not number: " << databaseBaseCommon->value(0) << std::endl;
    }

    client->server_list_return(validServerCount,tempRawData,tempRawDataSize);
    delete tempRawData;
}

void CharactersGroupForLogin::deleteCharacterNow(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_monster_by_character_id.empty())
    {
        std::cerr << "deleteCharacterNow() Query is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_buff.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_skill.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_character is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_all_item.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_item is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_by_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_recipes is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_reputation.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_reputation is empty, bug" << std::endl;
        return;
    }
    #endif

    std::string queryText=PreparedDBQueryCommon::db_query_monster_by_character_id;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::deleteCharacterNow_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return;
    }
    else
        deleteCharacterNowCharacterIdList.push_back(characterId);
}

void CharactersGroupForLogin::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->deleteCharacterNow_object();
}

void CharactersGroupForLogin::deleteCharacterNow_object()
{
    deleteCharacterNow_return(deleteCharacterNowCharacterIdList.front());
    deleteCharacterNowCharacterIdList.erase(deleteCharacterNowCharacterIdList.begin());
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::deleteCharacterNow_return(const uint32_t &characterId)
{
    bool ok;
    std::string queryText;
    while(databaseBaseCommon->next())
    {
        const uint32_t &monsterId=DatabaseFunction::stringtouint32(databaseBaseCommon->value(0),&ok);
        if(ok)
        {
            queryText=PreparedDBQueryCommon::db_query_delete_monster_buff;
            stringreplaceOne(queryText,"%1",std::to_string(monsterId));
            dbQueryWriteCommon(queryText);
            queryText=PreparedDBQueryCommon::db_query_delete_monster_skill;
            stringreplaceOne(queryText,"%1",std::to_string(monsterId));
            dbQueryWriteCommon(queryText);
        }
    }
    queryText=PreparedDBQueryCommon::db_query_delete_character;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_all_item;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_all_item_warehouse;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_monster_by_character;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_recipes;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_reputation;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
    queryText=PreparedDBQueryCommon::db_query_delete_allow;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteCommon(queryText);
}

int8_t CharactersGroupForLogin::addCharacter(void * const client,const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_character_by_pseudo.empty())
    {
        std::cerr << "addCharacter() Query is empty, bug" << std::endl;
        return 0x03;
    }
    #endif
    if(DictionaryLogin::dictionary_skin_internal_to_database.empty())
    {
        std::cerr << "Skin list is empty, unable to add charaters" << std::endl;
        //char tempData[1+4]={0x02,0x00,0x00,0x00,0x00};/* not htole32 because inverted is the same */
        //static_cast<EpollClientLoginSlave *>(client)->postReply(query_id,tempData,sizeof(tempData));
        return 0x03;
    }
    /** \warning Need be checked in real time because can be opened on multiple login server
     * if(static_cast<EpollClientLoginSlave *>(client)->accountCharatersCount>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        qDebug() << (std::stringLiteral("You can't create more account, you have already %1 on %2 allowed").arg(static_cast<EpollClientLoginSlave *>(client)->accountCharatersCount).arg(CommonSettingsCommon::commonSettingsCommon.max_character));
        return -1;
    }*/
    if(profileIndex>=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.size())
    {
        std::cerr << "profile index: " << profileIndex << " out of range (profileList size: " << EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.size() << ")" << std::endl;
        return -1;
    }
    if(pseudo.empty())
    {
        std::cerr << "pseudo is empty, not allowed" << std::endl;
        return -1;
    }
    if((uint32_t)pseudo.size()>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
    {
        std::cerr << "pseudo size is too big: " << pseudo.size() << " because is greater than " << CommonSettingsCommon::commonSettingsCommon.max_pseudo_size << std::endl;
        return -1;
    }
    if(skinId>=DictionaryLogin::dictionary_skin_internal_to_database.size())
    {
        std::cerr << "skin provided: " << skinId << " is not into skin listed" << std::endl;
        return -1;
    }
    const EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.at(profileIndex);
    if(std::find(profile.forcedskin.begin(),profile.forcedskin.end(),skinId)==profile.forcedskin.end())
    {
        std::cerr << "skin provided: " << skinId << " is not into profile " << profileIndex << " forced skin list" << std::endl;
        return -1;
    }
    AddCharacterParam addCharacterParam;
    addCharacterParam.query_id=query_id;
    addCharacterParam.profileIndex=profileIndex;
    addCharacterParam.pseudo=pseudo;
    addCharacterParam.skinId=skinId;
    addCharacterParam.client=client;

    std::string queryText=PreparedDBQueryCommon::db_query_get_character_count_by_account;
    stringreplaceOne(queryText,"%1",std::to_string(static_cast<EpollClientLoginSlave *>(client)->account_id));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::addCharacterStep1_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return 0x03;
    }
    else
    {
        addCharacterParamList.push_back(addCharacterParam);
        return 0x00;
    }
}

void CharactersGroupForLogin::addCharacterStep1_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->addCharacterStep1_object();
}

void CharactersGroupForLogin::addCharacterStep1_object()
{
    AddCharacterParam addCharacterParam=addCharacterParamList.front();
    addCharacterParamList.erase(addCharacterParamList.begin());
    addCharacterStep1_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep1_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo,const uint8_t &skinId)
{
    if(!databaseBaseCommon->next())
    {
        std::cerr << "Character count query return nothing" << std::endl;
        client->addCharacter_ReturnFailed(query_id,0x03);
        return;
    }
    bool ok;
    uint32_t characterCount=DatabaseFunction::stringtouint32(databaseBaseCommon->value(0),&ok);
    if(!ok)
    {
        std::cerr << "Character count query return not a number" << std::endl;
        client->addCharacter_ReturnFailed(query_id,0x03);
        return;
    }
    if(characterCount>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        std::cerr << "You can't create more account, you have already " << characterCount << " on " << CommonSettingsCommon::commonSettingsCommon.max_character << " allowed" << std::endl;
        client->addCharacter_ReturnFailed(query_id,0x02);
        return;
    }

    std::string queryText=PreparedDBQueryCommon::db_query_select_character_by_pseudo;
    stringreplaceOne(queryText,"%1",SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::addCharacterStep2_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return;
    }
    else
    {
        AddCharacterParam addCharacterParam;
        addCharacterParam.query_id=query_id;
        addCharacterParam.profileIndex=profileIndex;
        addCharacterParam.pseudo=pseudo;
        addCharacterParam.skinId=skinId;
        addCharacterParam.client=client;
        addCharacterParamList.push_back(addCharacterParam);
        return;
    }
}

void CharactersGroupForLogin::addCharacterStep2_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->addCharacterStep2_object();
}

void CharactersGroupForLogin::addCharacterStep2_object()
{
    AddCharacterParam addCharacterParam=addCharacterParamList.front();
    addCharacterParamList.erase(addCharacterParamList.begin());
    addCharacterStep2_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep2_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo,const uint8_t &skinId)
{
    if(databaseBaseCommon->next())
    {
        client->addCharacter_ReturnFailed(query_id,0x01);
        return;
    }
    const EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.at(profileIndex);

    const uint32_t &characterId=maxCharacterId.back();
    maxCharacterId.pop_back();
    unsigned int index=0;
    int monster_position=1;
    int tempBufferSize=0;
    std::string numberBuffer;

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[0],profile.preparedQuerySize[0]);
    tempBufferSize+=profile.preparedQuerySize[0];

    numberBuffer=std::to_string(characterId);
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.data(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[1],profile.preparedQuerySize[1]);
    tempBufferSize+=profile.preparedQuerySize[1];

    numberBuffer=std::to_string(client->account_id);
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.data(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[2],profile.preparedQuerySize[2]);
    tempBufferSize+=profile.preparedQuerySize[2];

    numberBuffer=SqlFunction::quoteSqlVariable(pseudo);
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.data(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[3],profile.preparedQuerySize[3]);
    tempBufferSize+=profile.preparedQuerySize[3];

    numberBuffer=std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId));
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.data(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[4],profile.preparedQuerySize[4]);
    tempBufferSize+=profile.preparedQuerySize[4];

    numberBuffer=std::to_string(sFrom1970());
    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,numberBuffer.data(),numberBuffer.size());
    tempBufferSize+=numberBuffer.size();

    memcpy(CharactersGroupForLogin::tempBuffer+tempBufferSize,profile.preparedQueryChar+profile.preparedQueryPos[5],profile.preparedQuerySize[5]);
    tempBufferSize+=profile.preparedQuerySize[5];

    CharactersGroupForLogin::tempBuffer[tempBufferSize]='\0';

    dbQueryWriteCommon(CharactersGroupForLogin::tempBuffer);
    while(index<profile.monsters.size())
    {
        const EpollServerLoginSlave::LoginProfile::Monster &monster=profile.monsters.at(index);
        uint32_t gender=Gender_Unknown;
        if(monster.ratio_gender!=-1)
        {
            if(rand()%101<monster.ratio_gender)
                gender=Gender_Female;
            else
                gender=Gender_Male;
        }

        uint32_t monster_id=maxMonsterId.back();
        maxMonsterId.pop_back();

        //insert the monster is db
        {
            std::string queryText=PreparedDBQueryCommon::db_query_insert_monster;
            stringreplaceOne(queryText,"%1",std::to_string(monster_id));
            stringreplaceOne(queryText,"%2",std::to_string(monster.hp));
            stringreplaceAll(queryText,"%3",std::to_string(characterId));
            stringreplaceOne(queryText,"%4",std::to_string(monster.id));
            stringreplaceOne(queryText,"%5",std::to_string(monster.level));
            stringreplaceOne(queryText,"%6",std::to_string(monster.captured_with));
            stringreplaceOne(queryText,"%7",std::to_string(gender));
            stringreplaceOne(queryText,"%8",std::to_string(monster_position));
            dbQueryWriteCommon(queryText);
            monster_position++;
        }

        //insert the skill
        unsigned int sub_index=0;
        while(sub_index<monster.skills.size())
        {
            const EpollServerLoginSlave::LoginProfile::Monster::Skill &skill=monster.skills.at(sub_index);
            std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
            stringreplaceOne(queryText,"%1",std::to_string(monster_id));
            stringreplaceOne(queryText,"%2",std::to_string(skill.id));
            stringreplaceOne(queryText,"%3",std::to_string(skill.level));
            stringreplaceOne(queryText,"%4",std::to_string(skill.endurance));
            dbQueryWriteCommon(queryText);
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<profile.reputation.size())
    {
        const EpollServerLoginSlave::LoginProfile::Reputation &reputation=profile.reputation.at(index);
        std::string queryText=PreparedDBQueryCommon::db_query_insert_reputation;
        stringreplaceOne(queryText,"%1",std::to_string(characterId));
        stringreplaceOne(queryText,"%2",std::to_string(reputation.reputationDatabaseId));
        stringreplaceOne(queryText,"%3",std::to_string(reputation.point));
        stringreplaceOne(queryText,"%4",std::to_string(reputation.level));
        dbQueryWriteCommon(queryText);
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_item;
        stringreplaceOne(queryText,"%1",std::to_string(profile.items.at(index).id));
        stringreplaceOne(queryText,"%2",std::to_string(characterId));
        stringreplaceOne(queryText,"%3",std::to_string(profile.items.at(index).quantity));
        dbQueryWriteCommon(queryText);
        index++;
    }

    //send the network reply
    client->removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(characterId);
    posOutput+=4;

    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

bool CharactersGroupForLogin::removeCharacter(void * const client,const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id.empty())
    {
        std::cerr << "removeCharacter() Query is empty, bug" << std::endl;
        return false;
    }
    #endif
    RemoveCharacterParam removeCharacterParam;
    removeCharacterParam.query_id=query_id;
    removeCharacterParam.characterId=characterId;
    removeCharacterParam.client=client;

    std::string queryText=PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    CatchChallenger::DatabaseBase::CallBack *callback=databaseBaseCommon->asyncRead(queryText,this,&CharactersGroupForLogin::removeCharacter_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return false;
    }
    else
    {
        removeCharacterParamList.push_back(removeCharacterParam);
        return true;
    }
}

void CharactersGroupForLogin::removeCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->removeCharacter_object();
}

void CharactersGroupForLogin::removeCharacter_object()
{
    RemoveCharacterParam removeCharacterParam=removeCharacterParamList.front();
    removeCharacterParamList.erase(removeCharacterParamList.begin());
    removeCharacter_return(static_cast<EpollClientLoginSlave *>(removeCharacterParam.client),removeCharacterParam.query_id,removeCharacterParam.characterId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::removeCharacter_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint32_t &characterId)
{
    if(!databaseBaseCommon->next())
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const uint32_t &account_id=DatabaseFunction::stringtouint32(databaseBaseCommon->value(0),&ok);
    if(!ok)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Account for character: "+databaseBaseCommon->value(0)+" is not an id");
        return;
    }
    if(client->account_id!=account_id)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Character: "+std::to_string(characterId)+" is not owned by the account: "+std::to_string(account_id));
        return;
    }
    const uint32_t &time_to_delete=DatabaseFunction::stringtouint32(databaseBaseCommon->value(1),&ok);
    if(ok && time_to_delete>0)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Character: "+std::to_string(characterId)+" is already in deleting for the account: "+std::to_string(account_id));
        return;
    }
    /// \todo don't and failed if timedrift
    std::string queryText=PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    stringreplaceOne(queryText,"%2",
                  //date to delete, not time (no sens on database, delete the date of removing
                  std::to_string(
                        sFrom1970()+
                        CommonSettingsCommon::commonSettingsCommon.character_delete_time
                    )
                  );
    dbQueryWriteCommon(queryText);

    //send the network reply
    client->removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;

    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void CharactersGroupForLogin::dbQueryWriteCommon(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        std::cerr << "dbQuery() Query is empty, bug" << std::endl;
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cerr << "Do common db write: " << queryText << std::endl;
    #endif
    databaseBaseCommon->asyncWrite(queryText);
}
