#include "SimpleBotInterface.h"
#include "../../../general/base/Version.hpp"

SimpleBotInterface::SimpleBotInterface() :
    move(false),
    randomText(false),
    bugInDirection(false),
    globalChatRandomReply(false)
{
}

SimpleBotInterface::~SimpleBotInterface()
{
}

bool SimpleBotInterface::getValue(const std::string &variable,bool &value)
{
    if(variable=="move")
    {
        value=move;
        return true;
    }
    if(variable=="randomText")
    {
        value=randomText;
        return true;
    }
    if(variable=="bugInDirection")
    {
        value=bugInDirection;
        return true;
    }
    if(variable=="globalChatRandomReply")
    {
        value=globalChatRandomReply;
        return true;
    }
    else
        return false;
}

bool SimpleBotInterface::setValue(const std::string &variable,const bool value)
{
    if(variable=="move")
    {
        move=value;
        return true;
    }
    if(variable=="randomText")
    {
        randomText=value;
        return true;
    }
    if(variable=="bugInDirection")
    {
        bugInDirection=value;
        return true;
    }
    if(variable=="globalChatRandomReply")
    {
        globalChatRandomReply=value;
        return true;
    }
    else
        return false;
}

std::vector<std::string> SimpleBotInterface::variablesList()
{
    std::vector<std::string> list;
    list.push_back("move");
    list.push_back("randomText");
    list.push_back("bugInDirection");
    list.push_back("globalChatRandomReply");
    return list;
}

void SimpleBotInterface::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    Player newPlayer;
    newPlayer.player=player;
    newPlayer.mapId=mapId;
    newPlayer.x=x;
    newPlayer.y=y;
    newPlayer.direction=direction;
    clientList[api]=newPlayer;
}

void SimpleBotInterface::removeClient(CatchChallenger::Api_protocol *api)
{
    clientList.erase(api);
}

std::string SimpleBotInterface::name()
{
    return "simple";
}

std::string SimpleBotInterface::version()
{
    return std::string("for CatchChallenger ")+CatchChallenger::Version::str;
}
