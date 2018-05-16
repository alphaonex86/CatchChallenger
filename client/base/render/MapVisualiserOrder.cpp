#include "MapVisualiserOrder.h"
#include <QDebug>

std::string MapVisualiserOrder::text_blockedtext=QStringLiteral("blockedtext");
std::string MapVisualiserOrder::text_en=QStringLiteral("en");
std::string MapVisualiserOrder::text_lang=QStringLiteral("lang");
std::string MapVisualiserOrder::text_Dyna_management=QStringLiteral("Dyna management");
std::string MapVisualiserOrder::text_Moving=QStringLiteral("Moving");
std::string MapVisualiserOrder::text_door=QStringLiteral("door");
std::string MapVisualiserOrder::text_Object=QStringLiteral("Object");
std::string MapVisualiserOrder::text_bot=QStringLiteral("bot");
std::string MapVisualiserOrder::text_bots=QStringLiteral("bots");
std::string MapVisualiserOrder::text_WalkBehind=QStringLiteral("WalkBehind");
std::string MapVisualiserOrder::text_Collisions=QStringLiteral("Collisions");
std::string MapVisualiserOrder::text_Grass=QStringLiteral("Grass");
std::string MapVisualiserOrder::text_animation=QStringLiteral("animation");
std::string MapVisualiserOrder::text_dotcomma=QStringLiteral(";");
std::string MapVisualiserOrder::text_ms=QStringLiteral("ms");
std::string MapVisualiserOrder::text_frames=QStringLiteral("frames");
std::string MapVisualiserOrder::text_map=QStringLiteral("map");
std::string MapVisualiserOrder::text_objectgroup=QStringLiteral("objectgroup");
std::string MapVisualiserOrder::text_name=QStringLiteral("name");
std::string MapVisualiserOrder::text_object=QStringLiteral("object");
std::string MapVisualiserOrder::text_type=QStringLiteral("type");
std::string MapVisualiserOrder::text_x=QStringLiteral("x");
std::string MapVisualiserOrder::text_y=QStringLiteral("y");
std::string MapVisualiserOrder::text_botfight=QStringLiteral("botfight");
std::string MapVisualiserOrder::text_property=QStringLiteral("property");
std::string MapVisualiserOrder::text_value=QStringLiteral("value");
std::string MapVisualiserOrder::text_file=QStringLiteral("file");
std::string MapVisualiserOrder::text_id=QStringLiteral("id");
std::string MapVisualiserOrder::text_slash=QStringLiteral("/");
std::string MapVisualiserOrder::text_dotxml=QStringLiteral(".xml");
std::string MapVisualiserOrder::text_dottmx=QStringLiteral(".tmx");
std::string MapVisualiserOrder::text_step=QStringLiteral("step");
std::string MapVisualiserOrder::text_properties=QStringLiteral("properties");
std::string MapVisualiserOrder::text_shop=QStringLiteral("shop");
std::string MapVisualiserOrder::text_learn=QStringLiteral("learn");
std::string MapVisualiserOrder::text_heal=QStringLiteral("heal");
std::string MapVisualiserOrder::text_market=QStringLiteral("market");
std::string MapVisualiserOrder::text_zonecapture=QStringLiteral("zonecapture");
std::string MapVisualiserOrder::text_fight=QStringLiteral("fight");
std::string MapVisualiserOrder::text_zone=QStringLiteral("zone");
std::string MapVisualiserOrder::text_fightid=QStringLiteral("fightid");
std::string MapVisualiserOrder::text_randomoffset=QStringLiteral("random-offset");
std::string MapVisualiserOrder::text_visible=QStringLiteral("visible");
std::string MapVisualiserOrder::text_true=QStringLiteral("true");
std::string MapVisualiserOrder::text_false=QStringLiteral("false");
std::string MapVisualiserOrder::text_trigger=QStringLiteral("trigger");

QRegularExpression MapVisualiserOrder::regexMs=QRegularExpression(QStringLiteral("^[0-9]{1,5}ms$"));
QRegularExpression MapVisualiserOrder::regexFrames=QRegularExpression(QStringLiteral("^[0-9]{1,3}frames$"));
QRegularExpression MapVisualiserOrder::regexTrigger=QRegularExpression("^start:([0-9]+)ms;([0-9]+)frames;leave:([0-9]+)ms;([0-9]+)frames;?(.*)$");
QRegularExpression MapVisualiserOrder::regexTriggerAgain=QRegularExpression("^again:([0-9]+)ms;([0-9]+)frames;?(.*)$");

MapVisualiserOrder::MapVisualiserOrder()
{
}

MapVisualiserOrder::~MapVisualiserOrder()
{
}

void MapVisualiserOrder::layerChangeLevelAndTagsChange(MapVisualiserOrder::Map_full *tempMapObject,bool hideTheDoors)
{
    //remove the hidden tags, and unknown layer
    int index=0;
    while(index<tempMapObject->tiledMap->layerCount())
    {
        if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
        {
            //remove the unknow layer
            if(objectGroup->name()==MapVisualiserOrder::text_Moving)
            {
                std::vector<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    const uint32_t &x=objects.at(index2)->x();
                    const uint32_t &y=objects.at(index2)->y()-1;
                    //remove the unknow object
                    if(objects.at(index2)->type()==MapVisualiserOrder::text_door)
                    {
                        if(hideTheDoors)
                        {
                            objectGroup->removeObject(objects.at(index2));
                            delete objects.at(index2);
                        }
                        else
                        {
                            const Tiled::Tile *tile=objects.at(index2)->cell().tile;
                            if(tile!=NULL)
                            {
                                //animation only for door
                                const std::string &animation=tile->property(MapVisualiserOrder::text_animation);
                                if(!animation.isEmpty())
                                {
                                    const QStringList &animationList=animation.split(MapVisualiserOrder::text_dotcomma);
                                    if(animationList.size()==2)
                                    {
                                        if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                        {
                                            std::string msString=animationList.at(0);
                                            std::string framesString=animationList.at(1);
                                            msString.remove(MapVisualiserOrder::text_ms);
                                            framesString.remove(MapVisualiserOrder::text_frames);
                                            uint16_t ms=msString.toUShort();
                                            uint8_t frames=static_cast<uint8_t>(framesString.toUShort());
                                            if(ms>0 && frames>1)
                                            {
                                                if(!tempMapObject->doors.contains(std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))))
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=door;
                                                }
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
                                    #ifndef ONLYMAPRENDER
                                    qDebug() << "animation properties not found for the door at " << x << "," << y;
                                    #endif
                                    objectGroup->removeObject(objects.at(index2));
                                    delete objects.at(index2);
                                }
                            }
                        }
                    }
                    else if(objects.at(index2)->type()=="border-bottom" ||
                            objects.at(index2)->type()=="border-top" ||
                            objects.at(index2)->type()=="border-right" ||
                            objects.at(index2)->type()=="border-left" ||
                            objects.at(index2)->type()=="rescue" ||
                            objects.at(index2)->type()=="door" ||
                            objects.at(index2)->type()=="teleport on push" ||
                            objects.at(index2)->type()=="teleport on it")
                    {
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    else
                    {
                        #ifndef ONLYMAPRENDER
                        qDebug() << "Layer Moving, unknown object at " << x << "," << y << " type: " << objects.at(index2)->type();
                        #endif
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                    }
                    index2++;
                }
                index++;
            }
            else if(objectGroup->name()==MapVisualiserOrder::text_Object)
            {
                std::vector<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    //remove the bot
                    if(objects.at(index2)->type()==MapVisualiserOrder::text_bot)
                    {
                        /// \see MapController::loadBotOnTheMap()
                        #ifndef ONLYMAPRENDER
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                        #endif
                    }
                    else if(objects.at(index2)->type()==MapVisualiserOrder::text_object)
                    {}
                    //remove the unknow object
                    else
                    {
                        #ifndef ONLYMAPRENDER
                        qDebug() << "layer Object, unknown object at " << objects.at(index2)->x() << "," << objects.at(index2)->y() << " type: " << objects.at(index2)->type();
                        #endif
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
        /// for the animation of tile see MapVisualiser::applyTheAnimationTimer()
        else
            index++;
    }


    //search WalkBehind layer
    {
        int index=0;
        const int &listSize=tempMapObject->tiledMap->layerCount();
        while(index<listSize)
        {
            const Tiled::Layer * layer=tempMapObject->tiledMap->layerAt(index);
            if(layer->name()==MapVisualiserOrder::text_WalkBehind)
            {
                tempMapObject->objectGroupIndex=index;
                tempMapObject->tiledMap->insertLayer(index,tempMapObject->objectGroup);
                break;
            }
            index++;
        }
        if(index==listSize)
        {
            //search Collisions layer
            index=listSize-1;
            while(index>=0)
            {
                if(tempMapObject->tiledMap->layerAt(index)->name()==MapVisualiserOrder::text_Collisions)
                {
                    tempMapObject->objectGroupIndex=index+1;
                    tempMapObject->tiledMap->insertLayer(index+1,tempMapObject->objectGroup);
                    break;
                }
                index--;
            }
            if(index<0)
            {
                qDebug() << QStringLiteral("Unable to locate the \"Collisions\" layer on the map: %1").arg(QString::fromStdString(tempMapObject->logicalMap.map_file));
                tempMapObject->tiledMap->addLayer(tempMapObject->objectGroup);
            }
        }
    }

    //move the Moving layer on dyna layer
    {
        int index=0;
        while(index<tempMapObject->tiledMap->layerCount())
        {
            if(Tiled::ObjectGroup *objectGroup = tempMapObject->tiledMap->layerAt(index)->asObjectGroup())
            {
                if(objectGroup->name()==MapVisualiserOrder::text_Moving)
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
            index++;
        }
    }
}

