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
    #endif
    AskLoginParam *askLoginParam=new AskLoginParam;
    SHA224(reinterpret_cast<const unsigned char *>(rawdata),CATCHCHALLENGER_SHA224HASH_SIZE,reinterpret_cast<unsigned char *>(askLoginParam->login));
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);

    const std::string &queryText=PreparedDBQueryLogin::db_query_login.compose(
                binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)
                );
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
            const uint8_t &blob_version=GlobalServerData::serverPrivateVariables.db_login->stringtouint8(GlobalServerData::serverPrivateVariables.db_login->value(2),&ok);
            if(!ok)
            {
                loginIsWrong(askLoginParam->query_id,0x04,"Blob version not a number");
                delete askLoginParam;
                return;
            }
            if(blob_version!=CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION)
            {
                loginIsWrong(askLoginParam->query_id,0x04,"Blob version incorrect");
                delete askLoginParam;
                return;
            }
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
    std::string queryText=PreparedDBQueryCommonForLogin::db_query_characters.compose(
                std::to_string(account_id)
                );
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

    const std::string &queryText=PreparedDBQueryLogin::db_query_login.compose(
                binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)
                );
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
        const std::string &queryText=PreparedDBQueryLogin::db_query_insert_login.compose(
                    std::to_string(account_id),
                    binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE),
                    binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE),
                    std::to_string(sFrom1970()),
                    std::to_string(CATCHCHALLENGER_SERVER_DATABASE_COMMON_BLOBVERSION)
                    );
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
    const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_select_server_time.compose(
                std::to_string(account_id)
                );
    CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db_common->asyncRead(queryText,this,&Client::server_list_static);
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
        const std::string &queryText=PreparedDBQueryCommon::db_query_insert_server_time.compose(
                    "0",
                    std::to_string(account_id),
                    std::to_string(sFrom1970())
                    );
        dbQueryWriteCommon(queryText);
    }
    else
    {
        const std::string &queryText=PreparedDBQueryCommon::db_query_update_server_time_last_connect.compose(
                    std::to_string(sFrom1970()),
                    "0",
                    std::to_string(account_id)
                    );
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
    if(PreparedDBQueryCommonForLogin::db_query_delete_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_character is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommonForLogin::db_query_delete_monster_by_character.empty())
    {
        std::cerr << "deleteCharacterNow() Query db_query_delete_monster_by_id is empty, bug" << std::endl;
        return;
    }
    #endif

    const std::string &characterIdString=std::to_string(characterId);
    {
        const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_delete_character.compose(
                    characterIdString
                    );
        dbQueryWriteCommon(queryText);
    }
    {
        const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_delete_monster_by_character.compose(
                    characterIdString
                    );
        dbQueryWriteCommon(queryText);
    }
}

/*void Client::deleteCharacterNow(const uint32_t &characterId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommonForLogin::db_query_delete_character.empty())
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
    DeleteCharacterNow *deleteCharacterNow=new DeleteCharacterNow;
    deleteCharacterNow->characterId=characterId;

    const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_characters_with_monsters.compose(
                std::to_string(characterId)
                );
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
}*/

void Client::addCharacter(const uint8_t &query_id, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo.empty())
    {
        std::cerr << "addCharacter() Query is empty, bug" << std::endl;
        return;
    }
    if(PreparedDBQueryCommon::db_query_insert_monster.empty())
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

    const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_select_character_by_pseudo.compose(
                SqlFunction::quoteSqlVariable(pseudo)
                );
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

    number_of_character++;
    GlobalServerData::serverPrivateVariables.maxCharacterId++;

    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    const ServerProfileInternal &serverProfileInternal=GlobalServerData::serverPrivateVariables.serverProfileInternalList.at(profileIndex);

    const uint32_t &characterId=GlobalServerData::serverPrivateVariables.maxCharacterId;
    const std::string &characterIdString=std::to_string(characterId);
    int monster_position=1;

    const auto &monsterGroup=profile.monstergroup.at(monsterGroupId);
    const auto &monsters=serverProfileInternal.monster_insert.at(monsterGroupId);
    const std::string &monster_encyclopedia_insert=serverProfileInternal.monster_encyclopedia_insert.at(monsterGroupId);
    if(!monsters.empty())
    {
        unsigned int index=0;
        while(index<monsters.size())
        {
            const auto &monster=monsterGroup.at(index);
            const Monster &monsterDatapack=CommonDatapack::commonDatapack.monsters.at(monster.id);
            const StringWithReplacement &monsterQuery=monsters.at(index);

            const uint32_t monster_id=GlobalServerData::serverPrivateVariables.maxCharacterId;
            GlobalServerData::serverPrivateVariables.maxCharacterId++;

            //insert the monster is db
            {
                const std::string &monster_id_string=std::to_string(monster_id);
                //id,gender,id
                if(monsterDatapack.ratio_gender!=-1)
                {
                    if(rand()%101<monsterDatapack.ratio_gender)
                        dbQueryWriteCommon(monsterQuery.compose(monster_id_string,characterIdString,Client::text_female,characterIdString));
                    else
                        dbQueryWriteCommon(monsterQuery.compose(monster_id_string,characterIdString,Client::text_male,characterIdString));
                }
                else
                    dbQueryWriteCommon(monsterQuery.compose(monster_id_string,characterIdString,characterIdString));

                monster_position++;
            }
            index++;
        }
    }

    const std::string &local_character_insert=serverProfileInternal.character_insert.compose(
                characterIdString,
                std::to_string(account_id),
                SqlFunction::quoteSqlVariable(pseudo),
                std::to_string(DictionaryLogin::dictionary_skin_internal_to_database.at(skinId)),
                std::to_string(sFrom1970()),
                monster_encyclopedia_insert
                );
    dbQueryWriteCommon(local_character_insert);

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
    if(PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id.empty())
    {
        std::cerr << "removeCharacter() Query is empty, bug" << std::endl;
        return;
    }
    #endif
    RemoveCharacterParam *removeCharacterParam=new RemoveCharacterParam;
    removeCharacterParam->query_id=query_id;
    removeCharacterParam->characterId=characterId;

    const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_account_time_to_delete_character_by_id.compose(
                std::to_string(characterId)
                );
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
    const std::string &queryText=PreparedDBQueryCommonForLogin::db_query_update_character_time_to_delete_by_id.compose(
                std::to_string(characterId),
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
