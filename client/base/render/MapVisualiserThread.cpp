#include "MapVisualiserThread.h"
#include "MapItem.h"
#include "../../general/base/FacilityLib.h"
#include <QFileInfo>
#include <QRegularExpression>
#include "../ClientVariable.h"

MapVisualiserThread::MapVisualiserThread()
{
    moveToThread(this);
    start(QThread::IdlePriority);
    hideTheDoors=true;
    regexMs=QRegularExpression(QStringLiteral("^[0-9]{1,5}ms$"));
    regexFrames=QRegularExpression(QStringLiteral("^[0-9]{1,3}frames$"));
}

MapVisualiserThread::~MapVisualiserThread()
{
    quit();
    wait();
}

void MapVisualiserThread::loadOtherMapAsync(const QString &fileName)
{
    /*cash due to the pointerif(mapCache.contains(fileName))
    {
        MapVisualiserThread::Map_full *tempMapObject=new MapVisualiserThread::Map_full();
        *tempMapObject=mapCache[fileName];
        emit asyncMapLoaded(fileName,tempMapObject);
        return;
    }*/
    MapVisualiserThread::Map_full *tempMapObject=loadOtherMap(fileName);
    /*if(mapCache.size()>200)
        mapCache.clear();
    mapCache[fileName]=*tempMapObject;*/
    emit asyncMapLoaded(fileName,tempMapObject);
}

QString MapVisualiserThread::error()
{
    return mLastError;
}

//open the file, and load it into the variables
MapVisualiserThread::Map_full *MapVisualiserThread::loadOtherMap(const QString &resolvedFileName)
{
    if(stopIt)
        return NULL;
    MapVisualiserThread::Map_full *tempMapObject=new MapVisualiserThread::Map_full();

    tempMapObject->displayed=false;
    tempMapObject->relative_x=0;
    tempMapObject->relative_y=0;
    tempMapObject->relative_x_pixel=0;
    tempMapObject->relative_y_pixel=0;
    tempMapObject->tiledMap=NULL;
    tempMapObject->tiledRender=NULL;
    tempMapObject->objectGroup=NULL;
    tempMapObject->objectGroupIndex=0;

    /*Tiled::Layer *grass;
    Tiled::Layer *grassOver;*/

    //load the map
    tempMapObject->tiledMap = reader.readMap(resolvedFileName);
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString();
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(reader.errorString());
        delete tempMapObject;
        return NULL;
    }
    if(stopIt)
    {
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }
    CatchChallenger::Map_loader map_loader;
    if(!map_loader.tryLoadMap(resolvedFileName))
    {
        mLastError=map_loader.errorString();
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(map_loader.errorString());
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
    if(stopIt)
    {
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    //copy the variables
    tempMapObject->logicalMap.xmlRoot                               = map_loader.map_to_send.xmlRoot;
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

    if(tempMapObject->tiledMap->tileHeight()!=CLIENT_BASE_TILE_SIZE || tempMapObject->tiledMap->tileWidth()!=CLIENT_BASE_TILE_SIZE)
    {
        mLastError=QStringLiteral("Map tile size not multiple of %1").arg(CLIENT_BASE_TILE_SIZE);
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(mLastError);
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

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
    //tempMapObject->tiledRender->setObjectBorder(false);

    //do the object group to move the player on it
    tempMapObject->objectGroup = new Tiled::ObjectGroup(QStringLiteral("Dyna management"),0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());

    //add a tags
    if(debugTags)
    {
        Tiled::MapObject * tagMapObject = new Tiled::MapObject();
        Tiled::Cell cell=tagMapObject->cell();
        cell.tile=tagTileset->tileAt(tagTilesetIndex);
        tagMapObject->setCell(cell);
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
                if(objectGroup->name()==QStringLiteral("Moving"))
                {
                    QList<Tiled::MapObject*> objects=objectGroup->objects();
                    int index2=0;
                    while(index2<objects.size())
                    {
                        //remove the unknow object
                        if(objects.at(index2)->type()!=QStringLiteral("door") || hideTheDoors)
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        index2++;
                    }
                    index++;
                }
                else if(objectGroup->name()==QStringLiteral("Object"))
                {
                    QList<Tiled::MapObject*> objects=objectGroup->objects();
                    int index2=0;
                    while(index2<objects.size())
                    {
                        //remove the bot
                        if(objects.at(index2)->type()!=QStringLiteral("bot"))
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
        if(tempMapObject->tiledMap->layerAt(index)->name()==QStringLiteral("WalkBehind"))
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
            if(tempMapObject->tiledMap->layerAt(index)->name()==QStringLiteral("Collisions"))
            {
                tempMapObject->objectGroupIndex=index+1;
                tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
                break;
            }
            index++;
        }
        if(index==tempMapObject->tiledMap->layerCount())
        {
            qDebug() << QStringLiteral("Unable to locate the \"Collisions\" layer on the map: %1").arg(resolvedFileName);
            tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
        }
    }

    //move the Moving layer on dyna layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            if(objectGroup->name()==QStringLiteral("Moving"))
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
            if(tileLayer->name()==QStringLiteral("Grass"))
            {
                /*grass = tempMapObject->tiledMap->takeLayerAt(index);
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
                 }*/
            }
        }
        index++;
    }
    //load the animation into tile layer
    index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::TileLayer *tileLayer = tempMapObject->tiledMap->layerAt(index)->asTileLayer())
        {
            int x=0,y;
            while(x<tileLayer->width())
            {
                y=0;
                while(y<tileLayer->height())
                {
                    Tiled::Cell cell=tileLayer->cellAt(x,y);
                    Tiled::Tile *tile=cell.tile;
                    if(tile!=NULL)
                    {
                        QString animation=tile->property(QStringLiteral("animation"));
                        if(!animation.isEmpty())
                        {
                            QStringList animationList=animation.split(QStringLiteral(";"));
                            if(animationList.size()==2)
                            {
                                if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                {
                                    QString msString=animationList.at(0);
                                    QString framesString=animationList.at(1);
                                    msString.remove(QStringLiteral("ms"));
                                    framesString.remove(QStringLiteral("frames"));
                                    quint16 ms=msString.toUShort();
                                    quint8 frames=framesString.toUShort();
                                    if(ms>0 && frames>1)
                                    {
                                        {
                                            Tiled::Cell cell;
                                            cell.tile=NULL;
                                            tileLayer->setCell(x,y,cell);
                                        }
                                        Tiled::ObjectGroup *objectGroup=NULL;
                                        if(index<(tempMapObject->tiledMap->layerCount()))
                                            if(Tiled::ObjectGroup *objectGroupTemp = tempMapObject->tiledMap->layerAt(index+1)->asObjectGroup())
                                                objectGroup=objectGroupTemp;
                                        if(objectGroup==NULL)
                                        {
                                            objectGroup=new Tiled::ObjectGroup;
                                            tempMapObject->tiledMap->insertLayer(index+1,objectGroup);
                                        }
                                        Tiled::MapObject *object=new Tiled::MapObject();
                                        objectGroup->addObject(object);
                                        object->setPosition(QPointF(x,y+1));
                                        Tiled::Cell cell=object->cell();
                                        cell.tile=tile;
                                        object->setCell(cell);
                                        if(!tempMapObject->animatedObject.contains(ms))
                                        {
                                            Map_animation tempAnimationDescriptor;
                                            tempAnimationDescriptor.count=0;
                                            tempAnimationDescriptor.frames=frames;
                                            tempMapObject->animatedObject[ms]=tempAnimationDescriptor;
                                        }
                                        /// \todo control the animation is not out of rame
                                        tempMapObject->animatedObject[ms].animatedObject << object;
                                    }
                                    else
                                        qDebug() << "ms is 0 or frame is <=1";
                                }
                                else
                                    qDebug() << "Wrong animation tile args regex match";
                            }
                            else
                                qDebug() << "Wrong animation tile args count";
                        }
                    }
                    y++;
                }
                x++;
            }
        }
        index++;
    }

    if(stopIt)
    {
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    if(!loadOtherMapClientPart(tempMapObject))
    {
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    return tempMapObject;
}

//drop and remplace by Map_loader info
bool MapVisualiserThread::loadOtherMapClientPart(MapVisualiserThread::Map_full *parsedMap)
{
    QString fileName=parsedMap->logicalMap.map_file;
    QFile mapFile(fileName);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        qDebug() << mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
        return false;
    }
    QByteArray xmlContent=mapFile.readAll();
    mapFile.close();
    if(stopIt)
            return false;
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return false;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="map")
    {
        qDebug() << QStringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }
    bool ok,ok2;
    //load the bots (map->bots)
    QDomElement child = root.firstChildElement(QStringLiteral("objectgroup"));
    while(!child.isNull())
    {
        if(!child.hasAttribute(QStringLiteral("name")))
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute(QStringLiteral("name"))).arg(child.lineNumber())));
        else
        {
            if(child.attribute(QStringLiteral("name"))==QStringLiteral("Object"))
            {
                QDomElement bot = child.firstChildElement(QStringLiteral("object"));
                while(!bot.isNull())
                {
                    if(!bot.hasAttribute(QStringLiteral("type")))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute(QStringLiteral("x")))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"x\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute(QStringLiteral("y")))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"y\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.isElement())
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(bot.tagName().arg(bot.attribute("type")).arg(bot.lineNumber())));
                    else
                    {
                        quint32 x=bot.attribute(QStringLiteral("x")).toUInt(&ok)/CLIENT_BASE_TILE_SIZE;
                        quint32 y=(bot.attribute(QStringLiteral("y")).toUInt(&ok2)/CLIENT_BASE_TILE_SIZE)-1;
                        if(ok && ok2 && (bot.attribute(QStringLiteral("type"))==QStringLiteral("bot") || bot.attribute(QStringLiteral("type"))==QStringLiteral("botfight")))
                        {
                            QDomElement properties = bot.firstChildElement(QStringLiteral("properties"));
                            while(!properties.isNull())
                            {
                                if(!properties.isElement())
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: properties.tagName(): %1, (at line: %2)").arg(properties.tagName().arg(properties.lineNumber())));
                                else
                                {
                                    QHash<QString,QString> property_parsed;
                                    QDomElement property = properties.firstChildElement(QStringLiteral("property"));
                                    while(!property.isNull())
                                    {
                                        if(!property.hasAttribute(QStringLiteral("name")))
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"name\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.hasAttribute(QStringLiteral("value")))
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"value\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.isElement())
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                        else
                                            property_parsed[property.attribute(QStringLiteral("name"))]=property.attribute(QStringLiteral("value"));
                                        property = property.nextSiblingElement(QStringLiteral("property"));
                                    }
                                    if(property_parsed.contains(QStringLiteral("file")) && property_parsed.contains(QStringLiteral("id")))
                                    {
                                        quint32 botId=property_parsed[QStringLiteral("id")].toUInt(&ok);
                                        if(ok)
                                        {
                                            QString botFile=QFileInfo(QFileInfo(fileName).absolutePath()+QStringLiteral("/")+property_parsed[QStringLiteral("file")]).absoluteFilePath();
                                            if(!botFile.endsWith(QStringLiteral(".xml")))
                                                botFile+=QStringLiteral(".xml");
                                            if(bot.attribute(QStringLiteral("type"))==QStringLiteral("bot"))
                                            {
                                                if(stopIt)
                                                        return false;
                                                loadBotFile(botFile);
                                                if(stopIt)
                                                        return false;
                                                if(botFiles.contains(botFile))
                                                {
                                                    if(botFiles[botFile].contains(botId))
                                                    {
                                                        #ifdef DEBUG_CLIENT_BOT
                                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Put bot %1 (%2) at %3 (%4,%5)").arg(botFile).arg(botId).arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
                                                        #endif
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)]=botFiles[botFile][botId];
                                                        property_parsed.remove(QStringLiteral("file"));
                                                        property_parsed.remove(QStringLiteral("id"));
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].properties=property_parsed;
                                                        parsedMap->logicalMap.bots[QPair<quint8,quint8>(x,y)].botId=botId;
                                                    }
                                                    else
                                                        CatchChallenger::DebugClass::debugConsole(
                                                                    QStringLiteral("No botId %1 into %2: properties.tagName(): %3, file: %4 (at line: %5)")
                                                                    .arg(botId)
                                                                    .arg(botFile)
                                                                    .arg(bot.tagName())
                                                                    .arg(parsedMap->logicalMap.map_file)
                                                                    .arg(bot.lineNumber())
                                                                    );
                                                }
                                                else
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("No file %1: properties.tagName(): %2, name: %3 (at line: %4)").arg(botFile).arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                            }
                                        }
                                        else
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not a number: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                    }
                                }
                                properties = properties.nextSiblingElement(QStringLiteral("properties"));
                            }
                        }
                    }
                    bot = bot.nextSiblingElement(QStringLiteral("object"));
                }
            }
        }
        child = child.nextSiblingElement(QStringLiteral("objectgroup"));
    }
    return true;
}

void MapVisualiserThread::loadBotFile(const QString &fileName)
{
    if(botFiles.contains(fileName))
        return;
    botFiles[fileName];//create the entry
    QFile mapFile(fileName);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        qDebug() << mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
        return;
    }
    QByteArray xmlContent=mapFile.readAll();
    mapFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("bots"))
    {
        qDebug() << QStringLiteral("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement(QStringLiteral("bot"));
    while(!child.isNull())
    {
        if(!child.hasAttribute(QStringLiteral("id")))
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 botId=child.attribute(QStringLiteral("id")).toUInt(&ok);
            if(ok)
            {
                if(botFiles[fileName].contains(botId))
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("bot already found with this id: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
                else
                {
                    QDomElement step = child.firstChildElement(QStringLiteral("step"));
                    while(!step.isNull())
                    {
                        if(!step.hasAttribute(QStringLiteral("id")))
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                        else if(!step.hasAttribute(QStringLiteral("type")))
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                        else if(!step.isElement())
                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
                        else
                        {
                            quint8 stepId=step.attribute(QStringLiteral("id")).toUShort(&ok);
                            if(ok)
                                botFiles[fileName][botId].step[stepId]=step;
                        }
                        step = step.nextSiblingElement(QStringLiteral("step"));
                    }
                    if(!botFiles[fileName][botId].step.contains(1))
                        botFiles[fileName].remove(botId);
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement(QStringLiteral("bot"));
    }
}

void MapVisualiserThread::resetAll()
{
    botFiles.clear();
}
