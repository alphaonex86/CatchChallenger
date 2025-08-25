#include "BaseServerEpoll.hpp"
#include "ServerPrivateVariablesEpoll.hpp"
#include "../base/GlobalServerData.hpp"

BaseServerEpoll::BaseServerEpoll()
{
    ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove=NULL;
}

void BaseServerEpoll::loadAndFixSettings()
{
    if(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync==0)
    {
    }
    else
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.positionSync.start(CatchChallenger::GlobalServerData::serverSettings.secondToPositionSync*1000);
    ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.ddosTimer.start(CatchChallenger::GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000);
    BaseServer::loadAndFixSettings();
}

void BaseServerEpoll::setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start)
{
    TimerEvents * const timer=new TimerEvents(static_cast<uint8_t>(event),static_cast<uint8_t>(value));
    ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timerEvents.push_back(timer);
    timer->start(time,start);
}

void BaseServerEpoll::preload_the_visibility_algorithm()
{
    switch(CatchChallenger::GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
        case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
        if(ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove!=NULL)
            delete ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove;
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove=new TimerSendInsertMoveRemove();
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove->start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);
        break;
        case CatchChallenger::MapVisibilityAlgorithmSelection_None:
        default:
        break;
    }

    BaseServer::preload_the_visibility_algorithm();
}

void BaseServerEpoll::unload_the_visibility_algorithm()
{
    if(ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove!=NULL)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove->stop();
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove->deleteLater();
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove=NULL;
        #else
        delete ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove;
        ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timer_to_send_insert_move_remove=NULL;
        #endif
    }
}

void BaseServerEpoll::unload_the_events()
{
    CatchChallenger::GlobalServerData::serverPrivateVariables.events.clear();
    unsigned int index=0;
    while(index<ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timerEvents.size())
    {
        delete ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timerEvents.at(index);
        index++;
    }
    ServerPrivateVariablesEpoll::serverPrivateVariablesEpoll.timerEvents.clear();
}

BaseClassSwitch::EpollObjectType BaseServerEpoll::getType() const
{
    return BaseClassSwitch::EpollObjectType::Server;
}

void BaseServerEpoll::preload_11_sync_the_players()
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
}
