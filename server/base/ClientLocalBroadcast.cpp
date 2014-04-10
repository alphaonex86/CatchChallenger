#include "ClientLocalBroadcast.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

using namespace CatchChallenger;

ClientLocalBroadcast::ClientLocalBroadcast()
{
}

ClientLocalBroadcast::~ClientLocalBroadcast()
{
}

void ClientLocalBroadcast::extraStop()
{
    player_informations=NULL;
    removeClient(map,true);
}

void ClientLocalBroadcast::sendLocalChatText(const QString &text)
{
    if(map==NULL)
        return;
    if(this->player_informations==NULL)
        return;
    emit message(QStringLiteral("[chat local] %1: %2").arg(this->player_informations->public_and_private_informations.public_informations.pseudo).arg(text));
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_chat_message(player_informations->public_and_private_informations.public_informations.pseudo,Chat_type_local,text);

    QByteArray finalData;
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)Chat_type_local;
        out << text;

        QByteArray outputData2;
        QDataStream out2(&outputData2, QIODevice::WriteOnly);
        out2.setVersion(QDataStream::Qt_4_4);
        if(GlobalServerData::serverSettings.dontSendPlayerType)
            out2 << (quint8)Player_type_normal;
        else
            out2 << (quint8)this->player_informations->public_and_private_informations.public_informations.type;
        finalData=ProtocolParsingOutput::computeFullOutcommingData(false,0xC2,0x0005,outputData+this->player_informations->rawPseudo+outputData2);
    }

    const int &size=static_cast<MapServer *>(map)->clientsForBroadcast.size();
    int index=0;
    while(index<size)
    {
        if(static_cast<MapServer *>(map)->clientsForBroadcast.at(index)!=this)
            static_cast<MapServer *>(map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData);
        index++;
    }
}

bool ClientLocalBroadcast::singleMove(const Direction &direction)
{
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QStringLiteral("ClientLocalBroadcast::singleMove(), can go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    CommonMap *old_map=map;
    CommonMap *new_map=map;
    MoveOnTheMap::move(direction,&new_map,&x,&y);
    if(old_map!=new_map)
    {
        map=old_map;
        removeClient(old_map);
        map=new_map;
        insertClient(map);
    }
    return true;
}

void ClientLocalBroadcast::insertClient(CommonMap *map)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<MapServer *>(map)->clientsForBroadcast.contains(this))
        emit message(QLatin1String("static_cast<MapServer *>(map)->clientsForBroadcast already have this"));
    else
    #endif
    static_cast<MapServer *>(map)->clientsForBroadcast << this;

    sendNearPlant();
}

void ClientLocalBroadcast::removeClient(CommonMap *map, const bool &withDestroy)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1)
        emit message(QStringLiteral("static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1: %1").arg(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)));
    #endif
    static_cast<MapServer *>(map)->clientsForBroadcast.removeOne(this);

    if(!withDestroy)
        removeNearPlant();
    map=NULL;
}

//map slots, transmited by the current ClientNetworkRead
void ClientLocalBroadcast::put_on_the_map(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    insertClient(map);
}

void ClientLocalBroadcast::teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    bool mapChange=this->map!=map;
    if(mapChange)
        removeNearPlant();
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
    if(mapChange)
        sendNearPlant();
}
