#include "ClientLocalBroadcast.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalData.h"

using namespace Pokecraft;

ClientLocalBroadcast::ClientLocalBroadcast()
{
}

ClientLocalBroadcast::~ClientLocalBroadcast()
{
}

void ClientLocalBroadcast::sendLocalChatText(const QString &text)
{
    if(map==NULL)
        return;
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(player_informations->public_and_private_informations.public_informations.pseudo,Chat_type_local,text);
    int index=0;
    int size=static_cast<Map_server *>(map)->clientsForBroadcast.size();
    while(index<size)
    {
        if(static_cast<Map_server *>(map)->clientsForBroadcast.at(index)!=this)
            static_cast<Map_server *>(map)->clientsForBroadcast.at(index)->receiveChatText(text,player_informations);
        index++;
    }
}

void ClientLocalBroadcast::receiveChatText(const QString &text,const Player_internal_informations *sender_informations)
{
    /* Multiple message when multiple player connected
    emit message(QString("receiveChatText(), text: %1, sender_player_id: %2, to player: %3").arg(text).arg(sender_player_id).arg(player_informations.id)); */
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)Chat_type_local;
    out << text;

    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    out2 << (quint8)sender_informations->public_and_private_informations.public_informations.type;
    emit sendPacket(0xC2,0x0005,outputData+sender_informations->rawPseudo+outputData2);
}

bool ClientLocalBroadcast::singleMove(const Direction &direction)
{
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QString("ClientLocalBroadcast::singleMove(), can go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    Map *old_map=map;
    MoveOnTheMap::move(direction,&map,&x,&y);
    if(old_map!=map)
    {
        static_cast<Map_server *>(old_map)->clientsForBroadcast.removeOne(this);
        static_cast<Map_server *>(map)->clientsForBroadcast << this;
    }
    return true;
}

//map slots, transmited by the current ClientNetworkRead
void ClientLocalBroadcast::put_on_the_map(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    static_cast<Map_server *>(map)->clientsForBroadcast << this;
}
