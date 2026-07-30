#include "SimpleAction.h"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../BotAbort.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>

SimpleAction::SimpleAction()
{
    if(!connect(&moveTimer,&QTimer::timeout,this,&SimpleAction::doMove))
        BOT_ABORT();
    if(!connect(&textTimer,&QTimer::timeout,this,&SimpleAction::doText))
        BOT_ABORT();
    moveTimer.start(1000);
    textTimer.start(1000);
    purgeCpuCache();
}

SimpleAction::~SimpleAction()
{
}

void SimpleAction::insert_player(CatchChallenger::Api_protocol *api, const CatchChallenger::Player_public_informations &player, const CATCHCHALLENGER_TYPE_MAPID &mapId, const COORD_TYPE &x, const COORD_TYPE &y, const CatchChallenger::Direction &direction)
{
    SimpleBotInterface::insert_player(api,player,mapId,x,y,direction);
    //Only this class needs the Qt protocol type (to reach its signals); the
    //object was created by the Qt front-end, so the cast is safe without RTTI.
    CatchChallenger::Api_protocol_Qt * const qtApi=static_cast<CatchChallenger::Api_protocol_Qt *>(api);
    if(!connect(qtApi,&CatchChallenger::Api_protocol_Qt::new_chat_text,this,&SimpleAction::new_chat_text,Qt::QueuedConnection))
        BOT_ABORT();
}

void SimpleAction::purgeCpuCache()
{
    const int size=16*1024*1024;
    std::vector<char> buffer(size,0);
    if(memcmp(buffer.data(),buffer.data(),size)!=0)
        BOT_ABORT();
}

void SimpleAction::doMove()
{
    if(!move)
        return;

    purgeCpuCache();
    std::unordered_map<CatchChallenger::Api_protocol *,Player>::iterator i=clientList.begin();
    while(i!=clientList.end())
    {
        CatchChallenger::Api_protocol * const api=i->first;
        Player &player=i->second;
        if(api->getCaracterSelected())
        {
            if(bugInDirection)
                api->send_player_move(0,player.direction);
            else
            {
                if(player.direction==CatchChallenger::Direction_look_at_bottom)
                {
                    player.direction=CatchChallenger::Direction_look_at_left;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_left)
                {
                    player.direction=CatchChallenger::Direction_look_at_top;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_top)
                {
                    player.direction=CatchChallenger::Direction_look_at_right;
                    api->send_player_move(0,player.direction);
                }
                else if(player.direction==CatchChallenger::Direction_look_at_right)
                {
                    player.direction=CatchChallenger::Direction_look_at_bottom;
                    api->send_player_move(0,player.direction);
                }
                else
                {
                    std::cerr << "Out of direction scope" << std::endl;
                    BOT_ABORT();
                }
            }
        }
        ++i;
    }
}

void SimpleAction::doText()
{
    if(!randomText)
        return;
    if(clientList.empty())
        return;

    purgeCpuCache();
    std::vector<CatchChallenger::Api_protocol *> clientListApi;
    std::unordered_map<CatchChallenger::Api_protocol *,Player>::const_iterator i=clientList.cbegin();
    while(i!=clientList.cend())
    {
        clientListApi.push_back(i->first);
        ++i;
    }
    CatchChallenger::Api_protocol * const api=clientListApi.at(rand()%clientListApi.size());
    if(api->getCaracterSelected())
    {
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettingsServer::commonSettingsServer.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

void SimpleAction::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,
                                 const std::string &pseudo,const CatchChallenger::Player_type &type)
{
    if(!globalChatRandomReply && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    purgeCpuCache();
    (void)type;
    CatchChallenger::Api_protocol_Qt *api = static_cast<CatchChallenger::Api_protocol_Qt *>(sender());
    if(api==NULL)
        return;

    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_all)
            switch(rand()%(100*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
            switch(rand()%(3*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
        {
            if(text=="version")
                api->sendPM(std::string("Version ")+name()+" "+version(),pseudo);
            else
                api->sendPM(std::string("Hello ")+pseudo+", I'm few bit busy for now",pseudo);
        }
        break;
        default:
        break;
    }
}
