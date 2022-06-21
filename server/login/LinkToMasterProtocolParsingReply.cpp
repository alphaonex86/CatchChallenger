#include "LinkToMaster.hpp"
#include "EpollClientLoginSlave.hpp"
#include "EpollServerLoginSlave.hpp"
#include "EpollClientLoginSlave.hpp"
#include "CharactersGroupForLogin.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/sha224/sha224.hpp"
#include "../epoll/EpollSocket.hpp"
#include "VariableLoginServer.hpp"

#include <iostream>

using namespace CatchChallenger;

//send reply
bool LinkToMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::Connected && mainCodeType==0xB8)
        {}
        else if(stat==Stat::ProtocolGood && mainCodeType==0xBD)
        {}
        else
        {
            parseNetworkReadError("is not logged, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+"): "+std::string(__FILE__)+", "+std::to_string(__LINE__));
            return false;
        }
    }
    //do the work here
    switch(mainCodeType)
    {
        //Protocol initialization and auth for master
        case 0xB8:
        {
            if(size<1)
            {
                std::cerr << "Need more size for protocol header " << std::endl;
                abort();
            }
            //Protocol initialization
            const uint8_t &returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                if(size!=(1+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "wrong size for protocol header " << returnCode << std::endl;
                    abort();
                }
                switch(returnCode)
                {
                    case 0x04:
                        CompressionProtocol::compressionTypeClient=CompressionProtocol::CompressionType::None;
                    break;
                    case 0x08:
                        CompressionProtocol::compressionTypeClient=CompressionProtocol::CompressionType::Zstandard;
                    break;
                    default:
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return false;
                }
                stat=Stat::ProtocolGood;
                //send the query 0x08
                {
                    unsigned char tempHashResult[CATCHCHALLENGER_SHA224HASH_SIZE];
                    SHA224 hash = SHA224();
                    hash.init();
                    hash.update(reinterpret_cast<const unsigned char *>(LinkToMaster::private_token_master),TOKEN_SIZE_FOR_MASTERAUTH);
                    hash.update(reinterpret_cast<const unsigned char *>(data)+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    hash.final(tempHashResult);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    std::cout << "SHA224(" << binarytoHexa(reinterpret_cast<const char *>(LinkToMaster::private_token_master),TOKEN_SIZE_FOR_MASTERAUTH)
                              << " " << binarytoHexa(data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT) << ") to auth on master" << std::endl;
                    #endif

                    //memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));//To reauth after disconnexion

                    //send the network query
                    registerOutputQuery(queryNumberList.back(),0xBD);
                    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xBD;
                    ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumberList.back();

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,tempHashResult,CATCHCHALLENGER_SHA224HASH_SIZE);

                    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+CATCHCHALLENGER_SHA224HASH_SIZE);

                    queryNumberList.pop_back();
                }
                return true;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return false;
            }
        }
        return true;
        //Register login server
        case 0xBD:
        {
            if(CharactersGroupForLogin::list.size()==0)
            {
                std::cerr << "CharactersGroupForLogin::list.size()==0, C211 never send? (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            unsigned int pos=0;
            {
                if((size-pos)<1)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                if(data[pos]!=1)
                {
                    std::cerr << "reply to 08 return code wrong too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                pos++;
            }
            {
                EpollClientLoginSlave::maxAccountIdList.reserve(EpollClientLoginSlave::maxAccountIdList.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                unsigned int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    if((size-pos)<4)
                    {
                        std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        return false;
                    }
                    EpollClientLoginSlave::maxAccountIdList.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                    pos+=4;
                    index++;
                }
                if(vectorHaveDuplicatesForSmallList(EpollClientLoginSlave::maxAccountIdList))
                {
                    std::cerr << "reply to 08: duplicate maxAccountIdList in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                    unsigned int index=0;
                    while(index<EpollClientLoginSlave::maxAccountIdList.size())
                    {
                        if(index>0)
                            std::cerr << ", ";
                        std::cerr << EpollClientLoginSlave::maxAccountIdList[index];
                        index++;
                    }
                    std::cerr << std::endl;
                    return false;
                }
            }
            {
                unsigned int groupIndex=0;
                while(groupIndex<CharactersGroupForLogin::list.size())
                {
                    CharactersGroupForLogin::list[groupIndex]->maxCharacterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                    unsigned int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        CharactersGroupForLogin::list[groupIndex]->maxCharacterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxCharacterId))
                    {
                        std::cerr << "reply to 08: duplicate maxCharacterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                        unsigned int index=0;
                        while(index<CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size())
                        {
                            if(index>0)
                                std::cerr << ", ";
                            std::cerr << CharactersGroupForLogin::list[groupIndex]->maxCharacterId[index];
                            index++;
                        }
                        std::cerr << std::endl;
                        abort();
                    }

                    CharactersGroupForLogin::list[groupIndex]->maxMonsterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                    index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        CharactersGroupForLogin::list[groupIndex]->maxMonsterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxMonsterId))
                    {
                        std::cerr << "reply to 08: duplicate maxMonsterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                        unsigned int index=0;
                        while(index<CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size())
                        {
                            if(index>0)
                                std::cerr << ", ";
                            std::cerr << CharactersGroupForLogin::list[groupIndex]->maxMonsterId[index];
                            index++;
                        }
                        std::cerr << std::endl;
                        abort();
                    }

                    groupIndex++;
                }
            }
            stat=Stat::Logged;
            if(!EpollServerLoginSlave::epollServerLoginSlave->isListening())
                if(!EpollServerLoginSlave::epollServerLoginSlave->tryListen())
                    abort();
        }
        return true;
        //Select character on master
        case 0xBE:
        {
            //messageParsingLayer("selected (0xBE: 190)");
            if(selectCharacterClients.find(queryNumber)!=selectCharacterClients.cend())
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=selectCharacterClients.at(queryNumber);
                if(dataForSelectedCharacterReturn.client!=NULL)
                {
                    if(size==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                    {
                        if(dataForSelectedCharacterReturn.client!=NULL)
                        {
                            if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                            {
                                if(static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)->stat!=EpollClientLoginSlave::EpollClientLoginStat::CharacterSelecting)
                                {
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadError("client in wrong state main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                                    return false;
                                }
                                //check again if the game server is not disconnected, don't check charactersGroupIndex because previously checked at EpollClientLoginSlave::selectCharacter()
                                const uint8_t &charactersGroupIndex=static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)->charactersGroupIndex;
                                const uint32_t &serverUniqueKey=static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)->serverUniqueKey;
                                if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
                                {
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadError("client server not found to proxy it main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                                    return false;
                                }
                                const CharactersGroupForLogin::InternalGameServer &server=CharactersGroupForLogin::list.at(charactersGroupIndex)->getServerInformation(serverUniqueKey);

                                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                ->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                                /// \todo do the async connect
                                /// linkToGameServer->stat=Stat::Connecting;
                                const std::string dest=server.host+":"+std::to_string(server.port);
                                const int &socketFd=LinkToGameServer::tryConnect(server.host.c_str(),server.port,5,1);
                                if(Q_LIKELY(socketFd>=0))
                                {
                                    duplicateWarning.erase(dest);
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->linkToGameServer=linkToGameServer;
                                    linkToGameServer->queryIdToReconnect=dataForSelectedCharacterReturn.client_query_id;
                                    linkToGameServer->stat=LinkToGameServer::Stat::Connected;
                                    linkToGameServer->client=static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client);
                                    memcpy(linkToGameServer->tokenForGameServer,data,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                                    //send the protocol
                                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                                    linkToGameServer->setConnexionSettings();
                                    linkToGameServer->parseIncommingData();
                                    /*const int &s = EpollSocket::make_non_blocking(socketFd);
                                    if(s == -1)
                                        std::cerr << "unable to make to socket non blocking" << std::endl;*/
                                }
                                else
                                {
                                    /*static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadMessage("not able to connect on the game server as proxy, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");*/
                                    if(duplicateWarning.find(dest)==duplicateWarning.cend())
                                    {
                                        duplicateWarning.insert(dest);
                                        std::cerr << "Error to connect on " << server.host << ":" << server.port << ", errno: " << errno << std::endl;
                                    }
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04);
                                    static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                    ->closeSocket();
                                    //continue to clean, return true;//if false mean the master is the fault
                                }
                            }
                            else
                            {
                                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                ->stat=EpollClientLoginSlave::EpollClientLoginStat::CharacterSelected;
                                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                ->selectCharacter_ReturnToken(dataForSelectedCharacterReturn.client_query_id,data);
                                static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                                ->closeSocket();
                            }
                        }
                    }
                    else if(size==1)
                    {
                        if(dataForSelectedCharacterReturn.client!=NULL)
                        {
                            std::cerr << "Error from master, size==1" << std::endl;
                            static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                            ->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,data[0]);
                            static_cast<EpollClientLoginSlave *>(dataForSelectedCharacterReturn.client)
                            ->closeSocket();
                        }
                    }
                    else
                        parseNetworkReadError("main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                }
                selectCharacterClients.erase(queryNumber);
            }
            else
                std::cerr << "parseFullReplyData() !selectCharacterClients.contains(queryNumber): mainCodeType: " << mainCodeType << ", queryNumber: " << queryNumber << std::endl;
        }
        return true;
        case 0xBF:
        {
            unsigned int pos=0;
            EpollClientLoginSlave::maxAccountIdList.reserve(EpollClientLoginSlave::maxAccountIdList.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                EpollClientLoginSlave::maxAccountIdList.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(EpollClientLoginSlave::maxAccountIdList))
            {
                std::cerr << "reply to 08: duplicate maxAccountIdList in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<EpollClientLoginSlave::maxAccountIdList.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << EpollClientLoginSlave::maxAccountIdList[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            EpollClientLoginSlave::maxAccountIdRequested=false;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cout << "Add more id to list: EpollClientLoginSlave::maxAccountIdList.size(): " << EpollClientLoginSlave::maxAccountIdList.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif
        }
        return true;
        case 0xC0:
        {
            const uint8_t groupIndex=LinkToMaster::queryNumberToCharacterGroup[queryNumber];

            unsigned int pos=0;
            CharactersGroupForLogin::list[groupIndex]->maxCharacterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                CharactersGroupForLogin::list[groupIndex]->maxCharacterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxCharacterId))
            {
                std::cerr << "reply to 08: duplicate maxCharacterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << CharactersGroupForLogin::list[groupIndex]->maxCharacterId[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            #ifdef DEBUG_MESSAGE_QUERY_IDLIST
            std::cout << "Add more id to list: CharactersGroupForLogin::list[" << std::to_string(groupIndex) << "]->maxCharacterId: " << CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif

            LinkToMaster::queryNumberToCharacterGroup[queryNumber]=0;
            CharactersGroupForLogin::list[groupIndex]->maxCharacterIdRequested=false;
        }
        return true;
        case 0xC1:
        {
            const uint8_t groupIndex=LinkToMaster::queryNumberToCharacterGroup[queryNumber];

            unsigned int pos=0;
            CharactersGroupForLogin::list[groupIndex]->maxMonsterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                CharactersGroupForLogin::list[groupIndex]->maxMonsterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxMonsterId))
            {
                std::cerr << "reply to 08: duplicate maxMonsterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << CharactersGroupForLogin::list[groupIndex]->maxMonsterId[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            #ifdef DEBUG_MESSAGE_QUERY_IDLIST
            std::cout << "Add more id to list: CharactersGroupForLogin::list[" << std::to_string(groupIndex) << "]->maxMonsterId.size(): " << CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif

            LinkToMaster::queryNumberToCharacterGroup[queryNumber]=0;
            CharactersGroupForLogin::list[groupIndex]->maxMonsterIdRequested=false;
        }
        return true;
        default:
            parseNetworkReadError("The master server responds to not coded value into the switch (or end with break not return): "+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+", file:"+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    return true;
}
