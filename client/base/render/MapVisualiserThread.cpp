#include "MapVisualiserThread.h"
#include "MapItem.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/tinyXML2/tinyxml2.h"
#include "../../tiled/tiled_mapobject.h"
#include <QFileInfo>
#include <QRegularExpression>
#include "../ClientVariable.h"
#include "../interface/DatapackClientLoader.h"
#include "../LanguagesSelect.h"
#include "../FacilityLibClient.h"
#include <QDebug>
#include <QTime>

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

void MapVisualiserThread::loadOtherMapAsync(const std::string &fileName)
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

std::string MapVisualiserThread::error()
{
    return mLastError;
}

//open the file, and load it into the variables
MapVisualiserThread::Map_full *MapVisualiserThread::loadOtherMap(const std::string &resolvedFileName)
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
    tempMapObject->logicalMap.width                                 = static_cast<uint8_t>(map_loader.map_to_send.width);
    tempMapObject->logicalMap.height                                = static_cast<uint8_t>(map_loader.map_to_send.height);
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
                if(DatapackClientLoader::datapackLoader.itemOnMap.value(resolvedFileName).contains(std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)))
                    newItem.indexOfItemOnMap=DatapackClientLoader::datapackLoader.itemOnMap.value(resolvedFileName)
                            .value(std::pair<uint8_t,uint8_t>(item.point.x,item.point.y));
                else
                    qDebug() << QStringLiteral("Map itemOnMap %1,%2 not found").arg(item.point.x).arg(item.point.y);
            }
            else
                qDebug() << QStringLiteral("Map itemOnMap %1 not found into: %2").arg(resolvedFileName).arg(QStringList(DatapackClientLoader::datapackLoader.itemOnMap.keys()).join(";"));
            tempMapObject->logicalMap.itemsOnMap[std::pair<uint8_t,uint8_t>(item.point.x,item.point.y)]=newItem;
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
        unsigned int index=0;
        while(index<map_loader.map_to_send.teleport.size())
        {
            const CatchChallenger::Map_semi_teleport &teleport=map_loader.map_to_send.teleport.at(index);
            tempMapObject->logicalMap.teleport_semi << teleport;
            tempMapObject->logicalMap.teleport_semi[index].map                      = QFileInfo(QFileInfo(resolvedFileName).absolutePath()+"/"+QString::fromStdString(tempMapObject->logicalMap.teleport_semi.at(index).map)).absoluteFilePath().toStdString();
            const TiXmlElement * item=teleport.conditionUnparsed;
            std::string conditionText;
            {
                if(item!=NULL)
                {
                    bool text_found=false;
                    const TiXmlElement * blockedtext = item->FirstChildElement(std::string("blockedtext"));
                    if(!language.isEmpty() && language!=MapVisualiserThread::text_en)
                        while(blockedtext!=NULL)
                        {
                            if(blockedtext->Attribute(std::string("lang"))!=NULL && *blockedtext->Attribute(std::string("lang"))==language.toStdString())
                            {
                                conditionText=blockedtext->GetText();
                                text_found=true;
                                break;
                            }
                            blockedtext = blockedtext->NextSiblingElement(std::string("blockedtext"));
                        }
                    if(!text_found)
                    {
                        blockedtext = item->FirstChildElement(std::string("blockedtext"));
                        while(blockedtext!=NULL)
                        {
                            if(blockedtext->Attribute(std::string("lang"))==NULL || *blockedtext->Attribute(std::string("lang"))=="en")
                            {
                                conditionText=blockedtext->GetText();
                                break;
                            }
                            blockedtext = blockedtext->NextSiblingElement(std::string("blockedtext"));
                        }
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
                            const std::string &animation=tile->property(MapVisualiserThread::text_animation);
                            if(!animation.isEmpty())
                            {
                                const QStringList &animationList=animation.split(MapVisualiserThread::text_dotcomma);
                                if(animationList.size()>=2)
                                {
                                    if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                    {
                                        std::string msString=animationList.at(0);
                                        std::string framesString=animationList.at(1);
                                        msString.remove(MapVisualiserThread::text_ms);
                                        framesString.remove(MapVisualiserThread::text_frames);
                                        const unsigned int temp_ms=msString.toUInt();
                                        const unsigned int temp_frames=framesString.toUInt();
                                        if(temp_ms>=10 && temp_ms<=65535)
                                        {
                                            if(temp_frames>1 && temp_frames<=255)
                                            {
                                                const uint16_t ms=static_cast<uint16_t>(temp_ms);
                                                const uint8_t frames=static_cast<uint8_t>(temp_frames);
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
                                                qDebug() << "frames is not in good range";
                                        }
                                        else
                                            qDebug() << "ms is not in good range";
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
                                    tempMapObject->triggerAnimations[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=new TriggerAnimation(
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
                                    const std::string &trigger=tile->property(MapVisualiserThread::text_trigger);
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
                                            std::string tempString=trigger;
                                            tempString.replace(regexTrigger,"\\1");
                                            content.msEnter=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\2");
                                            content.framesCountEnter=static_cast<uint8_t>(tempString.toUShort());
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\3");
                                            content.msLeave=tempString.toUShort();
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\4");
                                            content.framesCountLeave=static_cast<uint8_t>(tempString.toUShort());
                                            tempString=trigger;
                                            tempString.replace(regexTrigger,"\\5");
                                            //again here
                                            if(tempString.contains(regexTriggerAgain))
                                            {
                                                std::string againString=tempString;
                                                againString.replace(regexTriggerAgain,"\\1");
                                                content.msAgain=againString.toUShort();
                                                againString=tempString;
                                                againString.replace(regexTriggerAgain,"\\2");
                                                content.framesCountAgain=static_cast<uint8_t>(againString.toUShort());
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
                                                tempMapObject->triggerAnimations[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=new TriggerAnimation(
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
    const std::string &fileName=QString::fromStdString(parsedMap->logicalMap.map_file);
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(fileName.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(fileName.toStdString());
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
        std::string errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[fileName.toStdString()]=domDocument;
    }
    const tinyxml2::XMLElement &root = domDocument.RootElement();
    if(root.tagName()!=MapVisualiserThread::text_map)
    {
        qDebug() << fileName << QStringLiteral(", MapVisualiserThread::loadOtherMapClientPart(): \"map\" root balise not found for the xml file: ") << root.tagName();
        return false;
    }
    bool ok,ok2;
    //load the bots (map->bots)
    tinyxml2::XMLElement child = root.FirstChildElement(MapVisualiserThread::text_objectgroup);
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
                tinyxml2::XMLElement bot = child.FirstChildElement(MapVisualiserThread::text_object);
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
                            tinyxml2::XMLElement properties = bot.FirstChildElement(MapVisualiserThread::text_properties);
                            while(!properties.isNull())
                            {
                                if(!properties.isElement())
                                    qDebug() << (QStringLiteral("Is not an element: properties.tagName(): %1, (at line: %2)").arg(properties.tagName().arg(properties.lineNumber())));
                                else
                                {
                                    std::unordered_map<std::string,std::string> property_parsed;
                                    tinyxml2::XMLElement property = properties.FirstChildElement(MapVisualiserThread::text_property);
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
                                        property = property.NextSiblingElement(MapVisualiserThread::text_property);
                                    }
                                    if(property_parsed.find("file")!=property_parsed.cend() && property_parsed.find("id")!=property_parsed.cend())
                                    {
                                        const uint16_t &botId=stringtouint16(property_parsed.at("id"),&ok);
                                        if(ok)
                                        {
                                            std::string botFile(QFileInfo(QFileInfo(fileName).absolutePath()+MapVisualiserThread::text_slash+QString::fromStdString(property_parsed.at("file"))).absoluteFilePath());
                                            if(!botFile.endsWith(MapVisualiserThread::text_dotxml))
                                                botFile+=MapVisualiserThread::text_dotxml;
                                            if(bot.attribute(MapVisualiserThread::text_type)==MapVisualiserThread::text_bot)
                                            {
                                                if(stopIt)
                                                        return false;
                                                loadBotFile(botFile.toStdString());
                                                if(stopIt)
                                                        return false;
                                                if(botFiles.contains(botFile))
                                                {
                                                    if(botFiles.value(botFile).contains(botId))
                                                    {
                                                        #ifdef DEBUG_CLIENT_BOT
                                                        qDebug() << (QStringLiteral("Put bot %1 (%2) at %3 (%4,%5)").arg(botFile).arg(botId).arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
                                                        #endif
                                                        CatchChallenger::Bot &bot=parsedMap->logicalMap.bots[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                                                        bot=botFiles.value(botFile).value(botId);
                                                        property_parsed.erase("file");
                                                        property_parsed.erase("id");
                                                        bot.properties=property_parsed;
                                                        bot.botId=botId;

                                                        auto i=bot.step.begin();
                                                        while(i!=bot.step.cend())
                                                        {
                                                            auto step = i->second;
                                                            if(*step->Attribute(std::string("type"))=="shop")
                                                            {
                                                                if(step->Attribute(std::string("shop"))==NULL)
                                                                    qDebug() << (QStringLiteral("Has not attribute \"shop\": for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                {
                                                                    const uint16_t shop=stringtouint16(*step->Attribute(std::string("shop")),&ok);
                                                                    if(!ok)
                                                                        qDebug() << (QStringLiteral("shop is not a number: for bot id: %1 (%2)")
                                                                            .arg(botId).arg(botFile));
                                                                    else
                                                                        parsedMap->logicalMap.shops[std::pair<uint8_t,uint8_t>(x,y)].push_back(shop);

                                                                }
                                                            }
                                                            else if(*step->Attribute(std::string("type"))=="learn")
                                                            {
                                                                if(parsedMap->logicalMap.learn.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.learn.cend())
                                                                    qDebug() << (QStringLiteral("learn point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.learn.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(*step->Attribute(std::string("type"))=="heal")
                                                            {
                                                                if(parsedMap->logicalMap.heal.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.heal.cend())
                                                                    qDebug() << (QStringLiteral("heal point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.heal.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(*step->Attribute(std::string("type"))=="market")
                                                            {
                                                                if(parsedMap->logicalMap.market.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.market.cend())
                                                                    qDebug() << (QStringLiteral("market point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.market.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                            }
                                                            else if(*step->Attribute(std::string("type"))=="zonecapture")
                                                            {
                                                                if(step->Attribute(std::string("zone"))==NULL)
                                                                    qDebug() << (QStringLiteral("zonecapture point have not the zone attribute: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else if(parsedMap->logicalMap.zonecapture.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.zonecapture.cend())
                                                                    qDebug() << (QStringLiteral("zonecapture point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                    parsedMap->logicalMap.zonecapture[std::pair<uint8_t,uint8_t>(x,y)]=*step->Attribute(std::string("zone"));
                                                            }
                                                            else if(*step->Attribute(std::string("type"))=="fight")
                                                            {
                                                                if(parsedMap->logicalMap.botsFight.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->logicalMap.botsFight.cend())
                                                                    qDebug() << (QStringLiteral("botsFight point already on the map: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(botFile));
                                                                else
                                                                {
                                                                    const uint16_t &fightid=stringtouint16(*step->Attribute(std::string("fightid")),&ok);
                                                                    if(ok)
                                                                        if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightid)!=
                                                                                CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                                                                            parsedMap->logicalMap.botsFight[std::pair<uint8_t,uint8_t>(x,y)].push_back(fightid);
                                                                }
                                                            }
                                                            ++i;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        qDebug() << (
                                                                    QStringLiteral("No botId %1 into %2: properties.tagName(): %3, file: %4 (at line: %5)")
                                                                    .arg(botId)
                                                                    .arg(botFile)
                                                                    .arg(bot.tagName())
                                                                    .arg(QString::fromStdString(parsedMap->logicalMap.map_file))
                                                                    .arg(bot.lineNumber())
                                                                    );
                                                        /// \warn show something to continue to block the path
                                                        CatchChallenger::Bot &bot=parsedMap->logicalMap.bots[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                                                        bot=botFiles.value(botFile).value(botId);
                                                        property_parsed.erase("file");
                                                        property_parsed.erase("id");
                                                        bot.properties=property_parsed;
                                                        bot.botId=botId;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("No file %1: properties.tagName(): %2, name: %3 (at line: %4)").arg(botFile).arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Is not a number: properties.tagName(): %1, name: %2 (at line: %3)").arg(property.tagName().arg(property.attribute("name")).arg(property.lineNumber())));
                                    }
                                }
                                properties = properties.NextSiblingElement(MapVisualiserThread::text_properties);
                            }
                        }
                    }
                    bot = bot.NextSiblingElement(MapVisualiserThread::text_object);
                }
            }
        }
        child = child.NextSiblingElement(MapVisualiserThread::text_objectgroup);
    }
    return true;
}

bool MapVisualiserThread::loadOtherMapMetaData(MapVisualiserThread::Map_full *parsedMap)
{
    QDomDocument domDocument;
    //open and quick check the file
    std::string fileName=QString::fromStdString(parsedMap->logicalMap.map_file);
    fileName.replace(MapVisualiserThread::text_dottmx,MapVisualiserThread::text_dotxml);
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(fileName.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(fileName.toStdString());
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
        std::string errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return false;
        }
        CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt[fileName.toStdString()]=domDocument;
    }
    const tinyxml2::XMLElement &root = domDocument.RootElement();
    if(root.tagName()!=MapVisualiserThread::text_map)
    {
        qDebug() << fileName << QStringLiteral(", MapVisualiserThread::loadOtherMapMetaData(): \"map\" root balise not found for the xml file");
        return false;
    }
    if(root.hasAttribute(MapVisualiserThread::text_type))
        parsedMap->visualType=root.attribute(MapVisualiserThread::text_type);
    if(root.hasAttribute(MapVisualiserThread::text_zone))
        parsedMap->zone=root.attribute(MapVisualiserThread::text_zone);
    //load the name
    {
        bool name_found=false;
        tinyxml2::XMLElement name = root.FirstChildElement(MapVisualiserThread::text_name);
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
                name = name.NextSiblingElement(MapVisualiserThread::text_name);
            }
        if(!name_found)
        {
            name = root.FirstChildElement(MapVisualiserThread::text_name);
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
                name = name.NextSiblingElement(MapVisualiserThread::text_name);
            }
        }
    }
    return true;
}

void MapVisualiserThread::loadBotFile(const std::string &file)
{
    if(botFiles.contains(QString::fromStdString(file)))
        return;
    botFiles[QString::fromStdString(file)];//create the entry

    TiXmlDocument *domDocument;
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
        const bool loadOkay=domDocument->LoadFile(file);
        if(!loadOkay)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << domDocument->ErrorRow() << ", column " << domDocument->ErrorCol() << ": " << domDocument->ErrorDesc() << std::endl;
            return;
        }
    }

    const TiXmlElement * root = domDocument->RootElement();
    if(root==NULL)
        return;
    if(root->ValueStr()!="bots")
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found for reputation of the xml file" << std::endl;
        return;
    }
    bool ok;
    //load the bots
    const TiXmlElement * child = root->FirstChildElement("bot");
    while(child!=NULL)
    {
        if(child->Attribute("id")==NULL)
            qDebug() << (QStringLiteral("Has not attribute \"id\": child->Value(): %1 (at line: %2)").arg(child->Value()).arg("?"));
        else
        {
            const uint32_t &botId=stringtouint32(*child->Attribute(std::string("id")),&ok);
            if(ok)
            {
                if(botFiles.value(QString::fromStdString(file)).contains(botId))
                    qDebug() << (QStringLiteral("bot already found with this id: bot->Value(): %1 (at line: %2)").arg(child->Value()).arg("?"));
                else
                {
                    const TiXmlElement * step = child->FirstChildElement("step");
                    while(step!=NULL)
                    {
                        if(step->Attribute(std::string("id"))==NULL)
                            qDebug() << (QStringLiteral("Has not attribute \"type\": bot->Value(): %1 (at line: %2)").arg(step->Value()).arg("?"));
                        else if(step->Attribute(std::string("type"))==NULL)
                            qDebug() << (QStringLiteral("Has not attribute \"type\": bot->Value(): %1 (at line: %2)").arg(step->Value()).arg("?"));
                        else
                        {
                            uint8_t stepId=stringtouint8(*step->Attribute(std::string("id")),&ok);
                            if(ok)
                                botFiles[QString::fromStdString(file)][botId].step[stepId]=step;
                        }
                        step = step->NextSiblingElement("step");
                    }
                    //load the name
                    {
                        bool name_found=false;
                        const TiXmlElement * name = child->FirstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                            while(name!=NULL)
                            {
                                if(name->Attribute(std::string("lang"))!=NULL && *name->Attribute(std::string("lang"))==language.toStdString())
                                {
                                    botFiles[QString::fromStdString(file)][botId].name=name->GetText();
                                    name_found=true;
                                    break;
                                }
                                name = name->NextSiblingElement("name");
                            }
                        if(!name_found)
                        {
                            name = child->FirstChildElement("name");
                            while(name!=NULL)
                            {
                                if(name->Attribute(std::string("lang"))==NULL || *name->Attribute(std::string("lang"))=="en")
                                {
                                    botFiles[QString::fromStdString(file)][botId].name=name->GetText();
                                    name_found=true;
                                    break;
                                }
                                name = name->NextSiblingElement("name");
                            }
                        }
                    }
                    const CatchChallenger::Bot &bot=botFiles[QString::fromStdString(file)][botId];
                    if(bot.step.find(1)==bot.step.cend())
                        botFiles[QString::fromStdString(file)].remove(botId);
                }
            }
            else
                qDebug() << (QStringLiteral("Attribute \"id\" is not a number: bot->Value(): %1 (at line: %2)").arg(child->Value()).arg("?"));
        }
        child = child->NextSiblingElement("bot");
    }
}

void MapVisualiserThread::resetAll()
{
    CatchChallenger::Map_loader::teleportConditionsUnparsed.clear();

    botFiles.clear();
}
