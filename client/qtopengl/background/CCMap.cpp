#include "CCMap.hpp"
#include "../ConnexionManager.hpp"
#include "../../qt/QtDatapackClientLoader.hpp"
#include <QPainter>
#include <QTime>
#include <chrono>
#include <iostream>

CCMap::CCMap()
{
}

void CCMap::setVar(ConnexionManager *connexionManager)
{
    resetAll();
    connectAllSignals(connexionManager->client);
    datapackParsed();
    datapackParsedMainSub();
    //always after monster load on CatchChallenger::ClientFightEngine::fightEngine
    setDatapackPath(connexionManager->client->datapackPathBase(),connexionManager->client->mainDatapackCode());
}

QRectF CCMap::boundingRect() const
{
    return QRectF();
}

void CCMap::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *widget)
{
    mapItem->setPos(0,0);
    mapItem->paint(painter,o,widget);
}
