#include "MapVisualiser.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>

#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/DebugClass.h"
#include "../../general/libtiled/tile.h"

void MapVisualiser::resetAll()
{
    QSet<QString>::const_iterator i = displayed_map.constBegin();
    while (i != displayed_map.constEnd()) {
        mapItem->removeMap(all_map[*i]->tiledMap);
        displayed_map.remove(*i);
        i = displayed_map.constBegin();
    }
    displayed_map.clear();
    all_map.clear();
}

//open the file, and load it into the variables
QString MapVisualiser::loadOtherMap(const QString &fileName)
{
    QFileInfo fileInformations(fileName);
    QString resolvedFileName=fileInformations.absoluteFilePath();
    if(all_map.contains(resolvedFileName))
        return resolvedFileName;

    Map_full *tempMapObject=new Map_full();

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QString("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return QString();
    }
    Pokecraft::Map_loader map_loader;
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
        return QString();
    }

    //copy the variables
    tempMapObject->logicalMap.width                                 = map_loader.map_to_send.width;
    tempMapObject->logicalMap.height                                = map_loader.map_to_send.height;
    tempMapObject->logicalMap.parsed_layer.walkable                 = map_loader.map_to_send.parsed_layer.walkable;
    tempMapObject->logicalMap.parsed_layer.water                    = map_loader.map_to_send.parsed_layer.water;
    tempMapObject->logicalMap.parsed_layer.grass                    = map_loader.map_to_send.parsed_layer.grass;
    tempMapObject->logicalMap.parsed_layer.dirt                     = map_loader.map_to_send.parsed_layer.dirt;
    tempMapObject->logicalMap.map_file                              = resolvedFileName;
    tempMapObject->logicalMap.border.bottom.map                     = NULL;
    tempMapObject->logicalMap.border.top.map                        = NULL;
    tempMapObject->logicalMap.border.right.map                      = NULL;
    tempMapObject->logicalMap.border.left.map                       = NULL;

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
            qDebug() << QString("Unable to locate the \"Collisions\" layer on the map: %1").arg(fileInformations.fileName());
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
    all_map[resolvedFileName]=tempMapObject;

    return resolvedFileName;
}

void MapVisualiser::loadOtherMapClientPart(Map_full *parsedMap)
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
    //load the bots
    QDomElement child = root.firstChildElement("objectgroup");
    while(!child.isNull())
    {
        if(!child.hasAttribute("name"))
            Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            Pokecraft::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            if(child.attribute("name")=="Object")
            {
                QDomElement bot = child.firstChildElement("object");
                while(!bot.isNull())
                {
                    if(!bot.hasAttribute("type"))
                        Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute("x"))
                        Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"x\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute("y"))
                        Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"y\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.isElement())
                        Pokecraft::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(bot.tagName().arg(bot.attribute("type")).arg(bot.lineNumber())));
                    else
                    {
                        quint32 x=bot.attribute("x").toUInt(&ok)/16;
                        quint32 y=(bot.attribute("y").toUInt(&ok2)/16)-1;
                        if(ok && ok2 && bot.attribute("type")=="bot")
                        {
                            QDomElement properties = bot.firstChildElement("properties");
                            while(!properties.isNull())
                            {
                                if(!properties.isElement())
                                    Pokecraft::DebugClass::debugConsole(QString("Is not an element: properties.tagName(): %1, (at line: %2)").arg(properties.tagName().arg(properties.lineNumber())));
                                else
                                {
                                    QHash<QString,QString> property_parsed;
                                    QDomElement property = properties.firstChildElement("property");
                                    while(!property.isNull())
                                    {
                                        if(!property.hasAttribute("name"))
                                            Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"name\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.hasAttribute("value"))
                                            Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"value\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.isElement())
                                            Pokecraft::DebugClass::debugConsole(QString("Is not an element: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
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
                                            loadBotFile(botFile);
                                            if(botFiles.contains(botFile))
                                                if(botFiles[botFile].contains(botId))
                                                {
                                                    Pokecraft::DebugClass::debugConsole(QString("Put bot at %1 (%2,%3)").arg(botFile).arg(x).arg(y));
                                                    parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)]=botFiles[botFile][botId];
                                                }
                                        }
                                        else
                                            Pokecraft::DebugClass::debugConsole(QString("Is not a number: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
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

void MapVisualiser::loadBotFile(const QString &fileName)
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
            Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            Pokecraft::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
            {
                botFiles[fileName][id];
                QDomElement step = child.firstChildElement("step");
                while(!step.isNull())
                {
                    if(!step.hasAttribute("id"))
                        Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.hasAttribute("type"))
                        Pokecraft::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.isElement())
                        Pokecraft::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
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
                Pokecraft::DebugClass::debugConsole(QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement("bot");
    }
}

QSet<QString> MapVisualiser::loadMap(Map_full *map,const bool &display)
{
    QSet<QString> loadedNearMap=loadNearMap(map->logicalMap.map_file,true);
    QSet<QString> loadedTeleporter=loadTeleporter(map);

    //add the new map visible directly done into loadNearMap() to have position in pixel
    //remove the not displayed map
    if(display)
    {
        QSet<QString>::const_iterator i = displayed_map.constBegin();
        while (i != displayed_map.constEnd()) {
            if(!loadedNearMap.contains(*i))
            {
                mapItem->removeMap(all_map[*i]->tiledMap);
                displayed_map.remove(*i);
                i = displayed_map.constBegin();
            }
            else
                ++i;
        }
    }
    return loadedTeleporter+loadedNearMap;
}

void MapVisualiser::removeUnusedMap()
{
    ///remove the not used map, then where no player is susceptible to switch (by border or teleporter)
    QHash<QString,Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        if(!mapUsed.contains((*i)->logicalMap.map_file))
        {
            if((*i)->logicalMap.parsed_layer.walkable!=NULL)
                delete (*i)->logicalMap.parsed_layer.walkable;
            if((*i)->logicalMap.parsed_layer.water!=NULL)
                delete (*i)->logicalMap.parsed_layer.water;
            if((*i)->logicalMap.parsed_layer.grass!=NULL)
                delete (*i)->logicalMap.parsed_layer.grass;
            if((*i)->logicalMap.parsed_layer.dirt!=NULL)
                delete (*i)->logicalMap.parsed_layer.dirt;
            qDeleteAll((*i)->tiledMap->tilesets());
            delete (*i)->tiledMap;
            delete (*i)->tiledRender;
            delete (*i);
            all_map.remove((*i)->logicalMap.map_file);
            i = all_map.constBegin();//needed
        }
        else
            ++i;
    }
}

QSet<QString> MapVisualiser::loadTeleporter(Map_full *map)
{
    QSet<QString> mapUsed;
    //load the teleporter
    int index=0;
    while(index<map->logicalMap.teleport_semi.size())
    {
        QString mapIndex=loadOtherMap(map->logicalMap.teleport_semi[index].map);
        //if is correctly loaded
        if(!mapIndex.isEmpty())
        {
            //if the teleporter is in range
            if(map->logicalMap.teleport_semi[index].destination_x<all_map[mapIndex]->logicalMap.width && map->logicalMap.teleport_semi[index].destination_y<all_map[mapIndex]->logicalMap.height)
            {
                int virtual_position=map->logicalMap.teleport_semi[index].source_x+map->logicalMap.teleport_semi[index].source_y*map->logicalMap.width;
                map->logicalMap.teleporter[virtual_position].map=&all_map[mapIndex]->logicalMap;
                map->logicalMap.teleporter[virtual_position].x=map->logicalMap.teleport_semi[index].destination_x;
                map->logicalMap.teleporter[virtual_position].y=map->logicalMap.teleport_semi[index].destination_y;

                mapUsed << mapIndex;
            }
            else
                qDebug() << QString("The teleporter is out of range: %1").arg(mapIndex);
        }
        index++;
    }
    return mapUsed;
}

QSet<QString> MapVisualiser::loadNearMap(const QString &fileName, const bool &display, const qint32 &x, const qint32 &y, const qint32 &x_pixel, const qint32 &y_pixel,const QSet<QString> &previousLoadedNearMap)
{
    if(previousLoadedNearMap.contains(fileName))
        return QSet<QString>();

    Map_full *tempMapObject;
    if(!all_map.contains(fileName))
    {
        qDebug() << QString("loadNearMap(): the current map is unable to load: %1").arg(fileName);
        return QSet<QString>();
    }
    else
        tempMapObject=all_map[fileName];

    QSet<QString> loadedNearMap;
    loadedNearMap << fileName;

    QString mapIndex;
    QRect current_map_rect;
    if(current_map!=NULL)
        current_map_rect=QRect(0,0,current_map->logicalMap.width,current_map->logicalMap.height);
    else
        qDebug() << "The current map is not set, crash prevented";

    //reset the other map
    tempMapObject->logicalMap.teleporter.clear();
    tempMapObject->logicalMap.border.bottom.map=NULL;
    tempMapObject->logicalMap.border.top.map=NULL;
    tempMapObject->logicalMap.border.left.map=NULL;
    tempMapObject->logicalMap.border.right.map=NULL;

    //display a new map now visible
    if(display && !displayed_map.contains(fileName))
    {
        mapItem->addMap(tempMapObject->tiledMap,tempMapObject->tiledRender,tempMapObject->objectGroupIndex);
        displayed_map << fileName;
    }
    mapItem->setMapPosition(tempMapObject->tiledMap,x_pixel,y_pixel);

    //if have bottom border
    if(!tempMapObject->logicalMap.border_semi.bottom.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if((y+tempMapObject->logicalMap.height)<=current_map->logicalMap.height)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.bottom.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.top.fileName && tempMapObject->logicalMap.border_semi.bottom.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.bottom.x_offset-all_map[mapIndex]->logicalMap.border_semi.top.x_offset;
                    const qint32 x_sub=x+offset;
                    const qint32 y_sub=y+tempMapObject->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(current_map!=NULL && RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.bottom.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.bottom.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.top.x_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): bottom: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): bottom: not correctly loaded %1").arg(fileName);
        }
    }
    //if have top border
    if(!tempMapObject->logicalMap.border_semi.top.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if(y>=0)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.top.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.bottom.fileName && tempMapObject->logicalMap.border_semi.top.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.top.x_offset-all_map[mapIndex]->logicalMap.border_semi.bottom.x_offset;
                    const qint32 x_sub=x+offset;
                    const qint32 y_sub=y-all_map[mapIndex]->logicalMap.height;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(current_map!=NULL && RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.top.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.top.x_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.bottom.x_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): top: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): top: not correctly loaded %1").arg(fileName);
        }
    }
    //if have right border
    if(!tempMapObject->logicalMap.border_semi.right.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if((x+tempMapObject->logicalMap.width)<=current_map->logicalMap.width)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.right.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.left.fileName && tempMapObject->logicalMap.border_semi.right.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.right.y_offset-all_map[mapIndex]->logicalMap.border_semi.left.y_offset;
                    const qint32 x_sub=x+tempMapObject->logicalMap.width;
                    const qint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(current_map!=NULL && RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.right.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.right.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.left.y_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): right: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): right: not correctly loaded %1").arg(fileName);
        }
    }
    //if have left border
    if(!tempMapObject->logicalMap.border_semi.left.fileName.isEmpty())
    {
        //if the position is good to have border in range
        if(x>=0)
        {
            mapIndex=loadOtherMap(tempMapObject->logicalMap.border_semi.left.fileName);
            //if is correctly loaded
            if(!mapIndex.isEmpty())
            {
                //if both border match
                if(fileName==all_map[mapIndex]->logicalMap.border_semi.right.fileName && tempMapObject->logicalMap.border_semi.left.fileName==mapIndex)
                {
                    int offset=tempMapObject->logicalMap.border_semi.left.y_offset-all_map[mapIndex]->logicalMap.border_semi.right.y_offset;
                    const qint32 x_sub=x-all_map[mapIndex]->logicalMap.width;
                    const qint32 y_sub=y+offset;
                    QRect border_map_rect(x_sub,y_sub,all_map[mapIndex]->logicalMap.width,all_map[mapIndex]->logicalMap.height);
                    //if the new map touch the current map
                    if(current_map!=NULL && RectTouch(current_map_rect,border_map_rect))
                    {
                        tempMapObject->logicalMap.border.left.map=&all_map[mapIndex]->logicalMap;
                        tempMapObject->logicalMap.border.left.y_offset=-offset;
                        all_map[mapIndex]->logicalMap.border.right.y_offset=offset;

                        loadedNearMap+=loadNearMap(mapIndex,display,x_sub,y_sub,x_pixel+(x_sub-x)*tempMapObject->tiledMap->tileWidth(),y_pixel+(y_sub-y)*tempMapObject->tiledMap->tileHeight(),previousLoadedNearMap+loadedNearMap);
                    }
                }
                else
                    qDebug() << QString("loadNearMap(): left: have not mutual border %1").arg(fileName);
            }
            else
                qDebug() << QString("loadNearMap(): left: not correctly loaded %1").arg(fileName);
        }
    }
    return loadedNearMap;
}
