#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLib.h"

using namespace CatchChallenger;

//without verification of rights
void Client::sendSystemMessage(const QString &text,const bool &important)
{
    QByteArray finalData;
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        if(important)
            out << (quint8)0x08;
        else
            out << (quint8)0x07;
        {
            const QByteArray &tempText=text.toUtf8();
            if(tempText.size()>255)
            {
                DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
                return;
            }
            out << (quint8)tempText.size();
            outputData+=tempText;
            out.device()->seek(out.device()->pos()+tempText.size());
        }
        finalData.resize(16+outputData.size());
        finalData.resize(ProtocolParsingInputOutput::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,outputData.constData(),outputData.size()));
    }

    const int &size=clientBroadCastList.size();
    int index=0;
    while(index<size)
    {
        if(clientBroadCastList.at(index)!=this)
            clientBroadCastList.at(index)->sendRawSmallPacket(finalData);
        index++;
    }
}

void Client::clanChangeWithoutDb(const quint32 &clanId)
{
    if(clan!=NULL)
    {
        clan->players.removeOne(this);
        if(clan->players.isEmpty())
        {
            clanList.remove(public_and_private_informations.clan);
            delete clan;
        }
        clan=NULL;
        public_and_private_informations.clan=0;
    }
    if(clanId>0)
    {
        if(clanList.contains(clanId))
        {
            public_and_private_informations.clan=clanId;
            clan=clanList[clanId];
            clan->players << this;
        }
        else
            errorOutput("Clan not found for insert");
    }
}

void Client::sendPM(const QString &text,const QString &pseudo)
{
    if((privateChatDropTotalCache+privateChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate)
        return;
    privateChatDropNewValue++;
    if(this->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        errorOutput(QLatin1String("Can't send them self the PM"));
        return;
    }
    if(!playerByPseudo.contains(pseudo))
    {
        receiveSystemText(QStringLiteral("unable to found the connected player: pseudo: \"%1\"").arg(pseudo),false);
        if(GlobalServerData::serverSettings.anonymous)
            normalOutput(QStringLiteral("%1 have try send message to not connected user").arg(character_id));
        else
            normalOutput(QStringLiteral("%1 have try send message to not connected user: %2").arg(this->public_and_private_informations.public_informations.pseudo).arg(pseudo));
        return;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        normalOutput(QStringLiteral("[chat PM]: %1 -> %2: %3").arg(this->public_and_private_informations.public_informations.pseudo).arg(pseudo).arg(text));
    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(this->public_and_private_informations.public_informations.pseudo,Chat_type_pm,QStringLiteral("to %1: %2").arg(pseudo).arg(text));
    #endif
    playerByPseudo.value(pseudo)->receiveChatText(Chat_type_pm,text,this);
}

void Client::receiveChatText(const Chat_type &chatType,const QString &text,const Client *sender_informations)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)chatType;
    {
        const QByteArray &tempText=text.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }

    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        out2 << (quint8)Player_type_normal;
    else
        out2 << (quint8)sender_informations->public_and_private_informations.public_informations.type;
    sendPacket(0xCA,outputData+sender_informations->rawPseudo+outputData2);
}

void Client::receiveSystemText(const QString &text,const bool &important)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(important)
        out << (quint8)Chat_type_system_important;
    else
        out << (quint8)Chat_type_system;
    {
        const QByteArray &tempText=text.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    sendPacket(0xCA,outputData);
}

void Client::sendChatText(const Chat_type &chatType,const QString &text)
{
    if(chatType==Chat_type_clan)
    {
        if((clanChatDropTotalCache+clanChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
            return;
        if(clan==0)
            errorOutput(QLatin1String("Unable to chat with clan, you have not clan"));
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                normalOutput(QStringLiteral("[chat] %1: To the clan %2: %3").arg(public_and_private_informations.public_informations.pseudo).arg(clan->name).arg(text));
            #ifndef EPOLLCATCHCHALLENGERSERVER
            BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
            #endif
            const QList<Client *> &playerWithSameClan = clan->players;

            QByteArray finalData;
            {
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << (quint8)chatType;
                {
                    const QByteArray &tempText=text.toUtf8();
                    if(tempText.size()>255)
                    {
                        DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
                        return;
                    }
                    out << (quint8)tempText.size();
                    outputData+=tempText;
                    out.device()->seek(out.device()->pos()+tempText.size());
                }

                QByteArray outputData2;
                QDataStream out2(&outputData2, QIODevice::WriteOnly);
                out2.setVersion(QDataStream::Qt_4_4);
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out2 << (quint8)Player_type_normal;
                else
                    out2 << (quint8)this->public_and_private_informations.public_informations.type;
                QByteArray tempBuffer(outputData+rawPseudo+outputData2);
                finalData.resize(16+tempBuffer.size());
                finalData.resize(ProtocolParsingInputOutput::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,tempBuffer.data(),tempBuffer.size()));
            }

            const int &size=playerWithSameClan.size();
            int index=0;
            while(index<size)
            {
                if(playerWithSameClan.at(index)!=this)
                    playerWithSameClan.at(index)->sendRawSmallPacket(finalData);
                index++;
            }
        }
        return;
    }
/* Now do by command
 *	else if(chatType==Chat_type_system || chatType==Chat_type_system_important)*/
    else
    {
        if((generalChatDropTotalCache+generalChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral)
            return;
        if(!GlobalServerData::serverSettings.anonymous)
            normalOutput(QStringLiteral("[chat all] %1: %2").arg(public_and_private_informations.public_informations.pseudo).arg(text));
        #ifndef EPOLLCATCHCHALLENGERSERVER
        BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
        #endif

        QByteArray finalData;
        {
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)chatType;
            {
                const QByteArray &tempText=text.toUtf8();
                if(tempText.size()>255)
                {
                    DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
                    return;
                }
                out << (quint8)tempText.size();
                outputData+=tempText;
                out.device()->seek(out.device()->pos()+tempText.size());
            }

            QByteArray outputData2;
            QDataStream out2(&outputData2, QIODevice::WriteOnly);
            out2.setVersion(QDataStream::Qt_4_4);
            if(GlobalServerData::serverSettings.dontSendPlayerType)
                out2 << (quint8)Player_type_normal;
            else
                out2 << (quint8)this->public_and_private_informations.public_informations.type;
            QByteArray tempBuffer(outputData+rawPseudo+outputData2);
            finalData.resize(16+tempBuffer.size());
            finalData.resize(ProtocolParsingInputOutput::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,tempBuffer.constData(),tempBuffer.size()));
        }

        const int &size=clientBroadCastList.size();
        int index=0;
        while(index<size)
        {
            if(clientBroadCastList.at(index)!=this)
                clientBroadCastList.at(index)->sendRawSmallPacket(finalData);
            index++;
        }
        return;
    }
}

void Client::receive_instant_player_number(const quint16 &connected_players,const QByteArray &outputData)
{
    if(!character_loaded)
        return;
    if(this->connected_players==connected_players)
        return;
    this->connected_players=connected_players;
    sendRawSmallPacket(outputData);
}

void Client::sendBroadCastCommand(const QString &command,const QString &extraText)
{
    normalOutput(Client::text_command+command+Client::text_space+extraText);
    if(command==Client::text_chat)
    {
        QStringList list=extraText.split(Client::text_space);
        if(list.size()<2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(list.first()==Client::text_system)
        {
            list.removeFirst();
            sendChatText(Chat_type_system,list.join(Client::text_space));
            return;
        }
        if(list.first()==Client::text_system_important)
        {
            list.removeFirst();
            sendChatText(Chat_type_system_important,list.join(Client::text_space));
            return;
        }
        else
        {
            receiveSystemText(Client::text_commandnotunderstand+extraText);
            normalOutput(Client::text_commandnotunderstand+extraText);
            return;
        }
    }
    else if(command==Client::text_setrights)
    {
        QStringList list=extraText.split(Client::text_space);
        if(list.size()!=2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(!playerByPseudo.contains(list.first()))
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        if(list.last()==Client::text_normal)
            playerByPseudo.value(extraText)->setRights(Player_type_normal);
        else if(list.last()==Client::text_premium)
            playerByPseudo.value(extraText)->setRights(Player_type_premium);
        else if(list.last()==Client::text_gm)
            playerByPseudo.value(extraText)->setRights(Player_type_gm);
        else if(list.last()==Client::text_dev)
            playerByPseudo.value(extraText)->setRights(Player_type_dev);
        else
        {
            receiveSystemText(Client::text_unabletofoundthisrightslevel+list.last());
            normalOutput(Client::text_unabletofoundthisrightslevel+list.last());
            return;
        }
    }
    else if(command==Client::text_playerlist)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
        {
            QStringList playerStringList;
            QHash<QString,Client *>::const_iterator i_playerByPseudo=playerByPseudo.constBegin();
            QHash<QString,Client *>::const_iterator i_playerByPseudo_end=playerByPseudo.constEnd();
            while(i_playerByPseudo != i_playerByPseudo_end)
            {
                playerStringList << Client::text_startbold+i_playerByPseudo.value()->public_and_private_informations.public_informations.pseudo+Client::text_stopbold;
                ++i_playerByPseudo;
            }
            receiveSystemText(Client::text_playersconnectedspace+playerStringList.join(Client::text_commaspace));
        }
        return;
    }
    else if(command==Client::text_playernumber)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
            receiveSystemText(Client::text_startbold+QString::number(playerByPseudo.size())+Client::text_stopbold+Client::text_playersconnected);
        return;
    }
    else if(command==Client::text_kick)
    {
        //drop, and do the command here to separate the loop
        if(!playerByPseudo.contains(extraText))
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        playerByPseudo.value(extraText)->kick();
        sendSystemMessage(extraText+Client::text_havebeenkickedby+public_and_private_informations.public_informations.pseudo);
        return;
    }
    else
        normalOutput(Client::text_unknowcommand.arg(command).arg(extraText));
}

void Client::setRights(const Player_type& type)
{
    public_and_private_informations.public_informations.type=type;
}
