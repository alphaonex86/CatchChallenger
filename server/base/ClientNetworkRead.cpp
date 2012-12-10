#include "ClientNetworkRead.h"
#include "GlobalData.h"
#include "MapServer.h"

using namespace Pokecraft;

QRegExp ClientNetworkRead::commandRegExp=QRegExp("^/([a-z]+)( [^ ].*)?$");
QRegExp ClientNetworkRead::commandRegExpWithArgs=QRegExp("^/([a-z]+)( [^ ].*)$");

ClientNetworkRead::ClientNetworkRead(Player_internal_informations *player_informations,ConnectedSocket * socket) :
    ProtocolParsingInput(socket,PacketModeTransmission_Server)
{
    have_send_protocol=false;
    is_logging_in_progess=false;
    stopIt=false;
    this->player_informations=player_informations;
    this->socket=socket;

    queryNumberList.reserve(256);
    qDebug() << queryNumberList.size();
    int index=0;
    while(index<256)
    {
        queryNumberList << index;
        index++;
    }
}

void ClientNetworkRead::stopRead()
{
    stopIt=true;
//	if(socket)
//		disconnect(socket,SIGNAL(readyRead()),this,SLOT(readyRead()));
}

void ClientNetworkRead::fake_send_protocol()
{
    have_send_protocol=true;
}

void ClientNetworkRead::askIfIsReadyToStop()
{
    stopIt=true;
    emit isReadyToStop();
}

void ClientNetworkRead::teleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    if(queryNumberList.empty())
    {
        emit error(QString("Sorry, no free query number to send this query of teleportation"));
        return;
    }
    TeleportationPoint teleportationPoint;
    teleportationPoint.map=map;
    teleportationPoint.x=x;
    teleportationPoint.y=y;
    teleportationPoint.orientation=orientation;
    lastTeleportation << teleportationPoint;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(GlobalData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    out << (COORD_TYPE)x;
    out << (COORD_TYPE)y;
    out << (quint8)orientation;
    emit sendQuery(0x79,0x0001,queryNumberList.first(),outputData);
    queryNumberList.removeFirst();
}

void ClientNetworkRead::parseInputBeforeLogin(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("parseInputBeforeLogin(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            case 0x0001:
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    return;
                {
                    QString protocol;
                    in >> protocol;
                    if(GlobalData::serverPrivateVariables.connected_players>=GlobalData::serverSettings.max_players)
                    {
                        out << (quint8)0x03;		//server full
                        out << QString("Server full");
                        emit postReply(queryNumber,outputData);
                        emit error(QString("Server full (%1/%2)").arg(GlobalData::serverPrivateVariables.connected_players).arg(GlobalData::serverSettings.max_players));
                        return;
                    }
                    if(protocol==PROTOCOL_HEADER)
                    {
                        out << (quint8)0x01;		//protocol supported
                        emit postReply(queryNumber,outputData);
                        have_send_protocol=true;
                        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                        emit message("Protocol sended and replied");
                        #endif
                    }
                    else
                    {
                        out << (quint8)0x02;		//protocol not supported
                        emit postReply(queryNumber,outputData);
                        emit error("Wrong protocol");
                        return;
                    }
                }
            break;
            case 0x0002:
                if(!have_send_protocol)
                {
                    emit error("send login before the protocol");
                    return;
                }
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError("wrong size");
                    return;
                }
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    return;
                {
                    QString login;
                    in >> login;
                    if((data.size()-in.device()->pos())!=20)
                        parseError(QString("wrong size with the main ident: %1, because %2 != 20").arg(mainCodeType).arg(data.size()-in.device()->pos()));
                    else if(is_logging_in_progess)
                    {
                        out << (quint8)1;
                        emit postReply(queryNumber,outputData);
                        emit error("Loggin in progress");
                    }
                    else if(player_informations->is_logged)
                    {
                        out << (quint8)1;
                        emit postReply(queryNumber,outputData);
                        emit error("Already logged");
                    }
                    else
                    {
                        is_logging_in_progess=true;
                        QByteArray hash;
                        hash=data.right(data.size()-in.device()->pos());
                        emit askLogin(queryNumber,login,hash);
                    }
                    return;
                }
            break;
            default:
                parseError("wrong data before login with mainIdent: "+QString::number(mainCodeType)+", subIdent: "+QString::number(subCodeType));
            break;
        }
        break;
        default:
            parseError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QString("remaining data: parseInputBeforeLogin(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
}

void ClientNetworkRead::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!player_informations->is_logged)
    {
        parseError(QString("is not logged, parseMessage(%1)").arg(mainCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("parseMessage(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x40:
        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
        {
            parseError("Wrong size in move packet");
            return;
        }
        in >> previousMovedUnit;
        in >> direction;
        if(direction<1 || direction>8)
        {
            parseError(QString("Bad direction number: %1").arg(direction));
            return;
        }
        emit moveThePlayer(previousMovedUnit,(Direction)direction);
        break;
        default:
            parseError("unknow main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QString("remaining data: parseMessage(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void ClientNetworkRead::parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!player_informations->is_logged)
    {
        parseError(QString("is not logged, parseMessage(%1,%2)").arg(mainCodeType).arg(subCodeType));
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("parseMessage(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x42:
        switch(subCodeType)
        {
            //Chat
            case 0x0003:
            {
                if((data.size()-in.device()->pos())<((int)sizeof(quint8)))
                {
                    parseError("wrong remaining size for chat");
                    return;
                }
                quint8 chatType;
                in >> chatType;
                if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
                {
                    parseError("chat type error: "+QString::number(chatType));
                    return;
                }
                if(chatType==Chat_type_pm)
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString text;
                    in >> text;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString pseudo;
                    in >> pseudo;
                    emit message("/pm "+pseudo+" "+text);
                    emit sendPM(text,pseudo);
                }
                else
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        return;
                    QString text;
                    in >> text;

                    if(!text.startsWith('/'))
                    {
                        if(chatType==Chat_type_local)
                            emit sendLocalChatText(text);
                        else
                            emit sendChatText((Chat_type)chatType,text);
                    }
                    else
                    {
                        if(text.contains(commandRegExp))
                        {
                            //isolate the main command (the first word)
                            QString command=text;
                            command.replace(commandRegExp,"\\1");

                            //isolate the arguements
                            if(text.contains(commandRegExp))
                            {
                                text.replace(commandRegExp,"\\2");
                                text.replace(QRegExp("^ (.*)$"),"\\1");
                            }
                            else
                                text="";

                            //the normal player command
                            {
                                if(command=="playernumber")
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                    return;
                                }
                                if(command=="playerlist")
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                    return;
                                }
                            }
                            //the admin command
                            if(player_informations->public_and_private_informations.public_informations.type==Player_type_gm || player_informations->public_and_private_informations.public_informations.type==Player_type_dev)
                            {
                                if(command=="give")
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="take")
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="tp")
                                {
                                    emit sendHandlerCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="kick")
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="chat")
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="setrights")
                                {
                                    emit sendBroadCastCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="stop" || command=="restart")
                                {
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else if(command=="addbots" || command=="removebots")
                                {
                                    BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                                    emit message("send command: /"+command+" "+text);
                                }
                                else
                                {
                                    emit message("unknow send command: /"+command+" and \""+text+"\"");
                                    //emit sendChatText((Chat_type)chatType,text);
                                }
                            }
                            else
                                emit message("unknow send command: /"+command+" and \""+text+"\"");
                                //emit sendChatText((Chat_type)chatType,text);
                        }
                        else
                            emit message("commands seem not right: \""+text+"\"");
                    }
                }
                return;
            }
            break;
            default:
                parseError(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x50:
            switch(subCodeType)
            {
                //Destroy an object
                case 0x0002:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for destroy item id");
                        return;
                    }
                    quint32 itemId;
                    in >> itemId;
                    if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                    {
                        parseError("wrong remaining size for destroy quantity");
                        return;
                    }
                    quint32 quantity;
                    in >> quantity;
                    emit destroyObject(itemId,quantity);
                }
                break;
                default:
                    parseError(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseError("unknow main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QString("remaining data: parseMessage(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

//have query with reply
void ClientNetworkRead::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->is_logged)
    {
        parseError(QString("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    parseError(QString("no query with only the main code for now, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
    return;
}

void ClientNetworkRead::parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(stopIt)
        return;
    if(!player_informations->is_logged)
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            //Send datapack file list
            case 0x000C:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QString("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 number_of_file;
                in >> number_of_file;
                QStringList files;
                QList<quint64> timestamps;
                QString tempFileName;
                quint64 tempTimestamps;
                quint32 index=0;
                while(index<number_of_file)
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QString("error at datapack file list query"));
                        return;
                    }
                    in >> tempFileName;
                    files << tempFileName;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                    {
                        parseError(QString("wrong size for id with main ident: %1, subIdent: %2, remaining: %3, lower than: %4")
                            .arg(mainCodeType)
                            .arg(subCodeType)
                            .arg(in.device()->size()-in.device()->pos())
                            .arg((int)sizeof(quint32))
                            );
                        return;
                    }
                    in >> tempTimestamps;
                    timestamps << tempTimestamps;
                    index++;
                }
                emit datapackList(queryNumber,files,timestamps);
            }
            break;
            default:
                parseError(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        case 0x10:
        switch(subCodeType)
        {
            //Use seed into dirt
            case 0x0006:
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QString("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint8 plant_id;
                in >> plant_id;
                emit plantSeed(queryNumber,plant_id);
            }
            break;
            //Collect mature plant
            case 0x0007:
                emit collectPlant(queryNumber);
            break;
            //Usage of recipe
            case 0x0008:
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QString("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                quint32 recipe_id;
                in >> recipe_id;
                emit useRecipe(queryNumber,recipe_id);
            break;
            default:
                parseError(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseError("unknow main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QString("remaining data: parseQuery(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

//send reply
void ClientNetworkRead::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->is_logged)
    {
        parseError(QString("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    parseError(QString("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void ClientNetworkRead::parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    queryNumberList << queryNumber;
    if(stopIt)
        return;
    Q_UNUSED(data);
    if(!player_informations->is_logged)
    {
        parseError(QString("is not logged, parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
    if(stopIt)
        return;
    if(!player_informations->is_logged)
    {
        parseInputBeforeLogin(mainCodeType,subCodeType,queryNumber,data);
        return;
    }
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("parseQuery(%1,%2,%3,%4)").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data.toHex())));
    #endif
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x79:
        switch(subCodeType)
        {
            //teleportation
            case 0x0001:
                emit teleportValidatedTo(lastTeleportation.first().map,lastTeleportation.first().x,lastTeleportation.first().y,lastTeleportation.first().orientation);
                lastTeleportation.removeFirst();
            break;
            default:
                parseError(QString("ident: %1, unknow sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseError("unknow main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QString("remaining data: parseQuery(%1,%2,%3): %4 %5")
                   .arg(mainCodeType)
                   .arg(subCodeType)
                   .arg(queryNumber)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void ClientNetworkRead::parseError(const QString &errorString)
{
    if(GlobalData::serverSettings.tolerantMode)
        emit message(QString("Packed dropped, due to: %1").arg(errorString));
    else
        emit error(errorString);
}

/// \warning it need be complete protocol trame
void ClientNetworkRead::fake_receive_data(const QByteArray &data)
{
    Q_UNUSED(data);
//	parseInputAfterLogin(data);
}
