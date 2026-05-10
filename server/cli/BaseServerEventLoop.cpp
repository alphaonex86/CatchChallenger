#include "BaseServerEventLoop.hpp"
#include "ServerPrivateVariablesEventLoop.hpp"
#include "../base/GlobalServerData.hpp"

BaseServerEventLoop::BaseServerEventLoop()
{
    ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove=NULL;
}

void BaseServerEventLoop::loadAndFixSettings()
{
    if(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync==0)
    {
    }
    else
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.positionSync.start(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync*1000);
    ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.ddosTimer.start(CatchChallenger::GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000);
    BaseServer::loadAndFixSettings();
}

void BaseServerEventLoop::setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start)
{
    TimerEvents * const timer=new TimerEvents(static_cast<uint8_t>(event),static_cast<uint8_t>(value));
    ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timerEvents.push_back(timer);
    timer->start(time,start);
}

void BaseServerEventLoop::preload_the_visibility_algorithm()
{
    if(ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove!=NULL)
        delete ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove;
    if(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.enable)
    {
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove=new TimerSendInsertMoveRemove();
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove->start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);
    }

    BaseServer::preload_the_visibility_algorithm();
}

void BaseServerEventLoop::unload_the_visibility_algorithm()
{
    if(ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove!=NULL)
    {
        #ifndef CATCHCHALLENGER_SERVER
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove->stop();
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove->deleteLater();
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove=NULL;
        #else
        delete ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove;
        ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timer_to_send_insert_move_remove=NULL;
        #endif
    }
}

void BaseServerEventLoop::unload_the_events()
{
    CatchChallenger::GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timerEvents.size())
    {
        delete ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timerEvents.at(index);
        index++;
    }
    ServerPrivateVariablesEventLoop::serverPrivateVariablesEventLoop.timerEvents.clear();
}

BaseClassSwitch::EventLoopObjectType BaseServerEventLoop::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Server;
}

/*void BaseServerEventLoop::preload_11_sync_the_players()
{
    ClientWithMap::clients.clear();
    ClientWithMap::clients.resize(GlobalServerData::serverSettings.max_players);
    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED index=0;
    while(index<GlobalServerData::serverSettings.max_players)
    {
        ClientWithMap::clients.at(index).setToDefault();
        index++;
    }
    BaseServer::preload_11_sync_the_players();
}*/
