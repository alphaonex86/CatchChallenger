#include "Client.h"
#include "GlobalServerData.h"

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonMap.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/cpp11addition.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "DatabaseFunction.h"
#include "SqlFunction.h"
#include "PreparedDBQuery.h"
#include "DictionaryLogin.h"
#include "DictionaryServer.h"
#include "BaseServerMasterSendDatapack.h"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef EPOLLCATCHCHALLENGERSERVER
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB
    #endif
#endif

/// \todo solve disconnecting/destroy during the SQL loading

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
bool Client::askLogin(const uint8_t &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorOutput("askLogin() Query login is empty, bug");
        return false;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorOutput("askLogin() Query inset login is empty, bug");
        return false;
    }
    if(PreparedDBQueryCommon::db_query_characters.empty())
    {
        errorOutput("askLogin() Query characters is empty, bug");
        return false;
    }
    #endif
    AskLoginParam *askLoginParam=new AskLoginParam;
    SHA224(reinterpret_cast<const unsigned char *>(rawdata),CATCHCHALLENGER_SHA224HASH_SIZE,reinterpret_cast<unsigned char *>(askLoginParam->login));
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);

    std::string queryText=PreparedDBQueryLogin::db_query_login;
    stringreplaceOne(queryText,"%1",binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText,this,&Client::askLogin_static);
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,"Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_login->errorMessage());
        delete askLoginParam;
        return false;
    }
    else
    {
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
        return true;
    }
}

void Client::askLogin_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->askLogin_object();
    GlobalServerData::serverPrivateVariables.db_login->clear();
    //delete askLoginParam; -> not here because need reuse later
}

void Client::askLogin_object()
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
    askLogin_return(askLoginParam);
    //delete askLoginParam; -> not here because need reuse later
}

void Client::askLogin_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();
    {
        bool ok;
        if(!GlobalServerData::serverPrivateVariables.db_login->next())
        {
            //return creation query to client
            if(GlobalServerData::serverSettings.automatic_account_creation)
            {
                //network send
                *(Client::loginIsWrongBuffer+1)=(uint8_t)askLoginParam->query_id;
                *(Client::loginIsWrongBuffer+1+1+4)=(uint8_t)0x07;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(askLoginParam->query_id);
                #endif
                inputQueryNumberToPacketCode[askLoginParam->query_id]=0;
                internalSendRawSmallPacket(reinterpret_cast<char *>(Client::loginIsWrongBuffer),sizeof(Client::loginIsWrongBuffer));
                delete askLoginParam;
                stat=ClientStat::ProtocolGood;
                return;
/*                GlobalServerData::serverPrivateVariables.maxAccountId++;
                account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(std::string(askLoginParam->login.toHex())).arg(sFrom1970()));*/
            }
            else
            {
                loginIsWrong(askLoginParam->query_id,0x02,"Bad login for: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+", pass: "+binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE));
                delete askLoginParam;
                return;
            }
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::vector<char> tempAddedToken;
            std::vector<char> secretTokenBinary;
            #endif
            {
                int32_t tokenForAuthIndex=0;
                while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                {
                    const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[tokenForAuthIndex];
                    if(tokenLink.client==this)
                    {
                        const std::string &secretToken(GlobalServerData::serverPrivateVariables.db_login->value(1));
                        #ifndef CATCHCHALLENGER_EXTRA_CHECK
                        std::vector<char> secretTokenBinary;
                        #endif
                        secretTokenBinary=GlobalServerData::serverPrivateVariables.db_login->hexatoBinary(secretToken);
                        if(secretTokenBinary.size()!=CATCHCHALLENGER_SHA224HASH_SIZE)
                        {
                            std::cerr << "convertion to binary for pass failed for: " << GlobalServerData::serverPrivateVariables.db_login->value(1) << std::endl;
                            abort();
                        }
                        secretTokenBinary.resize(secretTokenBinary.size()+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(secretTokenBinary.data()+CATCHCHALLENGER_SHA224HASH_SIZE,tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        //append the token
                        tempAddedToken.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(tempAddedToken.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        if(secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                        {
                            std::cerr << "secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)" << std::endl;
                            abort();
                        }
                        #endif
                        SHA224(reinterpret_cast<const unsigned char *>(secretTokenBinary.data()),secretTokenBinary.size(),reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));
                        BaseServerLogin::tokenForAuthSize--;
                        //see to do with SIMD
                        if(BaseServerLogin::tokenForAuthSize>0)
                        {
                            while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                            {
                                BaseServerLogin::tokenForAuth[tokenForAuthIndex]=BaseServerLogin::tokenForAuth[tokenForAuthIndex+1];
                                tokenForAuthIndex++;
                            }
                            //don't work:memmove(BaseServerLogin::tokenForAuth+index*sizeof(TokenLink),BaseServerLogin::tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(BaseServerLogin::tokenForAuthSize-index));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(BaseServerLogin::tokenForAuth[0].client==NULL)
                                abort();
                            #endif
                        }
                        tokenForAuthIndex--;
                        break;
                    }
                    tokenForAuthIndex++;
                }
                if(tokenForAuthIndex>=(int32_t)BaseServerLogin::tokenForAuthSize)
                {
                    loginIsWrong(askLoginParam->query_id,0x02,"No temp auth token found");
                    return;
                }
            }
            if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                loginIsWrong(askLoginParam->query_id,0x03,"Password wrong: "+
                             binarytoHexa(secretTokenBinary)+
                             " + token "+
                             binarytoHexa(tempAddedToken)+
                             " = "+
                             " hashedToken: "+
                             binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             "sended pass + token: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #else
                loginIsWrong(askLoginParam->query_id,0x03,"Password wrong: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #endif
                delete askLoginParam;
                return;
            }
            else
            {
                account_id=GlobalServerData::serverPrivateVariables.db_login->stringtouint32(GlobalServerData::serverPrivateVariables.db_login->value(0),&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    delete askLoginParam;
                    return;
                }
                else
                {
                    flags|=0x08;
                }
                stat=ClientStat::Logged;
            }
        }
    }
    std::string queryText=PreparedDBQueryCommon::db_query_characters;
    stringreplaceOne(queryText,"%1",std::to_string(account_id));
    stringreplaceOne(queryText,"%2",std::to_string(CommonSettingsCommon::commonSettingsCommon.max_character+1));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::character_list_static);
    if(callback==NULL)
    {
        account_id=0;
        loginIsWrong(askLoginParam->query_id,0x04,"Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_common->errorMessage());
        delete askLoginParam;
        return;
    }
    else
    {
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
    }
}

bool Client::createAccount(const uint8_t &query_id, const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorOutput("createAccount() Query login is empty, bug");
        return false;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorOutput("createAccount() Query inset login is empty, bug");
        return false;
    }
    if(PreparedDBQueryCommon::db_query_characters.empty())
    {
        errorOutput("createAccount() Query characters is empty, bug");
        return false;
    }
    if(!GlobalServerData::serverSettings.automatic_account_creation)
    {
        errorOutput("createAccount() Creation account not premited");
        return false;
    }
    #endif
    /// \note No SHA224 because already double hashed before
    AskLoginParam *askLoginParam=new AskLoginParam;
    memcpy(askLoginParam->login,rawdata,CATCHCHALLENGER_SHA224HASH_SIZE);
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);
    askLoginParam->query_id=query_id;

    std::string queryText=PreparedDBQueryLogin::db_query_login;
    stringreplaceOne(queryText,"%1",binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_login->asyncRead(queryText,this,&Client::createAccount_static);
    if(callback==NULL)
    {
        stat=ClientStat::ProtocolGood;
        loginIsWrong(askLoginParam->query_id,0x03,"Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_login->errorMessage());
        delete askLoginParam;
        return false;
    }
    else
    {
        number_of_character++;
        paramToPassToCallBack.push(askLoginParam);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("AskLoginParam");
        #endif
        callbackRegistred.push(callback);
        return true;
    }
}

void Client::createAccount_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->createAccount_object();
    GlobalServerData::serverPrivateVariables.db_login->clear();
}

void Client::createAccount_object()
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
    createAccount_return(askLoginParam);
    delete askLoginParam;
}

void Client::createAccount_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();
    if(!GlobalServerData::serverPrivateVariables.db_login->next())
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        GlobalServerData::serverPrivateVariables.maxAccountId++;
        account_id=GlobalServerData::serverPrivateVariables.maxAccountId;
        std::string queryText=PreparedDBQueryLogin::db_query_insert_login;
        stringreplaceOne(queryText,"%1",std::to_string(account_id));
        stringreplaceOne(queryText,"%2",binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
        stringreplaceOne(queryText,"%3",binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE));
        stringreplaceOne(queryText,"%4",std::to_string(sFrom1970()));
        dbQueryWriteLogin(queryText);

        //send the network reply
        removeFromQueryReceived(askLoginParam->query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=askLoginParam->query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        stat=ClientStat::ProtocolGood;
    }
    else
        loginIsWrong(askLoginParam->query_id,0x02,"Login already used: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
}

void Client::character_list_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->character_list_object();
}

void Client::character_list_object()
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
    askLoginParam->characterOutputDataSize=character_list_return(ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->query_id);
    if(askLoginParam->characterOutputDataSize==0)
        return;
    askLoginParam->characterOutputData=(char *)malloc(askLoginParam->characterOutputDataSize);
    memcpy(askLoginParam->characterOutputData,ProtocolParsingBase::tempBigBufferForOutput,askLoginParam->characterOutputDataSize);
    //re use
    //delete askLoginParam;
    if(server_list())
        paramToPassToCallBack.push(askLoginParam);
}

uint32_t Client::character_list_return(char * data,const uint8_t &query_id)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();
    //send signals into the server
    #ifndef SERVERBENCHMARK
    normalOutput("Logged the account "+std::to_string(account_id));
    #endif
    //send the network reply
    uint32_t posOutput=0;

    {
        const auto &current_time=sFrom1970();
        std::vector<CharacterEntry> characterEntryList;
        bool ok;
        while(GlobalServerData::serverPrivateVariables.db_common->next() && characterEntryList.size()<CommonSettingsCommon::commonSettingsCommon.max_character)
        {
            CharacterEntry characterEntry;
            characterEntry.character_id=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
            if(ok)
            {
                uint32_t time_to_delete=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(3),&ok);
                if(!ok)
                {
                    normalOutput("time_to_delete is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(3)+" for "+std::to_string(account_id)+" fixed by 0");
                    time_to_delete=0;
                }
                characterEntry.played_time=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(4),&ok);
                if(!ok)
                {
                    normalOutput("played_time is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(4)+" for "+std::to_string(account_id)+" fixed by 0");
                    characterEntry.played_time=0;
                }
                characterEntry.last_connect=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(5),&ok);
                if(!ok)
                {
                    normalOutput("last_connect is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" for "+std::to_string(account_id)+" fixed by 0");
                    characterEntry.last_connect=current_time;
                }
                if(current_time>=time_to_delete && time_to_delete!=0)
                    deleteCharacterNow(characterEntry.character_id);
                else
                {
                    if(time_to_delete==0)
                        characterEntry.delete_time_left=0;
                    else
                        characterEntry.delete_time_left=time_to_delete-current_time;
                    characterEntry.pseudo=GlobalServerData::serverPrivateVariables.db_common->value(1);
                    const uint32_t &skinIdTemp=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
                    if(!ok)
                    {
                        normalOutput("character return skin is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(5)+" for "+std::to_string(account_id)+" fixed by 0");
                        characterEntry.skinId=0;
                        ok=true;
                    }
                    else
                    {
                        if(skinIdTemp>=(uint32_t)DictionaryLogin::dictionary_skin_database_to_internal.size())
                        {
                            normalOutput("character return skin out of range: "+
                                         GlobalServerData::serverPrivateVariables.db_common->value(5)+
                                         " for "+
                                         std::to_string(account_id)+
                                         "fixed by 0");
                            characterEntry.skinId=0;
                            ok=true;
                        }
                        else
                            characterEntry.skinId=DictionaryLogin::dictionary_skin_database_to_internal.at(skinIdTemp);
                    }
                    if(ok)
                        characterEntryList.push_back(characterEntry);
                }
            }
            else
                normalOutput("Character id is not number: "+GlobalServerData::serverPrivateVariables.db_common->value(0)+" for "+std::to_string(account_id));
        }
        if(CommonSettingsCommon::commonSettingsCommon.max_character==0 && characterEntryList.empty())
        {
            loginIsWrong(query_id,0x05,"Can't create character and don't have character");
            return 0;
        }

        data[posOutput]=0x01;//Number of characters group, characters group 0, all in one server
        posOutput+=1;

        number_of_character=characterEntryList.size();
        data[posOutput]=characterEntryList.size();
        posOutput+=1;
        unsigned int index=0;
        while(index<characterEntryList.size())
        {
            const CharacterEntry &characterEntry=characterEntryList.at(index);
            *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.character_id);
            posOutput+=4;
            {
                {
                    const std::string &text=characterEntry.pseudo;
                    data[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(data+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
            }
            data[posOutput]=characterEntry.skinId;
            posOutput+=1;
            *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.delete_time_left);
            posOutput+=4;
            /// \todo optimise by simple suming here, not SQL query
            *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.played_time);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(data+posOutput)=htole32(characterEntry.last_connect);
            posOutput+=4;
            index++;
        }
    }

    return posOutput;
}

bool Client::server_list()
{
    std::string queryText=PreparedDBQueryCommon::db_query_select_server_time;
    stringreplaceOne(queryText,"%1",std::to_string(account_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::server_list_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << PreparedDBQueryCommon::db_query_select_allow << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
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
    delete askLoginParam->characterOutputData;
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
            last_connect=current_time;
        }
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(last_connect);
        posOutput+=sizeof(uint32_t);

        validServerCount++;
    }
    #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
    if(validServerCount==0)
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_server_time;
        stringreplaceOne(queryText,"%1","0");
        stringreplaceOne(queryText,"%2",std::to_string(account_id));
        stringreplaceOne(queryText,"%3",std::to_string(sFrom1970()));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_update_server_time_last_connect;
        stringreplaceOne(queryText,"%1",std::to_string(sFrom1970()));
        stringreplaceOne(queryText,"%2","0");
        stringreplaceOne(queryText,"%3",std::to_string(account_id));
        dbQueryWriteCommon(queryText);
    }
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
    if(PreparedDBQueryCommon::db_query_monster_by_character_id.empty())
    {
        errorOutput("deleteCharacterNow() Query is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_buff.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_monster_buff is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_skill.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_monster_skill is empty, bug");
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_bot_already_beaten.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_bot_already_beaten is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_character.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_character is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_all_item.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_item is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_monster_by_character.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_monster is empty, bug");
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_plant.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_plant is empty, bug");
        return;
    }
    if(PreparedDBQueryServer::db_query_delete_quest.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_quest is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_recipes is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_recipes.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_recipes is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_delete_reputation.empty())
    {
        errorOutput("deleteCharacterNow() Query db_query_delete_reputation is empty, bug");
        return;
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=new DeleteCharacterNow;
    deleteCharacterNow->characterId=characterId;

    std::string queryText=PreparedDBQueryCommon::db_query_monster_by_character_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::deleteCharacterNow_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: "+queryText+", error: "+GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        delete deleteCharacterNow;
        return;
    }
    else
    {
        paramToPassToCallBack.push(deleteCharacterNow);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.push("DeleteCharacterNow");
        #endif
        callbackRegistred.push(callback);
    }
}

void Client::deleteCharacterNow_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->deleteCharacterNow_object();
    GlobalServerData::serverPrivateVariables.db_common->clear();
}

void Client::deleteCharacterNow_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.empty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    DeleteCharacterNow *deleteCharacterNow=static_cast<DeleteCharacterNow *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(deleteCharacterNow==NULL)
        abort();
    #endif
    deleteCharacterNow_return(deleteCharacterNow->characterId);
    delete deleteCharacterNow;
}

void Client::deleteCharacterNow_return(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="DeleteCharacterNow")
    {
        std::cerr << "is not DeleteCharacterNow" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    paramToPassToCallBackType.pop();
    #endif
    callbackRegistred.pop();
    bool ok;
    std::string queryText;
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const uint32_t &monsterId=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
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

    queryText=PreparedDBQueryServer::db_query_delete_plant;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteServer(queryText);
    queryText=PreparedDBQueryServer::db_query_delete_quest;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteServer(queryText);
    queryText=PreparedDBQueryServer::db_query_delete_all_item_market;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteServer(queryText);
    queryText=PreparedDBQueryServer::db_query_delete_bot_already_beaten;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    dbQueryWriteServer(queryText);
}

void Client::addCharacter(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_character_by_pseudo.empty())
    {
        errorOutput("addCharacter() Query is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster.empty())
    {
        errorOutput("addCharacter() Query db_query_insert_monster is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster_skill.empty())
    {
        errorOutput("addCharacter() Query db_query_insert_monster_skill is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_reputation.empty())
    {
        errorOutput("addCharacter() Query db_query_insert_reputation is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_item.empty())
    {
        errorOutput("addCharacter() Query db_query_insert_item is empty, bug");
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
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size

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
        errorOutput("profile common and server don't match");
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

    std::string queryText=PreparedDBQueryCommon::db_query_select_character_by_pseudo;
    stringreplaceOne(queryText,"%1",SqlFunction::quoteSqlVariable(pseudo));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::addCharacter_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size

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
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
        posOutput+=4;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        return;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    const std::vector<Profile::Monster> &monsters=profile.monstergroup.at(monsterGroupId);
    //const ServerProfile &serverProfile=CommonDatapack::commonDatapack.serverProfileList.at(profileIndex);
    const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    std::vector<unsigned int> monsterIdList;
    {
        bool ok;
        unsigned int index=0;
        while(index<monsters.size())
        {
            monsterIdList.push_back(getMonsterId(&ok));
            if(!ok)
            {
                std::cerr << "getMonsterId(&ok) have failed, no more id to get?" << __FILE__ << __LINE__ << std::endl;

                //send the network reply
                removeFromQueryReceived(query_id);
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+4);//set the dynamic size

                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
                posOutput+=1;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0x00000000;
                posOutput+=4;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                return;
            }
            index++;
        }
    }

    const uint32_t &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    unsigned int index=0;
    unsigned int monster_position=1;
    dbQueryWriteCommon(serverProfileInternal.preparedQueryAddCharacter.at(0)+
                 std::to_string(characterId)+
                 serverProfileInternal.preparedQueryAddCharacter.at(1)+
                 std::to_string(account_id)+
                 serverProfileInternal.preparedQueryAddCharacter.at(2)+
                 pseudo+
                 serverProfileInternal.preparedQueryAddCharacter.at(3)+
                 std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId))+
                 serverProfileInternal.preparedQueryAddCharacter.at(4)+
                 std::to_string(sFrom1970())+
                 serverProfileInternal.preparedQueryAddCharacter.at(5)
                 );
    while(index<monsters.size())
    {
        const Profile::Monster &profil_local_monster=monsters.at(index);
        const uint32_t &monsterId=profil_local_monster.id;
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monsterId)!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
        {
            const Monster &monster=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsterId);
            uint32_t gender=Gender_Unknown;
            if(monster.ratio_gender!=-1)
            {
                if(rand()%101<monster.ratio_gender)
                    gender=Gender_Female;
                else
                    gender=Gender_Male;
            }
            CatchChallenger::Monster::Stat stat=CatchChallenger::CommonFightEngine::getStat(monster,profil_local_monster.level);
            const std::vector<CatchChallenger::PlayerMonster::PlayerSkill> &skills=CommonFightEngine::generateWildSkill(monster,profil_local_monster.level);

            const uint32_t &monster_id=monsterIdList.back();
            monsterIdList.pop_back();
            {
                std::string queryText=PreparedDBQueryCommon::db_query_insert_monster;
                stringreplaceOne(queryText,"%1",std::to_string(monster_id));
                stringreplaceOne(queryText,"%2",std::to_string(stat.hp));
                stringreplaceAll(queryText,"%3",std::to_string(characterId));
                stringreplaceOne(queryText,"%4",std::to_string(monsterId));
                stringreplaceOne(queryText,"%5",std::to_string(profil_local_monster.level));
                stringreplaceOne(queryText,"%6",std::to_string(profil_local_monster.captured_with));
                stringreplaceOne(queryText,"%7",std::to_string(gender));
                stringreplaceOne(queryText,"%8",std::to_string(monster_position));
                dbQueryWriteCommon(queryText);
                monster_position++;
            }
            unsigned int sub_index=0;
            while(sub_index<skills.size())
            {
                uint8_t endurance=0;
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(skills.at(sub_index).skill)!=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                    if(skills.at(sub_index).level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(skills.at(sub_index).skill).level.size() && skills.at(sub_index).level>0)
                        endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(skills.at(sub_index).skill).level.at(skills.at(sub_index).level-1).endurance;
                std::string queryText=PreparedDBQueryCommon::db_query_insert_monster_skill;
                stringreplaceOne(queryText,"%1",std::to_string(monster_id));
                stringreplaceOne(queryText,"%2",std::to_string(skills.at(sub_index).skill));
                stringreplaceOne(queryText,"%3",std::to_string(skills.at(sub_index).level));
                stringreplaceOne(queryText,"%4",std::to_string(endurance));
                dbQueryWriteCommon(queryText);
                sub_index++;
            }
            index++;
        }
        else
        {
            errorOutput("monster not found to start: "+std::to_string(monsterId)+" is not into profile forced skin list: "+std::to_string(profileIndex));
            return;
        }
    }
    index=0;
    while(index<profile.reputation.size())
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_reputation;
        stringreplaceOne(queryText,"%1",std::to_string(characterId));
        stringreplaceOne(queryText,"%2",std::to_string(CommonDatapack::commonDatapack.reputation.at(profile.reputation.at(index).reputationId).reverse_database_id));
        stringreplaceOne(queryText,"%3",std::to_string(profile.reputation.at(index).point));
        stringreplaceOne(queryText,"%4",std::to_string(profile.reputation.at(index).level));
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
    removeFromQueryReceived(query_id);
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

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void Client::removeCharacterLater(const uint8_t &query_id, const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id.empty())
    {
        errorOutput("removeCharacter() Query is empty, bug");
        return;
    }
    if(PreparedDBQueryCommon::db_query_update_character_time_to_delete_by_id.empty())
    {
        errorOutput("removeCharacter() Query db_query_update_character_time_to_delete_by_id is empty, bug");
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    std::string queryText=PreparedDBQueryCommon::db_query_account_time_to_delete_character_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(characterId));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::removeCharacterLater_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;

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

//load linked data (like item, quests, ...)
void Client::loadLinkedData()
{
    loadPlayerAllow();
}

std::unordered_map<std::string,Client::DatapackCacheFile> Client::datapack_file_list(const std::string &path, const std::string &exclude, const bool withHash)
{
    std::unordered_map<std::string,DatapackCacheFile> filesList;

    std::vector<std::string> returnList;
    if(exclude.empty())
        returnList=FacilityLibGeneral::listFolder(path);
    else
        returnList=FacilityLibGeneral::listFolderWithExclude(path,exclude);
    int index=0;
    const int &size=returnList.size();
    while(index<size)
    {
        #ifdef _WIN32
        std::string fileName=returnList.at(index);
        #else
        const std::string &fileName=returnList.at(index);
        #endif
        if(regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
            if(!suffix.empty() &&
                    BaseServerMasterSendDatapack::extensionAllowed.find(suffix)
                    !=BaseServerMasterSendDatapack::extensionAllowed.cend())
            {
                DatapackCacheFile datapackCacheFile;
                if(withHash)
                {
                    struct stat buf;
                    if(::stat((path+returnList.at(index)).c_str(),&buf)==0)
                    {
                        if(buf.st_size<=CATCHCHALLENGER_MAX_FILE_SIZE)
                        {
                            FILE *filedesc = fopen((path+returnList.at(index)).c_str(), "rb");
                            if(filedesc!=NULL)
                            {
                                #ifdef _WIN32
                                stringreplaceAll(fileName,Client::text_antislash,Client::text_slash);//remplace if is under windows server
                                #endif
                                const std::vector<char> &data=FacilityLibGeneral::readAllFileAndClose(filedesc);
                                SHA224(reinterpret_cast<const unsigned char *>(data.data()),data.size(),reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));
                                datapackCacheFile.partialHash=*reinterpret_cast<const uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput);
                            }
                            else
                            {
                                datapackCacheFile.partialHash=0;
                                std::cerr << "Client::datapack_file_list fopen failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                            }
                        }
                        else
                        {
                            datapackCacheFile.partialHash=0;
                            std::cerr << "Client::datapack_file_list file too big failed on " +path+returnList.at(index)+ ":"+std::to_string(buf.st_size) << std::endl;
                        }
                    }
                    else
                    {
                        datapackCacheFile.partialHash=0;
                        std::cerr << "Client::datapack_file_list stat failed on " +path+returnList.at(index)+ ":"+std::to_string(errno) << std::endl;
                    }
                }
                else
                    datapackCacheFile.partialHash=0;
                filesList[fileName]=datapackCacheFile;
            }
        }
        index++;
    }
    return filesList;
}

#ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
std::unordered_map<std::string, Client::DatapackCacheFile> Client::datapack_file_list_cached_base()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_base==0)
        {
            Client::datapack_list_cache_timestamp_base=sFrom1970();
            Client::datapack_file_hash_cache_base=datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
        }
        return Client::datapack_file_hash_cache_base;
    }
    else
    {
        const uint64_t &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_base<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_base=currentTime;
            Client::datapack_file_hash_cache_base=datapack_file_list(GlobalServerData::serverSettings.datapack_basePath,"map/main/");
        }
        return Client::datapack_file_hash_cache_base;
    }
}

std::unordered_map<std::string, Client::DatapackCacheFile> Client::datapack_file_list_cached_main()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_main==0)
        {
            Client::datapack_list_cache_timestamp_main=sFrom1970();
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
        }
        return Client::datapack_file_hash_cache_main;
    }
    else
    {
        const auto &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_main<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_main=currentTime;
            Client::datapack_file_hash_cache_main=datapack_file_list(GlobalServerData::serverPrivateVariables.mainDatapackFolder,"sub/");
        }
        return Client::datapack_file_hash_cache_main;
    }
}

std::unordered_map<std::string, Client::DatapackCacheFile> Client::datapack_file_list_cached_sub()
{
    if(GlobalServerData::serverSettings.datapackCache==-1)
        return datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
    else if(GlobalServerData::serverSettings.datapackCache==0)
    {
        if(Client::datapack_list_cache_timestamp_sub==0)
        {
            Client::datapack_list_cache_timestamp_sub=sFrom1970();
            Client::datapack_file_hash_cache_sub=datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
        }
        return Client::datapack_file_hash_cache_sub;
    }
    else
    {
        const auto &currentTime=sFrom1970();
        if(Client::datapack_list_cache_timestamp_sub<(currentTime-GlobalServerData::serverSettings.datapackCache))
        {
            Client::datapack_list_cache_timestamp_sub=currentTime;
            Client::datapack_file_hash_cache_sub=datapack_file_list(GlobalServerData::serverPrivateVariables.subDatapackFolder,"");
        }
        return Client::datapack_file_hash_cache_sub;
    }
}

//check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
void Client::datapackList(const uint8_t &query_id,const std::vector<std::string> &files,const std::vector<uint32_t> &partialHashList)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    /// \see Client::parseFullQuery() already checked here
    #endif
    tempDatapackListReplyArray.clear();
    tempDatapackListReplyTestCount=0;
    BaseServerMasterSendDatapack::rawFilesBuffer.clear();
    BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
    BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
    tempDatapackListReply=0;
    tempDatapackListReplySize=0;
    std::string datapackPath;
    std::unordered_map<std::string,DatapackCacheFile> filesList;
    switch(datapackStatus)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case DatapackStatus::Base:
            filesList=datapack_file_list_cached_base();
            datapackPath=GlobalServerData::serverSettings.datapack_basePath;
        break;
        #endif
        case DatapackStatus::Main:
            filesList=datapack_file_list_cached_main();
            datapackPath=GlobalServerData::serverPrivateVariables.mainDatapackFolder;
        break;
        case DatapackStatus::Sub:
            filesList=datapack_file_list_cached_sub();
            datapackPath=GlobalServerData::serverPrivateVariables.subDatapackFolder;
        break;
        default:
        return;
    }
    std::vector<FileToSend> fileToSendList;

    uint32_t fileToDelete=0;
    const int &loop_size=files.size();
    //send the size to download on the client
    {
        //clone to drop the ask file and remain the missing client files
        std::unordered_map<std::string,DatapackCacheFile> filesListForSize(filesList);
        int index=0;
        uint32_t datapckFileNumber=0;
        uint32_t datapckFileSize=0;
        while(index<loop_size)
        {
            const std::string &fileName=files.at(index);
            const uint32_t &clientPartialHash=partialHashList.at(index);
            if(fileName.find(Client::text_dotslash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (1): "+fileName);
                return;
            }
            if(fileName.find(Client::text_antislash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (2): "+fileName);
                return;
            }
            if(fileName.find(Client::text_double_slash) != std::string::npos)
            {
                errorOutput("file name contains illegale char (3): "+fileName);
                return;
            }
            if(regex_search(fileName,fileNameStartStringRegex) || stringStartWith(fileName,Client::text_slash))
            {
                errorOutput("start with wrong string: "+fileName);
                return;
            }
            if(!regex_search(fileName,GlobalServerData::serverPrivateVariables.datapack_rightFileName))
            {
                errorOutput("file name sended wrong: "+fileName);
                return;
            }
            if(filesListForSize.find(fileName)!=filesListForSize.cend())
            {
                const uint32_t &serverPartialHash=filesListForSize.at(fileName).partialHash;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(serverPartialHash==0)
                {
                    errorOutput("serverPartialHash==0 at " __FILE__ ":"+std::to_string(__LINE__));
                    abort();
                }
                #endif
                if(clientPartialHash==serverPartialHash)
                    addDatapackListReply(false);//file found don't need be updated
                else
                {
                    //todo: be sure at the startup sll the file is readable
                    struct stat buf;
                    if(::stat((datapackPath+fileName).c_str(),&buf)!=-1)
                    {
                        addDatapackListReply(false);//found but need an update
                        datapckFileNumber++;
                        datapckFileSize+=buf.st_size;
                        FileToSend fileToSend;
                        fileToSend.file=fileName;
                        fileToSendList.push_back(fileToSend);
                    }
                }
                filesListForSize.erase(fileName);
            }
            else
            {
                addDatapackListReply(true);//to delete
                fileToDelete++;
            }
            index++;
        }
        auto i=filesListForSize.begin();
        while(i!=filesListForSize.cend())
        {
            FILE *filedesc = fopen((datapackPath+i->first).c_str(), "rb");
            if(filedesc!=NULL)
            {
                struct stat buf;
                if(::stat((datapackPath+i->first).c_str(),&buf)!=-1)
                {
                    datapckFileNumber++;
                    datapckFileSize+=buf.st_size;
                    FileToSend fileToSend;
                    fileToSend.file=i->first;
                    fileToSendList.push_back(fileToSend);
                }
                fclose(filedesc);
            }
            ++i;
        }
        {
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x75;
            posOutput+=1;

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileNumber);
            posOutput+=4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(datapckFileSize);
            posOutput+=4;

            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
    }
    if(fileToSendList.empty() && fileToDelete==0)
    {
        switch(datapackStatus)
        {
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            case DatapackStatus::Base:
                errorOutput("Ask datapack list where the checksum match Base");
            break;
            #endif
            case DatapackStatus::Main:
                errorOutput("Ask datapack list where the checksum match Main");
            break;
            case DatapackStatus::Sub:
                errorOutput("Ask datapack list where the checksum match Sub");
            break;
            default:
            return;
        }
        return;
    }
    std::sort(fileToSendList.begin(),fileToSendList.end());
    //validate, remove or update the file actualy on the client
    if(tempDatapackListReplyTestCount!=files.size())
    {
        errorOutput("Bit count return not match");
        return;
    }
    //send not in the list
    {
        unsigned int index=0;
        while(index<fileToSendList.size())
        {
            if(!sendFile(datapackPath,fileToSendList.at(index).file))
                return;
            index++;
        }
    }
    sendFileContent();
    sendCompressedFileContent();
    purgeDatapackListReply(query_id);

    switch(datapackStatus)
    {
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        case DatapackStatus::Base:
            datapackStatus=DatapackStatus::Main;
        break;
        #endif
        case DatapackStatus::Main:
            datapackStatus=DatapackStatus::Sub;
        break;
        case DatapackStatus::Sub:
            datapackStatus=DatapackStatus::Finished;
        break;
        default:
        return;
    }
}

void Client::addDatapackListReply(const bool &fileRemove)
{
    tempDatapackListReplyTestCount++;
    switch(tempDatapackListReplySize)
    {
        case 0:
            if(fileRemove)
                tempDatapackListReply|=0x01;
            else
                tempDatapackListReply&=~0x01;
        break;
        case 1:
            if(fileRemove)
                tempDatapackListReply|=0x02;
            else
                tempDatapackListReply&=~0x02;
        break;
        case 2:
            if(fileRemove)
                tempDatapackListReply|=0x04;
            else
                tempDatapackListReply&=~0x04;
        break;
        case 3:
            if(fileRemove)
                tempDatapackListReply|=0x08;
            else
                tempDatapackListReply&=~0x08;
        break;
        case 4:
            if(fileRemove)
                tempDatapackListReply|=0x10;
            else
                tempDatapackListReply&=~0x10;
        break;
        case 5:
            if(fileRemove)
                tempDatapackListReply|=0x20;
            else
                tempDatapackListReply&=~0x20;
        break;
        case 6:
            if(fileRemove)
                tempDatapackListReply|=0x40;
            else
                tempDatapackListReply&=~0x40;
        break;
        case 7:
            if(fileRemove)
                tempDatapackListReply|=0x80;
            else
                tempDatapackListReply&=~0x80;
        break;
        default:
        break;
    }
    tempDatapackListReplySize++;
    if(tempDatapackListReplySize>=8)
    {
        tempDatapackListReplyArray.push_back(tempDatapackListReply);
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
}

void Client::purgeDatapackListReply(const uint8_t &query_id)
{
    if(tempDatapackListReplySize>0)
    {
        tempDatapackListReplyArray.push_back(tempDatapackListReply);
        tempDatapackListReplySize=0;
        tempDatapackListReply=0;
    }
    if(tempDatapackListReplyArray.empty())
        tempDatapackListReplyArray.push_back(0x00);

    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(tempDatapackListReplyArray.size());//set the dynamic size

        if(tempDatapackListReplyArray.size()>64*1024)
        {
            errorOutput("Client::purgeDatapackListReply too big to reply");
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,tempDatapackListReplyArray.data(),tempDatapackListReplyArray.size());
        posOutput+=tempDatapackListReplyArray.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    tempDatapackListReplyArray.clear();
}

void Client::sendFileContent()
{
    if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>0 && BaseServerMasterSendDatapack::rawFilesBufferCount>0)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x76;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+BaseServerMasterSendDatapack::rawFilesBuffer.size());//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=BaseServerMasterSendDatapack::rawFilesBufferCount;
        posOutput+=1;
        if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            errorOutput("Client::sendFileContent too big to reply");
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,BaseServerMasterSendDatapack::rawFilesBuffer.data(),BaseServerMasterSendDatapack::rawFilesBuffer.size());
        posOutput+=BaseServerMasterSendDatapack::rawFilesBuffer.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        BaseServerMasterSendDatapack::rawFilesBuffer.clear();
        BaseServerMasterSendDatapack::rawFilesBufferCount=0;
    }
}

void Client::sendCompressedFileContent()
{
    if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>0 && BaseServerMasterSendDatapack::compressedFilesBufferCount>0)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x77;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+BaseServerMasterSendDatapack::compressedFilesBuffer.size());//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=BaseServerMasterSendDatapack::compressedFilesBufferCount;
        posOutput+=1;
        if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_MAX_PACKET_SIZE)
        {
            errorOutput("Client::sendFileContent too big to reply");
            return;
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,BaseServerMasterSendDatapack::compressedFilesBuffer.data(),BaseServerMasterSendDatapack::compressedFilesBuffer.size());
        posOutput+=BaseServerMasterSendDatapack::compressedFilesBuffer.size();

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        BaseServerMasterSendDatapack::compressedFilesBuffer.clear();
        BaseServerMasterSendDatapack::compressedFilesBufferCount=0;
    }
}

bool Client::sendFile(const std::string &datapackPath,const std::string &fileName)
{
    if(fileName.size()>255 || fileName.empty())
    {
        errorOutput("Unable to open into CatchChallenger::sendFile(): fileName.size()>255 || fileName.empty()");
        return false;
    }

    FILE *filedesc = fopen((datapackPath+fileName).c_str(), "rb");
    if(filedesc!=NULL)
    {
        const std::vector<char> &content=FacilityLibGeneral::readAllFileAndClose(filedesc);
        const int &contentsize=content.size();

        const std::string &suffix=FacilityLibGeneral::getSuffix(fileName);
        if(ProtocolParsing::compressionTypeServer!=ProtocolParsing::CompressionType::None &&
                BaseServerMasterSendDatapack::compressedExtension.find(suffix)!=BaseServerMasterSendDatapack::compressedExtension.cend() &&
                (
                    contentsize<CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
                    ||
                    contentsize>CATCHCHALLENGER_MAX_PACKET_SIZE
                )
            )
        {
            uint32_t posOutput=0;
            {
                const std::string &text=fileName;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                posOutput+=1;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
            posOutput+=4;

            binaryAppend(BaseServerMasterSendDatapack::compressedFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            binaryAppend(BaseServerMasterSendDatapack::compressedFilesBuffer,content);
            BaseServerMasterSendDatapack::compressedFilesBufferCount++;
            switch(ProtocolParsing::compressionTypeServer)
            {
                case ProtocolParsing::CompressionType::Xz:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
                default:
                case ProtocolParsing::CompressionType::Zlib:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
                case ProtocolParsing::CompressionType::Lz4:
                if(BaseServerMasterSendDatapack::compressedFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_LZ4_COMPRESSEDFILEPURGE_KB*1024 || BaseServerMasterSendDatapack::compressedFilesBufferCount>=255)
                    sendCompressedFileContent();
                break;
            }
        }
        else
        {
            if(contentsize>CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024)
            {
                if((1+fileName.size()+4+contentsize)>=CATCHCHALLENGER_MAX_PACKET_SIZE)
                {
                    normalOutput("Error: outputData2(1)+fileNameRaw("+
                                 std::to_string(fileName.size()+4)+
                                 ")+content("+
                                 std::to_string(contentsize)+
                                 ")>CATCHCHALLENGER_MAX_PACKET_SIZE for file "+
                                 fileName
                                 );
                    return false;
                }

                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x76;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(1+BaseServerMasterSendDatapack::rawFilesBuffer.size());//set the dynamic size

                //number of file
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=1;
                posOutput+=1;
                //filename
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                //file size
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,content.data(),contentsize);
                posOutput+=contentsize;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
            else
            {
                uint32_t posOutput=0;
                {
                    const std::string &text=fileName;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(contentsize);
                posOutput+=4;

                binaryAppend(BaseServerMasterSendDatapack::rawFilesBuffer,ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                binaryAppend(BaseServerMasterSendDatapack::rawFilesBuffer,content);
                BaseServerMasterSendDatapack::rawFilesBufferCount++;
                if(BaseServerMasterSendDatapack::rawFilesBuffer.size()>CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024 || BaseServerMasterSendDatapack::rawFilesBufferCount>=255)
                    sendFileContent();
            }
        }
        return true;
    }
    else
    {
        errorOutput("Unable to open into CatchChallenger::sendFile(): "+fileName);
        return false;
    }
}
#endif

bool CatchChallenger::operator<(const CatchChallenger::FileToSend &fileToSend1,const CatchChallenger::FileToSend &fileToSend2)
{
    if(fileToSend1.file<fileToSend2.file)
        return false;
    return true;
}

bool CatchChallenger::operator!=(const CatchChallenger::PlayerMonster &monster1,const CatchChallenger::PlayerMonster &monster2)
{
    if(monster1.remaining_xp!=monster2.remaining_xp)
        return true;
    if(monster1.sp!=monster2.sp)
        return true;
    if(monster1.egg_step!=monster2.egg_step)
        return true;
    if(monster1.id!=monster2.id)
        return true;
/* transfer in 0.6 and check it
    if(monster1.character_origin!=monster2.character_origin)
        return true;*/
    if(monster1.skills.size()!=monster2.skills.size())
        return true;
    unsigned int index=0;
    while(index<monster1.skills.size())
    {
        const PlayerMonster::PlayerSkill &skill1=monster1.skills.at(index);
        const PlayerMonster::PlayerSkill &skill2=monster2.skills.at(index);
        if(skill1.endurance!=skill2.endurance)
            return true;
        if(skill1.level!=skill2.level)
            return true;
        if(skill1.skill!=skill2.skill)
            return true;
        index++;
    }
    return false;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::dbQueryWriteLogin(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteLogin() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteLogin() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_login write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_login->asyncWrite(queryText);
}
#endif

void Client::dbQueryWriteCommon(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteCommon() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteCommon() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_common write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_common->asyncWrite(queryText);
}

void Client::dbQueryWriteServer(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorOutput("dbQueryWriteServer() Query is empty, bug");
        return;
    }
    if(stringStartWith(queryText,"SELECT"))
    {
            errorOutput("dbQueryWriteServer() Query is SELECT but here is the write queue, bug");
            return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    normalOutput("Do db_server write: "+queryText);
    #endif
    GlobalServerData::serverPrivateVariables.db_server->asyncWrite(queryText);
}

void Client::loadReputation()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommon::db_query_select_reputation_by_id.empty())
    {
        errorOutput("loadReputation() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryCommon::db_query_select_reputation_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::loadReputation_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        loadQuests();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadReputation_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadReputation_return();
}

void Client::loadReputation_return()
{
    callbackRegistred.pop();
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_common->next())
    {
        const unsigned int &type=GlobalServerData::serverPrivateVariables.db_common->stringtouint32(GlobalServerData::serverPrivateVariables.db_common->value(0),&ok);
        if(!ok)
        {
            normalOutput("reputation type is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(0));
            continue;
        }
        int32_t point=GlobalServerData::serverPrivateVariables.db_common->stringtoint32(GlobalServerData::serverPrivateVariables.db_common->value(1),&ok);
        if(!ok)
        {
            normalOutput("reputation point is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(1));
            continue;
        }
        const int32_t &level=GlobalServerData::serverPrivateVariables.db_common->stringtoint32(GlobalServerData::serverPrivateVariables.db_common->value(2),&ok);
        if(!ok)
        {
            normalOutput("reputation level is not a number, skip: "+GlobalServerData::serverPrivateVariables.db_common->value(2));
            continue;
        }
        if(level<-100 || level>100)
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
        if(level>=0)
        {
            if((uint32_t)level>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())
            {
                normalOutput("The reputation level "+std::to_string(type)+
                             " is wrong because is out of range (reputation level: "+std::to_string(level)+
                             " > max level: "+
                             std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size())+
                             ")");
                continue;
            }
        }
        else
        {
            if((uint32_t)(-level)>CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())
            {
                normalOutput("The reputation level "+std::to_string(type)+
                             " is wrong because is out of range (reputation level: "+std::to_string(level)+
                             " < max level: "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size())+")");
                continue;
            }
        }
        if(point>0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.size()==(uint32_t)(level+1))//start at level 0 in positive
            {
                normalOutput("The reputation level is already at max, drop point");
                point=0;
            }
            if(point>=CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(level+1))//start at level 0 in positive
            {
                normalOutput("The reputation point "+std::to_string(point)+
                             " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_positive.at(level)));
                continue;
            }
        }
        else if(point<0)
        {
            if(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.size()==(uint32_t)-level)//start at level -1 in negative
            {
                normalOutput("The reputation level is already at min, drop point");
                point=0;
            }
            if(point<CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(-level))//start at level -1 in negative
            {
                normalOutput("The reputation point "+std::to_string(point)+
                             " is greater than max "+std::to_string(CommonDatapack::commonDatapack.reputation.at(DictionaryLogin::dictionary_reputation_database_to_internal.at(type)).reputation_negative.at(level)));
                continue;
            }
        }
        PlayerReputation playerReputation;
        playerReputation.level=level;
        playerReputation.point=point;
        public_and_private_informations.reputation[DictionaryLogin::dictionary_reputation_database_to_internal.at(type)]=playerReputation;
    }
    loadQuests();
}

void Client::loadQuests()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryServer::db_query_select_quest_by_id.empty())
    {
        errorOutput("loadQuests() Query is empty, bug");
        return;
    }
    #endif
    std::string queryText=PreparedDBQueryServer::db_query_select_quest_by_id;
    stringreplaceOne(queryText,"%1",std::to_string(character_id));
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_server->asyncRead(queryText,this,&Client::loadQuests_static);
    if(callback==NULL)
    {
        std::cerr << "Sql error for: " << queryText << ", error: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        loadBotAlreadyBeaten();
        return;
    }
    else
        callbackRegistred.push(callback);
}

void Client::loadQuests_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->loadQuests_return();
}

void Client::loadQuests_return()
{
    callbackRegistred.pop();
    //do the query
    bool ok,ok2;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db_server->next())
    {
        PlayerQuest playerQuest;
        const uint32_t &id=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(0),&ok);
        playerQuest.finish_one_time=GlobalServerData::serverPrivateVariables.db_server->stringtobool(GlobalServerData::serverPrivateVariables.db_server->value(1));
        playerQuest.step=GlobalServerData::serverPrivateVariables.db_server->stringtouint32(GlobalServerData::serverPrivateVariables.db_server->value(2),&ok2);
        if(!ok || !ok2)
        {
            normalOutput("wrong value type for quest, skip: "+std::to_string(id));
            continue;
        }
        if(CommonDatapackServerSpec::commonDatapackServerSpec.quests.find(id)==CommonDatapackServerSpec::commonDatapackServerSpec.quests.cend())
        {
            normalOutput("quest is not into the quests list, skip: "+std::to_string(id));
            continue;
        }
        if((playerQuest.step<=0 && !playerQuest.finish_one_time) || playerQuest.step>CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(id).steps.size())
        {
            normalOutput("step out of quest range, skip: "+std::to_string(id));
            continue;
        }
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            normalOutput("can't be to step 0 if have never finish the quest, skip: "+std::to_string(id));
            continue;
        }
        public_and_private_informations.quests[id]=playerQuest;
        if(playerQuest.step>0)
            addQuestStepDrop(id,playerQuest.step);
    }
    loadBotAlreadyBeaten();
}
