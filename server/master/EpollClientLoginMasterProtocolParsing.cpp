#include "EpollClientLoginMaster.h"

#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x01:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(queryNumber);
            #endif
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
                    errorParsingLayer("Wrong protocol");
                    return;
                }
            }
            else
            {
                *(EpollClientLoginMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginMaster::protocolReplyProtocolNotSupported));
                errorParsingLayer("Wrong protocol");
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

void EpollClientLoginMaster::parseMessage(const quint8 &mainCodeType,const char * const data,const int &size)
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

void EpollClientLoginMaster::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char * const rawData,const int &size)
{
    (void)rawData;
    (void)size;
    (void)subCodeType;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void EpollClientLoginMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    Q_UNUSED(data);
    if(stat==EpollClientLoginMasterStat::None)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
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
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::loginPreviousToReplyCache),sizeof(EpollClientLoginMaster::loginPreviousToReplyCacheSize));
            //send the id list
            EpollClientLoginMaster::replyToRegisterLoginServer[0x01]=queryNumber;
            int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+EpollClientLoginMaster::replyToRegisterLoginServerBaseOffset+(CATCHCHALLENGER_SERVER_MAXIDBLOCK*0+index)*4/*size of int*/)=(quint32)htole32(maxAccountId+1+index);
                index++;
            }
            maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            {
                int charactersGroupIndex=0;
                while(charactersGroupIndex<CharactersGroup::charactersGroupList.size())
                {
                    CharactersGroup *charactersGroup=CharactersGroup::charactersGroupList.at(charactersGroupIndex);
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
                    charactersGroupIndex++;
                }
            }
            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToRegisterLoginServer),sizeof(EpollClientLoginMaster::replyToRegisterLoginServer));
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginMaster::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char * const rawData,const int &size)
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
                    if(!EpollClientLoginMaster::automatic_account_creation)
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
                    if(charactersGroupIndex>=CharactersGroup::charactersGroupList.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::charactersGroupList.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxCharacterId+1+index);
                        index++;
                    }
                    charactersGroup->maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
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
                    if(charactersGroupIndex>=CharactersGroup::charactersGroupList.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::charactersGroupList.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxMonsterId+1+index);
                        index++;
                    }
                    charactersGroup->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x04:
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
                    if(charactersGroupIndex>=CharactersGroup::charactersGroupList.size())
                    {
                        parseNetworkReadError("charactersGroupIndex>=CharactersGroup::charactersGroupList.size() EpollClientLoginMaster::parseFullQuery()"+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    CharactersGroup * const charactersGroup=CharactersGroup::charactersGroupList.at(charactersGroupIndex);
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htole32(charactersGroup->maxClanId+1+index);
                        index++;
                    }
                    charactersGroup->maxClanId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                default:
                    parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
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
void EpollClientLoginMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void EpollClientLoginMaster::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char * const data,const int &size)
{
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void EpollClientLoginMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
