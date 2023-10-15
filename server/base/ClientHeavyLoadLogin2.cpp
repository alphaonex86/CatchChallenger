#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "StaticText.hpp"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#endif
#ifndef CATCHCHALLENGER_DB_PREPAREDSTATEMENT
#include "SqlFunction.hpp"
#endif
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#ifdef CATCHCHALLENGER_DB_FILE
#include <sys/stat.h>
#endif

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
bool Client::server_list()
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_select_server_time.asyncRead(this,&Client::server_list_static,{std::to_string(account_id)});
    if(callback==NULL)
    {
        std::cerr << "Sql error, error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        errorOutput("Unable to get the server list");
        return false;
    }
    else
    {
        callbackRegistred.push(callback);
        return true;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    server_list_object();
    return true;
    #elif CATCHCHALLENGER_DB_FILE
    server_list_object();
    return true;
    #else
    #error Define what do here
    #endif
}

void Client::server_list_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->server_list_object();
}

void Client::server_list_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    if(paramToPassToCallBack.size()!=1)
    {
        std::cerr << "paramToPassToCallBack.size()!=1" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    server_list_return(askLoginParam->query_id,askLoginParam->characterOutputData,askLoginParam->characterOutputDataSize);
    delete[] askLoginParam->characterOutputData;
    delete askLoginParam;
}

void Client::server_list_return(const uint8_t &query_id, const char * const characterOutputData, const uint32_t &characterOutputDataSize)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    //send signals into the server

    //0x44 and 0x40, logical block and server list
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(Client::protocolMessageLogicalGroupAndServerList[0]!=0x44 || Client::protocolMessageLogicalGroupAndServerList[9]!=0x40)
    {
        std::cerr << "Client::protocolMessageLogicalGroupAndServerList corruption detected" << std::endl;
        abort();
    }
    #endif
    sendRawBlock((char *)Client::protocolMessageLogicalGroupAndServerList,Client::protocolMessageLogicalGroupAndServerListSize);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(Client::protocolReplyCharacterListSize==0)
    {
        std::cerr << "Client::protocolReplyCharacterListSize==0: never init" << std::endl;
        abort();
    }
    #endif

    //send the network reply
    uint32_t posOutput=0;
    //std::cout << "protocolReplyCharacterList: " << binarytoHexa(Client::protocolReplyCharacterList,Client::protocolReplyCharacterListSize) << std::endl;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,Client::protocolReplyCharacterList,Client::protocolReplyCharacterListSize);
    posOutput+=Client::protocolReplyCharacterListSize;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,characterOutputData,characterOutputDataSize);
    posOutput+=characterOutputDataSize;

    int tempRawDataSizeToSetServerCount=posOutput;
    posOutput+=1;

    uint8_t validServerCount=0;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const auto &current_time=sFrom1970();
    bool ok;
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        //server index
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0;
        posOutput+=1;

        //played_time
        unsigned int played_time=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            std::cerr << "played_time is not number: " << GlobalServerData::serverPrivateVariables.db_common->value(1) << " fixed by 0" << std::endl;
            played_time=0;
        }
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(played_time);
        posOutput+=sizeof(uint32_t);

        //last_connect
        unsigned int last_connect=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
        if(!ok)
        {
            std::cerr << "last_connect is not number: " << GlobalServerData::serverPrivateVariables.db_common->value(2) << " fixed by 0" << std::endl;
            last_connect=static_cast<uint32_t>(current_time);
        }
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(last_connect);
        posOutput+=sizeof(uint32_t);

        validServerCount++;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
        if(validServerCount==0)
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_server_time.asyncWrite({
                        "0",
                        std::to_string(account_id),
                        std::to_string(sFrom1970())
                        });
        else
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_server_time_last_connect.asyncWrite({
                        std::to_string(sFrom1970()),
                        "0",
                        std::to_string(account_id)
                        });
        #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    ProtocolParsingBase::tempBigBufferForOutput[tempRawDataSizeToSetServerCount]=validServerCount;

    //send the network reply
    removeFromQueryReceived(query_id);
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::deleteCharacterNow(const uint32_t &characterId)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_character is empty, bug" << std::endl;
        return;
    }
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_monster_by_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_by_id is empty, bug" << std::endl;
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const std::string &characterIdString=std::to_string(characterId);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_character.asyncWrite({characterIdString});
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_monster_by_character.asyncWrite({characterIdString});
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)characterId;
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::addCharacter(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.empty())
    {
        std::cerr << "addCharacter() Query is empty, bug" << std::endl;
        return;
    }
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_insert_monster.empty())
    {
        errorOutput("addCharacter() Query db_query_insert_monster is empty, bug");
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    std::cerr << "Client::addCharacter() for file" << std::endl;
    #else
    #error Define what do here
    #endif
    if(GlobalServerData::serverPrivateVariables.skinList.empty())
    {
        std::cerr << "Skin list is empty, unable to add charaters" << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(number_of_character>=CommonSettingsCommon::commonSettingsCommon.max_character)
    {
        errorOutput("You can't create more account, you have already "+std::to_string(number_of_character)+" on "+std::to_string(CommonSettingsCommon::commonSettingsCommon.max_character)+" allowed");
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.get_profileList().size()!=CommonDatapackServerSpec::commonDatapackServerSpec.get_serverProfileList().size())
    {
        errorOutput("Client::addCharacter() profile common and server don't match");
        return;
    }
    if(CommonDatapack::commonDatapack.get_profileList().size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        errorOutput("profile common and server internal don't match");
        return;
    }
    #endif
    if(profileIndex>=CommonDatapack::commonDatapack.get_profileList().size())
    {
        errorOutput("profile index: "+std::to_string(profileIndex)+" out of range (profileList size: "+std::to_string(CommonDatapack::commonDatapack.get_profileList().size())+")");
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).valid)
    {
        errorOutput("profile index: "+std::to_string(profileIndex)+" profil not valid");
        return;
    }
    if(pseudo.empty())
    {
        errorOutput("pseudo is empty, not allowed");
        return;
    }
    if(pseudo.find(" ")!=std::string::npos)
    {
        errorOutput("Your pseudo can't contains space");
        return;
    }
    //detect the incorrect char
    if(std::memchr(pseudo.data(),0x00,pseudo.size())!=NULL)
    {
        errorOutput("pseudo contain wrong data, not allowed");
        return;
    }
    if(pseudo.size()>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
    {
        errorOutput("pseudo size is too big: "+std::to_string(pseudo.size())+" because is greater than "+std::to_string(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
        return;
    }
    if(skinId>=GlobalServerData::serverPrivateVariables.skinList.size())
    {
        errorOutput("skin provided: "+std::to_string(skinId)+" is not into skin listed");
        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(profileIndex);
    if(!profile.forcedskin.empty() && !vectorcontainsAtLeastOne(profile.forcedskin,skinId))
    {
        errorOutput("skin provided: "+std::to_string(skinId)+" is not into profile "+std::to_string(profileIndex)+" forced skin list");
        return;
    }
    if(monsterGroupId>=profile.monstergroup.size())
    {
        errorOutput("monsterGroupId: "+std::to_string(skinId)+" is not into profile "+std::to_string(profileIndex));
        return;
    }
    if(profileIndex>=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        errorOutput("out of bound");
        std::cerr << "out of bound (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
        return;
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(monsterGroupId>=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.size())
    {
        errorOutput("out of bound");
        std::cerr << "out of bound (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
        return;
    }
    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex).preparedStatementForCreationByCommon.type.monsterGroup.at(monsterGroupId).character_insert.empty())
    {
        errorOutput("character_insert.empty()");
        std::cerr << "character_insert.empty() (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
        return;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    AddCharacterParam *addCharacterParam=new AddCharacterParam();
    addCharacterParam->query_id=query_id;
    addCharacterParam->profileIndex=profileIndex;
    addCharacterParam->pseudo=pseudo;
    addCharacterParam->monsterGroupId=monsterGroupId;
    addCharacterParam->skinId=skinId;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.asyncRead(this,&Client::addCharacter_static,{
        #ifdef CATCHCHALLENGER_DB_PREPAREDSTATEMENT
        pseudo
        #else
        SqlFunction::quoteSqlVariable(pseudo)
        #endif
        });
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_select_character_by_pseudo.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        delete addCharacterParam;
        return;
    }
    else
    {
        paramToPassToCallBack.push(addCharacterParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AddCharacterParam");
        #endif
        callbackRegistred.push(callback);
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    paramToPassToCallBack.push(addCharacterParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AddCharacterParam");
    #endif
    addCharacter_object();
    #elif CATCHCHALLENGER_DB_FILE
    paramToPassToCallBack.push(addCharacterParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("AddCharacterParam");
    #endif
    addCharacter_object();
    #else
    #error Define what do here
    #endif
}

void Client::addCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addCharacter_object();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_common->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::addCharacter_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AddCharacterParam *addCharacterParam=static_cast<AddCharacterParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(addCharacterParam==NULL)
        abort();
    #endif
    addCharacter_return(addCharacterParam->query_id,addCharacterParam->profileIndex,addCharacterParam->pseudo,addCharacterParam->monsterGroupId,addCharacterParam->skinId);
    delete addCharacterParam;
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_common->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::addCharacter_return(const uint8_t &query_id,const uint8_t &profileIndex,const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AddCharacterParam")
    {
        std::cerr << "is not AddCharacterParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    if(pseudo.size()<1)
    {
        std::cerr << "Client::addCharacter_return() pseudo.size()<1 abort() " << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    callbackRegistred.pop();
    //if character already exist then return error
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        std::cerr << "Client::addCharacter_return() character already exist then return error " << __FILE__ << " " << __LINE__ << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        return;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)pseudo;
    (void)skinId;
    #elif CATCHCHALLENGER_DB_FILE

    const std::string hexa=binarytoHexa(pseudo.c_str(),pseudo.size());
    if(hexa.size()<1)
    {
        std::cerr << "hexa.size()<1 abort() " << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    {
        struct stat sb;
        if(::stat(("database/characters/"+hexa).c_str(),&sb)==0)
        {
            std::cerr << "Client::addCharacter_return() character already exist " << hexa << " then return error " << __FILE__ << " " << __LINE__ << std::endl;

            //send the network reply
            removeFromQueryReceived(query_id);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
            posOutput+=1;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
            posOutput+=4;

            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

            return;
        }
    }
    {
        std::cerr << "save GlobalServerData::serverPrivateVariables.maxCharacterId++; for DB FILE" << std::endl;
        abort();
    }
    #else
    #error Define what do here
    #endif

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;
    const uint32_t &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;

    const Profile &profile=CommonDatapack::commonDatapack.get_profileList().at(profileIndex);
    if(GlobalServerData::serverPrivateVariables.serverProfileInternalList.size()<=profileIndex)
    {
        std::cerr << "GlobalServerData::serverPrivateVariables.serverProfileInternalList.size()<=profileIndex " << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[profileIndex];

    const std::vector<Profile::Monster> &monsterGroup=profile.monstergroup.at(monsterGroupId);

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    const std::string &characterIdString=std::to_string(characterId);
    int monster_position=1;

    if(serverProfileInternal.preparedStatementForCreationByCommon.type.monsterGroup.size()<=monsterGroupId)
    {
        std::cerr << "serverProfileInternal.preparedStatementForCreationByCommon.type.monsterGroup.size()<=monsterGroupId " << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    ServerProfileInternal::PreparedStatementForCreationMonsterGroup &monsterGroupInternal=serverProfileInternal.preparedStatementForCreationByCommon.type.monsterGroup[monsterGroupId];
    PreparedStatementUnit &character_insert=monsterGroupInternal.character_insert;
    std::vector<PreparedStatementUnit> &monsters=monsterGroupInternal.monster_insert;
    if(!monsters.empty())
    {
        unsigned int index=0;
        while(index<monsters.size())
        {
            const Profile::Monster &monster=monsterGroup.at(index);
            const Monster &monsterDatapack=CommonDatapack::commonDatapack.get_monsters().at(monster.id);
            PreparedStatementUnit &monsterQuery=monsters.at(index);

            GlobalServerData::serverPrivateVariables.maxMonsterId++;
            const uint32_t monster_id=GlobalServerData::serverPrivateVariables.maxMonsterId;

            //insert the monster is db
            {
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                const std::string &monster_id_string=std::to_string(monster_id);
                //id,gender,id
                if(monsterDatapack.ratio_gender!=-1)
                {
                    if(rand()%101<monsterDatapack.ratio_gender)
                        monsterQuery.asyncWrite({monster_id_string,characterIdString,StaticText::text_2,characterIdString});
                    else
                        monsterQuery.asyncWrite({monster_id_string,characterIdString,StaticText::text_1,characterIdString});
                }
                else
                    monsterQuery.asyncWrite({monster_id_string,characterIdString,characterIdString});
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                (void)character_insert;
                (void)monsterDatapack;
                (void)monsterQuery;
                (void)monster_id;
                #else
                #error Define what do here
                #endif

                monster_position++;
            }
            index++;
        }
    }

    if(!character_insert.asyncWrite({
                characterIdString,
                std::to_string(account_id),
                #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                pseudo,
                #else
                SqlFunction::quoteSqlVariable(pseudo),
                #endif
                std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId)),
                std::to_string(sFrom1970())
                }))
    {
        std::cerr << "character insert query failed " << __FILE__ << __LINE__ << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        return;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    {
        {
            //add into the char list
            std::vector<CharacterEntry> characterEntryList;

            {
                std::ifstream in_file("database/accounts/"+std::to_string(account_id), std::ifstream::binary);
                if(!in_file.good() || !in_file.is_open())
                {
                    std::cerr << "Unable to open data base file " << "database/accounts/" << account_id << " (abort)" << std::endl;
                    abort();
                    return;
                }
                hps::StreamInputBuffer s(in_file);
                s >> characterEntryList;
            }

            CharacterEntry newChar;
            newChar.charactersGroupIndex=profileIndex;
            newChar.character_id=characterId;
            newChar.delete_time_left=0;
            newChar.last_connect=0;
            newChar.played_time=0;
            newChar.pseudo=pseudo;
            newChar.skinId=skinId;//use dictionary here
            characterEntryList.push_back(newChar);

            {
                std::ofstream out_file("database/accounts/"+std::to_string(account_id), std::ifstream::binary);
                if(!out_file.good() || !out_file.is_open())
                {
                    std::cerr << "unable to save file into DB FILE mode (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                    return;
                }
                hps::to_stream(characterEntryList, out_file);
            }
        }

        //do file here ready to copy for each profile
        std::ofstream out_file(("database/characters/"+hexa), std::ofstream::binary);
        if(!out_file.good() || !out_file.is_open())
        {
            std::cerr << "unable to save file into DB FILE mode (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
            return;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()<=0)
        {
            std::cerr << "unable to save profile into DB FILE mode CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()<=0 (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        if(CommonDatapack::commonDatapack.get_items().itemMaxId<=0)
        {
            std::cerr << "unable to save profile into DB FILE mode CommonDatapack::commonDatapack.get_items().itemMaxId<=0 (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        if(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()<=0)
        {
            std::cerr << "unable to save profile into DB FILE mode CommonDatapack::commonDatapack.get_items().itemMaxId<=0 (abort) " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        /*Player_private_and_public_informations playerForProfile;
        std::string bot_already_beatenS;
        std::string encyclopedia_itemS;
        std::string encyclopedia_monsterS;
        std::string recipesS;
        bot_already_beatenS=std::string(playerForProfile.bot_already_beaten,CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
        playerForProfile.cash=profile.cash;
        playerForProfile.clan=0;
        playerForProfile.clan_leader=false;
        encyclopedia_itemS=std::string(playerForProfile.encyclopedia_item,CommonDatapack::commonDatapack.get_items().itemMaxId/8+1);
        for(unsigned int i = 0; i < profile.items.size(); i++)
        {
            const Profile::Item &item=profile.items.at(i);
            playerForProfile.encyclopedia_item[item.id/8]|=(1<<(7-item.id%8));
            playerForProfile.items[item.id]=item.quantity;
        }
        encyclopedia_monsterS=std::string(playerForProfile.encyclopedia_monster,CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
        playerForProfile.public_informations.monsterId=0;
        playerForProfile.public_informations.simplifiedId=0;
        playerForProfile.public_informations.skinId=0;
        playerForProfile.public_informations.speed=0;
        playerForProfile.public_informations.type=Player_type_normal;
        recipesS=std::string(playerForProfile.recipes,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
        playerForProfile.repel_step=0;
        playerForProfile.warehouse_cash=0;*/

        //the internal profile
        map=(CommonMap *)serverProfileInternal.map;
        last_direction=(Direction)serverProfileInternal.orientation;
        x=serverProfileInternal.x;
        y=serverProfileInternal.y;

        PlayerOnMap temp;
        temp.map=(CommonMap *)serverProfileInternal.map;
        temp.orientation=serverProfileInternal.orientation;
        temp.x=serverProfileInternal.x;
        temp.y=serverProfileInternal.y;

        map_entry=temp;
        rescue=temp;
        unvalidated_rescue=temp;

        //the general profile
        this->public_and_private_informations.cash=profile.cash;
        {
            this->public_and_private_informations.items.clear();
            unsigned int index=0;
            while(index<profile.items.size())
            {
                const Profile::Item item=profile.items.at(index);
                this->public_and_private_informations.items[item.id]=item.quantity;
                index++;
            }
        }
        {
            this->public_and_private_informations.reputation.clear();
            unsigned int index=0;
            while(index<profile.reputations.size())
            {
                const Profile::Reputation reputation=profile.reputations.at(index);
                PlayerReputation r;
                r.level=reputation.level;
                r.point=reputation.point;
                this->public_and_private_informations.reputation[reputation.internalIndex]=r;
                index++;
            }
        }
        this->public_and_private_informations.public_informations.skinId=skinId;
        this->public_and_private_informations.public_informations.type=Player_type::Player_type_normal;

        {
            unsigned int index=0;
            while(index<monsterGroup.size())
            {
                const Profile::Monster &monster=monsterGroup.at(index);
                const Monster &monsterDatapack=CommonDatapack::commonDatapack.get_monsters().at(monster.id);
                const Monster::Stat &stat=getStat(monsterDatapack,monster.level);
                PlayerMonster m;
                m.catched_with=monster.captured_with;
                m.character_origin=characterId;
                m.egg_step=0;
                if(monsterDatapack.ratio_gender>0 && monsterDatapack.ratio_gender<100)
                {
                    int8_t temp_ratio=random()%101;
                    if(temp_ratio<monsterDatapack.ratio_gender)
                        m.gender=Gender_Male;
                    else
                        m.gender=Gender_Female;
                }
                else
                {
                    switch(monsterDatapack.ratio_gender)
                    {
                        case 0:
                            m.gender=Gender_Male;
                        break;
                        case 100:
                            m.gender=Gender_Female;
                        break;
                        default:
                            m.gender=Gender_Unknown;
                        break;
                    }
                }
                m.hp=stat.hp;
                m.id=index;
                m.level=monster.level;
                m.monster=monster.id;
                m.remaining_xp=0;
                m.sp=0;
                m.skills=CommonFightEngine::generateWildSkill(monsterDatapack,monster.level);
                #ifdef CATCHCHALLENGER_DEBUG_FIGHT
                {
                    if(m.skills.empty())
                    {
                        if(monsterDef.learn.empty())
                            messageFightEngine("no skill to learn for random monster");
                        else
                        {
                            messageFightEngine("no skill for random monster, but skill to learn:");
                            unsigned int index=0;
                            while(index<monsterDef.learn.size())
                            {
                                messageFightEngine(std::to_string(monsterDef.learn.at(index).learnSkill)+" level "+
                                                   std::to_string(monsterDef.learn.at(index).learnSkillLevel)+" for monster at level "+
                                                   std::to_string(monsterDef.learn.at(index).learnAtLevel));
                                index++;
                            }
                        }
                    }
                    else
                    {
                        unsigned int index=0;
                        while(index<m.skills.size())
                        {
                            messageFightEngine("skill for random monster: "+std::to_string(m.skills.at(index).skill)+" level "+std::to_string(m.skills.at(index).level));
                            index++;
                        }
                    }
                    messageFightEngine("random monster id: "+std::to_string(playerMonster.monster));
                }
                #endif
                this->public_and_private_informations.playerMonster.push_back(m);
                index++;
            }
        }

        //should be not needed
        if(public_and_private_informations.recipes!=nullptr)
            std::cerr << "strange public_and_private_informations.recipes!=nullptr" << __FILE__ << ":" << __LINE__ << std::endl;
        public_and_private_informations.recipes=nullptr;
        if(public_and_private_informations.encyclopedia_monster!=nullptr)
            std::cerr << "strange public_and_private_informations.encyclopedia_monster!=nullptr" << __FILE__ << ":" << __LINE__ << std::endl;
        public_and_private_informations.encyclopedia_monster=nullptr;
        if(public_and_private_informations.encyclopedia_item!=nullptr)
            std::cerr << "strange public_and_private_informations.encyclopedia_item!=nullptr" << __FILE__ << ":" << __LINE__ << std::endl;
        public_and_private_informations.encyclopedia_item=nullptr;
        if(public_and_private_informations.bot_already_beaten!=nullptr)
            std::cerr << "strange public_and_private_informations.bot_already_beaten!=nullptr" << __FILE__ << ":" << __LINE__ << std::endl;
        public_and_private_informations.bot_already_beaten=nullptr;

        hps::to_stream(*this, out_file);
        out_file.close();

        //reset all the variables
        map=nullptr;
        last_direction=Direction_look_at_bottom;
        x=0;
        y=0;
        PlayerOnMap tempEmpty;
        tempEmpty.map=nullptr;
        tempEmpty.orientation=Orientation_bottom;
        tempEmpty.x=0;
        tempEmpty.y=0;
        map_entry=tempEmpty;
        rescue=tempEmpty;
        unvalidated_rescue=tempEmpty;
        this->public_and_private_informations.cash=0;
        this->public_and_private_informations.items.clear();
        this->public_and_private_informations.reputation.clear();
        this->public_and_private_informations.public_informations.skinId=0;
        this->public_and_private_informations.public_informations.type=Player_type::Player_type_normal;
        this->public_and_private_informations.playerMonster.clear();
    }
    #else
    #error Define what do here
    #endif

    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(characterId);
    posOutput+=4;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::removeCharacterLater(const uint8_t &query_id, const uint32_t &characterId)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.empty())
    {
        std::cerr << "removeCharacter() Query is empty, bug" << std::endl;
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    CatchChallenger::DatabaseBaseCallBack *callback=GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.asyncRead(this,&Client::removeCharacterLater_static,{std::to_string(characterId)});
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.queryText() << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        delete removeCharacterParam;
        return;
    }
    else
    {
        callbackRegistred.push(callback);
        paramToPassToCallBack.push(removeCharacterParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("RemoveCharacterParam");
        #endif
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    paramToPassToCallBack.push(removeCharacterParam);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    paramToPassToCallBackType.push("RemoveCharacterParam");
    #endif
    removeCharacterLater_object();
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::removeCharacterLater_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->removeCharacterLater_object();
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverPrivateVariables.db_common->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}

void Client::removeCharacterLater_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=static_cast<RemoveCharacterParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(removeCharacterParam==NULL)
        abort();
    #endif
    removeCharacterLater_return(removeCharacterParam->query_id,removeCharacterParam->characterId);
    delete removeCharacterParam;
}

void Client::removeCharacterLater_return(const uint8_t &query_id,const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="RemoveCharacterParam")
    {
        std::cerr << "is not RemoveCharacterParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    (void)characterId;
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    bool ok;
    const uint32_t &account_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
    if(!ok)
    {
        characterSelectionIsWrong(query_id,0x02,"Account for character: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" is not an id");
        return;
    }
    if(this->account_id!=account_id)
    {
        characterSelectionIsWrong(query_id,0x02,"Character: "+std::to_string(characterId)+" is not owned by the account: "+std::to_string(account_id)+"");
        return;
    }
    const uint32_t &time_to_delete=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
    if(ok && time_to_delete>0)
    {
        characterSelectionIsWrong(query_id,0x02,"Character: "+std::to_string(characterId)+" is already in deleting for the account: "+std::to_string(account_id));
        return;
    }
    /// \todo don't save and failed if timedrift detected
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_update_character_time_to_delete_by_id.asyncWrite({
                std::to_string(
                      sFrom1970()+
                      CommonSettingsCommon::commonSettingsCommon.character_delete_time
                  ),
                  std::to_string(characterId)
                });

    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1;

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    posOutput+=1;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
}
#endif
