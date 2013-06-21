#include "MapVisualiserThread.h"
#include "MapItem.h"
#include "../../general/base/FacilityLib.h"
#include <QFileInfo>

MapVisualiserThread::MapVisualiserThread()
{
    start();
}

//open the file, and load it into the variables
MapVisualiserThread::Map_full *MapVisualiserThread::loadOtherMap(const QString &resolvedFileName)
{
    MapVisualiserThread::Map_full *tempMapObject=new MapVisualiserThread::Map_full();

    Tiled::Layer *grass;
    Tiled::Layer *grassOver;

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return NULL;
    }
    CatchChallenger::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        mLastError=map_loader.errorString();
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(map_loader.errorString());
        int index=0;
        while(index<tempMapObject->tiledMap->tilesets().size())
        {
            delete tempMapObject->tiledMap->tilesets().at(index);
            index++;
        }
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    //copy the variables
    tempMapObject->logicalMap.width                                 = map_loader.map_to_send.width;
    tempMapObject->logicalMap.height                                = map_loader.map_to_send.height;
    tempMapObject->logicalMap.parsed_layer                          = map_loader.map_to_send.parsed_layer;
    tempMapObject->logicalMap.map_file                              = resolvedFileName;
    tempMapObject->logicalMap.border.bottom.map                     = NULL;
    tempMapObject->logicalMap.border.top.map                        = NULL;
    tempMapObject->logicalMap.border.right.map                      = NULL;
    tempMapObject->logicalMap.border.left.map                       = NULL;
    tempMapObject->logicalMap.grassMonster                          = map_loader.map_to_send.grassMonster;
    tempMapObject->logicalMap.waterMonster                          = map_loader.map_to_send.waterMonster;
    tempMapObject->logicalMap.caveMonster                           = map_loader.map_to_send.caveMonster;

    //load the string
    tempMapObject->logicalMap.border_semi                = map_loader.map_to_send.border;
    if(!map_loader.map_to_send.border.bottom.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.bottom.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.top.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.top.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.right.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.right.fileName).absoluteFilePath();
    if(!map_loader.map_to_send.border.left.fileName.isEmpty())
        tempMapObject->logicalMap.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.border_semi.left.fileName).absoluteFilePath();

    //load the string
    tempMapObject->logicalMap.teleport_semi.clear();
    int index=0;
    while(index<map_loader.map_to_send.teleport.size())
    {
        tempMapObject->logicalMap.teleport_semi << map_loader.map_to_send.teleport.at(index);
        tempMapObject->logicalMap.teleport_semi[index].map                      = QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+tempMapObject->logicalMap.teleport_semi.at(index).map).absoluteFilePath();
        index++;
    }

    tempMapObject->logicalMap.rescue_points            = map_loader.map_to_send.rescue_points;
    tempMapObject->logicalMap.bot_spawn_points         = map_loader.map_to_send.bot_spawn_points;

    //load the render
    switch (tempMapObject->tiledMap->orientation()) {
    case Tiled::Map::Isometric:
        tempMapObject->tiledRender = new Tiled::IsometricRenderer(tempMapObject->tiledMap);
        break;
    case Tiled::Map::Orthogonal:
    default:
        tempMapObject->tiledRender = new Tiled::OrthogonalRenderer(tempMapObject->tiledMap);
        break;
    }
    tempMapObject->tiledRender->setObjectBorder(false);

    //do the object group to move the player on it
    tempMapObject->objectGroup = new Tiled::ObjectGroup("Dyna management",0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());

    //add a tags
    if(debugTags)
    {
        Tiled::MapObject * tagMapObject = new Tiled::MapObject();
        tagMapObject->setTile(tagTileset->tileAt(tagTilesetIndex));
        tagMapObject->setPosition(QPoint(tempMapObject->logicalMap.width/2,tempMapObject->logicalMap.height/2+1));
        tempMapObject->objectGroup->addObject(tagMapObject);
        tagTilesetIndex++;
        if(tagTilesetIndex>=tagTileset->tileCount())
            tagTilesetIndex=0;
    }
    else //remove the hidden tags, and unknow layer
    {
        index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
            {
                //remove the unknow layer
                if(objectGroup->name()=="Moving")
                {
                    QList<Tiled::MapObject*> objects=objectGroup->objects();
                    int index2=0;
                    while(index2<objects.size())
                    {
                        //remove the unknow object
                        if(objects.at(index2)->type()!="door")
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        index2++;
                    }
                    index++;
                }
                else if(objectGroup->name()=="Object")
                {
                    QList<Tiled::MapObject*> objects=objectGroup->objects();
                    int index2=0;
                    while(index2<objects.size())
                    {
                        //remove the bot
                        if(objects.at(index2)->type()!="bot")
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        //remove the unknow object
                        else
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        index2++;
                    }
                    index++;
                }
                else
                    delete tempMapObject->tiledMap->takeLayerAt(index);
            }
            else
                index++;
        }
    }

    //search WalkBehind layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(tempMapObject->tiledMap->layerAt(index)->name()=="WalkBehind")
        {
            tempMapObject->objectGroupIndex=index;
            tempMapObject->tiledMap->insertLayer(index,tempMapObject->objectGroup);
            break;
        }
        index++;
    }
    if(index==tempMapObject->tiledMap->layerCount())
    {
        //search Collisions layer
        index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
            {
                tempMapObject->objectGroupIndex=index+1;
                tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
                break;
            }
            index++;
        }
        if(index==tempMapObject->tiledMap->layerCount())
        {
            qDebug() << QString("Unable to locate the \"Collisions\" layer on the map: %1").arg(resolvedFileName);
            tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
        }
    }

    //move the Moving layer on dyna layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            if(objectGroup->name()=="Moving")
            {
                Tiled::Layer *layer = tempMapObject->tiledMap->takeLayerAt(index);
                if(tempMapObject->objectGroupIndex-1<=0)
                    tempMapObject->tiledMap->insertLayer(0,layer);
                else
                {
                    if(index>tempMapObject->objectGroupIndex)
                        tempMapObject->objectGroupIndex++;
                    tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,layer);
                }
            }
        }
        else if(Tiled::TileLayer *tileLayer = tempMapObject->tiledMap->layerAt(index)->asTileLayer())
        {
            if(tileLayer->name()=="Grass")
            {
                grass = tempMapObject->tiledMap->takeLayerAt(index);
                if(tempMapObject->objectGroupIndex-1<=0)
                    tempMapObject->tiledMap->insertLayer(0,grass);
                else
                {
                    if(index>tempMapObject->objectGroupIndex)
                        tempMapObject->objectGroupIndex++;
                    tempMapObject->tiledMap->insertLayer(tempMapObject->objectGroupIndex-1,grass);
                }
                grassOver = grass->clone();
                grassOver->setName("Grass over");
                tempMapObject->tiledMap->addLayer(grassOver);

                QSet<Tiled::Tileset*> tilesets=grassOver->usedTilesets();
                QSet<Tiled::Tileset*>::const_iterator i = tilesets.constBegin();
                while (i != tilesets.constEnd()) {
                     Tiled::Tileset* oldTileset=*i;
                     Tiled::MapReader mapReader;
                     QFile tsxFile(oldTileset->fileName());
                     if(tsxFile.open(QIODevice::ReadOnly))
                     {
                         Tiled::Tileset* newTileset=mapReader.readTileset(&tsxFile,QFileInfo(tsxFile).absoluteFilePath());
                         if(newTileset!=NULL)
                         {
                             Tiled::Tile * currentTile;
                             QSet<Tiled::Tile *> tileUsed;
                             int indexTile=0;
                             while(indexTile<=newTileset->tileCount())
                             {
                                 currentTile=newTileset->tileAt(indexTile);
                                 if(currentTile!=NULL)
                                     if(!tileUsed.contains(currentTile))
                                     {
                                         qDebug() << "New tile" << tileUsed;
                                         QPixmap pixmap=currentTile->image();
                                         pixmap.fill();
                                         currentTile->setImage(pixmap);
                                         tileUsed << currentTile;
                                     }
                                 indexTile++;
                             }
                             grassOver->replaceReferencesToTileset(*i,newTileset);
                         }
                         else
                             qDebug() << "Unable to load the tileset:" << oldTileset->fileName() << ", error:" << mapReader.errorString();
                     }
                     else
                         qDebug() << "Unable to open the tsx file:" << tsxFile.fileName() << ", error:" << tsxFile.errorString();
                     ++i;
                 }
            }
        }
        index++;
    }

    loadOtherMapClientPart(tempMapObject);

    return tempMapObject;
}

//drop and remplace by Map_loader info
void MapVisualiserThread::loadOtherMapClientPart(MapVisualiserThread::Map_full *parsedMap)
{
    QString fileName=parsedMap->logicalMap.map_file;
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
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="map")
    {
        qDebug() << QString("\"map\" root balise not found for the xml file");
        return;
    }
    bool ok,ok2;
    //load the bots (map->bots)
    QDomElement child = root.firstChildElement("objectgroup");
    while(!child.isNull())
    {
        if(!child.hasAttribute("name"))
            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            if(child.attribute("name")=="Object")
            {
                QDomElement bot = child.firstChildElement("object");
                while(!bot.isNull())
                {
                    if(!bot.hasAttribute("type"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute("x"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"x\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute("y"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"y\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.isElement())
                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(bot.tagName().arg(bot.attribute("type")).arg(bot.lineNumber())));
                    else
                    {
                        quint32 x=bot.attribute("x").toUInt(&ok)/16;
                        quint32 y=(bot.attribute("y").toUInt(&ok2)/16)-1;
                        if(ok && ok2 && (bot.attribute("type")=="bot" || bot.attribute("type")=="botfight"))
                        {
                            QDomElement properties = bot.firstChildElement("properties");
                            while(!properties.isNull())
                            {
                                if(!properties.isElement())
                                    CatchChallenger::DebugClass::debugConsole(QString("Is not an element: properties.tagName(): %1, (at line: %2)").arg(properties.tagName().arg(properties.lineNumber())));
                                else
                                {
                                    QHash<QString,QString> property_parsed;
                                    QDomElement property = properties.firstChildElement("property");
                                    while(!property.isNull())
                                    {
                                        if(!property.hasAttribute("name"))
                                            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"name\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.hasAttribute("value"))
                                            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"value\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.isElement())
                                            CatchChallenger::DebugClass::debugConsole(QString("Is not an element: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                        else
                                            property_parsed[property.attribute("name")]=property.attribute("value");
                                        property = property.nextSiblingElement("property");
                                    }
                                    if(property_parsed.contains("file") && property_parsed.contains("id"))
                                    {
                                        quint8 botId=property_parsed["id"].toUShort(&ok);
                                        if(ok)
                                        {
                                            QString botFile=QFileInfo(QFileInfo(fileName).absolutePath()+"/"+property_parsed["file"]).absoluteFilePath();
                                            if(!botFile.endsWith(".xml"))
                                                botFile+=".xml";
                                            if(bot.attribute("type")=="bot")
                                            {
                                                loadBotFile(botFile);
                                                if(botFiles.contains(botFile))
                                                {
                                                    if(botFiles[botFile].contains(botId))
                                                    {
                                                        CatchChallenger::DebugClass::debugConsole(QString("Put bot %1 (%2) at %3 (%4,%5)").arg(botFile).arg(botId).arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)]=botFiles[botFile][botId];
                                                        property_parsed.remove("file");
                                                        property_parsed.remove("id");
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].properties=property_parsed;
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].botId=botId;
                                                    }
                                                    else
                                                        CatchChallenger::DebugClass::debugConsole(QString("No botId %1 into %2: properties.tagName(): %3, name: %4 (at line: %5)").arg(botId).arg(botFile).arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QString("No file %1: properties.tagName(): %2, name: %3 (at line: %4)").arg(botFile).arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                            }
                                        }
                                        else
                                            CatchChallenger::DebugClass::debugConsole(QString("Is not a number: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                    }
                                }
                                properties = properties.nextSiblingElement("properties");
                            }
                        }
                    }
                    bot = bot.nextSiblingElement("object");
                }
            }
        }
        child = child.nextSiblingElement("objectgroup");
    }
}

void MapVisualiserThread::loadBotFile(const QString &fileName)
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
    if(root.tagName()!="bots")
    {
        qDebug() << QString("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement("bot");
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
                QDomElement step = child.firstChildElement("step");
                while(!step.isNull())
                {
                    if(!step.hasAttribute("id"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.hasAttribute("type"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.isElement())
                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
                    else
                    {
                        quint32 stepId=step.attribute("id").toUInt(&ok);
                        if(ok)
                            botFiles[fileName][id].step[stepId]=step;
                    }
                    step = step.nextSiblingElement("step");
                }
                if(!botFiles[fileName][id].step.contains(1))
                    botFiles[fileName].remove(id);
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement("bot");
    }
}

void MapVisualiserThread::resetAll()
{
    botFiles.clear();
}
