#include "SimpleBotInterface.h"

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

QVariant SimpleBotInterface::getValue(const QString &variable)
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

bool SimpleBotInterface::setValue(const QString &variable,const QVariant &value)
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

QStringList SimpleBotInterface::variablesList()
{
    return QStringList() << QString("move") << QString("randomText") << QString("bugInDirection") << QString("globalChatRandomReply");
}

void SimpleBotInterface::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
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
    clientList.remove(api);
}

QString SimpleBotInterface::name()
{
    return QStringLiteral("simple");
}

QString SimpleBotInterface::version()
{
    return QStringLiteral("2.0.0.1");
}
