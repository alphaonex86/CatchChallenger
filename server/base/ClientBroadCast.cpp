#include "ClientBroadCast.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

QHash<QString,ClientBroadCast *> ClientBroadCast::playerByPseudo;
QMultiHash<CLAN_ID_TYPE,ClientBroadCast *> ClientBroadCast::playerByClan;
QList<ClientBroadCast *> ClientBroadCast::clientBroadCastList;
ClientBroadCast *ClientBroadCast::item;

QString ClientBroadCast::text_chat=QLatin1Literal("chat");
QString ClientBroadCast::text_space=QLatin1Literal(" ");
QString ClientBroadCast::text_system=QLatin1Literal("system");
QString ClientBroadCast::text_system_important=QLatin1Literal("system_important");
QString ClientBroadCast::text_setrights=QLatin1Literal("setrights");
QString ClientBroadCast::text_normal=QLatin1Literal("normal");
QString ClientBroadCast::text_premium=QLatin1Literal("premium");
QString ClientBroadCast::text_gm=QLatin1Literal("gm");
QString ClientBroadCast::text_dev=QLatin1Literal("dev");
QString ClientBroadCast::text_playerlist=QLatin1Literal("playerlist");
QString ClientBroadCast::text_startbold=QLatin1Literal("<b>");
QString ClientBroadCast::text_stopbold=QLatin1Literal("</b>");
QString ClientBroadCast::text_playernumber=QLatin1Literal("playernumber");
QString ClientBroadCast::text_kick=QLatin1Literal("kick");
QString ClientBroadCast::text_Youarealoneontheserver=QLatin1Literal("You are alone on the server!");
QString ClientBroadCast::text_playersconnected=QLatin1Literal(" players connected");
QString ClientBroadCast::text_playersconnectedspace=QLatin1Literal("players connected ");
QString ClientBroadCast::text_havebeenkickedby=QLatin1Literal(" have been kicked by ");
QString ClientBroadCast::text_unknowcommand=QLatin1Literal("unknow command: %1, text: %2");
QString ClientBroadCast::text_commandnotunderstand=QLatin1Literal("command not understand");
QString ClientBroadCast::text_command=QLatin1Literal("command: ");
QString ClientBroadCast::text_commaspace=QLatin1Literal(", ");
QString ClientBroadCast::text_unabletofoundtheconnectedplayertokick=QLatin1Literal("unable to found the connected player to kick");
QString ClientBroadCast::text_unabletofoundthisrightslevel=QLatin1Literal("unable to found this rights level: ");

ClientBroadCast::ClientBroadCast():
    connected_players(0),
    clan(0)
{
}

ClientBroadCast::~ClientBroadCast()
{
}

void ClientBroadCast::setVariable(Player_internal_informations *player_informations)
{
    this->player_informations=player_informations;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        connect(&GlobalServerData::serverPrivateVariables.player_updater,&PlayerUpdater::newConnectedPlayer,this,&ClientBroadCast::receive_instant_player_number,Qt::QueuedConnection);
}

//without verification of rights
void ClientBroadCast::sendSystemMessage(const QString &text,const bool &important)
{
    int size=clientBroadCastList.size();
    if(important)
    {
        int index=0;
        while(index<size)
        {
            if(clientBroadCastList.at(index)!=this)
                clientBroadCastList.at(index)->receiveSystemText(text,true);
            index++;
        }
    }
    else
    {
        int index=0;
        while(index<size)
        {
            if(clientBroadCastList.at(index)!=this)
                clientBroadCastList.at(index)->receiveSystemText(text);
            index++;
        }
    }
}

void ClientBroadCast::clanChange(const quint32 &clanId)
{
    if(clanId==0)
        playerByClan.remove(clan,this);
    else
    {
        if(clan>0)
            playerByClan.remove(clan,this);
        playerByClan.insert(clanId,this);
    }
    clan=clanId;
}

void ClientBroadCast::sendPM(const QString &text,const QString &pseudo)
{
    if(this->player_informations->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        emit error(QLatin1String("Can't send them self the PM"));
        return;
    }
    if(!playerByPseudo.contains(pseudo))
    {
        receiveSystemText(QStringLiteral("unable to found the connected player: pseudo: \"%1\"").arg(pseudo),false);
        if(GlobalServerData::serverSettings.anonymous)
            emit message(QStringLiteral("%1 have try send message to not connected user").arg(this->player_informations->character_id));
        else
            emit message(QStringLiteral("%1 have try send message to not connected user: %2").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(pseudo));
        return;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        emit message(QStringLiteral("[chat PM]: %1 -> %2: %3").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(pseudo).arg(text));
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(this->player_informations->public_and_private_informations.public_informations.pseudo,Chat_type_pm,QStringLiteral("to %1: %2").arg(pseudo).arg(text));
    playerByPseudo.value(pseudo)->receiveChatText(Chat_type_pm,text,this->player_informations);
}

void ClientBroadCast::askIfIsReadyToStop()
{
    playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
    if(clan>0)
        playerByClan.remove(clan,this);
    clientBroadCastList.removeOne(this);
    emit isReadyToStop();
}

void ClientBroadCast::receiveChatText(const Chat_type &chatType,const QString &text,const Player_internal_informations *sender_informations)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)chatType;
    out << text;

    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    out2 << (quint8)sender_informations->public_and_private_informations.public_informations.type;
    emit sendFullPacket(0xC2,0x0005,outputData+sender_informations->rawPseudo+outputData2);
}

void ClientBroadCast::receiveSystemText(const QString &text,const bool &important)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(important)
        out << (quint8)Chat_type_system_important;
    else
        out << (quint8)Chat_type_system;
    out << text;
    emit sendFullPacket(0xC2,0x0005,outputData);
}

void ClientBroadCast::sendChatText(const Chat_type &chatType,const QString &text)
{
    if(chatType==Chat_type_clan)
    {
        if(clan==0)
            emit error(QLatin1String("Unable to chat with clan, you have not clan"));
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                emit message(QStringLiteral("[chat] %1: To the clan %2: %3").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(clan).arg(text));
            BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(player_informations->public_and_private_informations.public_informations.pseudo,chatType,text);
            QList<ClientBroadCast *> playerWithSameClan = playerByClan.values(clan);
            int size=playerWithSameClan.size();
            int index=0;
            while(index<size)
            {
                if(playerWithSameClan.at(index)!=this)
                    playerWithSameClan.at(index)->receiveChatText(chatType,text,player_informations);
                index++;
            }
        }
        return;
    }
/* Now do by command
 *	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)
    {
        if(player_informations->public_and_private_informations.public_informations.type==Player_type_gm || player_informations->public_and_private_informations.public_informations.type==Player_type_dev)
        {
            if(!GlobalServerData::serverSettings.anonymous)
                emit message(QStringLiteral("[chat] %1: To the system chat: %2").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(text));
            BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(player_informations->public_and_private_informations.public_informations.pseudo,chatType,text);
            QSetIterator<ClientBroadCast *> i(clientBroadCastList);
            while (i.hasNext())
            {
                item=i.next();
                if(item!=this)
                    item->receiveSystemText(text);
            }
            return;
        }
        else
            emit error("Have not the right to send system message");
    }*/
    else
    {
        if(!GlobalServerData::serverSettings.anonymous)
            emit message(QStringLiteral("[chat all] %1: %2").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(text));
        BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(player_informations->public_and_private_informations.public_informations.pseudo,chatType,text);
        int size=clientBroadCastList.size();
        int index=0;
        while(index<size)
        {
            if(clientBroadCastList.at(index)!=this)
                clientBroadCastList.at(index)->receiveChatText(chatType,text,player_informations);
            index++;
        }
        return;
    }
}

void ClientBroadCast::send_player_informations()
{
    playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
    if(clan>0)
    {
        clan=player_informations->public_and_private_informations.clan;
        playerByClan.insert(clan,this);
    }
    clientBroadCastList << this;
}

void ClientBroadCast::receive_instant_player_number(qint32 connected_players)
{
    if(!player_informations->character_loaded)
        return;
    if(this->connected_players==connected_players)
        return;
    this->connected_players=connected_players;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(GlobalServerData::serverSettings.max_players<=255)
        out << (qint8)connected_players;
    else
        out << (qint16)connected_players;
    emit sendPacket(0xC3,outputData);
}

void ClientBroadCast::kick()
{
    emit kicked();
}

void ClientBroadCast::sendBroadCastCommand(const QString &command,const QString &extraText)
{
    emit message(ClientBroadCast::text_command+command+ClientBroadCast::text_space+extraText);
    if(command==ClientBroadCast::text_chat)
    {
        QStringList list=extraText.split(ClientBroadCast::text_space);
        if(list.size()<2)
        {
            receiveSystemText(ClientBroadCast::text_commandnotunderstand+command+ClientBroadCast::text_space+extraText);
            emit message(ClientBroadCast::text_commandnotunderstand+command+ClientBroadCast::text_space+extraText);
            return;
        }
        if(list.first()==ClientBroadCast::text_system)
        {
            list.removeFirst();
            sendChatText(Chat_type_system,list.join(ClientBroadCast::text_space));
            return;
        }
        if(list.first()==ClientBroadCast::text_system_important)
        {
            list.removeFirst();
            sendChatText(Chat_type_system_important,list.join(ClientBroadCast::text_space));
            return;
        }
        else
        {
            receiveSystemText(ClientBroadCast::text_commandnotunderstand+extraText);
            emit message(ClientBroadCast::text_commandnotunderstand+extraText);
            return;
        }
    }
    else if(command==ClientBroadCast::text_setrights)
    {
        QStringList list=extraText.split(ClientBroadCast::text_space);
        if(list.size()!=2)
        {
            receiveSystemText(ClientBroadCast::text_commandnotunderstand+command+ClientBroadCast::text_space+extraText);
            emit message(ClientBroadCast::text_commandnotunderstand+command+ClientBroadCast::text_space+extraText);
            return;
        }
        if(!playerByPseudo.contains(list.first()))
        {
            receiveSystemText(ClientBroadCast::text_unabletofoundtheconnectedplayertokick+extraText);
            emit message(ClientBroadCast::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        if(list.last()==ClientBroadCast::text_normal)
            playerByPseudo.value(extraText)->setRights(Player_type_normal);
        else if(list.last()==ClientBroadCast::text_premium)
            playerByPseudo.value(extraText)->setRights(Player_type_premium);
        else if(list.last()==ClientBroadCast::text_gm)
            playerByPseudo.value(extraText)->setRights(Player_type_gm);
        else if(list.last()==ClientBroadCast::text_dev)
            playerByPseudo.value(extraText)->setRights(Player_type_dev);
        else
        {
            receiveSystemText(ClientBroadCast::text_unabletofoundthisrightslevel+list.last());
            emit message(ClientBroadCast::text_unabletofoundthisrightslevel+list.last());
            return;
        }
    }
    else if(command==ClientBroadCast::text_playerlist)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(ClientBroadCast::text_Youarealoneontheserver);
        else
        {
            QStringList playerStringList;
            QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo=playerByPseudo.constBegin();
            QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo_end=playerByPseudo.constEnd();
            while (i_playerByPseudo != i_playerByPseudo_end)
            {
                playerStringList << ClientBroadCast::text_startbold+i_playerByPseudo.value()->player_informations->public_and_private_informations.public_informations.pseudo+ClientBroadCast::text_stopbold;
                ++i_playerByPseudo;
            }
            receiveSystemText(ClientBroadCast::text_playersconnectedspace+playerStringList.join(ClientBroadCast::text_commaspace));
        }
        return;
    }
    else if(command==ClientBroadCast::text_playernumber)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(ClientBroadCast::text_Youarealoneontheserver);
        else
            receiveSystemText(ClientBroadCast::text_startbold+QString::number(playerByPseudo.size())+ClientBroadCast::text_stopbold+ClientBroadCast::text_playersconnected);
        return;
    }
    else if(command==ClientBroadCast::text_kick)
    {
        //drop, and do the command here to separate the loop
        if(!playerByPseudo.contains(extraText))
        {
            receiveSystemText(ClientBroadCast::text_unabletofoundtheconnectedplayertokick+extraText);
            emit message(ClientBroadCast::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        playerByPseudo.value(extraText)->kick();
        sendSystemMessage(extraText+ClientBroadCast::text_havebeenkickedby+player_informations->public_and_private_informations.public_informations.pseudo);
        return;
    }
    else
        emit message(ClientBroadCast::text_unknowcommand.arg(command).arg(extraText));
}

void ClientBroadCast::setRights(const Player_type& type)
{
    player_informations->public_and_private_informations.public_informations.type=type;
}
