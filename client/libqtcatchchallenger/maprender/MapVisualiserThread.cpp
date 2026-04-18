#include "MapVisualiserThread.hpp"
#include "QMap_client.hpp"
#include "MapItem.hpp"
#include <QElapsedTimer>
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"
#include "../../general/tinyXML2/customtinyxml2.hpp"
#include <tiled/mapobject.h>
#include <tiled/isometricrenderer.h>
#include <tiled/orthogonalrenderer.h>
#include <QFileInfo>
#include <QRegularExpression>
#include "../libcatchchallenger/ClientVariable.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#if ! defined (CATCHCHALLENGER_BOT) && ! defined (ONLYMAPRENDER)
#include "../libqtcatchchallenger/Language.hpp"
#endif
#include <QDebug>
#include <QTime>
#include <iostream>

MapVisualiserThread::MapVisualiserThread()
{
    #ifndef NOTHREADS
    moveToThread(this);
    start(QThread::IdlePriority);
    #endif
    hideTheDoors=false;
    #if ! defined (CATCHCHALLENGER_BOT) && ! defined (ONLYMAPRENDER)
    language=Language::language.getLanguage().toStdString();
    #else
    language="en";
    #endif

    debugTags=false;
    tagTileset=nullptr;
    tagTilesetIndex=0;
    stopIt=false;
    hideTheDoors=false;
}

MapVisualiserThread::~MapVisualiserThread()
{
    #ifndef NOTHREADS
    quit();
    wait();
    #endif
}

void MapVisualiserThread::loadOtherMapAsync(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    QMap_client *tempMapObject=loadOtherMap(mapIndex);
    emit asyncMapLoaded(mapIndex,tempMapObject);
}

std::string MapVisualiserThread::error()
{
    return mLastError;
}

//open the file, and load it into the variables
QMap_client *MapVisualiserThread::loadOtherMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    if(stopIt)
        return NULL;
    if(mapIndex>=(CATCHCHALLENGER_TYPE_MAPID)QtDatapackClientLoader::datapackLoader->get_maps().size())
        return NULL;
    const std::string resolvedFileName=QFileInfo(QString::fromStdString(
        QtDatapackClientLoader::datapackLoader->getDatapackPath()+
        QtDatapackClientLoader::datapackLoader->getMainDatapackPath()+
        QtDatapackClientLoader::datapackLoader->get_maps().at(mapIndex)+".tmx")).absoluteFilePath().toStdString();
    std::cerr << "MapVisualiserThread::loadOtherMap() mapIndex=" << mapIndex
              << " resolvedFileName=" << resolvedFileName << std::endl;
    QMap_client *tempMapObject=new QMap_client();

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
        QElapsedTimer time;
        time.restart();
        tempMapObject->tiledMap = reader.readMap(QString::fromStdString(resolvedFileName));
        qDebug() << QStringLiteral("%1 loaded into %2ms").arg(QString::fromStdString(resolvedFileName)).arg(time.elapsed());
    }
    if (!tempMapObject->tiledMap)
    {
        mLastError=reader.errorString().toStdString();
        qDebug() << QStringLiteral("(1) Unable to load the map: %1, error: %2").arg(QString::fromStdString(resolvedFileName)).arg(reader.errorString());
        delete tempMapObject;
        return NULL;
    }
    if(stopIt)
    {
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }
    CatchChallenger::Map_loader map_loader;
    CatchChallenger::CommonMap tempLogicalMap;
    if(!map_loader.tryLoadMap(resolvedFileName,tempLogicalMap,true))
    {
        mLastError=map_loader.errorString();
        qDebug() << QStringLiteral("(2) Unable to load the map: %1, error: %2")
                    .arg(QString::fromStdString(resolvedFileName))
                    .arg(QString::fromStdString(map_loader.errorString()));
        /*int index=0;
        const int &listSize=tempMapObject->tiledMap->tilesets().size();
        while(index<listSize)
        {
            delete tempMapObject->tiledMap->tilesets().at(index);
            index++;
        }*/
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }
    if(stopIt)
    {
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    //logical map data now comes from DatapackClientLoader::getMap(mapIndex)
    //items now loaded via DatapackClientLoader
    #if defined(CATCHCHALLENGER_EXTRA_CHECK) && ! defined(MAPVISUALISER)
    if(mapIndex>=QtDatapackClientLoader::datapackLoader->get_maps().size())
    {
        mLastError="Map id out of range: "+std::to_string(mapIndex);
        qDebug() << QString::fromStdString(mLastError);
        delete tempMapObject;
        return NULL;
    }
    #endif
    //map id now comes from DatapackClientLoader::getMap(mapIndex)

    if(tempMapObject->tiledMap->tileHeight()!=CLIENT_BASE_TILE_SIZE || tempMapObject->tiledMap->tileWidth()!=CLIENT_BASE_TILE_SIZE)
    {
        mLastError="Map tile size not multiple of "+std::to_string(CLIENT_BASE_TILE_SIZE);
        qDebug() << QStringLiteral("(3) Unable to load the map: %1, error: %2")
                    .arg(QString::fromStdString(resolvedFileName))
                    .arg(QString::fromStdString(mLastError));
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    //borders now resolved via DatapackClientLoader::getMap(mapIndex)

    //teleporters and rescue points now in DatapackClientLoader::getMap(mapIndex)

    //load the render
    switch (tempMapObject->tiledMap->orientation()) {
    case Tiled::Map::Orthogonal:
        tempMapObject->tiledRender = new Tiled::OrthogonalRenderer(tempMapObject->tiledMap.get());
        break;
    default:
        return nullptr;
        break;
    }
    //tempMapObject->tiledRender->setObjectBorder(false);

    //do the object group to move the player on it
    tempMapObject->objectGroup = new Tiled::ObjectGroup("Dyna management",0,0);//tempMapObject->tiledMap->width(),tempMapObject->tiledMap->height()
    tempMapObject->objectGroup->setName("objectGroup for player layer");

    //add a tags
    if(debugTags && tagTileset)
    {
        Tiled::MapObject * tagMapObject = new Tiled::MapObject();
        Tiled::Cell cell=tagMapObject->cell();
        cell.setTile(tagTileset->tileAt(tagTilesetIndex));
        tagMapObject->setCell(cell);
        tagMapObject->setPosition(QPoint(QtDatapackClientLoader::datapackLoader->getMap(mapIndex).width/2,QtDatapackClientLoader::datapackLoader->getMap(mapIndex).height/2+1));
        tempMapObject->objectGroup->addObject(tagMapObject);
        tagTilesetIndex++;
        if(tagTilesetIndex>=tagTileset->tileCount())
            tagTilesetIndex=0;
    }
    else
        MapVisualiserOrder::layerChangeLevelAndTagsChange(tempMapObject,hideTheDoors);

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
                        Tiled::Tile *tile=cell.tile();
                        if(tile!=NULL)
                        {
                            const std::string &random=tile->property("random").toString().toStdString();
                            const std::string &animation=tile->property("animation").toString().toStdString();
                            if(!random.empty())
                            {
                                bool ok=false;
                                const uint8_t random_int=stringtouint8(random,&ok);
                                if(ok)
                                {
                                    cell.setTile(cell.tile()->tileset()->tileAt(cell.tile()->id()+rand()%random_int));
                                    tileLayer->setCell(x,y,cell);
                                }
                                else
                                    qDebug() << "random property not uint8" << tile->property("random");
                            }
                            else if(!animation.empty())
                            {
                                const std::vector<std::string> &animationList=stringsplit(animation,';');
                                if(animationList.size()>=2)
                                {
                                    if(QString::fromStdString(animationList.at(0)).contains(regexMs) &&
                                            QString::fromStdString(animationList.at(1)).contains(regexFrames))
                                    {
                                        std::string msString=animationList.at(0);
                                        std::string framesString=animationList.at(1);
                                        stringreplaceAll(msString,"ms","");
                                        stringreplaceAll(framesString,"frames","");
                                        const unsigned int temp_ms=stringtouint16(msString);
                                        if(temp_ms>=10 && temp_ms<=65535)
                                        {
                                            const unsigned int temp_frames=stringtouint8(framesString);
                                            if(temp_frames>1 && temp_frames<=255)
                                            {
                                                const uint16_t ms=static_cast<uint16_t>(temp_ms);
                                                const uint8_t frames=static_cast<uint8_t>(temp_frames);
                                                {
                                                    Tiled::Cell cell;
                                                    cell.setTile(NULL);
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
                                                const int tileId=tile->id();//to have const, ignore random
                                                if(tempMapObject->animatedObject.find(ms)==tempMapObject->animatedObject.cend() ||
                                                        tempMapObject->animatedObject.at(ms).find(tileId)==tempMapObject->animatedObject.at(ms).cend())
                                                {
                                                    Map_animation tempAnimationDescriptor;
                                                    tempAnimationDescriptor.minId=tileId;
                                                    tempAnimationDescriptor.maxId=tileId+frames;
                                                    tempMapObject->animatedObject[ms][tileId]=tempAnimationDescriptor;
                                                }
                                                Map_animation_object map_animation_object;
                                                if(animationList.size()>=3 && animationList.at(2)=="random-offset")
                                                    cell.setTile(tile->tileset()->tileAt(tileId+rand()%frames));
                                                else
                                                    cell.setTile(tile);
                                                object->setCell(cell);
                                                map_animation_object.animatedObject=object;
                                                /// \todo control the animation is not out of rame
                                                tempMapObject->animatedObject[ms][tileId].animatedObjectList.push_back(map_animation_object);
                                            }
                                            else
                                                qDebug() << "frames is not in good range" << tile->property("animation");
                                        }
                                        else
                                            qDebug() << "ms is not in good range" << tile->property("animation");
                                    }
                                    else
                                        qDebug() << "Wrong animation tile args regex match" << tile->property("animation");
                                }
                                else
                                    qDebug() << "Wrong animation tile args count (thread)" << tile->property("animation");
                            }
                            else
                            {
                                if(tileToTriggerAnimationContent.find(tile)!=tileToTriggerAnimationContent.cend())
                                {
                                    const TriggerAnimationContent &content=tileToTriggerAnimationContent.at(tile);
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
                                    cell.setTile(content.objectTile);
                                    object->setCell(cell);
                                    objectGroup->addObject(object);
                                    object->setPosition(QPointF(x,y+1));
                                    //do the layer over
                                    if(content.over)
                                    {
                                        objectOver=new Tiled::MapObject();
                                        Tiled::Cell cell;
                                        cell.setTile(content.objectTileOver);
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
                                        cell.setTile(NULL);
                                        tileLayer->setCell(x,y,cell);
                                    }
                                }
                                else
                                {
                                    const std::string &trigger=tile->property("trigger").toString().toStdString();
                                    if(!trigger.empty())
                                    {
                                        if(QString::fromStdString(trigger).contains(regexTrigger))
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
                                            tempString=QString::fromStdString(tempString).replace(regexTrigger,"\\1").toStdString();
                                            content.msEnter=stringtouint16(tempString);
                                            tempString=trigger;
                                            tempString=QString::fromStdString(tempString).replace(regexTrigger,"\\2").toStdString();
                                            content.framesCountEnter=stringtouint8(tempString);
                                            tempString=trigger;
                                            tempString=QString::fromStdString(tempString).replace(regexTrigger,"\\3").toStdString();
                                            content.msLeave=stringtouint16(tempString);
                                            tempString=trigger;
                                            tempString=QString::fromStdString(tempString).replace(regexTrigger,"\\4").toStdString();
                                            content.framesCountLeave=stringtouint8(tempString);
                                            tempString=trigger;
                                            tempString=QString::fromStdString(tempString).replace(regexTrigger,"\\5").toStdString();
                                            //again here
                                            if(QString::fromStdString(tempString).contains(regexTriggerAgain))
                                            {
                                                std::string againString=tempString;
                                                tempString=QString::fromStdString(againString).replace(regexTriggerAgain,"\\1").toStdString();
                                                content.msAgain=stringtouint16(againString);
                                                againString=tempString;
                                                tempString=QString::fromStdString(againString).replace(regexTriggerAgain,"\\2").toStdString();
                                                content.framesCountAgain=stringtouint8(againString);
                                            }
                                            //over here
                                            if(tempString.find("over")!=std::string::npos)
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
                                                cell.setTile(content.objectTile);
                                                object->setCell(cell);
                                                objectGroup->addObject(object);
                                                object->setPosition(QPointF(x,y+1));
                                                //do the layer over
                                                if(content.over)
                                                {
                                                    objectOver=new Tiled::MapObject();
                                                    Tiled::Cell cell;
                                                    cell.setTile(content.objectTileOver);
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
                                            cell.setTile(NULL);
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
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }

    if(!loadOtherMapClientPart(tempMapObject))
    {
        //delete tempMapObject->tiledMap;
        delete tempMapObject;
        return NULL;
    }
    loadOtherMapMetaData(tempMapObject);

    return tempMapObject;
}

//drop and remplace by Map_loader info
bool MapVisualiserThread::loadOtherMapClientPart(QMap_client *parsedMap)
{
    (void)parsedMap;
    //client part data (bots, shops, etc.) now loaded via DatapackClientLoader
    return true;
}
#if 0 //disabled: logicalMap no longer in QMap_client
    tinyxml2::XMLDocument *domDocument;
    //open and quick check the file
    const std::string &fileName=std::string();
    if(CatchChallenger::CommonDatapack::commonDatapack.has_xmlLoadedFile(fileName))
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(fileName);
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(fileName);
        const tinyxml2::XMLError loadOkay = domDocument->LoadFile(fileName.c_str());
        if(loadOkay!=0)
        {
            std::cerr << fileName+", "+tinyxml2errordoc(domDocument) << std::endl;
            return false;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(root->Name()==NULL)
    {
        qDebug() << QString::fromStdString(fileName)
                 << QStringLiteral(", MapVisualiserThread::loadOtherMapClientPart(): \"map\" root balise not found 2 for the xml file: ")
                 << root->Name();
        return false;
    }
    if(strcmp(root->Name(),"map")!=0)
    {
        qDebug() << QString::fromStdString(fileName)
                 << QStringLiteral(", MapVisualiserThread::loadOtherMapClientPart(): \"map\" root balise not found for the xml file: ")
                 << root->Name();
        return false;
    }
    bool ok,ok2;
    //load the bots (map->bots)
    const tinyxml2::XMLElement *child = root->FirstChildElement("objectgroup");
    while(child!=NULL)
    {
        if(child->Attribute("name")==NULL)
            qDebug() << (QStringLiteral("Has not attribute \"name\": child->Name(): %1").arg(child->Name()));
        else
        {
            if(strcmp(child->Attribute("name"),"Object")==0)
            {
                const tinyxml2::XMLElement *bot = child->FirstChildElement("object");
                while(bot!=NULL)
                {
                    if(bot->Attribute("type")==NULL)
                        qDebug() << (QStringLiteral("Has not attribute \"type\": bot->Name(): %1").arg(bot->Name()));
                    else if(bot->Attribute("x")==NULL)
                        qDebug() << (QStringLiteral("Has not attribute \"x\": bot->Name(): %1").arg(bot->Name()));
                    else if(bot->Attribute("y")==NULL)
                        qDebug() << (QStringLiteral("Has not attribute \"y\": bot->Name(): %1").arg(bot->Name()));
                    else
                    {
                        const uint32_t &x=stringtouint32(bot->Attribute("x"),&ok)/CLIENT_BASE_TILE_SIZE;
                        const uint32_t &y=stringtouint32(bot->Attribute("y"),&ok2)/CLIENT_BASE_TILE_SIZE-1;
                        if(ok && ok2 &&
                                (strcmp(bot->Attribute("type"),"bot")==0 ||
                                 strcmp(bot->Attribute("type"),"botfight")==0))
                        {
                            const tinyxml2::XMLElement *properties = bot->FirstChildElement("properties");
                            while(properties!=NULL)
                            {
                                std::unordered_map<std::string,std::string> property_parsed;
                                const tinyxml2::XMLElement *property = properties->FirstChildElement("property");
                                while(property!=NULL)
                                {
                                    if(property->Attribute("name")==NULL)
                                        qDebug() << (QStringLiteral("Has not attribute \"name\": property->Name(): %1").arg(property->Name())
                                                     );
                                    else if(property->Attribute("value")==NULL)
                                        qDebug() << (QStringLiteral("Has not attribute \"value\": property->Name(): %1").arg(property->Name())
                                                     );
                                    else
                                        property_parsed[property->Attribute("name")]=property->Attribute("value");
                                    property = property->NextSiblingElement("property");
                                }
                                if(property_parsed.find("file")!=property_parsed.cend() && property_parsed.find("id")!=property_parsed.cend())
                                {
                                    const uint16_t &botId=stringtouint16(property_parsed.at("id"),&ok);
                                    if(ok)
                                    {
                                        std::string botFile=QFileInfo(QFileInfo(QString::fromStdString(fileName)).absolutePath()+"/"+
                                                                      QString::fromStdString(property_parsed.at("file")))
                                                .absoluteFilePath().toStdString();
                                        if(!stringEndsWith(botFile,".xml"))
                                            botFile+=".xml";
                                        if(strcmp(bot->Attribute("type"),"bot")==0)
                                        {
                                            if(stopIt)
                                                    return false;
                                            loadBotFile(botFile);
                                            if(stopIt)
                                                    return false;
                                            if(botFiles.find(botFile)!=botFiles.cend())
                                            {
                                                if(botFiles.at(botFile).find(botId)!=botFiles.at(botFile).cend())
                                                {
                                                    #ifdef DEBUG_CLIENT_BOT
                                                    qDebug() << (QStringLiteral("Put bot %1 (%2) at %3 (%4,%5)").arg(botFile).arg(botId)
                                                                 .arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
                                                    #endif
                                                    CatchChallenger::Bot &bot=parsedMap->logicalMap.bots[std::pair<uint8_t,uint8_t>(
                                                        static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                                                    bot=botFiles.at(botFile).at(botId);
                                                    property_parsed.erase("file");
                                                    property_parsed.erase("id");
                                                    bot.properties=property_parsed;
                                                    bot.botId=botId;

                                                    std::unordered_map<uint8_t,const tinyxml2::XMLElement *>::iterator i=bot.step.begin();
                                                    while(i!=bot.step.cend())
                                                    {
                                                        const tinyxml2::XMLElement *step = i->second;
                                                        if(step->Attribute("type")==NULL)
                                                        {}
                                                        else if(strcmp(step->Attribute("type"),"shop")==0)
                                                        {
                                                            if(step->Attribute("shop")==NULL)
                                                                qDebug() << (QStringLiteral("Has not attribute \"shop\": for bot id: %1 (%2)")
                                                                    .arg(botId).arg(QString::fromStdString(botFile)));
                                                            else
                                                            {
                                                                const uint16_t shop=stringtouint16(step->Attribute("shop"),&ok);
                                                                if(!ok)
                                                                    qDebug() << (QStringLiteral("shop is not a number: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(QString::fromStdString(botFile)));
                                                                else
                                                                    parsedMap->logicalMap.shops[std::pair<uint8_t,uint8_t>(x,y)].push_back(shop);

                                                            }
                                                        }
                                                        else if(strcmp(step->Attribute("type"),"heal")==0)
                                                        {
                                                            if(parsedMap->logicalMap.heal.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->
                                                                logicalMap.heal.cend())
                                                                qDebug() << (QStringLiteral("heal point already on the map: for bot id: %1 (%2)")
                                                                    .arg(botId).arg(QString::fromStdString(botFile)));
                                                            else
                                                                parsedMap->logicalMap.heal.insert(std::pair<uint8_t,uint8_t>(x,y));
                                                        }
                                                        else if(strcmp(step->Attribute("type"),"zonecapture")==0)
                                                        {
                                                            if(step->Attribute("zone")==NULL)
                                                                qDebug() << (QStringLiteral("zonecapture point have not the zone attribute: for bot id: %1 (%2)")
                                                                    .arg(botId).arg(QString::fromStdString(botFile)));
                                                            else if(parsedMap->logicalMap.zonecapture.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->
                                                                logicalMap.zonecapture.cend())
                                                                qDebug() << (QStringLiteral("zonecapture point already on the map: for bot id: %1 (%2)")
                                                                    .arg(botId).arg(QString::fromStdString(botFile)));
                                                            else
                                                            {
                                                                const char * const zoneString=step->Attribute("zone");
                                                                if(QtDatapackClientLoader::datapackLoader->get_zonesExtra().find(zoneString)!=QtDatapackClientLoader::datapackLoader->get_zonesExtra().cend())
                                                                    parsedMap->logicalMap.zonecapture[std::pair<uint8_t,uint8_t>(x,y)]=QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(zoneString).id;
                                                                else
                                                                    qDebug() << (QStringLiteral("zoneString not resolved: for bot id: %1 (%2)")
                                                                        .arg(botId).arg(QString::fromStdString(botFile)));
                                                            }
                                                        }
                                                        else if(strcmp(step->Attribute("type"),"fight")==0)
                                                        {
                                                            if(parsedMap->logicalMap.botsFight.find(std::pair<uint8_t,uint8_t>(x,y))!=parsedMap->
                                                                logicalMap.botsFight.cend())
                                                                qDebug() << (QStringLiteral("botsFight point already on the map: for bot id: %1 (%2)")
                                                                    .arg(botId).arg(QString::fromStdString(botFile)));
                                                            else
                                                            {
                                                                const uint16_t &fightid=stringtouint16(step->Attribute("fightid"),&ok);
                                                                if(ok)
                                                                    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(fightid)!=
                                                                            CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
                                                                        parsedMap->logicalMap.botsFight[std::pair<uint8_t,uint8_t>(x,y)].push_back(fightid);
                                                            }
                                                        }
                                                        ++i;
                                                    }
                                                }
                                                else
                                                {
                                                    /// \warn show something to continue to block the path
                                                    if(botFiles.find(botFile)!=botFiles.cend())
                                                    {
                                                        const std::unordered_map<CATCHCHALLENGER_TYPE_BOTID,CatchChallenger::Bot> &botList=botFiles.at(botFile);
                                                        if(botList.find(botId)!=botList.cend())
                                                        {
                                                            qDebug() << (
                                                                        QStringLiteral("botId %1 into %2: properties->Name(): %3, file: %4")
                                                                        .arg(botId)
                                                                        .arg(QString::fromStdString(botFile))
                                                                        .arg(bot->Name())
                                                                        .arg(QString::fromStdString(parsedMap->logicalMap.map_file))
                                                                        );
                                                            CatchChallenger::Bot &bot=parsedMap->logicalMap.bots[std::pair<uint8_t,uint8_t>(
                                                                static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
                                                            bot=botFiles.at(botFile).at(botId);
                                                            property_parsed.erase("file");
                                                            property_parsed.erase("id");
                                                            bot.properties=property_parsed;
                                                            bot.botId=botId;
                                                        }
                                                        else
                                                            qDebug() << (
                                                                        QStringLiteral("No botId %1 into %2: properties->Name(): %3, file: %4")
                                                                        .arg(botId)
                                                                        .arg(QString::fromStdString(botFile))
                                                                        .arg(bot->Name())
                                                                        .arg(QString::fromStdString(parsedMap->logicalMap.map_file))
                                                                        );
                                                    }
                                                    else
                                                        qDebug() << (
                                                                    QStringLiteral("missing file botFile, botId %1 into %2: properties->Name(): %3, file: %4")
                                                                    .arg(botId)
                                                                    .arg(QString::fromStdString(botFile))
                                                                    .arg(bot->Name())
                                                                    .arg(QString::fromStdString(parsedMap->logicalMap.map_file))
                                                                    );
                                                }
                                            }
                                            else
                                                qDebug() << (QStringLiteral("No file %1: properties->Name()")
                                                             .arg(QString::fromStdString(botFile)));
                                        }
                                    }
                                    else
                                        qDebug() << QStringLiteral("Is not a number: properties->Name(): NULL");
                                }
                                properties = properties->NextSiblingElement("properties");
                            }
                        }
                    }
                    bot = bot->NextSiblingElement("object");
                }
            }
        }
        child = child->NextSiblingElement("objectgroup");
    }
    return true;
}

#endif //disabled loadOtherMapClientPart

bool MapVisualiserThread::loadOtherMapMetaData(QMap_client *parsedMap)
{
    (void)parsedMap;
    //metadata now loaded via DatapackClientLoader
    return true;
}
#if 0 //disabled: logicalMap no longer in QMap_client
    tinyxml2::XMLDocument *domDocument;
    //open and quick check the file
    std::string fileName;
    stringreplaceAll(fileName,".tmx",".xml");
    if(CatchChallenger::CommonDatapack::commonDatapack.has_xmlLoadedFile(fileName))
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(fileName);
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(fileName);
        const tinyxml2::XMLError loadOkay = domDocument->LoadFile(fileName.c_str());
        if(loadOkay!=0)
        {
            std::cerr << fileName+", "+tinyxml2errordoc(domDocument) << std::endl;
            return false;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(root==NULL)
        return false;
    if(root->Name()!=MapVisualiserThread::text_map)
    {
        qDebug() << QString::fromStdString(fileName) << QStringLiteral(", MapVisualiserThread::loadOtherMapMetaData(): \"map\" root balise not found for the xml file");
        return false;
    }
    if(root->Attribute("type")!=NULL)
        parsedMap->visualType=root->Attribute("type");
    if(root->Attribute("zone")!=NULL)
        parsedMap->zone=root->Attribute("zone");
    //load the name
    {
        bool name_found=false;
        const tinyxml2::XMLElement *name = root->FirstChildElement("name");
        if(!language.empty() && language!="en")
            while(name!=NULL)
            {
                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                {
                    parsedMap->name=name->GetText();
                    name_found=true;
                    break;
                }
                name = name->NextSiblingElement("name");
            }
        if(!name_found)
        {
            name = root->FirstChildElement("name");
            while(name!=NULL)
            {
                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                    if(name->GetText()!=NULL)
                    {
                        parsedMap->name=name->GetText();
                        name_found=true;
                        break;
                    }
                name = name->NextSiblingElement("name");
            }
        }
    }
    return true;
}

#endif //disabled loadOtherMapMetaData

void MapVisualiserThread::loadBotFile()
{
    //bot files now loaded via DatapackClientLoader
    return;
}
#if 0 //disabled: logicalMap no longer in QMap_client
    const std::string file;
    {
    if(botFiles.find(file)!=botFiles.cend())
        return;
    botFiles[file];//create the entry

    tinyxml2::XMLDocument *domDocument;
    if(CatchChallenger::CommonDatapack::commonDatapack.has_xmlLoadedFile(file))
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(file);
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw(file);
        const tinyxml2::XMLError loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
    }

    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
        return;
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found 2 for reputation of the xml file" << std::endl;
        return;
    }
    if(strcmp(root->Name(),"bots")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found for reputation of the xml file" << std::endl;
        return;
    }
    bool ok;
    //load the bots
    const tinyxml2::XMLElement * child = root->FirstChildElement("bot");
    while(child!=NULL)
    {
        if(child->Attribute("id")==NULL)
            qDebug() << (QStringLiteral("Has not attribute \"id\": child->Value(): %1").arg(child->Value()));
        else
        {
            const uint32_t &botId=stringtouint32(child->Attribute("id"),&ok);
            if(ok)
            {
                if(botFiles.at(file).find(botId)!=botFiles.at(file).cend())
                    qDebug() << (QStringLiteral("bot already found with this id: bot->Value(): %1").arg(child->Value()));
                else
                {
                    const tinyxml2::XMLElement * step = child->FirstChildElement("step");
                    while(step!=NULL)
                    {
                        if(step->Attribute("id")==NULL)
                            botFiles[file][botId].step[1]=step;
                        else if(step->Attribute("type")==NULL)
                            qDebug() << (QStringLiteral("Has not attribute \"type\": bot->Value(): %1").arg(step->Value()));
                        else
                        {
                            const uint8_t &stepId=stringtouint8(step->Attribute("id"),&ok);
                            if(ok)
                                botFiles[file][botId].step[stepId]=step;
                        }
                        step = step->NextSiblingElement("step");
                    }
                    //load the name
                    {
                        bool name_found=false;
                        const tinyxml2::XMLElement * name = child->FirstChildElement("name");
                        if(!language.empty() && language!="en")
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                {
                                    botFiles[file][botId].name=name->GetText();
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
                                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    if(name->GetText()!=NULL)
                                    {
                                        botFiles[file][botId].name=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                name = name->NextSiblingElement("name");
                            }
                        }
                    }
                    const CatchChallenger::Bot &bot=botFiles[file][botId];
                    if(bot.step.find(1)==bot.step.cend())
                        botFiles[file].erase(botId);
                }
            }
            else
                qDebug() << (QStringLiteral("Attribute \"id\" is not a number: bot->Value(): %1").arg(child->Value()));
        }
        child = child->NextSiblingElement("bot");
    }
}

#endif //disabled loadBotFile

void MapVisualiserThread::resetAll()
{
    CatchChallenger::Map_loader::teleportConditionsUnparsed.clear();

    botFiles.clear();
}
