#include "CharactersGroupForLogin.h"
#include "EpollServerLoginSlave.h"
#include "VariableLoginServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/SqlFunction.h"
#include "../base/PreparedDBQuery.h"
#include "../base/DictionaryLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

const std::string CharactersGroupForLogin::gender_male("1");
const std::string CharactersGroupForLogin::gender_female("2");

void CharactersGroupForLogin::character_list(EpollClientLoginSlave * const client,const uint32_t &account_id)
{
    CatchChallenger::DatabaseBase::CallBack *callback=preparedDBQueryCommonForLogin.db_query_characters.asyncRead(this,&CharactersGroupForLogin::character_list_static,{std::to_string(account_id)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << preparedDBQueryCommonForLogin.db_query_characters.queryText() << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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
        unsigned int character_id=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
        if(ok)
        {
            //delete
            uint32_t time_to_delete=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(3),&ok);
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
                    const uint32_t databaseSkinId=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(2),&ok);
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
                    unsigned int played_time=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(4),&ok);
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
                    unsigned int last_connect=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(5),&ok);
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

    client->character_list_return(this->charactersGroupIndex,tempRawData,tempRawDataSize);
    //delete tempRawData;//delete later to order the list, see EpollClientLoginSlave::server_list_return(), QMapIterator<uint8_t,CharacterListForReply> i(characterTempListForReply);, delete i.value().rawData;

    //get server list
    server_list(client,client->account_id);
}

void CharactersGroupForLogin::server_list(EpollClientLoginSlave * const client,const uint32_t &account_id)
{
    CatchChallenger::DatabaseBase::CallBack *callback=preparedDBQueryCommonForLogin.db_query_select_server_time.asyncRead(this,&CharactersGroupForLogin::server_list_static,{std::to_string(account_id)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << preparedDBQueryCommonForLogin.db_query_select_server_time.queryText() << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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

    char tempRawData[4*1024];
    //memset(tempRawData,0x00,sizeof(4*1024));
    int tempRawDataSize=0x00;

    const auto &current_time=sFrom1970();
    bool ok;
    uint8_t validServerCount=0;
    while(databaseBaseCommon->next() && validServerCount<CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        const unsigned int server_id=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
        if(ok)
        {
            //global over the server group
            if(servers.find(server_id)!=servers.cend())
            {
                //server index
                tempRawData[tempRawDataSize]=servers.at(server_id).indexOnFlatList;
                tempRawDataSize+=1;

                //played_time
                unsigned int played_time=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(1),&ok);
                if(!ok)
                {
                    std::cerr << "played_time is not number: " << databaseBaseCommon->value(1) << " fixed by 0" << std::endl;
                    played_time=0;
                }
                *reinterpret_cast<uint32_t *>(tempRawData+tempRawDataSize)=htole32(played_time);
                tempRawDataSize+=sizeof(uint32_t);

                //last_connect
                unsigned int last_connect=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(2),&ok);
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
}

void CharactersGroupForLogin::deleteCharacterNow(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedDBQueryCommonForLogin.db_query_delete_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_character is empty, bug" << std::endl;
        return;
    }
    if(preparedDBQueryCommonForLogin.db_query_delete_monster_by_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_by_id is empty, bug" << std::endl;
        return;
    }
    #endif

    const std::string &characterIdString=std::to_string(characterId);
    preparedDBQueryCommonForLogin.db_query_delete_character.asyncWrite({characterIdString});
    preparedDBQueryCommonForLogin.db_query_delete_monster_by_character.asyncWrite({characterIdString});
}

/*
void CharactersGroupForLogin::deleteCharacterNow(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedDBQueryCommonForLogin.db_query_delete_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_character is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_by_id.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_by_id is empty, bug" << std::endl;
        return;
    }
    #endif

    const std::string &queryText=preparedDBQueryCommonForLogin.db_query_characters_with_monsters.compose(
                std::to_string(characterId)
                );
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
    if(databaseBaseCommon->next())
    {
        const std::string &monster=databaseBaseCommon->value(0);
        if(monster.size()%(sizeof(uint32_t)*2)==0)
        {
            const std::vector<char> &data=hexatoBinary(monster);
            const uint16_t &size=data.size()/(sizeof(uint32_t));
            uint16_t index=0;
            while(index<size)
            {
                const uint32_t &monsterId=*reinterpret_cast<const uint32_t *>(data.data()+index*sizeof(uint32_t));
                const std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_by_id.compose(
                            std::to_string(monsterId)
                            );
                dbQueryWriteCommon(queryText);
                index++;
            }
        }
        else
            std::cerr << "haractersGroupForLogin::deleteCharacterNow_return() have incorrect monster blob lenght: " << monster.size() << ", characterId: " << characterId << std::endl;
    }
    {
        const std::string &monster_warehouse=databaseBaseCommon->value(1);
        if(monster_warehouse.size()%(sizeof(uint32_t)*2)==0)
        {
            const std::vector<char> &data=hexatoBinary(monster_warehouse);
            const uint16_t &size=data.size()/(sizeof(uint32_t));
            uint16_t index=0;
            while(index<size)
            {
                const uint32_t &monsterId=*reinterpret_cast<const uint32_t *>(data.data()+index*sizeof(uint32_t));
                const std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_by_id.compose(
                            std::to_string(monsterId)
                            );
                dbQueryWriteCommon(queryText);
                index++;
            }
        }
        else
            std::cerr << "haractersGroupForLogin::deleteCharacterNow_return() have incorrect monster_warehouse blob lenght: " << monster_warehouse.size() << ", characterId: " << characterId << std::endl;
    }
    {
        const std::string &monster_market=databaseBaseCommon->value(2);
        if(monster_market.size()%(sizeof(uint32_t)*2)==0)
        {
            const std::vector<char> &data=hexatoBinary(monster_market);
            const uint16_t &size=data.size()/(sizeof(uint32_t));
            uint16_t index=0;
            while(index<size)
            {
                const uint32_t &monsterId=*reinterpret_cast<const uint32_t *>(data.data()+index*sizeof(uint32_t));
                const std::string &queryText=PreparedDBQueryCommon::db_query_delete_monster_by_id.compose(
                            std::to_string(monsterId)
                            );
                dbQueryWriteCommon(queryText);
                index++;
            }
        }
        else
            std::cerr << "haractersGroupForLogin::deleteCharacterNow_return() have incorrect monster_market blob lenght: " << monster_market.size() << ", characterId: " << characterId << std::endl;
    }

    const std::string &queryText=preparedDBQueryCommonForLogin.db_query_delete_character.compose(
                std::to_string(characterId)
                );
    dbQueryWriteCommon(queryText);
}*/

int8_t CharactersGroupForLogin::addCharacter(void * const client,const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.empty())
    {
        std::cerr << "addCharacter() Query is empty, bug" << std::endl;
        return 0x03;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster.empty())
    {
        std::cerr << "addCharacter() Query db_query_insert_monster is empty, bug" << std::endl;
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
    if(monsterGroupId>=profile.monstergroup.size())
    {
        std::cerr << "monsterGroupId: " << monsterGroupId << " is not into profile " << profileIndex << std::endl;
        return -1;
    }
    AddCharacterParam addCharacterParam;
    addCharacterParam.query_id=query_id;
    addCharacterParam.profileIndex=profileIndex;
    addCharacterParam.pseudo=pseudo;
    addCharacterParam.monsterGroupId=monsterGroupId;
    addCharacterParam.skinId=skinId;
    addCharacterParam.client=client;

    CatchChallenger::DatabaseBase::CallBack *callback=preparedDBQueryCommonForLogin.db_query_get_character_count_by_account.asyncRead(this,&CharactersGroupForLogin::addCharacterStep1_static,{std::to_string(static_cast<EpollClientLoginSlave *>(client)->account_id)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << preparedDBQueryCommonForLogin.db_query_get_character_count_by_account.queryText() << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
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
    addCharacterStep1_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.monsterGroupId,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep1_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId)
{
    if(!databaseBaseCommon->next())
    {
        std::cerr << "Character count query return nothing" << std::endl;
        client->addCharacter_ReturnFailed(query_id,0x03);
        return;
    }
    bool ok;
    uint32_t characterCount=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
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

    CatchChallenger::DatabaseBase::CallBack *callback=preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.asyncRead(this,&CharactersGroupForLogin::addCharacterStep2_static,{
                                                                                                                                      #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
                                                                                                                                      pseudo
                                                                                                                                      #else
                                                                                                                                      SqlFunction::quoteSqlVariable(pseudo)
                                                                                                                                      #endif
                                                                                                                                      });
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.queryText() << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return;
    }
    else
    {
        AddCharacterParam addCharacterParam;
        addCharacterParam.query_id=query_id;
        addCharacterParam.profileIndex=profileIndex;
        addCharacterParam.pseudo=pseudo;
        addCharacterParam.monsterGroupId=monsterGroupId;
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
    addCharacterStep2_return(static_cast<EpollClientLoginSlave * const>(addCharacterParam.client),addCharacterParam.query_id,addCharacterParam.profileIndex,addCharacterParam.pseudo,addCharacterParam.monsterGroupId,addCharacterParam.skinId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::addCharacterStep2_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId)
{
    if(databaseBaseCommon->next())
    {
        client->addCharacter_ReturnFailed(query_id,0x01);
        return;
    }
    EpollServerLoginSlave::LoginProfile &profile=EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.at(profileIndex);

    if(maxCharacterId.empty())
    {
        client->addCharacter_ReturnFailed(query_id,0x03);
        std::cerr << "maxCharacterIdRequested is empty" << std::endl;
        return;
    }
    if(maxCharacterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK)
    {
        if(!maxCharacterIdRequested)
        {
            #ifdef DEBUG_MESSAGE_QUERY_IDLIST
            std::cout << "Ask more to master: maxCharacterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << std::to_string(maxCharacterId.size()) << "<" << std::to_string(CATCHCHALLENGER_SERVER_MINIDBLOCK) << " for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << std::endl;
            #endif
            if(LinkToMaster::linkToMaster->queryNumberList.empty())
            {
                std::cerr << "Unable to get query id at createAccount_return, maxCharacterIdRequested is empty for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
                client->addCharacter_ReturnFailed(query_id,0x03);
                return;
            }
            const uint8_t &queryNumber=LinkToMaster::linkToMaster->queryNumberList.back();
            if(LinkToMaster::queryNumberToCharacterGroup[queryNumber]!=0)
            {
                std::cerr << "LinkToMaster::queryNumberToCharacterGroup[queryNumber]!=0 for maxCharacterId: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
                client->addCharacter_ReturnFailed(query_id,0x03);
                return;
            }
            maxCharacterIdRequested=true;
            LinkToMaster::linkToMaster->queryNumberList.pop_back();

            //send the network reply
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xC0;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=this->charactersGroupIndex;
            posOutput+=1;

            LinkToMaster::linkToMaster->registerOutputQuery(queryNumber,0xC0);
            LinkToMaster::linkToMaster->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            LinkToMaster::queryNumberToCharacterGroup[queryNumber]=this->charactersGroupIndex;
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cout << "Don't ask more to master (already requested): maxCharacterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << std::to_string(maxCharacterId.size()) << "<" << std::to_string(CATCHCHALLENGER_SERVER_MINIDBLOCK) << " for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << std::endl;
            #endif
        }
    }

    const uint32_t &characterId=maxCharacterId.back();
    const std::string &characterIdString=std::to_string(characterId);
    maxCharacterId.pop_back();
    int monster_position=1;

    const std::vector<EpollServerLoginSlave::LoginProfile::Monster> &monsterGroup=profile.monstergroup.at(monsterGroupId);
    if(profile.preparedStatementForCreationByCommon.find(databaseBaseCommon)==profile.preparedStatementForCreationByCommon.cend())
    {
        std::cerr << "At addCharacterStep2_return, prepared query not found" << std::endl;
        abort();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(profile.preparedStatementForCreationByCommon.find(databaseBaseCommon)==profile.preparedStatementForCreationByCommon.cend())
    {
        std::cerr << "At addCharacterStep2_return, database not found" << std::endl;
        abort();
    }
    #endif
    EpollServerLoginSlave::LoginProfile::PreparedStatementForCreation &preparedStatementForCreation=profile.preparedStatementForCreationByCommon.at(databaseBaseCommon);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedStatementForCreation.type.size()>=profileIndex)
    {
        std::cerr << "At addCharacterStep2_return, profileIndex not found" << std::endl;
        abort();
    }
    #endif
    EpollServerLoginSlave::LoginProfile::PreparedStatementForCreationType &preparedStatementForCreationType=preparedStatementForCreation.type.at(profileIndex);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedStatementForCreationType.monsterGroup.size()>=monsterGroupId)
    {
        std::cerr << "At addCharacterStep2_return, monsterGroupId not found" << std::endl;
        abort();
    }
    #endif
    EpollServerLoginSlave::LoginProfile::PreparedStatementForCreationMonsterGroup &preparedStatementForCreationMonsterGroup=preparedStatementForCreationType.monsterGroup.at(monsterGroupId);
    PreparedStatementUnit &character_insert=preparedStatementForCreationMonsterGroup.character_insert;
    std::vector<PreparedStatementUnit> &monsters=preparedStatementForCreationMonsterGroup.monster_insert;
    if(!monsters.empty())
    {
        if(maxMonsterId.size()<monsters.size())
        {
            std::cerr << "maxMonsterId is empty or too small: " << maxMonsterId.size() << "<" << monsters.size() << " for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            client->addCharacter_ReturnFailed(query_id,0x03);
            return;
        }

        unsigned int index=0;
        while(index<monsters.size())
        {
            const EpollServerLoginSlave::LoginProfile::Monster &monster=monsterGroup.at(index);
            PreparedStatementUnit &monsterQuery=monsters.at(index);

            if(maxMonsterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK)
            {
                if(!maxMonsterIdRequested)
                {
                    #ifdef DEBUG_MESSAGE_QUERY_IDLIST
                    std::cout << "Ask more to master: maxMonsterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << std::to_string(maxMonsterId.size()) << "<" << std::to_string(CATCHCHALLENGER_SERVER_MINIDBLOCK) << " for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    #endif
                    if(LinkToMaster::linkToMaster->queryNumberList.empty())
                    {
                        std::cerr << "Unable to get query id at createAccount_return" << std::endl;
                        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
                        client->addCharacter_ReturnFailed(query_id,0x03);
                        return;
                    }
                    const uint8_t &queryNumber=LinkToMaster::linkToMaster->queryNumberList.back();
                    if(LinkToMaster::queryNumberToCharacterGroup[queryNumber]!=0)
                    {
                        std::cerr << "LinkToMaster::queryNumberToCharacterGroup[queryNumber]!=0 for maxMonsterId: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
                        client->addCharacter_ReturnFailed(query_id,0x03);
                        return;
                    }
                    maxMonsterIdRequested=true;
                    LinkToMaster::linkToMaster->queryNumberList.pop_back();

                    //send the network reply
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xC1;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=this->charactersGroupIndex;
                    posOutput+=1;

                    LinkToMaster::linkToMaster->registerOutputQuery(queryNumber,0xC1);
                    LinkToMaster::linkToMaster->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    LinkToMaster::queryNumberToCharacterGroup[queryNumber]=this->charactersGroupIndex;
                }
                else
                {
                    #ifdef DEBUG_MESSAGE_QUERY_IDLIST
                    std::cout << "Not ask more to master (already in progress): maxMonsterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << std::to_string(maxMonsterId.size()) << "<" << std::to_string(CATCHCHALLENGER_SERVER_MINIDBLOCK) << " for charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    #endif
                }
            }

            const uint32_t monster_id=maxMonsterId.back();
            maxMonsterId.pop_back();

            //insert the monster is db
            {
                const std::string &monster_id_string=std::to_string(monster_id);
                //id,gender,id
                if(monster.ratio_gender!=-1)
                {
                    if(rand()%101<monster.ratio_gender)
                        monsterQuery.asyncWrite({monster_id_string,characterIdString,CharactersGroupForLogin::gender_female,characterIdString});
                    else
                        monsterQuery.asyncWrite({monster_id_string,characterIdString,CharactersGroupForLogin::gender_male,characterIdString});
                }
                else
                    monsterQuery.asyncWrite({monster_id_string,characterIdString,characterIdString});

                monster_position++;
            }
            index++;
        }
    }

    character_insert.asyncWrite({
                characterIdString,
                std::to_string(client->account_id),
                #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
                pseudo,
                #else
                SqlFunction::quoteSqlVariable(pseudo),
                #endif
                std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId)),
                std::to_string(sFrom1970())
                });

    //send the network reply
    client->removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(characterId);
    posOutput+=4;

    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

bool CharactersGroupForLogin::removeCharacterLater(void * const client,const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.empty())
    {
        std::cerr << "removeCharacter() Query is empty, bug" << std::endl;
        return false;
    }
    #endif
    RemoveCharacterParam removeCharacterParam;
    removeCharacterParam.query_id=query_id;
    removeCharacterParam.characterId=characterId;
    removeCharacterParam.client=client;

    CatchChallenger::DatabaseBase::CallBack *callback=preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.asyncRead(this,&CharactersGroupForLogin::removeCharacterLater_static,{std::to_string(characterId)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.queryText() << ", error: " << databaseBaseCommon->errorMessage() << std::endl;
        return false;
    }
    else
    {
        removeCharacterParamList.push_back(removeCharacterParam);
        return true;
    }
}

void CharactersGroupForLogin::removeCharacterLater_static(void *object)
{
    if(object!=NULL)
        static_cast<CharactersGroupForLogin *>(object)->removeCharacterLater_object();
}

void CharactersGroupForLogin::removeCharacterLater_object()
{
    RemoveCharacterParam removeCharacterParam=removeCharacterParamList.front();
    removeCharacterParamList.erase(removeCharacterParamList.begin());
    removeCharacterLater_return(static_cast<EpollClientLoginSlave *>(removeCharacterParam.client),removeCharacterParam.query_id,removeCharacterParam.characterId);
    databaseBaseCommon->clear();
}

void CharactersGroupForLogin::removeCharacterLater_return(EpollClientLoginSlave * const client,const uint8_t &query_id,const uint32_t &characterId)
{
    if(!databaseBaseCommon->next())
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    bool ok;
    const uint32_t &account_id=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(0),&ok);
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
    const uint32_t &time_to_delete=databaseBaseCommon->stringtouint32(databaseBaseCommon->value(1),&ok);
    if(ok && time_to_delete>0)
    {
        client->removeCharacter_ReturnFailed(query_id,0x02,"Character: "+std::to_string(characterId)+" is already in deleting for the account: "+std::to_string(account_id));
        return;
    }
    /// \todo don't and failed if timedrift
    preparedDBQueryCommonForLogin.db_query_update_character_time_to_delete_by_id.asyncWrite({
                std::to_string(
                      sFrom1970()+
                      CommonSettingsCommon::commonSettingsCommon.character_delete_time
                  ),
                std::to_string(characterId)
                });

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
