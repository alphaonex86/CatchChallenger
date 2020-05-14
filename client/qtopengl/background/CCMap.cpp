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
    fightEngine=connexionManager->client->fightEngine;
    resetAll();
    connectAllSignals(connexionManager->client);
    datapackParsed();
    datapackParsedMainSub();
}

QRectF CCMap::boundingRect() const
{
    return QRectF();
}
