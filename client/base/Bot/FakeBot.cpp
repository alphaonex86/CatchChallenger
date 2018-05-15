#ifndef EPOLLCATCHCHALLENGERSERVER
#include "FakeBot.h"
#include "../Api_client_virtual.h"
#include "../../../general/base/CommonMap.h"
#include "../../../general/base/MoveOnTheMap.h"

#include <QHostAddress>

using namespace CatchChallenger;

int FakeBot::index_loop;
int FakeBot::loop_size;
QSemaphore FakeBot::wait_to_stop;

/// \todo ask player information at the insert
FakeBot::FakeBot() :
    socket(&fakeSocket),
    api(&socket)
{
    connect(&api,&Api_client_virtual::insert_player,            this,&FakeBot::insert_player);
    connect(&api,&Api_client_virtual::have_current_player_info, this,&FakeBot::have_current_player_info);
    connect(&api,&Api_client_virtual::newError,                 this,&FakeBot::newError);
    connect(&socket,static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error),                    this,&FakeBot::newSocketError);
    connect(&socket,&ConnectedSocket::disconnected,             this,&FakeBot::disconnected);
    connect(&socket,&ConnectedSocket::connected,                this,&FakeBot::tryLink,Qt::QueuedConnection);

    details=false;
    map=NULL;

    do_step=false;
    socket.connectToHost(QHostAddress::LocalHost,9999);

    x=0;
    y=0;

/*	predefinied_step << Direction_move_at_top;
    predefinied_step << Direction_move_at_bottom;*/
}

FakeBot::~FakeBot()
{
    wait_to_stop.acquire();
}

void FakeBot::disconnected()
{
    map=NULL;
}

void FakeBot::tryLink()
{
    api.sendProtocol();
    api.tryLogin("Fake","Fake");
}

void FakeBot::doStep()
{
    //qDebug() << (QStringLiteral("FakeBot::doStep(), do_step: %1, socket.isValid():%2, map!=NULL: %3").arg(do_step).arg(socket.isValid()).arg(map!=NULL));
    if(do_step && socket.isValid() && map!=NULL)
    {
        random_new_step();
/*		if(rand()%(GlobalServerData::botNumber*10)==0)
            api.sendChatText(Chat_type_local,"Hello world!");*/
    }
}

void FakeBot::start_step()
{
    do_step=true;
}

void FakeBot::random_new_step()
{
    Direction final_direction;
    while(predefinied_step.size()>0 && !canGoTo(predefinied_step.front(),*map,x,y,true))
    {
        qDebug() << (QStringLiteral("FakeBot::random_new_step(), step 1, id: %1, map: %2 (%3,%4), unable to go on: %5").arg(api.getId()).arg(
                         QString::fromStdString(map->map_file)
                         ).arg(x).arg(y).arg(
                         QString::fromStdString(MoveOnTheMap::directionToString(predefinied_step.front()))
                         ));
        predefinied_step.erase(predefinied_step.cbegin());
    }
    if(predefinied_step.size()>0)
    {
        final_direction=predefinied_step.front();
        predefinied_step.erase(predefinied_step.cbegin());
    }
    else
    {
        QList<Direction> directions_allowed;
        if(canGoTo(Direction_move_at_left,*map,x,y,true))
            directions_allowed << Direction_move_at_left;
        if(canGoTo(Direction_move_at_right,*map,x,y,true))
            directions_allowed << Direction_move_at_right;
        if(canGoTo(Direction_move_at_top,*map,x,y,true))
            directions_allowed << Direction_move_at_top;
        if(canGoTo(Direction_move_at_bottom,*map,x,y,true))
            directions_allowed << Direction_move_at_bottom;
        loop_size=directions_allowed.size();
        if(details)
        {
            QStringList directions_allowed_string;
            index_loop=0;
            while(index_loop<loop_size)
            {
                directions_allowed_string << QString::fromStdString(MoveOnTheMap::directionToString(directions_allowed.at(index_loop)));
                index_loop++;
            }
            qDebug() << (QStringLiteral("FakeBot::random_new_step(), step 1, id: %1, map: %2 (%3,%4), directions_allowed_string: %5").arg(api.getId()).arg(QString::fromStdString(map->map_file)).arg(x).arg(y).arg(directions_allowed_string.join(", ")));
        }
        if(loop_size<=0)
            return;
        int random = rand()%loop_size;
        final_direction=directions_allowed.at(random);
    }
    //to group the signle move into move line
    MoveOnTheMap::newDirection(final_direction);
    //to do the real move
    if(!move(final_direction,(CommonMap **)&map,&x,&y))
    {
        qDebug() << (QStringLiteral("FakeBot::random_new_step(), step 2, id: %1, x: %2, y:%3, can't move on direction of: %4").arg(api.getId()).arg(x).arg(y).arg(QString::fromStdString(MoveOnTheMap::directionToString(final_direction))));
        map=NULL;
        return;
    }
    if(details)
        qDebug() << (QStringLiteral("FakeBot::random_new_step(), step 3, id: %1, map: %2 (%3,%4) after move: %5").arg(api.getId()).arg(QString::fromStdString(map->map_file)).arg(x).arg(y).arg(QString::fromStdString(MoveOnTheMap::directionToString(final_direction))));
}

//uint32_t,QString,uint16_t,uint16_t,uint8_t,uint16_t
void FakeBot::insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    /*if(!GlobalServerData::serverPrivateVariables.id_map_to_map.contains(mapId))
    {
        /// \bug here pass after delete a party, create a new
        qDebug() << ("mapId id not found for bot");
        return;
    }
    if(details)
        qDebug() << (QStringLiteral("FakeBot::insert_player() id: %1, mapName: %2 (%3,%4), api.getId(): %5").arg(player.simplifiedId).arg(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId)).arg(x).arg(y).arg(api.getId()));
    if(player.simplifiedId==api.getId())
    {
        if(details)
            qDebug() << (QStringLiteral("FakeBot::insert_player() register id: %1, mapName: %2 (%3,%4)").arg(player.simplifiedId).arg(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId)).arg(x).arg(y));
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId)))
        {
            qDebug() << (QStringLiteral("FakeBot::insert_player(), map not found: %1").arg(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId)));
            return;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list.value(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId))->width)
        {
            qDebug() << (QStringLiteral("FakeBot::insert_player(), x>=GlobalServerData::serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(x).arg(GlobalServerData::serverPrivateVariables.map_list.value(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId))->width));
            return;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list.value(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId))->height)
        {
            qDebug() << (QStringLiteral("FakeBot::insert_player(), x>=GlobalServerData::serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(y).arg(GlobalServerData::serverPrivateVariables.map_list.value(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId))->height));
            return;
        }
        this->map=GlobalServerData::serverPrivateVariables.map_list.value(GlobalServerData::serverPrivateVariables.id_map_to_map.value(mapId));
        this->x=x;
        this->y=y;
        this->last_direction=direction;
    }*/
}

void FakeBot::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    Q_UNUSED(informations);
//    qDebug() << (QStringLiteral("FakeBot::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void FakeBot::newError(std::string error,std::string detailedError)
{
    qDebug() << (QStringLiteral("FakeBot::newError() error: %1, detailedError: %2")
                 .arg(QString::fromStdString(error))
                 .arg(QString::fromStdString(detailedError))
                 );
    socket.disconnectFromHost();
    this->map=NULL;
}

void FakeBot::newSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << (QStringLiteral("FakeBot::newError() error: %1").arg(error));
    this->map=NULL;
}

void FakeBot::stop_step()
{
    do_step=false;
}

void FakeBot::stop()
{
    stop_step();
    wait_to_stop.release();
//	emit disconnected();
}

void FakeBot::show_details()
{
    details=true;
    qDebug() << (QStringLiteral("FakeBot::show_details(), x: %1, y:%2").arg(x).arg(y));
}

uint64_t FakeBot::get_TX_size()
{
    return fakeSocket.getTXSize();
}

uint64_t FakeBot::get_RX_size()
{
    return fakeSocket.getRXSize();
}

void FakeBot::send_player_move(const uint8_t &moved_unit,const Direction &the_direction)
{
    api.send_player_move(moved_unit,the_direction);
}
#endif
