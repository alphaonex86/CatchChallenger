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
    switch(mainCodeType)
    {
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
