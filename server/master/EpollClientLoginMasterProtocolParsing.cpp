#include "EpollClientLoginMaster.h"

#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
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

void EpollClientLoginMaster::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
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

void EpollClientLoginMaster::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *rawData,const int &size)
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
void EpollClientLoginMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
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
            EpollClientLoginMaster::replyToRegisterLoginServer[0x01]=queryNumber;
            int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+10+(CATCHCHALLENGER_SERVER_MAXIDBLOCK*0+index)*4/*size of int*/)=(quint32)htobe32(maxAccountId+1+index);
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+10+(CATCHCHALLENGER_SERVER_MAXIDBLOCK*1+index)*4/*size of int*/)=(quint32)htobe32(maxCharacterId+1+index);
                *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToRegisterLoginServer+10+(CATCHCHALLENGER_SERVER_MAXIDBLOCK*2+index)*4/*size of int*/)=(quint32)htobe32(maxMonsterId+1+index);
                index++;
            }
            maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
            internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToRegisterLoginServer),sizeof(EpollClientLoginMaster::replyToRegisterLoginServer));
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginMaster::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
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
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htobe32(maxAccountId+1+index);
                        index++;
                    }
                    maxAccountId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x02:
                {
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htobe32(maxCharacterId+1+index);
                        index++;
                    }
                    maxCharacterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x03:
                {
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htobe32(maxMonsterId+1+index);
                        index++;
                    }
                    maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::replyToIdListBuffer),sizeof(EpollClientLoginMaster::replyToIdListBuffer));
                }
                break;
                case 0x04:
                {
                    EpollClientLoginMaster::replyToIdListBuffer[0x01]=queryNumber;
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        *reinterpret_cast<quint32 *>(EpollClientLoginMaster::replyToIdListBuffer+2+index*4/*size of int*/)=(quint32)htobe32(maxClanId+1+index);
                        index++;
                    }
                    maxClanId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
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
void EpollClientLoginMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void EpollClientLoginMaster::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
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
