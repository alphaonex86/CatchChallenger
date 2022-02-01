#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "StaticText.hpp"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.hpp"
#endif
#ifndef CATCHCHALLENGER_DB_PREPAREDSTATEMENT
#include "SqlFunction.hpp"
#endif
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/sha224/sha224.hpp"
#include "SqlFunction.hpp"

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
bool Client::server_list()
{
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
    callbackRegistred.pop();
    //send signals into the server

    //C20F and C2OE, logical block and server list
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

    const auto &current_time=sFrom1970();
    bool ok;
    uint8_t validServerCount=0;
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
    ProtocolParsingBase::tempBigBufferForOutput[tempRawDataSizeToSetServerCount]=validServerCount;

    //send the network reply
    removeFromQueryReceived(query_id);
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::deleteCharacterNow(const uint32_t &characterId)
{
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

    const std::string &characterIdString=std::to_string(characterId);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_character.asyncWrite({characterIdString});
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_delete_monster_by_character.asyncWrite({characterIdString});
}

void Client::addCharacter(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
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
    if(CommonDatapack::commonDatapack.profileList.size()!=CommonDatapackServerSpec::commonDatapackServerSpec.serverProfileList.size())
    {
        errorOutput("Client::addCharacter() profile common and server don't match");
        return;
    }
    if(CommonDatapack::commonDatapack.profileList.size()!=GlobalServerData::serverPrivateVariables.serverProfileInternalList.size())
    {
        errorOutput("profile common and server internal don't match");
        return;
    }
    #endif
    if(profileIndex>=CommonDatapack::commonDatapack.profileList.size())
    {
        errorOutput("profile index: "+std::to_string(profileIndex)+" out of range (profileList size: "+std::to_string(CommonDatapack::commonDatapack.profileList.size())+")");
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
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
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
    AddCharacterParam *addCharacterParam=new AddCharacterParam();
    addCharacterParam->query_id=query_id;
    addCharacterParam->profileIndex=profileIndex;
    addCharacterParam->pseudo=pseudo;
    addCharacterParam->monsterGroupId=monsterGroupId;
    addCharacterParam->skinId=skinId;

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
}

void Client::addCharacter_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addCharacter_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    callbackRegistred.pop();
    if(GlobalServerData::serverPrivateVariables.db_common->next())
    {
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

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList[profileIndex];

    const uint32_t &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    const std::string &characterIdString=std::to_string(characterId);
    int monster_position=1;

    const std::vector<Profile::Monster> &monsterGroup=profile.monstergroup.at(monsterGroupId);
    ServerProfileInternal::PreparedStatementForCreationMonsterGroup &monsterGroupInternal=serverProfileInternal.preparedStatementForCreationByCommon.type.monsterGroup[monsterGroupId];
    PreparedStatementUnit &character_insert=monsterGroupInternal.character_insert;
    std::vector<PreparedStatementUnit> &monsters=monsterGroupInternal.monster_insert;
    if(!monsters.empty())
    {
        unsigned int index=0;
        while(index<monsters.size())
        {
            const auto &monster=monsterGroup.at(index);
            const Monster &monsterDatapack=CommonDatapack::commonDatapack.monsters.at(monster.id);
            PreparedStatementUnit &monsterQuery=monsters.at(index);

            GlobalServerData::serverPrivateVariables.maxMonsterId++;
            const uint32_t monster_id=GlobalServerData::serverPrivateVariables.maxMonsterId;

            //insert the monster is db
            {
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

                monster_position++;
            }
            index++;
        }
    }

    character_insert.asyncWrite({
                characterIdString,
                std::to_string(account_id),
                #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
                pseudo,
                #else
                SqlFunction::quoteSqlVariable(pseudo),
                #endif
                std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId)),
                std::to_string(sFrom1970())
                });

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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.db_query_account_time_to_delete_character_by_id.empty())
    {
        std::cerr << "removeCharacter() Query is empty, bug" << std::endl;
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

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
}

void Client::removeCharacterLater_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->removeCharacterLater_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
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
    if(!GlobalServerData::serverPrivateVariables.db_common->next())
    {
        characterSelectionIsWrong(query_id,0x02,"Result return query to remove wrong");
        return;
    }
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
}
#endif
