#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"
#include "../epoll/Epoll.h"

using namespace CatchChallenger;

void LinkToGameServer::parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size)
{
    Q_UNUSED(queryNumber);
    Q_UNUSED(size);
    Q_UNUSED(data);
    switch(mainCodeType)
    {
        case 0x03:
        {
            if(stat==Stat::Reconnecting)
            {
                //Protocol initialization
                if(size!=(sizeof(quint8)))
                {
                    parseNetworkReadError(QStringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                const quint8 &returnCode=data[0x00];
                if(returnCode>=0x04 && returnCode<=0x07)
                {
                    if(!LinkToGameServer::compressionSet)
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
                            case 0x07:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Lz4;
                            break;
                            default:
                                parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                    else
                    {
                        ProtocolParsing::CompressionType tempCompression;
                        switch(returnCode)
                        {
                            case 0x04:
                                tempCompression=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x05:
                                tempCompression=ProtocolParsing::CompressionType::Zlib;
                            break;
                            case 0x06:
                                tempCompression=ProtocolParsing::CompressionType::Xz;
                            break;
                            case 0x07:
                                tempCompression=ProtocolParsing::CompressionType::Lz4;
                            break;
                            default:
                                parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                        if(tempCompression!=ProtocolParsing::compressionTypeClient)
                        {
                            parseNetworkReadError(QStringLiteral("compression change main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                    }

                    //send token to game server
                    packFullOutcommingQuery(0x02,0x06,queryIdToReconnect/*query number*/,tokenForGameServer,sizeof(tokenForGameServer));
                    return;
                }
                else
                {
                    if(client!=NULL)
                        client->postReply(queryIdToReconnect,data,size);
                    if(returnCode==0x03)
                        parseNetworkReadError("Server full");
                    else
                        parseNetworkReadError(QStringLiteral("Unknown error %1").arg(returnCode));
                    return;
                }
            }
            else
            {
                //Protocol initialization
                if(size==(sizeof(quint8)))
                {
                    if(client!=NULL)
                    {
                        client->postReply(queryNumber,data,size);
                        if(data[0x00]==0x03)
                            parseNetworkReadError("Server full");
                        else
                            parseNetworkReadError(QStringLiteral("Unknown error %1").arg(data[0x00]));
                    }
                    else
                        parseNetworkReadError(QStringLiteral("size==(sizeof(quint8)) and client null with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                if(size!=(sizeof(quint8)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    parseNetworkReadError(QStringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                const quint8 &returnCode=data[0x00];
                if(returnCode>=0x04 && returnCode<=0x07)
                {
                    if(!LinkToGameServer::compressionSet)
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
                            case 0x07:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Lz4;
                            break;
                            default:
                                parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                    else
                    {
                        ProtocolParsing::CompressionType tempCompression;
                        switch(returnCode)
                        {
                            case 0x04:
                                tempCompression=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x05:
                                tempCompression=ProtocolParsing::CompressionType::Zlib;
                            break;
                            case 0x06:
                                tempCompression=ProtocolParsing::CompressionType::Xz;
                            break;
                            case 0x07:
                                tempCompression=ProtocolParsing::CompressionType::Lz4;
                            break;
                            default:
                                parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                        if(tempCompression!=ProtocolParsing::compressionTypeClient)
                        {
                            parseNetworkReadError(QStringLiteral("compression change main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                            return;
                        }
                    }
                    char newData[size];
                    //the gateway can different compression than server connected
                    switch(ProtocolParsing::compressionTypeServer)
                    {
                        case ProtocolParsing::CompressionType::None:
                            newData[0x00]=0x04;
                        break;
                        case ProtocolParsing::CompressionType::Zlib:
                            newData[0x00]=0x05;
                        break;
                        case ProtocolParsing::CompressionType::Xz:
                            newData[0x00]=0x06;
                        break;
                        case ProtocolParsing::CompressionType::Lz4:
                            newData[0x00]=0x07;
                        break;
                        default:
                            parseNetworkReadError(QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    qDebug() << "Transmit the token: " << QString(QByteArray(data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).toHex());
                    memcpy(newData+1,data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    //send token to game server
                    if(client!=NULL)
                        client->postReply(queryNumber,newData,size);
                    else
                    {
                        parseNetworkReadError("Protocol initialised but client already disconnected");
                        return;
                    }
                    stat=ProtocolGood;
                    return;
                }
                else
                {
                    parseNetworkReadError(QStringLiteral("Unknown error %1").arg(returnCode));
                    return;
                }
            }
        }
        break;
        default:
            parseNetworkReadError(QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
}

void LinkToGameServer::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    if(stat!=Stat::ProtocolGood)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType));
        return;
    }
    (void)data;
    (void)size;
    client->packOutcommingData(mainCodeType,data,size);
}

void LinkToGameServer::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const rawData,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    if(stat!=Stat::ProtocolGood)
    {
        if(mainCodeType==0xC2 && subCodeType==0x0F)//send Logical group
        {}
        else if(mainCodeType==0xC2 && subCodeType==0x0E)//send Send server list to real player
        {
            if(size>0)
            {
                switch(rawData[0x00])
                {
                    case 0x01:
                    break;
                    case 0x02:
                        gameServerMode=GameServerMode::Proxy;
                    return;
                    default:
                        parseNetworkReadError("parseFullMessage() wrong server list mode: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                    return;
                }
                char newOutputBuffer[size];
                newOutputBuffer[0x00]=0x02;
                unsigned int posOutputBuffer=1;

                unsigned int pos=1;
                if((size-pos)<1)
                {
                    parseNetworkReadError("parseFullMessage() missing data for server list size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                    return;
                }
                const quint8 &serverListSize=rawData[pos];
                pos+=1;
                quint8 serverListIndex=0;
                while(serverListIndex<serverListSize)
                {
                    if((size-pos)<(1+4+1))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server unique key size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    //charactersgroup
                    const quint8 &charactersGroupIndex=rawData[pos];
                    newOutputBuffer[posOutputBuffer]=rawData[pos];
                    posOutputBuffer+=1;
                    pos+=1;
                    //unique key
                    const quint32 &uniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+pos)));
                    memcpy(newOutputBuffer+posOutputBuffer,rawData+pos,4);
                    posOutputBuffer+=4;
                    pos+=4;
                    ServerReconnect serverReconnect;
                    //host
                    {
                        const quint8 &stringSize=rawData[pos];
                        pos+=1;
                        if((size-pos)<stringSize)
                        {
                            parseNetworkReadError("parseFullMessage() missing data for server host string size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                            return;
                        }
                        serverReconnect.host=QString::fromUtf8(rawData,stringSize);
                        if(serverReconnect.host.isEmpty())
                        {
                            parseNetworkReadError("parseFullMessage() server list, host can't be empty: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                            return;
                        }
                        pos+=stringSize;
                    }
                    if((size-pos)<(2+2))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server port start description size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    //port
                    serverReconnect.port=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                    pos+=2;
                    //skip description
                    {
                        const quint16 &stringSize=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                        pos+=2;
                        if((size-pos)<stringSize)
                        {
                            parseNetworkReadError("parseFullMessage() missing data for server host string size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                            return;
                        }
                        memcpy(newOutputBuffer+posOutputBuffer,rawData+pos-2,2+stringSize);
                        posOutputBuffer+=2+stringSize;

                        pos+=stringSize;
                    }
                    //skip Logical group and max player
                    if((size-pos)<(1+2))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server port start description size: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                        return;
                    }
                    memcpy(newOutputBuffer+posOutputBuffer,rawData+pos,1+2);
                    posOutputBuffer+=1+2;

                    pos+=1+2;

                    serverReconnectList[charactersGroupIndex][uniqueKey]=serverReconnect;
                    serverListIndex++;
                }

                //Second list part with same size
                memcpy(newOutputBuffer+posOutputBuffer,rawData+pos,2*serverListSize);
                posOutputBuffer+=2*serverListSize;

                gameServerMode=GameServerMode::Reconnect;

                client->packFullOutcommingData(mainCodeType,subCodeType,newOutputBuffer,posOutputBuffer);
                return;
            }
        }
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
            return;
        }
    }
    switch(mainCodeType)
    {
        case 0xC2:
        {
            switch(subCodeType)
            {
                //file as input
                case 0x03://raw
                case 0x04://compressed
                {
                    unsigned int pos=0;
                    if((size-pos)<(int)(sizeof(quint8)))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 fileListSize=rawData[pos];
                    pos++;
                    int index=0;
                    while(index<fileListSize)
                    {
                        if((size-pos)<(int)(sizeof(quint8)))
                        {
                            parseNetworkReadError(QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        QString fileName;
                        quint8 fileNameSize=rawData[pos];
                        pos++;
                        if(fileNameSize>0)
                        {
                            if((size-pos)<(int)fileNameSize)
                            {
                                parseNetworkReadError(QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            fileName=QString::fromUtf8(rawData+pos,fileNameSize);
                            pos+=fileNameSize;
                        }
                        if(!DatapackDownloaderBase::extensionAllowed.contains(QFileInfo(fileName).suffix()))
                        {
                            parseNetworkReadError(QStringLiteral("extension not allowed: %4 with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(QFileInfo(fileName).suffix()));
                            return;
                        }
                        if((size-pos)<(int)(sizeof(quint32)))
                        {
                            parseNetworkReadError(QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        const quint32 &fileSize=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+pos)));
                        pos+=4;
                        if((size-pos)<fileSize)
                        {
                            parseNetworkReadError(QStringLiteral("wrong file data size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        const QByteArray dataFile(rawData+pos,fileSize);
                        pos+=fileSize;
                        if(subCodeType==0x03)
                            DebugClass::debugConsole(QStringLiteral("Raw file to create: %1").arg(fileName));
                        else
                            DebugClass::debugConsole(QStringLiteral("Compressed file to create: %1").arg(fileName));

                        if(reply04inWait!=NULL)
                            DatapackDownloaderBase::datapackDownloaderBase->writeNewFileBase(fileName,dataFile);
                        else if(reply0205inWait!=NULL)
                        {
                            if(DatapackDownloaderMainSub::datapackDownloaderMainSub.contains(main))
                            {
                                if(DatapackDownloaderMainSub::datapackDownloaderMainSub.value(main).contains(sub))
                                    DatapackDownloaderMainSub::datapackDownloaderMainSub.value(main).value(sub)->writeNewFileToRoute(fileName,dataFile);
                                else
                                    parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return, sub datapack code is not found: "+sub);
                            }
                            else
                                parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return, main datapack code is not found: "+main);
                        }
                        else
                            parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return");

                        index++;
                    }
                    return;//no remaining data, because all remaing is used as file data
                }
                break;
                default:
                break;
            }
        }
        break;
        default:
        break;
    }
    client->packFullOutcommingData(mainCodeType,subCodeType,rawData,size);
}

//have query with reply
void LinkToGameServer::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    Q_UNUSED(data);
    if(stat!=Stat::ProtocolGood)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    client->packOutcommingQuery(mainCodeType,queryNumber,data,size);
}

void LinkToGameServer::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const rawData,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    //intercept the file sending
    if(mainCodeType==0x02 && subCodeType==0x0C)
    {
        if(reply04inWait!=NULL)
            DatapackDownloaderBase::datapackDownloaderBase->datapackFileList(rawData,size);
        else if(reply0205inWait==NULL)
        {
            if(size>39 && rawData[0x00]==0x01)//all is good, change the reply
            {
                unsigned int pos=39;

                {
                    if((size-pos)<1)
                    {
                        delete reply0205inWait;
                        reply0205inWait=NULL;
                        parseNetworkReadError("need more size");
                        return;
                    }
                    const quint8 &stringSize=rawData[pos];
                    pos+=1;
                    if((size-pos)<stringSize)
                    {
                        delete reply0205inWait;
                        reply0205inWait=NULL;
                        parseNetworkReadError("need more size");
                        return;
                    }
                    main=QString::fromLatin1(rawData+pos,stringSize);
                    pos+=stringSize;
                }
                {
                    if((size-pos)<1)
                    {
                        delete reply0205inWait;
                        reply0205inWait=NULL;
                        parseNetworkReadError("need more size");
                        return;
                    }
                    const quint8 &stringSize=rawData[pos];
                    pos+=1;
                    if((size-pos)<stringSize)
                    {
                        delete reply0205inWait;
                        reply0205inWait=NULL;
                        parseNetworkReadError("need more size");
                        return;
                    }
                    sub=QString::fromLatin1(rawData+pos,stringSize);
                    pos+=stringSize;
                }

                DatapackDownloaderMainSub *downloader=NULL;
                {
                    if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.contains(main))
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    else if(!DatapackDownloaderMainSub::datapackDownloaderMainSub.value(main).contains(sub))
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.value(main).value(sub);
                }
                if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    delete reply0205inWait;
                    reply0205inWait=NULL;
                    parseNetworkReadError("need more size");
                    return;
                }
                downloader->sendedHashMain=QByteArray(rawData+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
                pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
                if(!sub.isEmpty())
                {
                    if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
                    {
                        delete reply0205inWait;
                        reply0205inWait=NULL;
                        parseNetworkReadError("need more size");
                        return;
                    }
                    downloader->sendedHashSub=QByteArray(rawData+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
                    pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
                }
                if((size-pos)<1)
                {
                    delete reply0205inWait;
                    reply0205inWait=NULL;
                    parseNetworkReadError("need more size");
                    return;
                }
                const quint16 startString=pos;
                const quint8 &stringSize=rawData[pos];
                pos+=1;
                if((size-pos)<stringSize)
                {
                    delete reply0205inWait;
                    reply0205inWait=NULL;
                    parseNetworkReadError("need more size");
                    return;
                }
                CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=QString::fromLatin1(rawData+pos,stringSize);
                pos+=stringSize;
                unsigned int remainingSize=size-pos;
                reply0205inWaitSize=startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size()+remainingSize;
                reply0205inWait=new char[reply0205inWaitSize];
                memcpy(reply0205inWait+0,rawData,startString);
                memcpy(reply0205inWait+startString,LinkToGameServer::httpDatapackMirrorRewriteBase.constData(),LinkToGameServer::httpDatapackMirrorRewriteBase.size());
                memcpy(reply0205inWait+startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size(),rawData+pos,remainingSize);

                if(downloader->hashMain.isEmpty() || (!sub.isEmpty() && downloader->hashSub.isEmpty()))//checksum never done
                {
                    reply0205inWaitQueryNumber=queryNumber;
                    downloader->clientInSuspend.push_back(this);
                    if(downloader->clientInSuspend.size()==1)
                        downloader->sendDatapackContentMainSub();
                }
                else if(downloader->hashMain!=downloader->sendedHashMain || (!sub.isEmpty() && downloader->hashSub!=downloader->sendedHashSub))//need download the datapack content
                {
                    reply0205inWaitQueryNumber=queryNumber;
                    downloader->clientInSuspend.push_back(this);
                    if(downloader->clientInSuspend.size()==1)
                        downloader->sendDatapackContentMainSub();
                }
                else
                {
                    client->postReply(queryNumber,reply0205inWait,reply0205inWaitSize);
                    delete reply0205inWait;
                    reply0205inWait=NULL;
                    reply0205inWaitSize=0;
                }
                return;
            }
        }
        else
            parseNetworkReadError("unable to route mainCodeType==0x02 && subCodeType==0x0C return");
        return;
    }
    //do the work here
    client->packFullOutcommingQuery(mainCodeType,subCodeType,queryNumber,rawData,size);
}

//send reply
void LinkToGameServer::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    if(mainCodeType==0x03 && stat==Stat::Connected)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here

    /* intercept part here */
    if(mainCodeType==0x04)
    {
        if(size>(14+CATCHCHALLENGER_SHA224HASH_SIZE) && data[0x00]==0x01)//all is good, change the reply
        {
            unsigned int pos=14;
            if(reply04inWait!=NULL)
            {
                delete reply04inWait;
                reply04inWait=NULL;
                parseNetworkReadError("another reply04inWait in suspend");
                return;
            }
            {
                if(DatapackDownloaderBase::datapackDownloaderBase==NULL)
                    DatapackDownloaderBase::datapackDownloaderBase=new DatapackDownloaderBase(LinkToGameServer::mDatapackBase);
            }
            DatapackDownloaderBase::datapackDownloaderBase->sendedHashBase=QByteArray(data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
            pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
            if((size-pos)<1)
            {
                delete reply04inWait;
                reply04inWait=NULL;
                parseNetworkReadError("need more size");
                return;
            }
            const quint16 startString=pos;
            const quint8 &stringSize=data[pos];
            pos+=1;
            if((size-pos)<stringSize)
            {
                delete reply04inWait;
                reply04inWait=NULL;
                parseNetworkReadError("need more size");
                return;
            }
            CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=QString::fromLatin1(data+pos,stringSize);
            pos+=stringSize;
            unsigned int remainingSize=size-pos;
            if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
            {
                delete reply0205inWait;
                reply0205inWait=NULL;
                parseNetworkReadError("need more size");
                return;
            }
            reply04inWaitSize=startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size()+remainingSize;
            reply04inWait=new char[reply04inWaitSize];
            memcpy(reply04inWait+0,data,startString);
            memcpy(reply04inWait+startString,LinkToGameServer::httpDatapackMirrorRewriteBase.constData(),LinkToGameServer::httpDatapackMirrorRewriteBase.size());
            memcpy(reply04inWait+startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size(),data+pos,remainingSize);

            if(DatapackDownloaderBase::datapackDownloaderBase->hashBase.isEmpty())//checksum never done
            {
                reply04inWaitQueryNumber=queryNumber;
                DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.push_back(this);
                if(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.size()==1)
                    DatapackDownloaderBase::datapackDownloaderBase->sendDatapackContentBase();
            }
            else if(DatapackDownloaderBase::datapackDownloaderBase->hashBase!=DatapackDownloaderBase::datapackDownloaderBase->sendedHashBase)//need download the datapack content
            {
                reply04inWaitQueryNumber=queryNumber;
                DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.push_back(this);
                if(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.size()==1)
                    DatapackDownloaderBase::datapackDownloaderBase->sendDatapackContentBase();
            }
            else
            {
                client->postReply(queryNumber,reply04inWait,reply04inWaitSize);
                delete reply04inWait;
                reply04inWait=NULL;
                reply04inWaitSize=0;
            }
            return;
        }
    }

    client->postReply(queryNumber,data,size);
}

void LinkToGameServer::parseFullReplyData(const quint8 &mainCodeType, const quint8 &subCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return;
    }
    (void)mainCodeType;
    (void)subCodeType;
    (void)data;
    (void)size;
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
            switch(subCodeType)
            {
                case 0x05:
                if(gameServerMode==GameServerMode::Reconnect)
                {
                    if(selectedServer.host.isEmpty())
                    {
                        parseNetworkReadError("Reply of 0205 with empty host");
                        return;
                    }
                    if(size!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                        break;

                    const int &socketFd=LinkToGameServer::tryConnect(selectedServer.host.toLatin1(),selectedServer.port,5,1);
                    if(Q_LIKELY(socketFd>=0))
                    {
                        epollSocket.reopen(socketFd);
                        this->socketFd=socketFd;

                        epoll_event event;
                        event.data.ptr = this;
                        event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                        {
                            const int &s = Epoll::epoll.ctl(EPOLL_CTL_ADD, socketFd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                disconnectClient();
                                return;
                            }
                        }

                        queryIdToReconnect=queryNumber;
                        stat=Stat::Reconnecting;
                        memcpy(tokenForGameServer,data,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                        //send the protocol
                        //wait readTheFirstSslHeader() to sendProtocolHeader();
                        haveTheFirstSslHeader=false;
                        setConnexionSettings();
                        parseIncommingData();
                    }
                    else
                    {
                        static_cast<EpollClientLoginSlave * const>(client)
                        ->parseNetworkReadError(QStringLiteral("not able to connect on the game server as proxy, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
                    }
                    selectedServer.host.clear();
                }
                break;
                default:
                break;
            }
        break;
        default:
        break;
    }

    client->postReply(queryNumber,data,size);
}

void LinkToGameServer::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
