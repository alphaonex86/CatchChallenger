#include "EpollClientLoginMaster.h"
#include "EpollServerLoginMaster.h"
#include "../../general/base/CommonSettingsCommon.h"

#include <iostream>
#include <QDateTime>

using namespace CatchChallenger;

void EpollClientLoginMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x01:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(queryNumber);
            #endif
            if(size==sizeof(EpollClientLoginMaster::protocolHeaderToMatch)+sizeof(EpollClientLoginMaster::private_token))
            {
                if(memcmp(data,EpollClientLoginMaster::protocolHeaderToMatch,sizeof(EpollClientLoginMaster::protocolHeaderToMatch))==0)
                {
                    if(memcmp(data+sizeof(EpollClientLoginMaster::protocolHeaderToMatch),EpollClientLoginMaster::private_token,sizeof(EpollClientLoginMaster::private_token))==0)
                    {
                        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                        switch(ProtocolParsing::compressionType)
                        {
                            case CompressionType_None:
                                *(EpollClientLoginMaster::protocolReplyCompressionNone+1)=queryNumber;
                                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionNone),sizeof(EpollClientLoginMaster::protocolReplyCompressionNone));
                            break;
                            case CompressionType_Zlib:
                                *(EpollClientLoginMaster::protocolReplyCompresssionZlib+1)=queryNumber;
                                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompresssionZlib),sizeof(EpollClientLoginMaster::protocolReplyCompresssionZlib));
                            break;
                            case CompressionType_Xz:
                                *(EpollClientLoginMaster::protocolReplyCompressionXz+1)=queryNumber;
                                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionXz),sizeof(EpollClientLoginMaster::protocolReplyCompressionXz));
                            break;
                            default:
                                errorParsingLayer("Compression selected wrong");
                            return;
                        }
                        #else
                        *(EpollClientLoginMaster::protocolReplyCompressionNone+1)=queryNumber;
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyCompressionNone),sizeof(EpollClientLoginMaster::protocolReplyCompressionNone));
                        #endif
                        stat=EpollClientLoginMasterStat::Logged;
                        messageParsingLayer("Protocol sended and replied");
                    }
                    else
                    {
                        *(EpollClientLoginMaster::protocolReplyWrongAuth+1)=queryNumber;
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyWrongAuth),sizeof(EpollClientLoginMaster::protocolReplyWrongAuth));
                        errorParsingLayer("Wrong protocol token");
                        return;
                    }
                }
                else
                {
                    /*don't send packet to prevent DDOS
                    *(EpollClientLoginMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginMaster::protocolReplyProtocolNotSupported));*/
                    errorParsingLayer("Wrong protocol magic number");
                    return;
                }
            }
            else
            {
                /*don't send packet to prevent DDOS
                *(EpollClientLoginMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginMaster::protocolReplyProtocolNotSupported));*/
                errorParsingLayer("Wrong protocol magic number size");
                return;
            }
        break;
        case 0x02:
            if(stat!=EpollClientLoginMasterStat::Logged)
            {
                errorParsingLayer("send login before the protocol");
                return;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void EpollClientLoginMaster::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginMaster::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const rawData,const unsigned int &size)
{
    (void)rawData;
    (void)size;
    (void)subCodeType;
    switch(mainCodeType)
    {
        case 0x45:
        switch(subCodeType)
        {
            case 0x01:
            {
                if(stat!=EpollClientLoginMasterStat::GameServer)
                {
                    parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                if(charactersGroupForGameServer==NULL)
                {
                    parseNetworkReadError("charactersGroupForGameServer==NULL main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                charactersGroupForGameServer->unlockTheCharacter(characterId);
                charactersGroupForGameServerInformation->lockedAccount.remove(characterId);
            }
            break;
            case 0x02:
            {
                if(stat!=EpollClientLoginMasterStat::GameServer)
                {
                    parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                if(charactersGroupForGameServerInformation==NULL)
                {
                    parseNetworkReadError("charactersGroupForGameServerInformation==NULL main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=2)
                {
                    parseNetworkReadError("size!=2 main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                #endif
                charactersGroupForGameServerInformation->currentPlayer=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData)));
                if(charactersGroupForGameServerInformation->currentPlayer>charactersGroupForGameServerInformation->maxPlayer)
                {
                    parseNetworkReadError("charactersGroupForGameServerInformation->currentPlayer > charactersGroupForGameServerInformation->maxPlayer main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                }
                {
                    /*do the partial alteration of:
                    EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
                    EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();*/
                    const int index=gameServers.indexOf(this);
                    if(index==-1)
                    {
                        parseNetworkReadError("gameServers.indexOf(this)==-1 main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                        return;
                    }
                    const int posFromZero=gameServers.size()-1-index;
                    const unsigned int bufferPos=2-2*/*last is zero*/posFromZero;
                    *reinterpret_cast<quint16 *>(EpollClientLoginMaster::serverServerList+EpollClientLoginMaster::serverServerListSize-bufferPos)=*reinterpret_cast<quint16 *>(const_cast<char *>(rawData));
                    *reinterpret_cast<quint16 *>(EpollClientLoginMaster::loginPreviousToReplyCache+EpollClientLoginMaster::loginPreviousToReplyCacheSize-bufferPos)=*reinterpret_cast<quint16 *>(const_cast<char *>(rawData));
                }
                currentPlayerForGameServerToUpdate=true;
            }
            break;
            default:
                parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void EpollClientLoginMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat==EpollClientLoginMasterStat::None)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        case 0x07:
        {
            unsigned int pos=0;

            QString charactersGroup;
            QString host;
            QString xml;
            quint16 port=0;
            quint32 uniqueKey=0;
            quint32 logicalGroupIndex=0;
            quint16 currentPlayer=0,maxPlayer=0;
            //charactersGroup
            {
                if((size-pos)<(int)sizeof(quint8))
                {
                    parseNetworkReadError("wrong utf8 to QString size for text size");
                    return;
                }
                const quint8 &textSize=data[pos];
                pos+=sizeof(quint8);
                if(textSize>0)
                {
                    if((size-pos)<(int)textSize)
                    {
                        parseNetworkReadError("wrong utf8 to QString size for text");
                        return;
                    }
                    charactersGroup=QString::fromUtf8(data+pos,textSize);
                    pos+=textSize;
                }
            }
            if(!CharactersGroup::hash.contains(charactersGroup))
            {
                EpollClientLoginMaster::tempBuffer[0x00]=0x03;
                postReplyData(queryNumber,EpollClientLoginMaster::tempBuffer,1);
                parseNetworkReadError("charactersGroup not found: "+charactersGroup);
                return;
            }
            charactersGroupForGameServer=CharactersGroup::hash.value(charactersGroup);
            //uniqueKey
            if((size-pos)<(int)sizeof(quint32))
            {
                parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                return;
            }
            uniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+pos)));
            pos+=sizeof(quint32);
            //host
            {
                if((size-pos)<(int)sizeof(quint8))
                {
                    parseNetworkReadError("wrong utf8 to QString size for text size");
                    return;
                }
                const quint8 &textSize=data[pos];
                pos+=sizeof(quint8);
                if(textSize>0)
                {
                    if((size-pos)<(int)textSize)
                    {
                        parseNetworkReadError("wrong utf8 to QString size for text");
                        return;
                    }
                    host=QString::fromUtf8(data+pos,textSize);
                    pos+=textSize;
                }
                else
                {
                    parseNetworkReadError("host can't be empty");
                    return;
                }
            }
            //port
            if((size-pos)<(int)sizeof(quint16))
            {
                parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                return;
            }
            port=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data+pos)));
            pos+=sizeof(quint16);
            //xml
            {
                if((size-pos)<(int)sizeof(quint16))
                {
                    parseNetworkReadError("wrong utf8 to QString size for text size");
                    return;
                }
                const quint16 textSize=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data+pos)));
                pos+=sizeof(quint16);
                if(textSize>0)
                {
                    if((size-pos)<(int)textSize)
                    {
                        parseNetworkReadError("wrong utf8 to QString size for text");
                        return;
                    }
                    xml=QString::fromUtf8(data+pos,textSize);
                    pos+=textSize;
                }
            }
            //logical group
            {
                if((size-pos)<(int)sizeof(quint8))
                {
                    parseNetworkReadError("wrong utf8 to QString size for text size");
                    return;
                }
                QString logicalGroup;
                const quint8 &textSize=data[pos];
                pos+=sizeof(quint8);
                if(textSize>0)
                {
                    if((size-pos)<(int)textSize)
                    {
                        parseNetworkReadError("wrong utf8 to QString size for text");
                        return;
                    }
                    logicalGroup=QString::fromUtf8(data+pos,textSize);
                    pos+=textSize;
                }
                if(EpollClientLoginMaster::logicalGroupHash.contains(logicalGroup))
                    logicalGroupIndex=EpollClientLoginMaster::logicalGroupHash.value(logicalGroup);
                else
                {
                    logicalGroupIndex=0;
                    parseNetworkReadError(QStringLiteral("logicalGroup \"%1\" not found for %2:%3 at %4:%5")
                                          .arg(logicalGroup)
                                          .arg(host)
                                          .arg(port)
                                          .arg(__FILE__)
                                          .arg(__LINE__)
                                          );
                    return;
                }
            }
            //current player
            if((size-pos)<(int)sizeof(quint16))
            {
                parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                return;
            }
            currentPlayer=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data+pos)));
            pos+=sizeof(quint16);
            //max player
            if((size-pos)<(int)sizeof(quint16))
            {
                parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                return;
            }
            maxPlayer=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data+pos)));
            pos+=sizeof(quint16);
            QSet<quint32/*characterId*/> connectedPlayer;
            //connected player
            {
                unsigned int index=0;
                if((size-pos)<(int)sizeof(quint16))
                {
                    parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                    return;
                }
                unsigned int disconnectedPlayerNumber=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(data+pos)));
                pos+=sizeof(quint16);
                while(index<disconnectedPlayerNumber)
                {
                    if((size-pos)<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size at %1:%2").arg(__FILE__).arg(__LINE__));
                        return;
                    }
                    unsigned int characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(quint32);
                    connectedPlayer << characterId;
                    charactersGroupForGameServerInformation->lockedAccount.remove(characterId);
                    index++;
                }
            }
            unsigned int pos;
            if(Q_UNLIKELY(charactersGroupForGameServer->gameServers.contains(uniqueKey)))
            {
                quint32 newUniqueKey;
                do
                {
                    newUniqueKey = rng();
                } while(Q_UNLIKELY(charactersGroupForGameServer->gameServers.contains(newUniqueKey)));
                uniqueKey=newUniqueKey;
                std::cerr << "Generate new unique key for a game server" << std::endl;
                pos=1+4;
                EpollClientLoginMaster::tempBuffer[0x00]=0x02;
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::tempBuffer+1)=(quint32)htole32(newUniqueKey);
            }
            else
            {
                pos=1;
                EpollClientLoginMaster::tempBuffer[0x00]=0x01;
            }
            //monster id list
            {
                if((pos+4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)>=sizeof(EpollClientLoginMaster::replyToIdListBuffer))
                {
                    std::cerr << "EpollClientLoginMaster::replyToIdListBuffer out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+pos+index*4/*size of int*/)=(quint32)htole32(charactersGroupForGameServer->maxMonsterId+1+index);
                    index++;
                }
                pos+=4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                charactersGroupForGameServer->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            }
            //clan id list
            {
                if((pos+4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)>=sizeof(EpollClientLoginMaster::replyToIdListBuffer))
                {
                    std::cerr << "EpollClientLoginMaster::replyToIdListBuffer out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                {
                    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+pos+index*4/*size of int*/)=(quint32)htole32(charactersGroupForGameServer->maxClanId+1+index);
                    index++;
                }
                pos+=4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
                charactersGroupForGameServer->maxClanId+=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
            }
            if((pos+1+2+1+2)>=sizeof(EpollClientLoginMaster::replyToIdListBuffer))
            {
                std::cerr << "EpollClientLoginMaster::replyToIdListBuffer out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            //Max player monsters
            {
                EpollClientLoginMaster::replyToIdListBuffer[pos]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                pos+=1;
            }
            //Max warehouse player monsters
            {
                *reinterpret_cast<quint16 *>(EpollClientLoginMaster::replyToIdListBuffer+pos)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
                pos+=2;
            }
            //Max player items
            {
                EpollClientLoginMaster::replyToIdListBuffer[pos]=CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                pos+=1;
            }
            //Max warehouse player monsters
            {
                *reinterpret_cast<quint16 *>(EpollClientLoginMaster::replyToIdListBuffer+pos)=htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
                pos+=2;
            }


            postReplyData(queryNumber,EpollClientLoginMaster::tempBuffer,pos);

            //only game server will receive query
            {
                queryNumberList.reserve(50);
                int index=0;
                while(index<50)
                {
                    queryNumberList.push_back(index);
                    index++;
                }
            }
            EpollClientLoginMaster::gameServers << this;
            charactersGroupForGameServerInformation=charactersGroupForGameServer->addGameServerUniqueKey(
                        this,uniqueKey,host,port,xml,logicalGroupIndex,currentPlayer,maxPlayer,connectedPlayer);
            stat=EpollClientLoginMasterStat::GameServer;
            EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
            EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();
            currentPlayerForGameServerToUpdate=false;
            EpollClientLoginMaster::broadcastGameServerChange();

            std::cout << "Online: " << loginServers.size() << " login server and " << gameServers.size() << " game server" << std::endl;
        }
        break;
        case 0x08:
        {
            if(stat!=EpollClientLoginMasterStat::Logged)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::Logged: "+QString::number(stat)+" to register as login server");
                return;
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
            //send the id list
            unsigned int pos=EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset;
            if((pos+4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)>=sizeof(EpollClientLoginMaster::replyToRegisterLoginServer))
            {
                std::cerr << "EpollClientLoginMaster::replyToRegisterLoginServer out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset+index*4/*size of int*/)=(quint32)htole32(maxAccountId+1+index);
                index++;
            }
            pos+=4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            {
                if((pos+2*4*CATCHCHALLENGER_SERVER_MAXIDBLOCK*CharactersGroup::list.size())>=sizeof(EpollClientLoginMaster::replyToRegisterLoginServer))
                {
                    std::cerr << "EpollClientLoginMaster::replyToRegisterLoginServer out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                int charactersGroupIndex=0;
                while(charactersGroupIndex<CharactersGroup::list.size())
                {
                    CharactersGroup *charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset+
                                                     (CATCHCHALLENGER_SERVER_MAXIDBLOCK*1+index)*4/*size of int*/+
                                                     charactersGroupIndex*(CATCHCHALLENGER_SERVER_MAXIDBLOCK*2*sizeof(quint32))
                                                     )=(quint32)htole32(charactersGroup->maxCharacterId+1+index);
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset+
                                                     (CATCHCHALLENGER_SERVER_MAXIDBLOCK*2+index)*4/*size of int*/+
                                                     charactersGroupIndex*(CATCHCHALLENGER_SERVER_MAXIDBLOCK*2*sizeof(quint32))
                                                     )=(quint32)htole32(charactersGroup->maxMonsterId+1+index);
                        index++;
                    }
                    charactersGroup->maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    pos+=2*4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    charactersGroupIndex++;
                }
            }
            postReplyData(queryNumber,reinterpret_cast<char *>(EpollClientLoginMaster::replyToRegisterLoginServer),pos);
        }
        EpollClientLoginMaster::loginServers << this;

        std::cout << "Online: " << loginServers.size() << " login server and " << gameServers.size() << " game server" << std::endl;
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginMaster::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat==EpollClientLoginMasterStat::None)
    {
        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()");
        return;
    }
    switch(mainCodeType)
    {
        case 0x02:
            if(stat!=EpollClientLoginMasterStat::LoginServer)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::LoginServer: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery(): "+QString::number(mainCodeType));
                return;
            }
            switch(subCodeType)
            {
                case 0x07:
                {
                    const quint8 &charactersGroupIndex=rawData[0];
                    const quint32 &serverUniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                    const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+5)));
                    const quint32 &accountId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+9)));
                    selectCharacter(queryNumber,serverUniqueKey,charactersGroupIndex,characterId,accountId);
                }
                break;
                default:
                    parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                break;
            }
        break;
        case 0x11:
            if(stat!=EpollClientLoginMasterStat::LoginServer)
            {
                parseNetworkReadError("stat!=EpollClientLoginMasterStat::LoginServer: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery(): "+QString::number(mainCodeType));
                return;
            }
            switch(subCodeType)
            {
                case 0x01:
                {
                    if(stat!=EpollClientLoginMasterStat::LoginServer)
                    {
                        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    if(!CommonSettingsCommon::commonSettingsCommon.automatic_account_creation)
                    {
                        parseNetworkReadError("!EpollClientLoginMaster::automatic_account_creation then why ask account id? EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(maxAccountId+1+index);
                        index++;
                    }
                    maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    replyOutputSize.remove(queryNumber);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x02:
                {
                    if(stat!=EpollClientLoginMasterStat::LoginServer)
                    {
                        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    if(size!=1)
                    {
                        parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    const unsigned char &charactersGroupIndex=*rawData;
                    if(charactersGroupIndex>=CharactersGroup::list.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxCharacterId+1+index);
                        index++;
                    }
                    charactersGroup->maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    replyOutputSize.remove(queryNumber);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x03:
                {
                    if(stat!=EpollClientLoginMasterStat::LoginServer)
                    {
                        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    if(size!=1)
                    {
                        parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    const unsigned char &charactersGroupIndex=*rawData;
                    if(charactersGroupIndex>=CharactersGroup::list.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxMonsterId+1+index);
                        index++;
                    }
                    charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    replyOutputSize.remove(queryNumber);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x07:
                {
                    if(stat!=EpollClientLoginMasterStat::GameServer)
                    {
                        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    if(size!=1)
                    {
                        parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    const unsigned char &charactersGroupIndex=*rawData;
                    if(charactersGroupIndex>=CharactersGroup::list.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxMonsterId+1+index);
                        index++;
                    }
                    charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    replyOutputSize.remove(queryNumber);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x08:
                {
                    if(stat!=EpollClientLoginMasterStat::GameServer)
                    {
                        parseNetworkReadError("stat==EpollClientLoginMasterStat::None: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    if(size!=1)
                    {
                        parseNetworkReadError("size!=1 EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    const unsigned char &charactersGroupIndex=*rawData;
                    if(charactersGroupIndex>=CharactersGroup::list.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::list.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxClanId+1+index);
                        index++;
                    }
                    charactersGroup->maxClanId+=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    replyOutputSize.remove(queryNumber);
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                default:
                    parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType)+" sub ident: "+QString::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void EpollClientLoginMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void EpollClientLoginMaster::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    //do the work here
    switch(mainCodeType)
    {
        case 0x81:
            switch(subCodeType)
            {
                case 0x01:
                {
                    if(stat!=EpollClientLoginMasterStat::GameServer)
                    {
                        parseNetworkReadError("stat!=EpollClientLoginMasterStat::GameServer: "+QString::number(stat)+" EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    //orderned mode drop: if(loginServerReturnForCharaterSelect.contains(queryNumber)), use first
                    {
                        const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=loginServerReturnForCharaterSelect.first();
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
                                charactersGroupForGameServerInformation->lockedAccount.remove(dataForSelectedCharacterReturn.characterId);
                            }
                            else
                            {
                                //account already locked
                                charactersGroupForGameServer->lockTheCharacter(dataForSelectedCharacterReturn.characterId);
                                charactersGroupForGameServerInformation->lockedAccount << dataForSelectedCharacterReturn.characterId;
                                qDebug() << "The master have not detected nothing but the game server " << charactersGroupForGameServerInformation->host << ":" << charactersGroupForGameServerInformation->port << " have reply than the account " << dataForSelectedCharacterReturn.characterId << " is already locked";
                            }
                            if(dataForSelectedCharacterReturn.loginServer!=NULL)
                                dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,data[0]);
                        }
                        else
                            parseNetworkReadError("main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType)+", reply size for 8101 wrong");
                        loginServerReturnForCharaterSelect.removeFirst();
                    }
                    /*orderned mode else
                        std::cerr << "parseFullReplyData() !loginServerReturnForCharaterSelect.contains(queryNumber): mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;*/
                }
                return;
                default:
                    parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void EpollClientLoginMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
