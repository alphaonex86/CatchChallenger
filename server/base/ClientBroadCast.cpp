#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLib.h"
#include "PreparedDBQuery.h"

using namespace CatchChallenger;

//without verification of rights
void Client::sendSystemMessage(const std::string &text,const bool &important)
{
    QByteArray finalData;
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        if(important)
            out << (uint8_t)0x08;
        else
            out << (uint8_t)0x07;
        {
            const QByteArray tempText(text.data(),text.size());
            if(tempText.size()>255)
            {
                errorOutput("text in Utf8 too big, line: "+std::to_string(__LINE__));
                return;
            }
            out << (uint8_t)tempText.size();
            outputData+=tempText;
            out.device()->seek(out.device()->pos()+tempText.size());
        }
        finalData.resize(16+outputData.size());
        finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,outputData.constData(),outputData.size()));
    }

    const int &size=clientBroadCastList.size();
    int index=0;
    while(index<size)
    {
        Client * const client=clientBroadCastList.at(index);
        if(client!=this)
            client->sendRawBlock(finalData.constData(),finalData.size());
        index++;
    }
}

void Client::clanChangeWithoutDb(const uint32_t &clanId)
{
    if(clan!=NULL)
    {
        vectorremoveOne(clan->players,this);
        if(clan->players.empty())
        {
            clanList.erase(public_and_private_informations.clan);
            delete clan;
        }
        clan=NULL;
        public_and_private_informations.clan=0;
    }
    if(clanId>0)
    {
        if(clanList.find(clanId)!=clanList.cend())
        {
            public_and_private_informations.clan=clanId;
            clan=clanList[clanId];
            clan->players.push_back(this);
        }
        else
            errorOutput("Clan not found for insert");
    }
}

void Client::sendPM(const std::string &text,const std::string &pseudo)
{
    if((privateChatDropTotalCache+privateChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate)
        return;
    privateChatDropNewValue++;
    if(this->public_and_private_informations.public_informations.pseudo==pseudo)
    {
        errorOutput("Can't send them self the PM");
        return;
    }
    if(playerByPseudo.find(pseudo)==playerByPseudo.cend())
    {
        receiveSystemText("unable to found the connected player: pseudo: \""+pseudo+"\"",false);
        if(GlobalServerData::serverSettings.anonymous)
            normalOutput(std::to_string(character_id)+" have try send message to not connected user");
        else
            normalOutput(this->public_and_private_informations.public_informations.pseudo+" have try send message to not connected user: "+pseudo);
        return;
    }
    if(!GlobalServerData::serverSettings.anonymous)
        normalOutput("[chat PM]: "+this->public_and_private_informations.public_informations.pseudo+" -> "+pseudo+": "+text);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(this->public_and_private_informations.public_informations.pseudo,Chat_type_pm,std::stringLiteral("to %1: %2").arg(pseudo).arg(text));
    #endif
    playerByPseudo.at(pseudo)->receiveChatText(Chat_type_pm,text,this);
}

void Client::receiveChatText(const Chat_type &chatType,const std::string &text,const Client *sender_informations)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)chatType;
    {
        const QByteArray tempText(text.data(),text.size());
        if(tempText.size()>255)
        {
            std::cerr << "text in Utf8 too big, line: " << __LINE__ << std::endl;
            return;
        }
        out << (uint8_t)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }

    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        out2 << (uint8_t)Player_type_normal;
    else
        out2 << (uint8_t)sender_informations->public_and_private_informations.public_informations.type;
    const QByteArray newData(outputData+sender_informations->rawPseudo+outputData2);
    sendMessage(0x5C,newData.constData(),newData.size());
}

void Client::receiveSystemText(const std::string &text,const bool &important)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(important)
        out << (uint8_t)Chat_type_system_important;
    else
        out << (uint8_t)Chat_type_system;
    {
        const QByteArray tempText(text.data(),text.size());
        if(tempText.size()>255)
        {
            std::cerr << "text in Utf8 too big, line: " << __LINE__ << std::endl;
            return;
        }
        out << (uint8_t)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    sendMessage(0x5C,outputData.constData(),outputData.size());
}

void Client::sendChatText(const Chat_type &chatType,const std::string &text)
{
    if(chatType==Chat_type_clan)
    {
        if((clanChatDropTotalCache+clanChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
            return;
        if(clan==0)
            errorOutput("Unable to chat with clan, you have not clan");
        else
        {
            if(!GlobalServerData::serverSettings.anonymous)
                normalOutput("[chat] "+public_and_private_informations.public_informations.pseudo+
                             ": To the clan "+clan->name+": "+text
                            );
            #ifndef EPOLLCATCHCHALLENGERSERVER
            BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
            #endif
            const std::vector<Client *> &playerWithSameClan = clan->players;

            QByteArray finalData;
            {
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
                out << (uint8_t)chatType;
                {
                    const QByteArray tempText(text.data(),text.size());
                    if(tempText.size()>255)
                    {
                        errorOutput("text in Utf8 too big, line: "+std::to_string(__LINE__));
                        return;
                    }
                    out << (uint8_t)tempText.size();
                    outputData+=tempText;
                    out.device()->seek(out.device()->pos()+tempText.size());
                }

                QByteArray outputData2;
                QDataStream out2(&outputData2, QIODevice::WriteOnly);
                out2.setVersion(QDataStream::Qt_4_4);
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out2 << (uint8_t)Player_type_normal;
                else
                    out2 << (uint8_t)this->public_and_private_informations.public_informations.type;
                QByteArray tempBuffer(outputData+rawPseudo+outputData2);
                finalData.resize(16+tempBuffer.size());
                finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,tempBuffer.data(),tempBuffer.size()));
            }

            const int &size=playerWithSameClan.size();
            int index=0;
            while(index<size)
            {
                Client * const client=playerWithSameClan.at(index);
                if(client!=this)
                    client->sendRawBlock(finalData.constData(),finalData.size());
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
            normalOutput("[chat all] "+public_and_private_informations.public_informations.pseudo+": "+text);
        #ifndef EPOLLCATCHCHALLENGERSERVER
        BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(public_and_private_informations.public_informations.pseudo,chatType,text);
        #endif

        QByteArray finalData;
        {
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
            out << (uint8_t)chatType;
            {
                const QByteArray tempText(text.data(),text.size());
                if(tempText.size()>255)
                {
                    errorOutput("text in Utf8 too big, line: "+std::to_string(__LINE__));
                    return;
                }
                out << (uint8_t)tempText.size();
                outputData+=tempText;
                out.device()->seek(out.device()->pos()+tempText.size());
            }

            QByteArray outputData2;
            QDataStream out2(&outputData2, QIODevice::WriteOnly);
            out2.setVersion(QDataStream::Qt_4_4);
            if(GlobalServerData::serverSettings.dontSendPlayerType)
                out2 << (uint8_t)Player_type_normal;
            else
                out2 << (uint8_t)this->public_and_private_informations.public_informations.type;
            QByteArray tempBuffer(outputData+rawPseudo+outputData2);
            finalData.resize(16+tempBuffer.size());
            finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,tempBuffer.constData(),tempBuffer.size()));
        }

        const int &size=clientBroadCastList.size();
        int index=0;
        while(index<size)
        {
            Client * const client=clientBroadCastList.at(index);
            if(client!=this)
                client->sendRawBlock(finalData.constData(),finalData.size());
            index++;
        }
        return;
    }
}

void Client::receive_instant_player_number(const uint16_t &connected_players, const char * const data, const uint8_t &size)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(size!=2 && size!=3)
        return;
    #endif
    if(!character_loaded)
        return;
    if(this->connected_players==connected_players)
        return;
    this->connected_players=connected_players;
    sendRawBlock(data,size);
}

void Client::sendBroadCastCommand(const std::string &command,const std::string &extraText)
{
    normalOutput(Client::text_command+command+Client::text_space+extraText);
    if(command==Client::text_chat)
    {
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()<2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(list.front()==Client::text_system)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system,stringimplode(list,' '));
            return;
        }
        if(list.front()==Client::text_system_important)
        {
            list.erase(list.begin());
            sendChatText(Chat_type_system_important,stringimplode(list,' '));
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
        std::vector<std::string> list=stringsplit(extraText,' ');
        if(list.size()!=2)
        {
            receiveSystemText(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            normalOutput(Client::text_commandnotunderstand+command+Client::text_space+extraText);
            return;
        }
        if(playerByPseudo.find(list.front())==playerByPseudo.cend())
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        Client * client=playerByPseudo.at(list.front());
        if(client==NULL)
        {
            qDebug() << "Internal bug";
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        if(list.back()==Client::text_normal)
            client->setRights(Player_type_normal);
        else if(list.back()==Client::text_premium)
            client->setRights(Player_type_premium);
        else if(list.back()==Client::text_gm)
            client->setRights(Player_type_gm);
        else if(list.back()==Client::text_dev)
            client->setRights(Player_type_dev);
        else
        {
            receiveSystemText(Client::text_unabletofoundthisrightslevel+list.back());
            normalOutput(Client::text_unabletofoundthisrightslevel+list.back());
            return;
        }
    }
    else if(command==Client::text_playerlist)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
        {
            std::vector<std::string> playerStringList;
            auto i=playerByPseudo.begin();
            while(i!=playerByPseudo.cend())
            {
                playerStringList.push_back(Client::text_startbold+i->second->public_and_private_informations.public_informations.pseudo+Client::text_stopbold);
                ++i;
            }
            receiveSystemText(Client::text_playersconnectedspace+stringimplode(playerStringList,Client::text_commaspace));
        }
        return;
    }
    else if(command==Client::text_playernumber)
    {
        if(playerByPseudo.size()==1)
            receiveSystemText(Client::text_Youarealoneontheserver);
        else
            receiveSystemText(Client::text_startbold+std::to_string(playerByPseudo.size())+Client::text_stopbold+Client::text_playersconnected);
        return;
    }
    else if(command==Client::text_kick)
    {
        //drop, and do the command here to separate the loop
        if(playerByPseudo.find(extraText)==playerByPseudo.cend())
        {
            receiveSystemText(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            normalOutput(Client::text_unabletofoundtheconnectedplayertokick+extraText);
            return;
        }
        playerByPseudo.at(extraText)->kick();
        sendSystemMessage(extraText+Client::text_havebeenkickedby+public_and_private_informations.public_informations.pseudo);
        return;
    }
    else
        normalOutput("unknow command: "+command+", text: "+extraText);
}

void Client::setRights(const Player_type& type)
{
    public_and_private_informations.public_informations.type=type;
    const int &newType=type/0x10-1;
    std::string queryText=PreparedDBQueryCommon::db_query_change_right;
    stringreplaceOne(queryText,"%1",std::to_string(account_id));
    stringreplaceOne(queryText,"%2",std::to_string(newType));
    dbQueryWriteCommon(queryText);
}
