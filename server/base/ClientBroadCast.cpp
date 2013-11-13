#include "ClientBroadCast.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

QHash<QString,ClientBroadCast *> ClientBroadCast::playerByPseudo;
QMultiHash<CLAN_ID_TYPE,ClientBroadCast *> ClientBroadCast::playerByClan;
QList<ClientBroadCast *> ClientBroadCast::clientBroadCastList;
ClientBroadCast *ClientBroadCast::item;

ClientBroadCast::ClientBroadCast()
{
    connected_players=0;
    clan=0;
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
        emit error("Can't send them self the PM");
        return;
    }
    if(!playerByPseudo.contains(pseudo))
    {
        receiveSystemText(QString("unable to found the connected player: pseudo: \"%1\"").arg(pseudo),false);
        if(GlobalServerData::serverSettings.anonymous)
            emit message(QString("%1 have try send message to not connected user").arg(this->player_informations->character_id));
        else
            emit message(QString("%1 have try send message to not connected user: %2").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(pseudo));
        return;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        emit message(QString("[chat PM]: %1 -> %2: %3").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(pseudo).arg(text));
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(this->player_informations->public_and_private_informations.public_informations.pseudo,Chat_type_pm,QString("to %1: %2").arg(pseudo).arg(text));
    playerByPseudo[pseudo]->receiveChatText(Chat_type_pm,text,this->player_informations);
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
            emit error("Unable to chat with clan, you have not clan");
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                emit message(QString("[chat] %1: To the clan %2: %3").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(clan).arg(text));
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
                emit message(QString("[chat] %1: To the system chat: %2").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(text));
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
            emit message(QString("[chat all] %1: %2").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(text));
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
    emit message(QString("command: %1, text: %2").arg(command).arg(extraText));
    if(command=="chat")
    {
        QStringList list=extraText.split(' ');
        if(list.size()<2)
        {
            receiveSystemText(QString("command not understand").arg(extraText));
            emit message(QString("command not understand").arg(extraText));
            return;
        }
        if(list.first()=="system")
        {
            list.removeFirst();
            sendChatText(Chat_type_system,list.join(" "));
            return;
        }
        if(list.first()=="system_important")
        {
            list.removeFirst();
            sendChatText(Chat_type_system_important,list.join(" "));
            return;
        }
        else
        {
            receiveSystemText(QString("command not understand").arg(extraText));
            emit message(QString("command not understand").arg(extraText));
            return;
        }
    }
    else if(command=="setrights")
    {
        QStringList list=extraText.split(' ');
        if(list.size()!=2)
        {
            receiveSystemText(QString("command not understand").arg(extraText));
            emit message(QString("command not understand").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(list.first()))
        {
            receiveSystemText(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(list.first()));
            emit message(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(list.first()));
            return;
        }
        if(list.last()=="normal")
            playerByPseudo[extraText]->setRights(Player_type_normal);
        else if(list.last()=="premium")
            playerByPseudo[extraText]->setRights(Player_type_premium);
        else if(list.last()=="gm")
            playerByPseudo[extraText]->setRights(Player_type_gm);
        else if(list.last()=="dev")
            playerByPseudo[extraText]->setRights(Player_type_dev);
        else
        {
            receiveSystemText(QString("unable to found this rights level: \"%1\"").arg(list.last()));
            emit message(QString("unable to found this rights level: \"%1\"").arg(list.last()));
            return;
        }
    }
    else if(command=="playerlist")
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(QString("You are alone on the server!"));
        else
        {
            QStringList playerStringList;
            QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo=playerByPseudo.constBegin();
            QHash<QString,ClientBroadCast *>::const_iterator i_playerByPseudo_end=playerByPseudo.constEnd();
            while (i_playerByPseudo != i_playerByPseudo_end)
            {
                playerStringList << "<b>"+i_playerByPseudo.value()->player_informations->public_and_private_informations.public_informations.pseudo+"</b>";
                ++i_playerByPseudo;
            }
            receiveSystemText(QString("players connected: %1").arg(playerStringList.join(", ")));
        }
        return;
    }
    else if(command=="playernumber")
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(QString("You are alone on the server!"));
        else
            receiveSystemText(QString("<b>%1</b> players connected").arg(playerByPseudo.size()));
        return;
    }
    else if(command=="kick")
    {
        //drop, and do the command here to separate the loop
        if(!playerByPseudo.contains(extraText))
        {
            receiveSystemText(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(extraText));
            emit message(QString("unable to found the connected player to kick: pseudo: \"%1\"").arg(extraText));
            return;
        }
        playerByPseudo[extraText]->kick();
        sendSystemMessage(QString("%1 have been kicked by %2").arg(extraText).arg(player_informations->public_and_private_informations.public_informations.pseudo));
        return;
    }
    else
        emit message(QString("unknow command: %1, text: %2").arg(command).arg(extraText));
}

void ClientBroadCast::setRights(const Player_type& type)
{
    player_informations->public_and_private_informations.public_informations.type=type;
}
