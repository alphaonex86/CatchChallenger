#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "../base/BaseServerLogin.h"
#include "../../general/base/CommonSettingsCommon.h"

#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginSlave::doDDOSComputeAll()
{
    unsigned int index=0;
    while(index<client_list.size())
    {
        client_list.at(index)->doDDOSCompute();
        index++;
    }
}

void EpollClientLoginSlave::doDDOSCompute()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE<0 || CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE>CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        qDebug() << "CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE out of range:" << CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        return;
    }
    #endif
    {
        movePacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
            {
                parseNetworkReadError("index out of range <0, movePacketKick");
                return;
            }
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
            {
                parseNetworkReadError("index out of range >, movePacketKick");
                return;
            }
            if(movePacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITMOVE*2)
            {
                parseNetworkReadError("index out of range in array for index "+std::to_string(movePacketKick[index])+", movePacketKick");
                return;
            }
            #endif
            movePacketKick[index]=movePacketKick[index+1];
            movePacketKickTotalCache+=movePacketKick[index];
            index++;
        }
        movePacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=movePacketKickNewValue;
        movePacketKickTotalCache+=movePacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(movePacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITMOVE*2)
        {
            parseNetworkReadError("bug in DDOS calculation count");
            return;
        }
        #endif
        movePacketKickNewValue=0;
    }
    {
        chatPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, chatPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, chatPacketKick";
            if(chatPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
                qDebug() << "index out of range in array for index " << chatPacketKick[index] << ", chatPacketKick";
            #endif
            chatPacketKick[index]=chatPacketKick[index+1];
            chatPacketKickTotalCache+=chatPacketKick[index];
            index++;
        }
        chatPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=chatPacketKickNewValue;
        chatPacketKickTotalCache+=chatPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(chatPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITCHAT*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
        chatPacketKickNewValue=0;
    }
    {
        otherPacketKickTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(index<0)
                qDebug() << "index out of range <0, otherPacketKick";
            if((index+1)>=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
                qDebug() << "index out of range >, otherPacketKick";
            if(otherPacketKick[index]>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
                qDebug() << "index out of range in array for index " << otherPacketKick[index] << ", chatPacketKick";
            #endif
            otherPacketKick[index]=otherPacketKick[index+1];
            otherPacketKickTotalCache+=otherPacketKick[index];
            index++;
        }
        otherPacketKick[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=otherPacketKickNewValue;
        otherPacketKickTotalCache+=otherPacketKickNewValue;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(otherPacketKickTotalCache>CATCHCHALLENGER_DDOS_KICKLIMITOTHER*2)
            qDebug() << "bug in DDOS calculation count";
        #endif
        otherPacketKickNewValue=0;
    }
}

bool EpollClientLoginSlave::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0xA0:
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                stat=EpollClientLoginStat::ProtocolGood;

                stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                /// \todo do the async connect
                /// linkToGameServer->stat=Stat::Connecting;
                const int &socketFd=LinkToGameServer::tryConnect(EpollServerLoginSlave::epollServerLoginSlave->destination_server_ip,EpollServerLoginSlave::epollServerLoginSlave->destination_server_port,5,1);
                if(Q_LIKELY(socketFd>=0))
                {
                    stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                    this->linkToGameServer=linkToGameServer;
                    linkToGameServer->stat=LinkToGameServer::Stat::Connected;
                    linkToGameServer->client=this;
                    linkToGameServer->protocolQueryNumber=queryNumber;
                    //send the protocol
                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                    linkToGameServer->setConnexionSettings();
                    linkToGameServer->parseIncommingData();
                }
                else
                {
                    parseNetworkReadError("not able to connect on the game server as gateway, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
                    return false;
                }

                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                std::cout << "Protocol sended and replied" << std::endl;
                return true;
                #endif
            }
            else
            {
                parseNetworkReadError("Wrong protocol");
                return false;
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType)+", stat: "+std::to_string(stat));
            return false;
        break;
    }
    return true;
}

bool EpollClientLoginSlave::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if(linkToGameServer==NULL)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }
    switch(mainCodeType)
    {
        case 0x02:
            if((movePacketKickTotalCache+movePacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITMOVE)
            {
                parseNetworkReadError("Too many move in sort time, check DDOS limit");
                return false;
            }
            movePacketKickNewValue++;
        break;
        case 0x03:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITCHAT)
            {
                parseNetworkReadError("Too many chat in sort time, check DDOS limit");
                return false;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
            {
                parseNetworkReadError("Too many packet in sort time, check DDOS limit");
                return false;
            }
            otherPacketKickNewValue++;
        break;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size
        posOutput+=1+4;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
        posOutput+=size;

        linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return true;
    }
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
}

//have query with reply
bool EpollClientLoginSlave::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    Q_UNUSED(data);
    if(linkToGameServer==NULL && mainCodeType!=0xA0)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }



    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        switch(mainCodeType)
        {
            //Select the character
            case 0xAC:
            if(linkToGameServer->gameServerMode==LinkToGameServer::GameServerMode::Reconnect)
            {
                if(size!=(1+4+4))
                {
                    parseNetworkReadError("Select character: wrong size");
                    return false;
                }
                const quint8 &charactersGroupIndex=data[0x00];
                const quint32 &uniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(data+1)));
                if(linkToGameServer->serverReconnectList.find(charactersGroupIndex)==linkToGameServer->serverReconnectList.cend())
                {
                    parseNetworkReadError("Select character: charactersGroupIndex not found");
                    return false;
                }
                if(linkToGameServer->serverReconnectList.at(charactersGroupIndex).find(uniqueKey)==linkToGameServer->serverReconnectList.at(charactersGroupIndex).cend())
                {
                    parseNetworkReadError("Select character: unique key not found");
                    return false;
                }
                linkToGameServer->selectedServer=linkToGameServer->serverReconnectList.at(charactersGroupIndex).at(uniqueKey);
                linkToGameServer->serverReconnectList.clear();
                return true;
            }
            break;
            //Send datapack file list
            case 0xA1:
            {
                #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
                switch(datapackStatus)
                {
                    case DatapackStatus::Base:
                        if(LinkToGameServer::httpDatapackMirrorRewriteBase.size()>1)
                        {
                            if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()>1)
                            {
                                parseNetworkReadError("Can't use because mirror is defined");
                                return false;
                            }
                            else
                                datapackStatus=DatapackStatus::Main;
                        }
                    break;
                    case DatapackStatus::Sub:
                        if(linkToGameServer->sub.isEmpty())
                        {
                            parseNetworkReadError("linkToGameServer->sub.isEmpty()");
                            return false;
                        }
                    case DatapackStatus::Main:
                        if(LinkToGameServer::httpDatapackMirrorRewriteMainAndSub.size()>1)
                        {
                            parseNetworkReadError("Can't use because mirror is defined");
                            return false;
                        }
                    break;
                    default:
                        parseNetworkReadError("Double datapack send list");
                    return false;
                }

                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);

                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 stepToSkip;
                in >> stepToSkip;
                switch(stepToSkip)
                {
                    case 0x01:
                        if(datapackStatus==DatapackStatus::Base)
                        {}
                        else
                        {
                            parseNetworkReadError(QStringLiteral("step out of range to get datapack base but already in highter level"));
                            return;
                        }
                    break;
                    case 0x02:
                        if(datapackStatus==DatapackStatus::Base)
                            datapackStatus=DatapackStatus::Main;
                        else if(datapackStatus==DatapackStatus::Main)
                        {}
                        else
                        {
                            parseNetworkReadError(QStringLiteral("step out of range to get datapack base but already in highter level"));
                            return;
                        }
                    break;
                    case 0x03:
                        if(datapackStatus==DatapackStatus::Base)
                            datapackStatus=DatapackStatus::Sub;
                        else if(datapackStatus==DatapackStatus::Main)
                            datapackStatus=DatapackStatus::Sub;
                        else if(datapackStatus==DatapackStatus::Sub)
                        {}
                        else
                        {
                            parseNetworkReadError(QStringLiteral("step out of range to get datapack base but already in highter level"));
                            return;
                        }
                    break;
                    default:
                        parseNetworkReadError(QStringLiteral("step out of range to get datapack: %1").arg(stepToSkip));
                    return;
                }

                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 textSize;
                quint32 number_of_file;
                in >> number_of_file;
                QStringList files;
                QList<quint32> partialHashList;
                QString tempFileName;
                quint32 partialHash;
                quint32 index=0;
                while(index<number_of_file)
                {
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseNetworkReadError("wrong utf8 to QString size in PM for text size");
                            return;
                        }
                        in >> textSize;
                        //control the regex file into Client::datapackList()
                        if(textSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                            {
                                parseNetworkReadError(QStringLiteral("wrong utf8 to QString size for file name: parseQuery(%1,%2,%3): %4 %5")
                                           .arg(mainCodeType)
                                           .arg(subCodeType)
                                           .arg(queryNumber)
                                           .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                                           .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                                           );
                                return;
                            }
                            const QByteArray &rawText=data.mid(in.device()->pos(),textSize);
                            tempFileName=std::string(rawText.data(),rawText.size());
                            in.device()->seek(in.device()->pos()+rawText.size());
                        }
                    }
                    if((in.device()->size()-in.device()->pos())<1)
                    {
                        parseNetworkReadError(QStringLiteral("missing header utf8 datapack file list query"));
                        return;
                    }
                    files << tempFileName;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseNetworkReadError(QStringLiteral("wrong size for id with main ident: %1, subIdent: %2, remaining: %3, lower than: %4")
                            .arg(mainCodeType)
                            .arg(subCodeType)
                            .arg(in.device()->size()-in.device()->pos())
                            .arg((int)sizeof(quint32))
                            );
                        return;
                    }
                    in >> partialHash;
                    partialHashList << partialHash;
                    index++;
                }
                datapackList(queryNumber,files,partialHashList);
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
                               .arg(mainCodeType)
                               .arg(subCodeType)
                               .arg(queryNumber)
                               .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                               .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                               );
                    return;
                }
                return true;
                #else
                parseNetworkReadError("CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR");
                return false;
                #endif
            }
            break;
            default:
            break;
        }

        //send the network query
        linkToGameServer->registerOutputQuery(queryNumber);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
        posOutput+=size;

        return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }

    return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
}

//send reply
bool EpollClientLoginSlave::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat==EpollClientLoginStat::GameServerConnecting)
    {
        parseNetworkReadError("main ident while game server connecting: "+std::to_string(mainCodeType));
        return false;
    }
    if((otherPacketKickTotalCache+otherPacketKickNewValue)>=CATCHCHALLENGER_DDOS_KICKLIMITOTHER)
    {
        parseNetworkReadError("Too many packet in sort time, check DDOS limit");
        return false;
    }
    otherPacketKickNewValue++;
    if(linkToGameServer==NULL)
    {
        parseNetworkReadError("linkToGameServer==NULL");
        return false;
    }
    if(stat==EpollClientLoginStat::GameServerConnected)
    {
        if(Q_LIKELY(linkToGameServer))
        {
            //send the network reply
            removeFromQueryReceived(queryNumber);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
            posOutput+=size;

            return linkToGameServer->sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        }
        else
        {
            parseNetworkReadError("linkToGameServer==NULL when stat==EpollClientLoginStat::GameServerConnected main ident: "+std::to_string(mainCodeType));
            return false;
        }
    }
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError("The server for now not ask anything: "+std::to_string(mainCodeType)+", "+std::to_string(queryNumber));
    return false;
}

void EpollClientLoginSlave::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
