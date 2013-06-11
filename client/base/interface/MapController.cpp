#include "MapController.h"
#include "../Api_client_real.h"

MapController* MapController::mapController=NULL;

MapController::MapController(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapControllerMP(centerOnPlayer,debugTags,useCache,OpenGL)
{
    qRegisterMetaType<CatchChallenger::Plant_collect>("CatchChallenger::Plant_collect");
    connect(CatchChallenger::Api_client_real::client,SIGNAL(insert_plant(quint32,quint16,quint16,quint8,quint16)),this,SLOT(insert_plant(quint32,quint16,quint16,quint8,quint16)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(remove_plant(quint32,quint16,quint16)),this,SLOT(remove_plant(quint32,quint16,quint16)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(seed_planted(bool)),this,SLOT(seed_planted(bool)));
    connect(CatchChallenger::Api_client_real::client,SIGNAL(plant_collected(CatchChallenger::Plant_collect)),this,SLOT(plant_collected(CatchChallenger::Plant_collect)));
}

MapController::~MapController()
{
}

void MapController::resetAll()
{
    delayedPlantInsert.clear();
    MapControllerMP::resetAll();
}

void MapController::datapackParsed()
{
    if(mHaveTheDatapack)
        return;
    MapControllerMP::datapackParsed();
    int index;

    index=0;
    while(index<delayedPlantInsert.size())
    {
        insert_plant(delayedPlantInsert.at(index).mapId,delayedPlantInsert.at(index).x,delayedPlantInsert.at(index).y,delayedPlantInsert.at(index).plant_id,delayedPlantInsert.at(index).seconds_to_mature);
        index++;
    }
    delayedPlantInsert.clear();
}

bool MapController::canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::Map map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision)
{
    if(!MapVisualiserPlayerWithFight::canGoTo(direction,map,x,y,checkCollision))
        return false;
    CatchChallenger::Map *new_map=&map;
    CatchChallenger::MoveOnTheMap::move(direction,&new_map,&x,&y,false);
    if(all_map[new_map->map_file]->logicalMap.bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    return true;
}

void MapController::loadBotOnTheMap(Map_full *parsedMap,const quint32 &botId,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin)
{
    if(skin.isEmpty())
        return;
    if(!ObjectGroupItem::objectGroupLink.contains(parsedMap->objectGroup))
    {
        qDebug() << QString("loadBotOnTheMap(), ObjectGroupItem::objectGroupLink not contains parsedMap->objectGroup");
        return;
    }
    CatchChallenger::Direction direction;
    int baseTile=-1;
    if(lookAt=="left")
    {
        baseTile=10;
        direction=CatchChallenger::Direction_move_at_left;
    }
    else if(lookAt=="right")
    {
        baseTile=4;
        direction=CatchChallenger::Direction_move_at_right;
    }
    else if(lookAt=="top")
    {
        baseTile=1;
        direction=CatchChallenger::Direction_move_at_top;
    }
    else
    {
        if(lookAt!="bottom")
            CatchChallenger::DebugClass::debugConsole(QString("Wrong direction for the bot at %1 (%2,%3)").arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
        baseTile=7;
        direction=CatchChallenger::Direction_move_at_bottom;
    }
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject=new Tiled::MapObject();
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset=new Tiled::Tileset("bot",16,24);
    QString skinPath=datapackPath+DATAPACK_BASE_PATH_SKIN+"/"+skin+"/trainer.png";
    if(!parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->loadFromImage(QImage(skinPath),skinPath))
    {
        qDebug() << "Unable the load the bot tileset";
        if(!parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->loadFromImage(QImage(":/images/player_default/trainer.png"),":/images/player_default/trainer.png"))
            qDebug() << "Unable the load the default bot tileset";
    }
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setTile(parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].tileset->tileAt(baseTile));
    ObjectGroupItem::objectGroupLink[parsedMap->objectGroup]->addObject(parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject);
    //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
    parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject->setPosition(QPoint(x,y+1));
    MapObjectItem::objectLink[parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)].mapObject]->setZValue(y);

    if(parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].step.contains(1))
    {
        QDomElement stepBot=parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].step[1];
        if(stepBot.hasAttribute("type") && stepBot.attribute("type")=="fight" && stepBot.hasAttribute("fightid") && stepBot.hasAttribute("fightfile"))
        {
            bool ok;
            quint32 fightid=stepBot.attribute("fightid").toUInt(&ok);
            if(ok)
            {
                QString botFightFile=QFileInfo(datapackMapPath+"/"+stepBot.attribute("fightfile")).absoluteFilePath();
                if(!botFightFile.endsWith(".xml"))
                    botFightFile+=".xml";
                loadBotFightFile(botFightFile);
                if(botFiles.contains(botFightFile))
                {
                    if(botFightFiles[botFightFile].contains(fightid))
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Put bot fight point %1 (%2) at %3 (%4,%5) in direction: %6").arg(botFightFile).arg(fightid).arg(parsedMap->logicalMap.map_file).arg(x).arg(y).arg(direction));
                        quint8 temp_x=x,temp_y=y;
                        int index=0;
                        CatchChallenger::Map *map=&parsedMap->logicalMap;
                        CatchChallenger::Map *old_map=map;
                        while(index<CATCHCHALLENGER_BOTFIGHT_RANGE)
                        {
                            if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                break;
                            if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                break;
                            if(map!=old_map)
                                break;
                            CatchChallenger::BotFightOnMap botFightOnMap;
                            botFightOnMap.botId=botId;
                            botFightOnMap.position=QPair<quint8,quint8>(x,y);
                            botFightOnMap.step=1;
                            parsedMap->logicalMap.botsFight.insert(QPair<quint8,quint8>(temp_x,temp_y),botFightOnMap);
                            index++;
                        }
                    }
                    else
                        CatchChallenger::DebugClass::debugConsole(QString("No fightid %1 into %2 at MapController::loadBotOnTheMap").arg(fightid).arg(botFightFile));
                }
                else
                    CatchChallenger::DebugClass::debugConsole(QString("No file %1 at MapController::loadBotOnTheMap").arg(botFightFile));
            }
        }
    }
}

void MapController::loadBotFightFile(const QString &fileName)
{
    if(botFiles.contains(fileName))
        return;
    botFiles[fileName];//create the entry
    QFile mapFile(fileName);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        qDebug() << mapFile.fileName()+": "+mapFile.errorString();
        return;
    }
    QByteArray xmlContent=mapFile.readAll();
    mapFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="fights")
    {
        qDebug() << QString("\"fights\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement("fight");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
            {
                QDomElement monster = child.firstChildElement("monster");
                while(!monster.isNull())
                {
                    if(!monster.hasAttribute("id"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(monster.tagName()).arg(monster.lineNumber()));
                    else if(!monster.isElement())
                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                    else
                    {
                        CatchChallenger::BotFight::BotFightMonster botFightMonster;
                        botFightMonster.level=1;
                        botFightMonster.id=monster.attribute("id").toUInt(&ok);
                        if(ok)
                        {
                            if(!monster.hasAttribute("level"))
                            {
                                botFightMonster.level=monster.attribute("level").toUShort(&ok);
                                if(!ok)
                                {
                                    CatchChallenger::DebugClass::debugConsole(QString("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                                    botFightMonster.level=1;
                                }
                                if(botFightMonster.level<1)
                                {
                                    CatchChallenger::DebugClass::debugConsole(QString("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(monster.tagName().arg(monster.attribute("type")).arg(monster.lineNumber())));
                                    botFightMonster.level=1;
                                }
                            }
                            QDomElement attack = monster.firstChildElement("attack");
                            while(!attack.isNull())
                            {
                                quint8 attackLevel=1;
                                if(!attack.hasAttribute("id"))
                                    CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(attack.tagName()).arg(attack.lineNumber()));
                                else if(!attack.isElement())
                                    CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName().arg(attack.attribute("type")).arg(attack.lineNumber())));
                                else
                                {
                                    quint32 attackId=attack.attribute("id").toUInt(&ok);
                                    if(ok)
                                    {
                                        if(!attack.hasAttribute("level"))
                                        {
                                            attackLevel=attack.attribute("level").toUShort(&ok);
                                            if(!ok)
                                            {
                                                CatchChallenger::DebugClass::debugConsole(QString("The level is not a number: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName().arg(attack.attribute("type")).arg(attack.lineNumber())));
                                                attackLevel=1;
                                            }
                                            if(attackLevel<1)
                                            {
                                                CatchChallenger::DebugClass::debugConsole(QString("Can't be 0 or negative: bot.tagName(): %1, type: %2 (at line: %3)").arg(attack.tagName().arg(attack.attribute("type")).arg(attack.lineNumber())));
                                                attackLevel=1;
                                            }
                                        }
                                        CatchChallenger::BotFight::BotFightAttack botFightAttack;
                                        botFightAttack.id=attackId;
                                        botFightAttack.level=attackLevel;
                                        botFightMonster.attacks << botFightAttack;
                                    }
                                }
                                attack = attack.nextSiblingElement("attack");
                            }
                            if(!botFightFiles[fileName].contains(id))
                                botFightFiles[fileName].remove(id);
                            botFightFiles[fileName][id].monsters << botFightMonster;
                        }
                    }
                    monster = monster.nextSiblingElement("monster");
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement("fight");
    }
}
