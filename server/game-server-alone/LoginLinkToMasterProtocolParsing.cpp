#include "LoginLinkToMaster.h"
#include "../base/Client.h"
#include "../base/GlobalServerData.h"
#include <iostream>

using namespace CatchChallenger;

void LoginLinkToMaster::parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char *data, const unsigned int &size)
{
    Q_UNUSED(queryNumber);
    Q_UNUSED(size);
    Q_UNUSED(data);
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void LoginLinkToMaster::parseMessage(const quint8 &mainCodeType,const char *data,const unsigned int &size)
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

void LoginLinkToMaster::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char *rawData,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
        return;
    }
    (void)rawData;
    (void)size;
    switch(mainCodeType)
    {
        case 0xF1:
            switch(subCodeType)
            {
                default:
                    parseNetworkReadError("unknown sub ident: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
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
void LoginLinkToMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::Logged)
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

void LoginLinkToMaster::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        case 0x81:
            switch(subCodeType)
            {
                //query because need wait the return (sync/async problem) to send the token for the client connect
                //check if the characterId is linked to the correct account on login server
                case 0x01:
                    if(Q_UNLIKELY(size!=(4+4)))
                    {
                        parseNetworkReadError("unknown sub ident: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    else
                    {
                        const char * const token=Client::addAuthGetToken(
                                    le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData))),
                                    le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+4)))
                                    );
                        if(token!=NULL)
                        {
                            memcpy(LoginLinkToMaster::protocolReplyGetToken+0x03,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                            internalSendRawSmallPacket(LoginLinkToMaster::protocolReplyNoMoreToken,sizeof(LoginLinkToMaster::protocolReplyNoMoreToken));
                        }
                        else
                        {
                            LoginLinkToMaster::protocolReplyNoMoreToken[0x01]=queryNumber;
                            internalSendRawSmallPacket(LoginLinkToMaster::protocolReplyNoMoreToken,sizeof(LoginLinkToMaster::protocolReplyNoMoreToken));
                        }
                    }
                break;
                default:
                    parseNetworkReadError("unknown sub ident: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
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
void LoginLinkToMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    switch(mainCodeType)
    {
        case 0x01:
        {
            //Protocol initialization
            const quint8 &returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x05:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zlib;
                    break;
                    case 0x06:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Xz;
                    break;
                    default:
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return;
                }
                stat=Stat::ProtocolGood;
                registerGameServer(CommonSettingsServer::commonSettingsServer.exportedXml);
                return;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else if(returnCode==0x07)
                    std::cerr << "Token auth wrong" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return;
            }
        }
        break;
        case 0x07:
        {
            if(size<1)
            {
                std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            unsigned int pos=0;
            switch(data[0x00])
            {
                case 0x02:
                if((size-pos)<4)
                {
                    std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                {
                    const quint32 &uniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+pos)));
                    pos+=4;
                    settings->setValue(QLatin1Literal("uniqueKey"),uniqueKey);
                }
                case 0x01:
                {
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        GlobalServerData::serverPrivateVariables.maxMonsterId.push_back(le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        GlobalServerData::serverPrivateVariables.maxClanId.push_back(le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                }
                return;
                case 0x03:
                    std::cerr << "charactersGroup not found" << settings->value(QLatin1Literal("charactersGroup")).toString().toStdString() << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
                default:
                    std::cerr << "reply return code error (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
            }

            stat=Stat::Logged;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LoginLinkToMaster::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        std::cerr << "parseFullReplyData() reply to unknown query: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
        abort();
    }
    (void)data;
    (void)size;
    queryNumberList.push_back(queryNumber);
    //do the work here
    switch(mainCodeType)
    {
        case 0x11:
        switch(subCodeType)
        {
            case 0x07:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4)
            {
                std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                abort();
            }
            #endif
            {
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    GlobalServerData::serverPrivateVariables.maxMonsterId.push_back(
                                le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+index*sizeof(unsigned int))))
                                );
                    index++;
                }
            }
            break;
            case 0x08:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4)
            {
                std::cerr << "parseFullReplyData() size!=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK*4: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                abort();
            }
            #endif
            {
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
                {
                    GlobalServerData::serverPrivateVariables.maxClanId.push_back(
                                le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+index*sizeof(unsigned int))))
                                );
                    index++;
                }
            }
            break;
            default:
                std::cerr << "mainCodeType: " << mainCodeType << ", unknown subCodeType: " << subCodeType << " " << __FILE__ << ":" <<__LINE__ << std::endl;
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

void LoginLinkToMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
