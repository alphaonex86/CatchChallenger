#include "EpollClientLoginSlave.hpp"
#include "EpollServerLoginSlave.hpp"
#include "CharactersGroupForLogin.hpp"
#include "../base/PreparedDBQuery.hpp"
#include "../base/GlobalServerData.hpp"
#include <iostream>
#include <chrono>
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/sha224/sha224.hpp"
#include "VariableLoginServer.hpp"

using namespace CatchChallenger;

void EpollClientLoginSlave::askLogin(const uint8_t &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorParsingLayer("askLogin() Query login is empty, bug");
        return;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorParsingLayer("askLogin() Query inset login is empty, bug");
        return;
    }
    #endif
    AskLoginParam *askLoginParam=new AskLoginParam;
    SHA224 ctx = SHA224();
    ctx.init();
    ctx.update(reinterpret_cast<const unsigned char *>(rawdata),CATCHCHALLENGER_SHA224HASH_SIZE);
    ctx.final(reinterpret_cast<unsigned char *>(askLoginParam->login));
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);

    DatabaseBaseCallBack *callback=PreparedDBQueryLogin::db_query_login.asyncRead(this,&EpollClientLoginSlave::askLogin_static,{binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)});
    if(callback==NULL)
    {
        loginIsWrong(askLoginParam->query_id,0x04,"Sql error for: "+PreparedDBQueryLogin::db_query_login.queryText()+", error: "+databaseBaseLogin.errorMessage());
        delete askLoginParam;
        askLoginParam=NULL;
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

void EpollClientLoginSlave::askStatClient(const uint8_t &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorParsingLayer("askLogin() Query login is empty, bug");
        return;
    }
    #endif

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
                //append the token
                memcpy(LinkToMaster::private_token_statclient+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                secretTokenBinary.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                memcpy(secretTokenBinary.data(),LinkToMaster::private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                tempAddedToken.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                memcpy(tempAddedToken.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                if(secretTokenBinary.size()!=(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT) askStatClient()" << std::endl;
                    abort();
                }
                #endif
                SHA224 ctx = SHA224();
                ctx.init();
                ctx.update(LinkToMaster::private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                ctx.final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));

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
            removeFromQueryReceived(query_id);//all list dropped at client destruction
            ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
            ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x02;
            internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
            errorParsingLayer("No temp auth token found");
            return;
        }
    }
    if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
    {
        removeFromQueryReceived(query_id);//all list dropped at client destruction
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
        ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x02;
        internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        errorParsingLayer("Password wrong: "+
                     binarytoHexa(secretTokenBinary)+
                     " + token "+
                     binarytoHexa(tempAddedToken)+
                     " = "+
                     " hashedToken: "+
                     binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)+
                     " sended pass + token: "+
                     binarytoHexa(rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)
                     );
        #else
        errorParsingLayer("Password wrong: "+
                     binarytoHexa(rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)
                     );
        #endif
        return;
    }
    else
    {
        removeFromQueryReceived(query_id);//all list dropped at client destruction
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
        ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x01;
        internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::serverLogicalGroupAndServerList),EpollClientLoginSlave::serverLogicalGroupAndServerListSize);

        stat=EpollClientLoginStat::LoggedStatClient;
        //flags|=0x08;->just listen

        unsigned int index=0;
        while(index<client_list.size())
        {
            const EpollClientLoginSlave * const client=client_list.at(index);
            if(this==client)
            {
                client_list.erase(client_list.begin()+index);
                break;
            }
            index++;
        }
        stat_client_list.push_back(this);
    }
}

void EpollClientLoginSlave::askLogin_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->askLogin_object();
    databaseBaseLogin.clear();
    //delete askLoginParam; -> not here because need reuse later
}

void EpollClientLoginSlave::askLogin_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());//but not delete because will reinster at the end, then not take!
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    askLogin_return(askLoginParam);
    //delete askLoginParam; -> not here because need reuse later
}

void EpollClientLoginSlave::askLogin_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    callbackRegistred.pop();
    {
        bool ok;
        if(!databaseBaseLogin.next())
        {
            //create a new account
            if(CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
            {
                //network send
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                //removeFromQueryReceived(askLoginParam->query_id);//all list dropped at client destruction
                #endif
                *(EpollClientLoginSlave::loginIsWrongBufferReply+1)=(uint8_t)askLoginParam->query_id;
                *(EpollClientLoginSlave::loginIsWrongBufferReply+1+1+4)=(uint8_t)0x07;
                removeFromQueryReceived(askLoginParam->query_id);
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));
                stat=EpollClientLoginStat::ProtocolGood;
                paramToPassToCallBack.pop();
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType.pop();
                #endif
                delete askLoginParam;
                askLoginParam=NULL;
                return;
/*                PreparedDBQuery::maxAccountId++;
                account_id=PreparedDBQuery::maxAccountId;
                dbQueryWrite(PreparedDBQuery::db_query_insert_login.arg(account_id).arg(std::string(askLoginParam->login.toHex())).arg(std::string(askLoginParam->pass.toHex())).arg(sFrom1970()));*/
            }
            else
            {
                loginIsWrong(askLoginParam->query_id,0x02,"Bad login for: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+", pass: "+binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE));
                paramToPassToCallBack.pop();
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType.pop();
                #endif
                delete askLoginParam;
                askLoginParam=NULL;
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
                        const std::string &secretToken(databaseBaseLogin.value(1));
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        secretTokenBinary=databaseBaseLogin.hexatoBinary(secretToken);
                        #else
                        std::vector<char> secretTokenBinary=databaseBaseLogin.hexatoBinary(secretToken);
                        #endif
                        if(secretTokenBinary.size()!=CATCHCHALLENGER_SHA224HASH_SIZE)
                        {
                            std::cerr << "convertion to binary for pass failed for: " << databaseBaseLogin.value(1) << std::endl;
                            abort();
                        }

                        //append the token
                        secretTokenBinary.resize(secretTokenBinary.size()+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(secretTokenBinary.data()+CATCHCHALLENGER_SHA224HASH_SIZE,tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        tempAddedToken.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        memcpy(tempAddedToken.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        if(secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                        {
                            std::cerr << "secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)" << std::endl;
                            abort();
                        }
                        #endif
                        SHA224 ctx = SHA224();
                        ctx.init();
                        ctx.update(reinterpret_cast<const unsigned char *>(secretTokenBinary.data()),secretTokenBinary.size());
                        ctx.final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));

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
                             binarytoHexa(secretTokenBinary.data(),secretTokenBinary.size()-tempAddedToken.size())+
                             " + token "+
                             binarytoHexa(tempAddedToken)+
                             " = "+
                             " hashedToken: "+
                             binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " sended pass + token: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #else
                loginIsWrong(askLoginParam->query_id,0x03,"Password wrong: "+
                             binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE)+
                             " for the login: "+
                             binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)
                             );
                #endif
                paramToPassToCallBack.pop();
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType.pop();
                #endif
                delete askLoginParam;
                askLoginParam=NULL;
                return;
            }
            else
            {
                account_id=databaseBaseLogin.stringtouint32(databaseBaseLogin.value(0),&ok);
                if(!ok)
                {
                    account_id=0;
                    loginIsWrong(askLoginParam->query_id,0x03,"Account id is not a number");
                    paramToPassToCallBack.pop();
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    paramToPassToCallBackType.pop();
                    #endif
                    delete askLoginParam;
                    askLoginParam=NULL;
                    return;
                }
                else
                    flags|=0x08;
            }
        }
    }

    {
        serverListForReplyInSuspend+=CharactersGroupForLogin::list.size();
        unsigned int index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin::list.at(index)->character_list(this,account_id);
            index++;
        }
    }
}

void EpollClientLoginSlave::askLogin_cancel()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
    paramToPassToCallBack.pop();
    loginIsWrong(askLoginParam->query_id,0x04,"Canceled by the Charaters group");
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(askLoginParam==NULL)
        abort();
    #endif
    delete askLoginParam;
    askLoginParam=NULL;
}

void EpollClientLoginSlave::character_list_return(const uint8_t &characterGroupIndex,char * const tempRawData,const int &tempRawDataSize)
{
    characterTempListForReply[characterGroupIndex].rawData=tempRawData;
    characterTempListForReply[characterGroupIndex].rawDataSize=tempRawDataSize;
    characterListForReplyInSuspend--;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(
            //if all the query is finished
            (characterListForReplyInSuspend==0 && serverListForReplyInSuspend==0)
            &&
            //and the group character list size to send don't match with real character list size
            (characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
             )
    {
        std::cerr << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
}

void EpollClientLoginSlave::server_list_return(const uint8_t &serverCount,char * const tempRawData,const int &tempRawDataSize)
{
    if(serverCount>0)
    {
        if(serverListForReplyRawData==NULL)
        {
            serverListForReplyRawDataSize=1;
            serverListForReplyRawData=static_cast<char *>(malloc(512*1024));
            serverListForReplyRawData[0x00]=0;
        }
        serverListForReplyRawData[0x00]+=serverCount;
        memcpy(serverListForReplyRawData+serverListForReplyRawDataSize,tempRawData,tempRawDataSize);
        serverListForReplyRawDataSize+=tempRawDataSize;
    }
    serverListForReplyInSuspend--;
    if(serverListForReplyInSuspend==0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(
                //if all the query is finished
                (characterListForReplyInSuspend==0 && serverListForReplyInSuspend==0)
                &&
                //and the group character list size to send don't match with real character list size
                (characterTempListForReply.size()!=CharactersGroupForLogin::list.size())
                 )
        {
            std::cerr << "serverListForReplyInSuspend==0 && characterTempListForReply.size()!=CharactersGroupForLogin::list.size()" << __FILE__ << __LINE__ << std::endl;
            abort();
        }
        #endif

        unsigned int tempSize=EpollClientLoginSlave::loginGoodSize;

        //characters group
        EpollClientLoginSlave::loginGood[tempSize]=characterTempListForReply.size();
        tempSize+=sizeof(uint8_t);

        {
            auto i=characterTempListForReply.begin();
            while(i!=characterTempListForReply.cend())
            {
                //copy buffer
                memcpy(EpollClientLoginSlave::loginGood+tempSize,i->second.rawData,i->second.rawDataSize);
                tempSize+=i->second.rawDataSize;
                //remove the old buffer
                delete[] i->second.rawData;
                ++i;
            }
        }
        characterTempListForReply.clear();
        //Server list
        if(serverListForReplyRawData!=NULL)
        {
            memcpy(EpollClientLoginSlave::loginGood+tempSize,serverListForReplyRawData,serverListForReplyRawDataSize);
            tempSize+=serverListForReplyRawDataSize;
            //delete serverListForReplyRawData;//do into caller: CharactersGroupForLogin::character_list_object()
        }
        else
        {
            EpollClientLoginSlave::loginGood[tempSize]=0;
            tempSize+=sizeof(uint8_t);
        }

        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(paramToPassToCallBackType.front()!="AskLoginParam")
        {
            std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
            abort();
        }
        #endif
        AskLoginParam *askLoginParam=static_cast<AskLoginParam *>(paramToPassToCallBack.front());
        paramToPassToCallBack.pop();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(askLoginParam==NULL)
            abort();
        #endif
        //send C20F and C20E
        internalSendRawSmallPacket(EpollClientLoginSlave::serverLogicalGroupAndServerList,EpollClientLoginSlave::serverLogicalGroupAndServerListSize);
        //send the reply
        removeFromQueryReceived(askLoginParam->query_id);
        EpollClientLoginSlave::loginGood[0x01]=askLoginParam->query_id;
        *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::loginGood+1+1)=htole32(tempSize-1-1-4);//set the dynamic size
        internalSendRawSmallPacket(EpollClientLoginSlave::loginGood,tempSize);
        //paramToPassToCallBack.pop();double call, look above
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        paramToPassToCallBackType.pop();
        #endif
        delete askLoginParam;
        askLoginParam=NULL;
        stat=EpollClientLoginStat::Logged;
    }
}

void EpollClientLoginSlave::createAccount(const uint8_t &query_id, const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorParsingLayer("createAccount() Query login is empty, bug");
        return;
    }
    if(PreparedDBQueryLogin::db_query_insert_login.empty())
    {
        errorParsingLayer("createAccount() Query inset login is empty, bug");
        return;
    }
    if(!CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
    {
        errorParsingLayer("createAccount() Creation account not premited");
        return;
    }
    #endif
    if(maxAccountIdList.empty())
    {
        loginIsWrong(query_id,0x04,"maxAccountIdList is empty");
        return;
    }
    AskLoginParam *askLoginParam=new AskLoginParam;
    memcpy(askLoginParam->login,rawdata,CATCHCHALLENGER_SHA224HASH_SIZE);
    askLoginParam->query_id=query_id;
    memcpy(askLoginParam->pass,rawdata+CATCHCHALLENGER_SHA224HASH_SIZE,CATCHCHALLENGER_SHA224HASH_SIZE);
    askLoginParam->query_id=query_id;

    DatabaseBaseCallBack *callback=PreparedDBQueryLogin::db_query_login.asyncRead(this,&EpollClientLoginSlave::createAccount_static,{binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE)});
    if(callback==NULL)
    {
        stat=EpollClientLoginStat::ProtocolGood;
        loginIsWrong(askLoginParam->query_id,0x03,"Sql error for: "+PreparedDBQueryLogin::db_query_login.queryText()+", error: "+databaseBaseLogin.errorMessage());
        delete askLoginParam;
        askLoginParam=NULL;
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

void EpollClientLoginSlave::createAccount_static(void *object)
{
    if(object!=NULL)
        static_cast<EpollClientLoginSlave *>(object)->createAccount_object();
    databaseBaseLogin.clear();
}

void EpollClientLoginSlave::createAccount_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.empty())
    {
        std::cerr << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__ << std::endl;
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
    askLoginParam=NULL;
}

void EpollClientLoginSlave::createAccount_return(AskLoginParam *askLoginParam)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.front()!="AskLoginParam")
    {
        std::cerr << "is not AskLoginParam" << stringimplode(paramToPassToCallBackType,';') << __FILE__ << __LINE__ << std::endl;
        abort();
    }
    #endif
    callbackRegistred.pop();
    if(!databaseBaseLogin.next())
    {
        //network send
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //removeFromQueryReceived(askLoginParam->query_id);//->only if use fast path
        #endif
        if(maxAccountIdList.empty())
        {
            loginIsWrong(askLoginParam->query_id,0x04,"maxAccountIdList is empty");
            return;
        }
        if(maxAccountIdList.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK)
        {
            if(!EpollClientLoginSlave::maxAccountIdRequested)
            {
                #ifdef DEBUG_MESSAGE_QUERY_IDLIST
                std::cout << "Ask more to master: maxAccountIdList.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << maxAccountIdList.size() << "<" << CATCHCHALLENGER_SERVER_MINIDBLOCK << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                #endif
                EpollClientLoginSlave::maxAccountIdRequested=true;
                if(LinkToMaster::linkToMaster->queryNumberList.empty())
                {
                    std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
                    errorParsingLayer("Unable to get query id at createAccount_return");
                    return;
                }
                const uint8_t &queryNumber=LinkToMaster::linkToMaster->queryNumberList.back();
                LinkToMaster::linkToMaster->queryNumberList.pop_back();

                //send the network reply
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0xBF;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1;

                LinkToMaster::linkToMaster->registerOutputQuery(queryNumber,0xBF);
                LinkToMaster::linkToMaster->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }
            else
            {
                #ifdef DEBUG_MESSAGE_QUERY_IDLIST
                std::cout << "Don't ask more to master (already in progress): maxAccountIdList.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK: " << maxAccountIdList.size() << "<" << CATCHCHALLENGER_SERVER_MINIDBLOCK << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                #endif
            }
        }
        account_id=maxAccountIdList.front();
        maxAccountIdList.erase(maxAccountIdList.begin());
        {
            PreparedDBQueryLogin::db_query_insert_login.asyncWrite({
                        std::to_string(account_id),
                        binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE),
                        binarytoHexa(askLoginParam->pass,CATCHCHALLENGER_SHA224HASH_SIZE),
                        std::to_string(sFrom1970())
                        });
        }
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

        stat=EpollClientLoginStat::ProtocolGood;
    }
    else
        loginIsWrong(askLoginParam->query_id,0x02,"Login already used: "+binarytoHexa(askLoginParam->login,CATCHCHALLENGER_SHA224HASH_SIZE));
}

void EpollClientLoginSlave::dbQueryWriteLogin(const std::string &queryText)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryText.empty())
    {
        errorParsingLayer("dbQuery() Query is empty, bug");
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "Do login db write: " << queryText << std::endl;
    #endif
    databaseBaseLogin.asyncWrite(queryText);
}

void EpollClientLoginSlave::loginIsWrong(const uint8_t &query_id, const uint8_t &returnCode, const std::string &debugMessage)
{
    //network send
    EpollClientLoginSlave::loginIsWrongBufferReply[1]=query_id;
    EpollClientLoginSlave::loginIsWrongBufferReply[1+1+4]=returnCode;
    removeFromQueryReceived(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));

    //send to server to stop the connection
    errorParsingLayer(debugMessage);
}

void EpollClientLoginSlave::selectCharacter(const uint8_t &query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId)
{
    if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    /// \note account id verified on game server when loading the character, do here is big performance mistake

    //check if the server exists
    if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
    {
        //work around, previous disconnect do unable to send clean reply
        disconnectClient();
        return;

        //send the network reply
        if(haveInputQuery(query_id))
            removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x05;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }

    //send to master to know if the character is not already locked
    if(!LinkToMaster::linkToMaster->trySelectCharacter(this,query_id,serverUniqueKey,charactersGroupIndex,characterId))
    {
        //work around, previous disconnect do unable to send clean reply
        disconnectClient();
        return;

        //send the network reply
        if(haveInputQuery(query_id))
            removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x04;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        /*message directly into the functionerrorParsingLayer("EpollClientLoginSlave::selectCharacter() out of query to request the master server: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;*/
        return;
    }

    stat=CharacterSelecting;
    this->charactersGroupIndex=charactersGroupIndex;
    this->serverUniqueKey=serverUniqueKey;
}

void EpollClientLoginSlave::selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token)
{
    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Reconnect)
    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
        posOutput+=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //connect on the game server and pass in proxy mode
        errorParsingLayer("EpollClientLoginSlave::selectCharacter_ReturnToken() proxy mode not supported from now");
    }
}

void EpollClientLoginSlave::selectCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode,const std::string &customError)
{
    //send the network reply
    removeFromQueryReceived(query_id);

    ProtocolParsingBase::tempBigBufferForOutput[0]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    ProtocolParsingBase::tempBigBufferForOutput[1]=query_id;
    ProtocolParsingBase::tempBigBufferForOutput[1+1+0]=0x01;
    ProtocolParsingBase::tempBigBufferForOutput[1+1+1]=0x00;
    ProtocolParsingBase::tempBigBufferForOutput[1+1+2]=0x00;
    ProtocolParsingBase::tempBigBufferForOutput[1+1+3]=0x00;
    ProtocolParsingBase::tempBigBufferForOutput[1+1+4]=errorCode;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,7);

    //closeSocket(); do into LinkToMaster::parseReplyData()
    switch(errorCode)
    {
        case 0x03:
            errorParsingLayer("Master have relply: Character already connected/online");
        break;
        case 0x05:
        break;
        case 0x08:
            errorParsingLayer("Master have relply: Character too recently disconnected");
        break;
        default:
            if(customError.empty())
                errorParsingLayer("Master have relply: EpollClientLoginSlave::selectCharacter_ReturnFailed() errorCode:"+std::to_string(errorCode)+", query_id: "+std::to_string(query_id));
            else
                errorParsingLayer(customError);
        break;
    }
}

void EpollClientLoginSlave::addCharacter(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId,const uint8_t &skinId)
{
    if(characterGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::addCharacter() charactersGroupIndex is out of range");
        return;
    }
    if(pseudo.size()>255)
    {
        errorParsingLayer("EpollClientLoginSlave::addCharacter() pseudo.size()>255");
        return;
    }
    const int8_t &addCharacter=CharactersGroupForLogin::list.at(characterGroupIndex)->addCharacter(this,query_id,profileIndex,pseudo,monsterGroupId,skinId);
    //error case
    if(addCharacter!=0)
    {
        if(addCharacter>0 && addCharacter<=3)
        {
            EpollClientLoginSlave::addCharacterIsWrongBuffer[1]=query_id;
            EpollClientLoginSlave::addCharacterIsWrongBuffer[3]=0x03;
            removeFromQueryReceived(query_id);

            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterIsWrongBuffer),sizeof(EpollClientLoginSlave::addCharacterIsWrongBuffer));
        }
        if(addCharacter<0)
            errorParsingLayer("EpollClientLoginSlave::addCharacter() hack detected");
        return;
    }
}

void EpollClientLoginSlave::removeCharacterLater(const uint8_t &query_id, const uint8_t &characterGroupIndex, const uint32_t &characterId)
{
    if(characterGroupIndex>=CharactersGroupForLogin::list.size())
    {
        errorParsingLayer("EpollClientLoginSlave::removeCharacterLater() charactersGroupIndex is out of range");
        return;
    }
    if(!CharactersGroupForLogin::list.at(characterGroupIndex)->removeCharacterLater(this,query_id,characterId))
    {
        EpollClientLoginSlave::loginIsWrongBufferReply[1]=query_id;
        EpollClientLoginSlave::loginIsWrongBufferReply[1+1+4]=0x02;
        removeFromQueryReceived(query_id);
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginIsWrongBufferReply),sizeof(EpollClientLoginSlave::loginIsWrongBufferReply));
        errorParsingLayer("EpollClientLoginSlave::removeCharacterLater() out of query to request the master server: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
        return;
    }
}

void EpollClientLoginSlave::addCharacter_ReturnOk(const uint8_t &query_id,const uint32_t &characterId)
{
    EpollClientLoginSlave::addCharacterReply[1]=query_id;
    EpollClientLoginSlave::addCharacterReply[2]=0x00;
    removeFromQueryReceived(query_id);
    *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::addCharacterReply+0x03)=htole32(characterId);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterReply),sizeof(EpollClientLoginSlave::addCharacterReply));
}

void EpollClientLoginSlave::addCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode)
{
    EpollClientLoginSlave::addCharacterIsWrongBuffer[1]=query_id;
    EpollClientLoginSlave::addCharacterIsWrongBuffer[2]=errorCode;
    removeFromQueryReceived(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::addCharacterIsWrongBuffer),sizeof(addCharacterIsWrongBuffer));
    if(errorCode!=0x01)
    {
        errorParsingLayer("EpollClientLoginSlave::addCharacter() in waiting to more id form master server or unknown error: "+std::to_string(errorCode)+": "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
    }
}

void EpollClientLoginSlave::removeCharacter_ReturnOk(const uint8_t &query_id)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=0x01;
    removeFromQueryReceived(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
}

void EpollClientLoginSlave::removeCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode,const std::string &errorString)
{
    EpollClientLoginSlave::removeCharacterReply[1]=query_id;
    EpollClientLoginSlave::removeCharacterReply[3]=errorCode;
    removeFromQueryReceived(query_id);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::removeCharacterReply),sizeof(EpollClientLoginSlave::removeCharacterReply));
    if(errorString.empty())
    {
        errorParsingLayer("EpollClientLoginSlave::removeCharacter() out of query to request the master server: "+std::to_string(errorCode)+": "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        std::cerr << LinkToMaster::linkToMaster->listTheRunningQuery() << std::endl;
    }
    else
        errorParsingLayer(errorString+": "+std::to_string(errorCode));
}
