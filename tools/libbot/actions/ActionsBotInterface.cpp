#include "ActionsBotInterface.h"
#include "../../general/base/Version.hpp"
#include <iostream>

std::map<CatchChallenger::Api_protocol_Qt  *,ActionsBotInterface::Player> ActionsBotInterface::clientList;
std::map<CatchChallenger::Api_protocol_Qt  *,std::vector<ActionsBotInterface::DelayedMapPlayerChange> > ActionsBotInterface::delayedMessage;

ActionsBotInterface::ActionsBotInterface() :
    randomText(false),
    globalChatRandomReply(false)
{
}

ActionsBotInterface::~ActionsBotInterface()
{
}

CatchChallenger::Api_protocol_Qt *ActionsBotInterface::toQt(CatchChallenger::Api_protocol *api)
{
    return static_cast<CatchChallenger::Api_protocol_Qt *>(api);
}

bool ActionsBotInterface::getValue(const std::string &variable,bool &value)
{
    if(variable=="randomText")
    {
        value=randomText;
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

bool ActionsBotInterface::setValue(const std::string &variable,const bool value)
{
    if(variable=="randomText")
    {
        randomText=value;
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

std::vector<std::string> ActionsBotInterface::variablesList()
{
    std::vector<std::string> list;
    list.push_back("move");
    list.push_back("randomText");
    list.push_back("bugInDirection");
    list.push_back("globalChatRandomReply");
    return list;
}

void ActionsBotInterface::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    (void)direction;
    CatchChallenger::Player_private_and_public_informations &playerApi=api->get_player_informations();
    playerApi.public_informations=player;
    Player &newPlayer=clientList[toQt(api)];
    //newPlayer.player=player;
    newPlayer.mapId=mapId;
    newPlayer.x=x;
    newPlayer.y=y;
    newPlayer.canMoveOnMap=true;
}

void ActionsBotInterface::removeClient(CatchChallenger::Api_protocol *api)
{
    std::cerr << "ActionsBotInterface::removeClient" << std::endl;
    clientList.erase(toQt(api));
}

std::string ActionsBotInterface::name()
{
    return "action";
}

std::string ActionsBotInterface::version()
{
    return std::string("1.0.0.0 for CatchChallenger ")+CatchChallenger::Version::str;
}
