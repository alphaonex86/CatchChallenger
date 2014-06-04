#include "BroadCastWithoutSender.h"
#include "ClientBroadCast.h"
#include "GlobalServerData.h"
#include "../../general/base/ProtocolParsing.h"

using namespace CatchChallenger;

BroadCastWithoutSender BroadCastWithoutSender::broadCastWithoutSender;

BroadCastWithoutSender::BroadCastWithoutSender()
{
}

void BroadCastWithoutSender::emit_serverCommand(const QString &command,const QString &extraText)
{
    /*emit */serverCommand(command,extraText);
}

void BroadCastWithoutSender::emit_new_player_is_connected(const Player_internal_informations &newPlayer)
{
    /*emit */new_player_is_connected(newPlayer);
}

void BroadCastWithoutSender::emit_player_is_disconnected(const QString &oldPlayer)
{
    /*emit */player_is_disconnected(oldPlayer);
}

void BroadCastWithoutSender::emit_new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text)
{
    /*emit */new_chat_message(pseudo,type,text);
}

void BroadCastWithoutSender::receive_instant_player_number(const qint16 &connected_players)
{
    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        if(ClientBroadCast::clientBroadCastList.isEmpty())
            return;

        QByteArray finalData;
        {
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            if(GlobalServerData::serverSettings.max_players<=255)
                out << (qint8)connected_players;
            else
                out << (qint16)connected_players;
            finalData=ProtocolParsingOutput::computeOutcommingData(false,0xC3,outputData);
        }

        int index=0;
        const int &list_size=ClientBroadCast::clientBroadCastList.size();
        while(index<list_size)
        {
            ClientBroadCast::clientBroadCastList.at(index)->receive_instant_player_number(connected_players,finalData);
            index++;
        }
    }
}

void BroadCastWithoutSender::doDDOSAction()
{
    if(ClientBroadCast::generalChatDrop.size()==CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        ClientBroadCast::generalChatDropTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            ClientBroadCast::generalChatDrop[index]=ClientBroadCast::generalChatDrop[index+1];
            ClientBroadCast::generalChatDropTotalCache+=ClientBroadCast::generalChatDrop[index];
            index++;
        }
        ClientBroadCast::generalChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=ClientBroadCast::generalChatDropNewValue;
        ClientBroadCast::generalChatDropTotalCache+=ClientBroadCast::generalChatDropNewValue;
        ClientBroadCast::generalChatDropNewValue=0;
    }
    if(ClientBroadCast::clanChatDrop.size()==CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        ClientBroadCast::clanChatDropTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            ClientBroadCast::clanChatDrop[index]=ClientBroadCast::clanChatDrop[index+1];
            ClientBroadCast::clanChatDropTotalCache+=ClientBroadCast::clanChatDrop[index];
            index++;
        }
        ClientBroadCast::clanChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=ClientBroadCast::clanChatDropNewValue;
        ClientBroadCast::clanChatDropTotalCache+=ClientBroadCast::clanChatDropNewValue;
        ClientBroadCast::clanChatDropNewValue=0;
    }
    if(ClientBroadCast::privateChatDrop.size()==CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        ClientBroadCast::privateChatDropTotalCache=0;
        int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1))
        {
            ClientBroadCast::privateChatDrop[index]=ClientBroadCast::privateChatDrop[index+1];
            ClientBroadCast::privateChatDropTotalCache+=ClientBroadCast::privateChatDrop[index];
            index++;
        }
        ClientBroadCast::privateChatDrop[CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-1]=ClientBroadCast::privateChatDropNewValue;
        ClientBroadCast::privateChatDropTotalCache+=ClientBroadCast::privateChatDropNewValue;
        ClientBroadCast::privateChatDropNewValue=0;
    }
}
