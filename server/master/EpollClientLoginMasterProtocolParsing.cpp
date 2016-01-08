#include "EpollClientLoginMaster.h"
#include "EpollServerLoginMaster.h"
#include "../../general/base/CommonSettingsCommon.h"

#include <iostream>
#include <openssl/sha.h>

using namespace CatchChallenger;

bool EpollClientLoginMaster::parseInputBeforeLogin(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    switch(mainCodeType)
    {
        //Protocol initialization and auth for master
        case 0xB8:
            removeFromQueryReceived(queryNumber);
            if(size==sizeof(EpollClientLoginMaster::protocolHeaderToMatch))
            {
                if(memcmp(data,EpollClientLoginMaster::protocolHeaderToMatch,sizeof(EpollClientLoginMaster::protocolHeaderToMatch))==0)
                {
                    {
                        if(!tokenForAuth.empty())
                        {
                            errorParsingLayer("!tokenForAuth.isEmpty()");
                            return false;
                        }
                        tokenForAuth.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        const int &returnedSize=fread(tokenForAuth.data(),1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,EpollClientLoginMaster::fpRandomFile);
                        if(returnedSize!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
                        {
                            std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_MASTERAUTH << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
                            abort();
                        }
                    }
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    switch(ProtocolParsing::compressionType)
                    {
                        case CompressionType_None:
                            *(EpollClientLoginMaster::protocolReplyCompressionNone+1)=queryNumber;
                            memcpy(EpollClientLoginMaster::protocolReplyCompressionNone+7,tokenForAuth.constData(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionNone),sizeof(EpollClientLoginMaster::protocolReplyCompressionNone));
                        break;
                        case CompressionType_Zlib:
                            *(EpollClientLoginMaster::protocolReplyCompresssionZlib+1)=queryNumber;
                            memcpy(EpollClientLoginMaster::protocolReplyCompresssionZlib+7,tokenForAuth.constData(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompresssionZlib),sizeof(EpollClientLoginMaster::protocolReplyCompresssionZlib));
                        break;
                        case CompressionType_Xz:
                            *(EpollClientLoginMaster::protocolReplyCompressionXz+1)=queryNumber;
                            memcpy(EpollClientLoginMaster::protocolReplyCompressionXz+7,tokenForAuth.constData(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionXz),sizeof(EpollClientLoginMaster::protocolReplyCompressionXz));
                        break;
                        case CompressionType_Lz4:
                            *(EpollClientLoginMaster::protocolReplyCompressionLz4+1)=queryNumber;
                            memcpy(EpollClientLoginMaster::protocolReplyCompressionLz4+7,tokenForAuth.constData(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionLz4),sizeof(EpollClientLoginMaster::protocolReplyCompressionLz4));
                        break;
                        default:
                            errorParsingLayer("Compression selected wrong");
                        return;
                    }
                    #else
                    *(EpollClientLoginMaster::protocolReplyCompressionNone+1)=queryNumber;
                    memcpy(EpollClientLoginMaster::protocolReplyCompressionNone+7,tokenForAuth.data(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionNone),sizeof(EpollClientLoginMaster::protocolReplyCompressionNone));
                    #endif
                    stat=EpollClientLoginMasterStat::Logged;
                    //messageParsingLayer("Protocol sended and replied");

                    //not after due to B2 Register game server:
                    flags|=0x08;
                }
                else
                {
                    /*don't send packet to prevent DDOS
                    *(EpollClientLoginMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginMaster::protocolReplyProtocolNotSupported));*/
                    errorParsingLayer("Wrong protocol magic number");
                    return false;
                }
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(EpollClientLoginMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginMaster::protocolReplyProtocolNotSupported));*/
                errorParsingLayer("Wrong protocol magic number size");
                return false;
            }
        break;
        //Select character on master
        case 0xBE:
            if(stat!=EpollClientLoginMasterStat::Logged)
            {
                errorParsingLayer("send login before the protocol");
                return false;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        break;
    }
    return true;
}

bool EpollClientLoginMaster::parseMessage(const uint8_t &mainCodeType, const char * const parseMessagedata, const unsigned int &size)
{
    (void)parseMessagedata;
    (void)size;
    switch(mainCodeType)
    {
        //Unlock and unregister charater id (disconnected)
        case 0x3E:
        {
            if(stat!=EpollClientLoginMasterStat::GameServer)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer main ident: "+std::to_string(mainCodeType));
                return false;
            }
            if(charactersGroupForGameServer==NULL)
            {
                parseNetworkReadError("charactersGroupForGameServer==NULL main ident: "+std::to_string(mainCodeType));
                return false;
            }
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(parseMessagedata)));
            charactersGroupForGameServer->unlockTheCharacter(characterId);
            charactersGroupForGameServerInformation->lockedAccount.erase(characterId);
            return true;
        }
        break;
        //Current player number (Game server to master)
        case 0x3F:
        {
            if(stat!=EpollClientLoginMasterStat::GameServer)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer main ident: "+std::to_string(mainCodeType));
                return false;
            }
            if(charactersGroupForGameServerInformation==NULL)
            {
                parseNetworkReadError("charactersGroupForGameServerInformation==NULL main ident: "+std::to_string(mainCodeType));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=2)
            {
                parseNetworkReadError("size!=2 main ident: "+std::to_string(mainCodeType));
                return false;
            }
            #endif
            charactersGroupForGameServerInformation->currentPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(parseMessagedata)));
            if(charactersGroupForGameServerInformation->currentPlayer>charactersGroupForGameServerInformation->maxPlayer)
            {
                parseNetworkReadError("charactersGroupForGameServerInformation->currentPlayer > charactersGroupForGameServerInformation->maxPlayer main ident: "+std::to_string(mainCodeType));
                return false;
            }
            {
                /*do the partial alteration of:
                EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
                EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();*/
                const int index=vectorindexOf(gameServers,this);
                if(index==-1)
                {
                    parseNetworkReadError("gameServers.indexOf(this)==-1 main ident: "+std::to_string(mainCodeType));
                    return false;
                }
                const int posFromZero=gameServers.size()-1-index;
                const unsigned int bufferPos=2-2*/*last is zero*/posFromZero;
                *reinterpret_cast<uint16_t *>(EpollClientLoginMaster::serverServerList+EpollClientLoginMaster::serverServerListSize-bufferPos)=*reinterpret_cast<uint16_t *>(const_cast<char *>(parseMessagedata));
                *reinterpret_cast<uint16_t *>(EpollClientLoginMaster::loginPreviousToReplyCache+EpollClientLoginMaster::loginPreviousToReplyCacheSize-bufferPos)=*reinterpret_cast<uint16_t *>(const_cast<char *>(parseMessagedata));
            }
            currentPlayerForGameServerToUpdate=true;
            return true;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
}

//have query with reply
bool EpollClientLoginMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat==EpollClientLoginMasterStat::None)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return false;
    }
    switch(mainCodeType)
    {
        //get maxMonsterId block
        case 0xB0:
        {
            if(stat!=EpollClientLoginMasterStat::GameServer)
            {
                parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            if(size!=1)
            {
                parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            const unsigned char &charactersGroupIndex=*data;
            if(charactersGroupIndex>=CharactersGroup::list.size())
            {
                parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_SERVER_MAXIDBLOCK*4);//set the dynamic size

            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4+index*4/*size of int*/)=(uint32_t)htole32(charactersGroup->maxMonsterId+1+index);
                index++;
            }
            charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            posOutput+=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        //get maxClanId block
        case 0xB1:
        {
            if(stat!=EpollClientLoginMasterStat::GameServer)
            {
                parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            if(size!=1)
            {
                parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            const unsigned char &charactersGroupIndex=*data;
            if(charactersGroupIndex>=CharactersGroup::list.size())
            {
                parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4);//set the dynamic size

            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1+4+index*4/*size of int*/)=(uint32_t)htole32(charactersGroup->maxClanId+1+index);
                index++;
            }
            charactersGroup->maxClanId+=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
            posOutput+=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        //Register game server
        case 0xB2:
        {
            unsigned int pos=0;

            //token auth
            {
                if((size-pos)<(int)CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    parseNetworkReadError("wrong size for master auth hash");
                    return false;
                }
                if(tokenForAuth.empty())
                {
                    parseNetworkReadError("tokenForAuth.isEmpty()");
                    return false;
                }
                SHA256_CTX hash;
                if(SHA224_Init(&hash)!=1)
                {
                    std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
                    abort();
                }
                SHA224_Update(&hash,EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH);
                SHA224_Update(&hash,tokenForAuth.data(),tokenForAuth.size());
                SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hash);
                if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,data+pos,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    std::cout << "SHA224(" << binarytoHexa(EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH)
                              << " " << binarytoHexa(tokenForAuth) << ") to auth on master failed: " << __FILE__ << ":" << __LINE__ << std::endl;
                    #endif
                    *(EpollClientLoginMaster::protocolReplyWrongAuth+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyWrongAuth),sizeof(EpollClientLoginMaster::protocolReplyWrongAuth));
                    errorParsingLayer("Wrong protocol token");
                    return false;
                }
                pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
            }
            //flags|=0x08;

            std::string charactersGroup;
            std::string host;
            std::string xml;
            uint16_t port=0;
            uint32_t uniqueKey=0;
            uint32_t logicalGroupIndex=0;
            uint16_t currentPlayer=0,maxPlayer=0;
            //charactersGroup
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong utf8 to std::string size for text size");
                    return false;
                }
                const uint8_t &textSize=data[pos];
                pos+=sizeof(uint8_t);
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        parseNetworkReadError("wrong utf8 to std::string size for text");
                        return false;
                    }
                    charactersGroup=std::string(data+pos,textSize);
                    pos+=textSize;
                }
            }
            if(CharactersGroup::hash.find(charactersGroup)==CharactersGroup::hash.cend())
            {
                //send the network reply
                removeFromQueryReceived(queryNumber);
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x03;
                posOutput+=1;

                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                parseNetworkReadError("charactersGroup not found: "+charactersGroup);
                return false;
            }
            charactersGroupForGameServer=CharactersGroup::hash.at(charactersGroup);
            //uniqueKey
            if((size-pos)<(int)sizeof(uint32_t))
            {
                parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            //host
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong utf8 to std::string size for text size");
                    return false;
                }
                const uint8_t &textSize=data[pos];
                pos+=sizeof(uint8_t);
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        parseNetworkReadError("wrong utf8 to std::string size for text");
                        return false;
                    }
                    host=std::string(data+pos,textSize);
                    pos+=textSize;
                }
                else
                {
                    parseNetworkReadError("host can't be empty");
                    return false;
                }
            }
            //port
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint16_t);
            //xml
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseNetworkReadError("wrong utf8 to std::string size for text size");
                    return false;
                }
                const uint16_t textSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint16_t);
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        parseNetworkReadError("wrong utf8 to std::string size for text");
                        return false;
                    }
                    xml=std::string(data+pos,textSize);
                    pos+=textSize;
                }
            }
            //logical group
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseNetworkReadError("wrong utf8 to std::string size for text size");
                    return false;
                }
                std::string logicalGroup;
                const uint8_t &textSize=data[pos];
                pos+=sizeof(uint8_t);
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        parseNetworkReadError("wrong utf8 to std::string size for text");
                        return false;
                    }
                    logicalGroup=std::string(data+pos,textSize);
                    pos+=textSize;
                }
                if(EpollClientLoginMaster::logicalGroupHash.find(logicalGroup)!=EpollClientLoginMaster::logicalGroupHash.cend())
                    logicalGroupIndex=EpollClientLoginMaster::logicalGroupHash.at(logicalGroup);
                else
                {
                    logicalGroupIndex=0;
                    parseNetworkReadError(std::string("logicalGroup \"")+logicalGroup+"\" not found for "+host+":"+std::to_string(port)+" at "+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
            }
            //current player
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            currentPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint16_t);
            //max player
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            maxPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint16_t);
            std::unordered_set<uint32_t/*characterId*/> connectedPlayer;
            //connected player
            {
                unsigned int index=0;
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
                unsigned int disconnectedPlayerNumber=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint16_t);
                while(index<disconnectedPlayerNumber)
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseNetworkReadError(std::string("wrong size at ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    unsigned int characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint32_t);
                    connectedPlayer.insert(characterId);
                    charactersGroupForGameServerInformation->lockedAccount.erase(characterId);
                    index++;
                }
            }
            //compose the reply
            {
                //send the network reply
                removeFromQueryReceived(queryNumber);
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1+4;

                if(Q_UNLIKELY(charactersGroupForGameServer->gameServers.find(uniqueKey)!=charactersGroupForGameServer->gameServers.cend()))
                {
                    uint32_t newUniqueKey;
                    do
                    {
                        newUniqueKey = rng();
                    } while(Q_UNLIKELY(charactersGroupForGameServer->gameServers.find(newUniqueKey)!=charactersGroupForGameServer->gameServers.cend()));
                    uniqueKey=newUniqueKey;
                    std::cerr << "Generate new unique key for a game server" << std::endl;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
                    posOutput+=1;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=(uint32_t)htole32(newUniqueKey);
                    posOutput+=4;
                }
                else
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                    posOutput+=1;
                }
                //monster id list
                {
                    if((posOutput+4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
                    {
                        std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                    }
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroupForGameServer->maxMonsterId+1+index);
                        index++;
                    }
                    posOutput+=4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    charactersGroupForGameServer->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                }
                //clan id list
                {
                    if((posOutput+4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
                    {
                        std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                    }
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                    {
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroupForGameServer->maxClanId+1+index);
                        index++;
                    }
                    posOutput+=4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
                    charactersGroupForGameServer->maxClanId+=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
                }
                if((posOutput+1+2+1+2)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
                {
                    std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                //Max player monsters
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                    posOutput+=1;
                }
                //Max warehouse player monsters
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
                    posOutput+=2;
                }
                //Max player items
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                    posOutput+=1;
                }
                //Max warehouse player monsters
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
                    posOutput+=2;
                }

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
                sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            }

            //only game server will receive query
            {
                queryNumberList.reserve(CATCHCHALLENGER_MAXPROTOCOLQUERY);
                int index=0;
                while(index<CATCHCHALLENGER_MAXPROTOCOLQUERY)
                {
                    queryNumberList.push_back(index);
                    index++;
                }
            }
            EpollClientLoginMaster::gameServers.push_back(this);
            charactersGroupForGameServerInformation=charactersGroupForGameServer->addGameServerUniqueKey(
                        this,uniqueKey,host,port,xml,logicalGroupIndex,currentPlayer,maxPlayer,connectedPlayer);
            stat=EpollClientLoginMasterStat::GameServer;
            EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
            EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();
            currentPlayerForGameServerToUpdate=false;
            EpollClientLoginMaster::broadcastGameServerChange();

            updateConsoleCountServer();
        }
        break;
        //Register login server
        case 0xBD:
        {
            //token auth
            {
                if(size<(int)CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    parseNetworkReadError("wrong size for master auth hash");
                    return false;
                }
                if(tokenForAuth.empty())
                {
                    parseNetworkReadError("tokenForAuth.isEmpty()");
                    return false;
                }
                SHA256_CTX hash;
                if(SHA224_Init(&hash)!=1)
                {
                    std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
                    abort();
                }
                SHA224_Update(&hash,EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH);
                SHA224_Update(&hash,tokenForAuth.data(),tokenForAuth.size());
                SHA224_Final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput),&hash);
                if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,data,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    std::cout << "SHA224(" << binarytoHexa(EpollClientLoginMaster::private_token,TOKEN_SIZE_FOR_MASTERAUTH)
                              << " " << binarytoHexa(tokenForAuth) << ") to auth on master failed: " << __FILE__ << ":" << __LINE__ << std::endl;
                    #endif
                    *(EpollClientLoginMaster::protocolReplyWrongAuth+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyWrongAuth),sizeof(EpollClientLoginMaster::protocolReplyWrongAuth));
                    errorParsingLayer("Wrong protocol token");
                    return false;
                }
            }
            //flags|=0x08;

            if(stat!=EpollClientLoginMasterStat::Logged)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::Logged: "+std::to_string(stat)+" to register as login server");
                return false;
            }
            stat=EpollClientLoginMasterStat::LoginServer;
            //send logical group list + Raw server list master to login + Login settings and Characters group
            if(EpollClientLoginMaster::loginPreviousToReplyCacheSize!=0)
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::loginPreviousToReplyCache),EpollClientLoginMaster::loginPreviousToReplyCacheSize);
            else
            {
                std::cerr << "EpollClientLoginMaster::loginPreviousToReplyCacheSize==0 (abort)" << std::endl;
                abort();
            }

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;

            //send the id list
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                posOutput++;
            }
            if((posOutput+4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
            {
                std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(maxAccountId+1+index);
                index++;
            }
            posOutput+=4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            {
                if((posOutput+2*4*CATCHCHALLENGER_SERVER_MAXIDBLOCK*CharactersGroup::list.size())>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
                {
                    std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                unsigned int charactersGroupIndex=0;
                while(charactersGroupIndex<CharactersGroup::list.size())
                {
                    CharactersGroup *charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint32_t)htole32(charactersGroup->maxCharacterId+1+index);
                        posOutput+=4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint32_t)htole32(charactersGroup->maxMonsterId+1+index);
                        posOutput+=4;
                        index++;
                    }
                    charactersGroup->maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    charactersGroupIndex++;
                }
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        EpollClientLoginMaster::loginServers.push_back(this);

        updateConsoleCountServer();
        break;
        case 0xBE:
        {
            const uint8_t &charactersGroupIndex=data[0];
            const uint32_t &serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+5)));
            const uint32_t &accountId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+9)));
            selectCharacter(queryNumber,serverUniqueKey,charactersGroupIndex,characterId,accountId);
        }
        break;
        //get maxAccountId block
        case 0xBF:
        {
            if(stat!=EpollClientLoginMasterStat::LoginServer)
            {
                parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            if(!CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
            {
                parseNetworkReadError("!EpollClientLoginMaster::automatic_account_creation then why ask account id? EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_SERVER_MAXIDBLOCK*4);//set the dynamic size

            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(maxAccountId+1+index);
                index++;
            }
            maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            posOutput+=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        //get maxCharacterId block
        case 0xC0:
        {
            if(stat!=EpollClientLoginMasterStat::LoginServer)
            {
                parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            if(size!=1)
            {
                parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            const unsigned char &charactersGroupIndex=*data;
            if(charactersGroupIndex>=CharactersGroup::list.size())
            {
                parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_SERVER_MAXIDBLOCK*4);//set the dynamic size

            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroup->maxCharacterId+1+index);
                index++;
            }
            charactersGroup->maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            posOutput+=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        //get maxMonsterId block
        case 0xC1:
        {
            if(stat!=EpollClientLoginMasterStat::LoginServer)
            {
                parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            if(size!=1)
            {
                parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            const unsigned char &charactersGroupIndex=*data;
            if(charactersGroupIndex>=CharactersGroup::list.size())
            {
                parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);

            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_SERVER_MAXIDBLOCK*4);//set the dynamic size

            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroup->maxMonsterId+1+index);
                index++;
            }
            charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            posOutput+=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    return true;
}

//send reply
bool EpollClientLoginMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    //do the work here
    switch(mainCodeType)
    {
        //Get token for character select
        case 0xF8:
        {
            if(stat!=EpollClientLoginMasterStat::GameServer)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer: "+std::to_string(stat)+" EpollClientLoginMaster::parseFullQuery()"+std::to_string(mainCodeType));
                return false;
            }
            //orderned mode drop: if(loginServerReturnForCharaterSelect.contains(queryNumber)), use first
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=loginServerReturnForCharaterSelect.front();
                if(size==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                {
                    if(dataForSelectedCharacterReturn.loginServer!=NULL)
                        dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnToken(dataForSelectedCharacterReturn.client_query_id,data);
                }
                else if(size==1)
                {
                    if(data[0]!=0x03)
                    {
                        //internal error, no more token, ...
                        charactersGroupForGameServer->unlockTheCharacter(dataForSelectedCharacterReturn.characterId);
                        charactersGroupForGameServerInformation->lockedAccount.erase(dataForSelectedCharacterReturn.characterId);
                    }
                    else
                    {
                        //account already locked
                        charactersGroupForGameServer->lockTheCharacter(dataForSelectedCharacterReturn.characterId);
                        charactersGroupForGameServerInformation->lockedAccount.insert(dataForSelectedCharacterReturn.characterId);
                        std::cerr << "The master have not detected nothing but the game server " << charactersGroupForGameServerInformation->host << ":" << charactersGroupForGameServerInformation->port << " have reply than the account " << dataForSelectedCharacterReturn.characterId << " is already locked" << std::endl;
                    }
                    if(dataForSelectedCharacterReturn.loginServer!=NULL)
                        dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,data[0]);
                }
                else
                    parseNetworkReadError("main ident: "+std::to_string(mainCodeType)+", reply size for 8101 wrong");
                loginServerReturnForCharaterSelect.erase(loginServerReturnForCharaterSelect.begin());
            }
            /*orderned mode else
                std::cerr << "parseFullReplyData() !loginServerReturnForCharaterSelect.contains(queryNumber): mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;*/
        }
        return true;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    parseNetworkReadError(std::string("The server for now not ask anything: ")+std::to_string(mainCodeType)+", "+std::to_string(queryNumber));
}

void EpollClientLoginMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
