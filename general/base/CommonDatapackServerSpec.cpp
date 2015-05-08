#include "CommonDatapackServerSpec.h"
#include "CommonDatapack.h"
#include "GeneralVariable.h"
#include "FacilityLib.h"
#include "Map_loader.h"
#include "../fight/FightLoader.h"
#include "DatapackGeneralLoader.h"

#include <QDebug>
#include <QFile>
#include <QDomElement>
#include <QDomDocument>
#include <QByteArray>
#include <QDir>
#include <QFileInfoList>
#include <QMutexLocker>

using namespace CatchChallenger;

CommonDatapackServerSpec CommonDatapackServerSpec::commonDatapackServerSpec;

CommonDatapackServerSpec::CommonDatapackServerSpec()
{
}

void CommonDatapackServerSpec::parseDatapack(const QString &datapackPath)
{
    if(isParsedSpec)
        return;
    QMutexLocker mutexLocker(&inProgressSpec);

    CommonDatapack::commonDatapack.parseDatapack(datapackPath);

    parseQuests();
    parseBotFights();
    parseMonstersCollision();
    parseShop();

    #ifdef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif

    isParsedSpec=true;
}

void CommonDatapackServerSpec::parseQuests()
{
    quests=DatapackGeneralLoader::loadQuests(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_QUESTS));
    qDebug() << QStringLiteral("%1 quest(s) loaded").arg(quests.size());
}

void CommonDatapackServerSpec::parseShop()
{
    shops=DatapackGeneralLoader::preload_shop(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SHOP)+QStringLiteral("shop.xml"),CommonDatapack::commonDatapack.items.item);
    qDebug() << QStringLiteral("%1 monster items(s) to learn loaded").arg(shops.size());
}

void CommonDatapackServerSpec::parseBotFights()
{
    botFights=FightLoader::loadFight(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_FIGHT),CommonDatapack::commonDatapack.monsters,CommonDatapack::commonDatapack.monsterSkills,CommonDatapack::commonDatapack.items.item);
    qDebug() << QStringLiteral("%1 bot fight(s) loaded").arg(botFights.size());
}

void CommonDatapackServerSpec::parseMonstersCollision()
{
    monstersCollision=DatapackGeneralLoader::loadMonstersCollision(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+QStringLiteral("layers.xml"),CommonDatapack::commonDatapack.items.item,CommonDatapack::commonDatapack.events);
    qDebug() << QStringLiteral("%1 monster(s) collisions loaded").arg(monstersCollision.size());
}

void CommonDatapackServerSpec::unload()
{
    QMutexLocker mutexLocker(&inProgressSpec);
    if(!isParsedSpec)
        return;
    botFights.clear();
    quests.clear();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    monstersCollision.clear();
    shops.clear();
    CommonDatapack::commonDatapack.unload();
    isParsedSpec=false;
}
