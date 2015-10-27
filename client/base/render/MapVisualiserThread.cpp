#include "MapVisualiserThread.h"
#include "MapItem.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../tiled/tiled_mapobject.h"
#include <QFileInfo>
#include <QRegularExpression>
#include "../ClientVariable.h"
#include "../interface/DatapackClientLoader.h"
#include "../LanguagesSelect.h"
#include "../FacilityLibClient.h"
#include <QDebug>

MapVisualiserThread::MapVisualiserThread()
{
    moveToThread(this);
    start(QThread::IdlePriority);
    hideTheDoors=false;
    language=LanguagesSelect::languagesSelect->getCurrentLanguages();
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
        *tempMapObject=mapCache.value(fileName);
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

    tileToTriggerAnimationContent.clear();

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
    {
        QTime time;
        time.restart();
        tempMapObject->tiledMap = reader.readMap(resolvedFileName);
        qDebug() << QStringLiteral("%1 loaded into %2ms").arg(resolvedFileName).arg(time.elapsed());
    }
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
    if(!map_loader.tryLoadMap(resolvedFileName.toStdString()))
    {
        mLastError=QString::fromStdString(map_loader.errorString());
        qDebug() << QStringLiteral("Unable to load the map: %1, error: %2").arg(resolvedFileName).arg(QString::fromStdString(map_loader.errorString()));
        int index=0;
        const int &listSize=tempMapObject->tiledMap->tilesets().size();
        while(index<listSize)
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
    tempMapObject->logicalMap.map_file                              = resolvedFileName.toStdString();
    tempMapObject->logicalMap.border.bottom.map                     = NULL;
    tempMapObject->logicalMap.border.top.map                        = NULL;
    tempMapObject->logicalMap.border.right.map                      = NULL;
    tempMapObject->logicalMap.border.left.map                       = NULL;
    //load the item
    {
        unsigned int index=0;
        while(index<map_loader.map_to_send.items.size())
        {
            const CatchChallenger::Map_to_send::ItemOnMap_Semi &item=map_loader.map_to_send.items.at(index);
            CatchChallenger::Map_client::ItemOnMapForClient newItem;
            newItem.infinite=item.infinite;
            newItem.item=item.item;
            newItem.tileObject=NULL;
            newItem.indexOfItemOnMap=0;
            if(DatapackClientLoader::datapackLoader.itemOnMap.contains(resolvedFileName))
            {
                if(DatapackClientLoader::datapackLoader.itemOnMap.value(resolvedFileName).contains(QPair<uint8_t,uint8_t>(item.point.x,item.point.y)))
                    newItem.indexOfItemOnMap=DatapackClientLoader::datapackLoader.itemOnMap.value(resolvedFileName).value(QPair<uint8_t,uint8_t>(item.point.x,item.point.y));
                else
                    qDebug() << QStringLiteral("Map itemOnMap %1,%2 not found").arg(item.point.x).arg(item.point.y);
            }
            else
                qDebug() << QStringLiteral("Map itemOnMap %1 not found into: %2").arg(resolvedFileName).arg(QStringList(DatapackClientLoader::datapackLoader.itemOnMap.keys()).join(";"));
            tempMapObject->logicalMap.itemsOnMap[QPair<uint8_t,uint8_t>(item.point.x,item.point.y)]=newItem;
            index++;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!DatapackClientLoader::datapackLoader.fullMapPathToId.contains(resolvedFileName))
    {
        mLastError=QStringLiteral("Map id unresolved %1").arg(resolvedFileName);
        const QStringList &mapList=DatapackClientLoader::datapackLoader.fullMapPathToId.keys();
        qDebug() << QStringLiteral("Map id unresolved %1 into %2").arg(resolvedFileName).arg(mapList.join(";"));
        delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }
    #endif
    tempMapObject->logicalMap.id                                    = DatapackClientLoader::datapackLoader.fullMapPathToId.value(resolvedFileName);

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
    if(!map_loader.map_to_send.border.bottom.fileName.empty())
        tempMapObject->logicalMap.border_semi.bottom.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.bottom.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.top.fileName.empty())
        tempMapObject->logicalMap.border_semi.top.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.top.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.right.fileName.empty())
        tempMapObject->logicalMap.border_semi.right.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.right.fileName)).absoluteFilePath().toStdString();
    if(!map_loader.map_to_send.border.left.fileName.empty())
        tempMapObject->logicalMap.border_semi.left.fileName=QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.border_semi.left.fileName)).absoluteFilePath().toStdString();

    //load the string
    tempMapObject->logicalMap.teleport_semi.clear();
    {
        int index=0;
        const int &listSize=map_loader.map_to_send.teleport.size();
        while(index<listSize)
        {
            tempMapObject->logicalMap.teleport_semi << map_loader.map_to_send.teleport.at(index);
            tempMapObject->logicalMap.teleport_semi[index].map                      = QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.teleport_semi.at(index).map)).absoluteFilePath().toStdString();
            QDomElement item=map_loader.map_to_send.teleport.at(index).conditionUnparsed;
            QString conditionText;
            {
                bool text_found=false;
                QDomElement blockedtext = item.firstChildElement(MapVisualiserThread::text_blockedtext);
                if(!language.isEmpty() && language!=MapVisualiserThread::text_en)
                    while(!blockedtext.isNull())
                    {
                        if(blockedtext.isElement())
                        {
                            if(blockedtext.hasAttribute(MapVisualiserThread::text_lang) && blockedtext.attribute(MapVisualiserThread::text_lang)==language)
                            {
                                conditionText=blockedtext.text();
                                text_found=true;
                                break;
                            }
                        }
                        blockedtext = blockedtext.nextSiblingElement(MapVisualiserThread::text_blockedtext);
                    }
                if(!text_found)
                {
                    blockedtext = item.firstChildElement(MapVisualiserThread::text_blockedtext);
                    while(!blockedtext.isNull())
                    {
                        if(blockedtext.isElement())
                        {
                            if(!blockedtext.hasAttribute(MapVisualiserThread::text_lang) || blockedtext.attribute(MapVisualiserThread::text_lang)==MapVisualiserThread::text_en)
                            {
                                conditionText=blockedtext.text();
                                break;
                            }
                        }
                        blockedtext = blockedtext.nextSiblingElement(MapVisualiserThread::text_blockedtext);
                    }
                }
            }
            tempMapObject->logicalMap.teleport_condition_texts << conditionText;
            index++;
        }
    }

    tempMapObject->logicalMap.rescue_points            = CatchChallenger::stdvectorToQList(map_loader.map_to_send.rescue_points);

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
    tempMapObject->objectGroup = new Tiled::ObjectGroup(MapVisualiserThread::text_Dyna_management,0,0,tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height());
    tempMapObject->objectGroup->setName("objectGroup for player layer");

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
    else
        MapVisualiserThread::layerChangeLevelAndTagsChange(tempMapObject,hideTheDoors);

    //load the animation into tile layer
    {
        int index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::TileLayer *tileLayer = tempMapObject->tiledMap->layerAt(index)->asTileLayer())
            {
                const int &width=tileLayer->width();
                const int &height=tileLayer->height();
                int x=0,y;
                while(x<width)
                {
                    y=0;
                    while(y<height)
                    {
                        Tiled::Cell cell=tileLayer->cellAt(x,y);
                        Tiled::Tile *tile=cell.tile;
                        if(tile!=NULL)
                        {
                            const QString &animation=tile->property(MapVisualiserThread::text_animation);
                            if(!animation.isEmpty())
                            {
                                const QStringList &animationList=animation.split(MapVisualiserThread::text_dotcomma);
                                if(animationList.size()>=2)
                                {
                                    if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                    {
                                        QString msString=animationList.at(0);
                                        QString framesString=animationList.at(1);
                                        msString.remove(MapVisualiserThread::text_ms);
                                        framesString.remove(MapVisualiserThread::text_frames);
                                        uint16_t ms=msString.toUShort();
                                        uint8_t frames=framesString.toUShort();
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
                                                objectGroup->setName("Layer for animation "+tileLayer->name());
                                                tempMapObject->tiledMap->insertLayer(index+1,objectGroup);
                                            }
                                            Tiled::MapObject *object=new Tiled::MapObject();
                                            objectGroup->addObject(object);
                                            object->setPosition(QPointF(x,y+1));
                                            Tiled::Cell cell=object->cell();
                                            if(!tempMapObject->animatedObject.contains(ms))
                                            {
                                                Map_animation tempAnimationDescriptor;
                                                tempAnimationDescriptor.count=0;
                                                tempAnimationDescriptor.frameCountTotal=frames;
                                                tempMapObject->animatedObject[ms]=tempAnimationDescriptor;
                                            }
                                            Map_animation_object map_animation_object;
                                            map_animation_object.randomOffset=0;
                                            if(animationList.size()>=3 && animationList.at(2)==MapVisualiserThread::text_randomoffset)
                                                map_animation_object.randomOffset=rand()%frames;
                                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                            map_animation_object.minId=tile->id();
                                            map_animation_object.maxId=tile->id()+frames;
                                            #endif
                                            cell.tile=tile->tileset()->tileAt(tile->id()+map_animation_object.randomOffset);
                                            object->setCell(cell);
                                            map_animation_object.animatedObject=object;
                                            /// \todo control the animation is not out of rame
                                            tempMapObject->animatedObject[ms].animatedObjectList << map_animation_object;
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
                            else
                            {
                                if(tileToTriggerAnimationContent.contains(tile))
                                {
                                    const TriggerAnimationContent &content=tileToTriggerAnimationContent.value(tile);
                                    Tiled::ObjectGroup *objectGroup=NULL;
                                    if(index<(tempMapObject->tiledMap->layerCount()))
                                        if(Tiled::ObjectGroup *objectGroupTemp = tempMapObject->tiledMap->layerAt(index+1)->asObjectGroup())
                                            objectGroup=objectGroupTemp;
                                    if(objectGroup==NULL)
                                    {
                                        objectGroup=new Tiled::ObjectGroup;
                                        objectGroup->setName("From cache under "+tileLayer->name());
                                        tempMapObject->tiledMap->insertLayer(index+1,objectGroup);
                                    }
                                    Tiled::MapObject* object=new Tiled::MapObject();
                                    Tiled::MapObject* objectOver=NULL;
                                    Tiled::Cell cell;
                                    cell.tile=content.objectTile;
                                    object->setCell(cell);
                                    objectGroup->addObject(object);
                                    object->setPosition(QPointF(x,y+1));
                                    //do the layer over
                                    if(content.over)
                                    {
                                        objectOver=new Tiled::MapObject();
                                        Tiled::Cell cell;
                                        cell.tile=content.objectTileOver;
                                        objectOver->setCell(cell);
                                        tempMapObject->objectGroup->addObject(objectOver);
                                        objectOver->setPosition(QPointF(x,y+1));
                                    }
                                    //register on map
                                    tempMapObject->triggerAnimations[QPair<uint8_t,uint8_t>(x,y)]=new TriggerAnimation(
                                                object,
                                                objectOver,
                                                content.framesCountEnter,content.msEnter,
                                                content.framesCountLeave,content.msLeave,
                                                content.framesCountAgain,content.msAgain,
                                                content.over
                                                );
                                    {
                                        Tiled::Cell cell;
                                        cell.tile=NULL;
                                        tileLayer->setCell(x,y,cell);
                                    }
                                }
                                else
                                {
                                    const QString &trigger=tile->property(MapVisualiserThread::text_trigger);
                                    if(!trigger.isEmpty())
                                    {
                                        if(trigger.contains(regexTrigger))
                                        {
                                            TriggerAnimationContent content;
                                            content.objectTile=tile;
                                            content.objectTileOver=NULL;
                                            content.framesCountEnter=0;
                                            content.msEnter=0;
                                            content.framesCountLeave=0;
                                            content.msLeave=0;
                                            content.framesCountAgain=0;
                                            content.msAgain=0;
                                            content.over=false;
                                            QString tempString=trigger;
                                            tempString.replace(regexTrigger,"\\1");
                                            content.msEnter=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\2");
                                            content.framesCountEnter=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\3");
                                            content.msLeave=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\4");
                                            content.framesCountLeave=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\5");
                                            //again here
                                            if(tempString.contains(regexTriggerAgain))
                                            {
                                                QString againString=tempString;
                                                againString.replace(regexTriggerAgain,"\\1");
                                                content.msAgain=againString.toUShort();
                                                againString=tempString;
                                                againString.replace(regexTriggerAgain,"\\2");
                                                content.framesCountAgain=againString.toUShort();
                                            }
                                            //over here
                                            if(tempString.contains(QStringLiteral("over")))
                                            {
                                                content.over=true;
                                                content.objectTileOver=tile->tileset()->tileAt(tile->id()+tile->tileset()->columnCount());
                                            }
                                            tileToTriggerAnimationContent[tile]=content;
                                            //create tile and add
                                            {
                                                Tiled::ObjectGroup *objectGroup=NULL;
                                                if(index<(tempMapObject->tiledMap->layerCount()))
                                                    if(Tiled::ObjectGroup *objectGroupTemp = tempMapObject->tiledMap->layerAt(index+1)->asObjectGroup())
                                                        objectGroup=objectGroupTemp;
                                                if(objectGroup==NULL)
                                                {
                                                    objectGroup=new Tiled::ObjectGroup;
                                                    objectGroup->setName("Under "+tileLayer->name());
                                                    tempMapObject->tiledMap->insertLayer(index+1,objectGroup);
                                                }
                                                Tiled::MapObject* object=new Tiled::MapObject();
                                                Tiled::MapObject* objectOver=NULL;
                                                Tiled::Cell cell;
                                                cell.tile=content.objectTile;
                                                object->setCell(cell);
                                                objectGroup->addObject(object);
                                                object->setPosition(QPointF(x,y+1));
                                                //do the layer over
                                                if(content.over)
                                                {
                                                    objectOver=new Tiled::MapObject();
                                                    Tiled::Cell cell;
                                                    cell.tile=content.objectTileOver;
                                                    objectOver->setCell(cell);
                                                    tempMapObject->objectGroup->addObject(objectOver);
                                                    objectOver->setPosition(QPointF(x,y+1));
                                                }
                                                //register on map
                                                tempMapObject->triggerAnimations[QPair<uint8_t,uint8_t>(x,y)]=new TriggerAnimation(
                                                            object,
                                                            objectOver,
                                                            content.framesCountEnter,content.msEnter,
                                                            content.framesCountLeave,content.msLeave,
                                                            content.framesCountAgain,content.msAgain,
                                                            content.over
                                                            );
                                            }
                                            Tiled::Cell cell;
                                            cell.tile=NULL;
                                            tileLayer->setCell(x,y,cell);
                                        }
                                        else
                                            qDebug() << "Wrong animation trigger string";
                                    }
                                }
                            }
                        }
                        y++;
                    }
                    x++;
                }
            }
            index++;
        }
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
    loadOtherMapMetaData(tempMapObject);

    return tempMapObject;
}

//drop and remplace by Map_loader info
bool MapVisualiserThread::loadOtherMapClientPart(MapVisualiserThread::Map_full *parsedMap)
{
    QDomDocument domDocument;
    //open and quick check the file
    const QString &fileName=QString::fromStdString(parsedMap->logicalMap.map_file);
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(fileName.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(fileName.toStdString());
    else
    {
        QFile mapFile(fileName);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
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
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[fileName.toStdString()]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=MapVisualiserThread::text_map)
    {
        qDebug() << QStringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }
    bool ok,ok2;
    //load the bots (map->bots)
    QDomElement child = root.firstChildElement(MapVisualiserThread::text_objectgroup);
    while(!child.isNull())
    {
        if(!child.hasAttribute(MapVisualiserThread::text_name))
            qDebug() << (QStringLiteral("Has not attribute \"name\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            qDebug() << (QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute(QStringLiteral("name"))).arg(child.lineNumber())));
        else
        {
            if(child.attribute(MapVisualiserThread::text_name)==MapVisualiserThread::text_Object)
            {
                QDomElement bot = child.firstChildElement(MapVisualiserThread::text_object);
                while(!bot.isNull())
                {
                    if(!bot.hasAttribute(MapVisualiserThread::text_type))
                        qDebug() << (QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute(MapVisualiserThread::text_x))
                        qDebug() << (QStringLiteral("Has not attribute \"x\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.hasAttribute(MapVisualiserThread::text_y))
                        qDebug() << (QStringLiteral("Has not attribute \"y\": bot.tagName(): %1 (at line: %2)").arg(bot.tagName()).arg(bot.lineNumber()));
                    else if(!bot.isElement())
                        qDebug() << (QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(bot.tagName().arg(bot.attribute("type")).arg(bot.lineNumber())));
                    else
                    {
                        const uint32_t &x=bot.attribute(MapVisualiserThread::text_x).toUInt(&ok)/CLIENT_BASE_TILE_SIZE;
                        const uint32_t &y=(bot.attribute(MapVisualiserThread::text_y).toUInt(&ok2)/CLIENT_BASE_TILE_SIZE)-1;
                        if(ok && ok2 && (bot.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_bot || bot.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_botfight))
                        {
                            QDomElement properties = bot.firstChildElement(MapVisualiserThread::text_properties);
                            while(!properties.isNull())
                            {
                                if(!properties.isElement())
                                    qDebug() << (QStringLiteral("Is not an element: properties.tagName(): %1, (at line: %2)").arg(properties.tagName().arg(properties.lineNumber())));
                                else
                                {
                                    std::unordered_map<std::string,std::string> property_parsed;
                                    QDomElement property = properties.firstChildElement(MapVisualiserThread::text_property);
                                    while(!property.isNull())
                                    {
                                        if(!property.hasAttribute(MapVisualiserThread::text_name))
                                            qDebug() << (QStringLiteral("Has not attribute \"name\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.hasAttribute(MapVisualiserThread::text_value))
                                            qDebug() << (QStringLiteral("Has not attribute \"value\": property.tagName(): %1 (at line: %2)").arg(property.tagName()).arg(property.lineNumber()));
                                        else if(!property.isElement())
                                            qDebug() << (QStringLiteral("Is not an element: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                        else
                                            property_parsed[property.attribute(MapVisualiserThread::text_name).toStdString()]=property.attribute(MapVisualiserThread::text_value).toStdString();
                                        property = property.nextSiblingElement(MapVisualiserThread::text_property);
                                    }
                                    if(property_parsed.find("file")!=property_parsed.cend() && property_parsed.find("id")!=property_parsed.cend())
                                    {
                                        const uint32_t &botId=stringtouint32(property_parsed.at("id"),&ok);
                                        if(ok)
                                        {
                                            QString botFile(QFileInfo(QFileInfo(fileName).absolutePath()+MapVisualiserThread::text_slash+QString::fromStdString(property_parsed.at("file"))).absoluteFilePath());
                                            if(!botFile.endsWith(MapVisualiserThread::text_dotxml))
                                                botFile+=MapVisualiserThread::text_dotxml;
                                            if(bot.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_bot)
                                            {
                                                if(stopIt)
                                                        return false;
                                                loadBotFile(botFile);
                                                if(stopIt)
                                                        return false;
                                                if(botFiles.contains(botFile))
                                                {
                                                    if(botFiles.value(botFile).contains(botId))
                                                    {
                                                        #ifdef DEBUG_CLIENT_BOT
                                                        qDebug() << (QStringLiteral("Put bot %1 (%2) at %3 (%4,%5)").arg(botFile).arg(botId).arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
                                                        #endif
                                                        CatchChallenger::Bot &bot=parsedMap->logicalMap.bots[QPair<uint8_t,uint8_t>(x,y)];
                                                        bot=botFiles.value(botFile).value(botId);
                                                        property_parsed.erase("file");
                                                        property_parsed.erase("id");
                                                        bot.properties=property_parsed;
                                                        bot.botId=botId;

                                                        auto i=bot.step.begin();
                                                        while(i!=bot.step.cend())
                                                        {
                                                            QDomElement step = i->second;
                                                            if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_shop)
                                                            {
                                                                if(!step.hasAttribute(MapVisualiserThread::text_shop))
                                                                    qDebug() << (QStringLiteral("Has not attribute \"shop\": for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                {
                                                                    uint32_t shop=step.attribute(MapVisualiserThread::text_shop).toUInt(&ok);
                                                                    if(!ok)
                                                                        qDebug() << (QStringLiteral("shop is not a number: for bot id: %1 (%2)")
                                                                            .arg(botId).arg(botFile));
                                                                    else
                                                                        parsedMap->logicalMap.shops[std::pair<uint8_t,uint8_t>(x,y)].push_back(shop);

                                                                }
                                                            }
                                                            else if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_learn)
                                                            {
                                                                if(parsedMap->logicalMap.learn.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.learn.cend())
                                                                    qDebug() << (QStringLiteral("learn point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.learn.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_heal)
                                                            {
                                                                if(parsedMap->logicalMap.heal.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.heal.cend())
                                                                    qDebug() << (QStringLiteral("heal point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.heal.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_market)
                                                            {
                                                                if(parsedMap->logicalMap.market.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.market.cend())
                                                                    qDebug() << (QStringLiteral("market point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.market.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_zonecapture)
                                                            {
                                                                if(!step.hasAttribute(MapVisualiserThread::text_zone))
                                                                    qDebug() << (QStringLiteral("zonecapture point have not the zone attribute: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else if(parsedMap->logicalMap.zonecapture.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.zonecapture.cend())
                                                                    qDebug() << (QStringLiteral("zonecapture point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.zonecapture[std::pair<uint8_t,uint8_t>(x,y)]=step.attribute(MapVisualiserThread::text_zone).toStdString();
                                                            }
                                                            else if(step.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_fight)
                                                            {
                                                                if(parsedMap->logicalMap.botsFight.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.botsFight.cend())
                                                                    qDebug() << (QStringLiteral("botsFight point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                {
                                                                    const uint32_t &fightid=step.attribute(MapVisualiserThread::text_fightid).toUInt(&ok);
                                                                    if(ok)
                                                                        if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightid)!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                                                                            parsedMap->logicalMap.botsFight[std::pair<uint8_t,uint8_t>(x,y)].push_back(fightid);
                                                                }
                                                            }
                                                            ++i;
                                                        }
                                                    }
                                                    else
                                                        qDebug() << (
                                                                    QStringLiteral("No botId %1 into %2: properties.tagName(): %3, file: %4 (at line: %5)")
                                                                    .arg(botId)
                                                                    .arg(botFile)
                                                                    .arg(bot.tagName())
                                                                    .arg(QString::fromStdString(parsedMap->logicalMap.map_file))
                                                                    .arg(bot.lineNumber())
                                                                    );
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("No file %1: properties.tagName(): %2, name: %3 (at line: %4)").arg(botFile).arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Is not a number: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                    }
                                }
                                properties = properties.nextSiblingElement(MapVisualiserThread::text_properties);
                            }
                        }
                    }
                    bot = bot.nextSiblingElement(MapVisualiserThread::text_object);
                }
            }
        }
        child = child.nextSiblingElement(MapVisualiserThread::text_objectgroup);
    }
    return true;
}

bool MapVisualiserThread::loadOtherMapMetaData(MapVisualiserThread::Map_full *parsedMap)
{
    QDomDocument domDocument;
    //open and quick check the file
    QString fileName=QString::fromStdString(parsedMap->logicalMap.map_file);
    fileName.replace(MapVisualiserThread::text_dottmx,MapVisualiserThread::text_dotxml);
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(fileName.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(fileName.toStdString());
    else
    {
        QFile mapFile(fileName);
        if(!mapFile.exists())
            return false;
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapFile.fileName()+QStringLiteral(": ")+mapFile.errorString();
            return false;
        }
        const QByteArray &xmlContent=mapFile.readAll();
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
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[fileName.toStdString()]=domDocument;
    }
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=MapVisualiserThread::text_map)
    {
        qDebug() << QStringLiteral("\"map\" root balise not found for the xml file");
        return false;
    }
    if(root.hasAttribute(MapVisualiserThread::text_type))
        parsedMap->visualType=root.attribute(MapVisualiserThread::text_type);
    if(root.hasAttribute(MapVisualiserThread::text_zone))
        parsedMap->zone=root.attribute(MapVisualiserThread::text_zone);
    //load the name
    {
        bool name_found=false;
        QDomElement name = root.firstChildElement(MapVisualiserThread::text_name);
        if(!language.isEmpty() && language!=MapVisualiserThread::text_en)
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(name.hasAttribute(MapVisualiserThread::text_lang) && name.attribute(MapVisualiserThread::text_lang)==language)
                    {
                        parsedMap->name=name.text();
                        name_found=true;
                        break;
                    }
                }
                name = name.nextSiblingElement(MapVisualiserThread::text_name);
            }
        if(!name_found)
        {
            name = root.firstChildElement(MapVisualiserThread::text_name);
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute(MapVisualiserThread::text_lang) || name.attribute(MapVisualiserThread::text_lang)==MapVisualiserThread::text_en)
                    {
                        parsedMap->name=name.text();
                        name_found=true;
                        break;
                    }
                }
                name = name.nextSiblingElement(MapVisualiserThread::text_name);
            }
        }
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
    const QByteArray &xmlContent=mapFile.readAll();
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
    const QDomElement &root = domDocument.documentElement();
    if(root.tagName()!=MapVisualiserThread::text_bots)
    {
        qDebug() << QStringLiteral("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement(MapVisualiserThread::text_bot);
    while(!child.isNull())
    {
        if(!child.hasAttribute(MapVisualiserThread::text_id))
            qDebug() << (QStringLiteral("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            qDebug() << (QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            const uint32_t &botId=child.attribute(MapVisualiserThread::text_id).toUInt(&ok);
            if(ok)
            {
                if(botFiles.value(fileName).contains(botId))
                    qDebug() << (QStringLiteral("bot already found with this id: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
                else
                {
                    QDomElement step = child.firstChildElement(MapVisualiserThread::text_step);
                    while(!step.isNull())
                    {
                        if(!step.hasAttribute(MapVisualiserThread::text_id))
                            qDebug() << (QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                        else if(!step.hasAttribute(MapVisualiserThread::text_type))
                            qDebug() << (QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                        else if(!step.isElement())
                            qDebug() << (QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
                        else
                        {
                            uint8_t stepId=step.attribute(MapVisualiserThread::text_id).toUShort(&ok);
                            if(ok)
                                botFiles[fileName][botId].step[stepId]=step;
                        }
                        step = step.nextSiblingElement(MapVisualiserThread::text_step);
                    }
                    //load the name
                    {
                        bool name_found=false;
                        QDomElement name = child.firstChildElement(MapVisualiserThread::text_name);
                        if(!language.isEmpty() && language!=MapVisualiserThread::text_en)
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute(MapVisualiserThread::text_lang) && name.attribute(MapVisualiserThread::text_lang)==language)
                                    {
                                        botFiles[fileName][botId].name=name.text().toStdString();
                                        name_found=true;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement(MapVisualiserThread::text_name);
                            }
                        if(!name_found)
                        {
                            name = child.firstChildElement(MapVisualiserThread::text_name);
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute(MapVisualiserThread::text_lang) || name.attribute(MapVisualiserThread::text_lang)==MapVisualiserThread::text_en)
                                    {
                                        botFiles[fileName][botId].name=name.text().toStdString();
                                        name_found=true;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement(MapVisualiserThread::text_name);
                            }
                        }
                    }
                    if(botFiles[fileName][botId].step.find(1)==botFiles[fileName][botId].step.cend())
                        botFiles[fileName].remove(botId);
                }
            }
            else
                qDebug() << (QStringLiteral("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement(MapVisualiserThread::text_bot);
    }
}

void MapVisualiserThread::resetAll()
{
    botFiles.clear();
}
