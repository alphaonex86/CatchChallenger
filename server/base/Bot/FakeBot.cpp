#include "FakeBot.h"
#include "../GlobalServerData.h"
#include "../ClientMapManagement/MapBasicMove.h"
#include "../Api_client_virtual.h"

#include <QHostAddress>

using namespace CatchChallenger;

int FakeBot::index_loop;
int FakeBot::loop_size;
QSemaphore FakeBot::wait_to_stop;

/// \todo ask player information at the insert
FakeBot::FakeBot() :
    socket(&fakeSocket),
    api(&socket,GlobalServerData::serverSettings.datapack_basePath)
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
    api.startReadData();
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
    //DebugClass::debugConsole(QString("FakeBot::doStep(), do_step: %1, socket.isValid():%2, map!=NULL: %3").arg(do_step).arg(socket.isValid()).arg(map!=NULL));
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
    while(predefinied_step.size()>0 && !canGoTo(predefinied_step.first(),*map,x,y,true))
    {
        DebugClass::debugConsole(QString("FakeBot::random_new_step(), step 1, id: %1, map: %2 (%3,%4), unable to go on: %5").arg(api.getId()).arg(map->map_file).arg(x).arg(y).arg(MoveOnTheMap::directionToString(predefinied_step.first())));
        predefinied_step.removeFirst();
    }
    if(predefinied_step.size()>0)
    {
        final_direction=predefinied_step.first();
        predefinied_step.removeFirst();
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
                directions_allowed_string << MoveOnTheMap::directionToString(directions_allowed.at(index_loop));
                index_loop++;
            }
            DebugClass::debugConsole(QString("FakeBot::random_new_step(), step 1, id: %1, map: %2 (%3,%4), directions_allowed_string: %5").arg(api.getId()).arg(map->map_file).arg(x).arg(y).arg(directions_allowed_string.join(", ")));
        }
        if(loop_size<=0)
            return;
        int random = rand()%loop_size;
        final_direction=directions_allowed.at(random);
    }
    //to group the signle move into move line
    MoveOnTheMap::newDirection(final_direction);
    //to do the real move
    if(!move(final_direction,(Map **)&map,&x,&y))
    {
        DebugClass::debugConsole(QString("FakeBot::random_new_step(), step 2, id: %1, x: %2, y:%3, can't move on direction of: %4").arg(api.getId()).arg(x).arg(y).arg(MoveOnTheMap::directionToString(final_direction)));
        map=NULL;
        return;
    }
    if(details)
        DebugClass::debugConsole(QString("FakeBot::random_new_step(), step 3, id: %1, map: %2 (%3,%4) after move: %5").arg(api.getId()).arg(map->map_file).arg(x).arg(y).arg(MoveOnTheMap::directionToString(final_direction)));
}

//quint32,QString,quint16,quint16,quint8,quint16
void FakeBot::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    if(!GlobalServerData::serverPrivateVariables.id_map_to_map.contains(mapId))
    {
        /// \bug here pass after delete a party, create a new
        DebugClass::debugConsole("mapId id not found for bot");
        return;
    }
    if(details)
        DebugClass::debugConsole(QString("FakeBot::insert_player() id: %1, mapName: %2 (%3,%4), api.getId(): %5").arg(player.simplifiedId).arg(GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]).arg(x).arg(y).arg(api.getId()));
    if(player.simplifiedId==api.getId())
    {
        if(details)
            DebugClass::debugConsole(QString("FakeBot::insert_player() register id: %1, mapName: %2 (%3,%4)").arg(player.simplifiedId).arg(GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]).arg(x).arg(y));
        if(!GlobalServerData::serverPrivateVariables.map_list.contains(GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]))
        {
            DebugClass::debugConsole(QString("FakeBot::insert_player(), map not found: %1").arg(GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]));
            return;
        }
        if(x>=GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]]->width)
        {
            DebugClass::debugConsole(QString("FakeBot::insert_player(), x>=GlobalServerData::serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(x).arg(GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]]->width));
            return;
        }
        if(y>=GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]]->height)
        {
            DebugClass::debugConsole(QString("FakeBot::insert_player(), x>=GlobalServerData::serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(y).arg(GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]]->height));
            return;
        }
        this->map=GlobalServerData::serverPrivateVariables.map_list[GlobalServerData::serverPrivateVariables.id_map_to_map[mapId]];
        this->x=x;
        this->y=y;
        this->last_direction=direction;
    }
}

void FakeBot::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    Q_UNUSED(informations);
//    DebugClass::debugConsole(QString("FakeBot::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void FakeBot::newError(QString error,QString detailedError)
{
    DebugClass::debugConsole(QString("FakeBot::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
    socket.disconnectFromHost();
    this->map=NULL;
}

void FakeBot::newSocketError(QAbstractSocket::SocketError error)
{
    DebugClass::debugConsole(QString("FakeBot::newError() error: %1").arg(error));
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
    DebugClass::debugConsole(QString("FakeBot::show_details(), x: %1, y:%2").arg(x).arg(y));
}

quint64 FakeBot::get_TX_size()
{
    return fakeSocket.getTXSize();
}

quint64 FakeBot::get_RX_size()
{
    return fakeSocket.getRXSize();
}

void FakeBot::send_player_move(const quint8 &moved_unit,const Direction &the_direction)
{
    api.send_player_move(moved_unit,the_direction);
}
