#include "ActionsBotInterface.h"
#include "../../general/base/Version.h"
#include <iostream>

QHash<CatchChallenger::Api_protocol *,ActionsBotInterface::Player> ActionsBotInterface::clientList;
QHash<CatchChallenger::Api_protocol *,std::vector<ActionsBotInterface::DelayedMapPlayerChange> > ActionsBotInterface::delayedMessage;

ActionsBotInterface::ActionsBotInterface() :
    randomText(false),
    globalChatRandomReply(false)
{
}

ActionsBotInterface::~ActionsBotInterface()
{
}

QVariant ActionsBotInterface::getValue(const QString &variable)
{
    if(variable==QStringLiteral("randomText"))
        return randomText;
    if(variable==QStringLiteral("globalChatRandomReply"))
        return globalChatRandomReply;
    else
        return QVariant();
}

bool ActionsBotInterface::setValue(const QString &variable,const QVariant &value)
{
    if(variable==QStringLiteral("randomText"))
    {
        randomText=value.toBool();
        return true;
    }
    if(variable==QStringLiteral("globalChatRandomReply"))
    {
        globalChatRandomReply=value.toBool();
        return true;
    }
    else
        return false;
}

QStringList ActionsBotInterface::variablesList()
{
    return QStringList() << QString("move") << QString("randomText") << QString("bugInDirection") << QString("globalChatRandomReply");
}

void ActionsBotInterface::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    (void)direction;
    CatchChallenger::Player_private_and_public_informations &playerApi=api->get_player_informations();
    playerApi.public_informations=player;
    Player &newPlayer=clientList[api];
    //newPlayer.player=player;
    newPlayer.mapId=mapId;
    newPlayer.x=x;
    newPlayer.y=y;
    newPlayer.canMoveOnMap=true;
}

void ActionsBotInterface::removeClient(CatchChallenger::Api_protocol *api)
{
    std::cerr << "ActionsBotInterface::removeClient" << std::endl;
    clientList.remove(api);
}

QString ActionsBotInterface::name()
{
    return QStringLiteral("action");
}

QString ActionsBotInterface::version()
{
    return QStringLiteral("1.0.0.0 for CatchChallenger " CATCHCHALLENGER_VERSION);
}
