#include "ActionsBotInterface.h"
#include "../../general/base/Version.h"

ActionsBotInterface::ActionsBotInterface() :
    move(false),
    randomText(false),
    bugInDirection(false),
    globalChatRandomReply(false)
{
}

ActionsBotInterface::~ActionsBotInterface()
{
}

QVariant ActionsBotInterface::getValue(const QString &variable)
{
    if(variable==QStringLiteral("move"))
        return move;
    if(variable==QStringLiteral("randomText"))
        return randomText;
    if(variable==QStringLiteral("bugInDirection"))
        return bugInDirection;
    if(variable==QStringLiteral("globalChatRandomReply"))
        return globalChatRandomReply;
    else
        return QVariant();
}

bool ActionsBotInterface::setValue(const QString &variable,const QVariant &value)
{
    if(variable==QStringLiteral("move"))
    {
        move=value.toBool();
        return true;
    }
    if(variable==QStringLiteral("randomText"))
    {
        randomText=value.toBool();
        return true;
    }
    if(variable==QStringLiteral("bugInDirection"))
    {
        bugInDirection=value.toBool();
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
    Player newPlayer;
    newPlayer.player=player;
    newPlayer.mapId=mapId;
    newPlayer.x=x;
    newPlayer.y=y;
    newPlayer.direction=direction;
    clientList[api]=newPlayer;
}

void ActionsBotInterface::removeClient(CatchChallenger::Api_protocol *api)
{
    clientList.remove(api);
}

QString ActionsBotInterface::name()
{
    return QStringLiteral("simple");
}

QString ActionsBotInterface::version()
{
    return QStringLiteral("2.0.0.1 for CatchChallenger " CATCHCHALLENGER_VERSION);
}
