#include "MapVisualiserOrder.h"
#include <QDebug>

QString MapVisualiserOrder::text_blockedtext=QStringLiteral("blockedtext");
QString MapVisualiserOrder::text_en=QStringLiteral("en");
QString MapVisualiserOrder::text_lang=QStringLiteral("lang");
QString MapVisualiserOrder::text_Dyna_management=QStringLiteral("Dyna management");
QString MapVisualiserOrder::text_Moving=QStringLiteral("Moving");
QString MapVisualiserOrder::text_door=QStringLiteral("door");
QString MapVisualiserOrder::text_Object=QStringLiteral("Object");
QString MapVisualiserOrder::text_bot=QStringLiteral("bot");
QString MapVisualiserOrder::text_bots=QStringLiteral("bots");
QString MapVisualiserOrder::text_WalkBehind=QStringLiteral("WalkBehind");
QString MapVisualiserOrder::text_Collisions=QStringLiteral("Collisions");
QString MapVisualiserOrder::text_Grass=QStringLiteral("Grass");
QString MapVisualiserOrder::text_animation=QStringLiteral("animation");
QString MapVisualiserOrder::text_dotcomma=QStringLiteral(";");
QString MapVisualiserOrder::text_ms=QStringLiteral("ms");
QString MapVisualiserOrder::text_frames=QStringLiteral("frames");
QString MapVisualiserOrder::text_map=QStringLiteral("map");
QString MapVisualiserOrder::text_objectgroup=QStringLiteral("objectgroup");
QString MapVisualiserOrder::text_name=QStringLiteral("name");
QString MapVisualiserOrder::text_object=QStringLiteral("object");
QString MapVisualiserOrder::text_type=QStringLiteral("type");
QString MapVisualiserOrder::text_x=QStringLiteral("x");
QString MapVisualiserOrder::text_y=QStringLiteral("y");
QString MapVisualiserOrder::text_botfight=QStringLiteral("botfight");
QString MapVisualiserOrder::text_property=QStringLiteral("property");
QString MapVisualiserOrder::text_value=QStringLiteral("value");
QString MapVisualiserOrder::text_file=QStringLiteral("file");
QString MapVisualiserOrder::text_id=QStringLiteral("id");
QString MapVisualiserOrder::text_slash=QStringLiteral("/");
QString MapVisualiserOrder::text_dotxml=QStringLiteral(".xml");
QString MapVisualiserOrder::text_dottmx=QStringLiteral(".tmx");
QString MapVisualiserOrder::text_step=QStringLiteral("step");
QString MapVisualiserOrder::text_properties=QStringLiteral("properties");
QString MapVisualiserOrder::text_shop=QStringLiteral("shop");
QString MapVisualiserOrder::text_learn=QStringLiteral("learn");
QString MapVisualiserOrder::text_heal=QStringLiteral("heal");
QString MapVisualiserOrder::text_market=QStringLiteral("market");
QString MapVisualiserOrder::text_zonecapture=QStringLiteral("zonecapture");
QString MapVisualiserOrder::text_fight=QStringLiteral("fight");
QString MapVisualiserOrder::text_zone=QStringLiteral("zone");
QString MapVisualiserOrder::text_fightid=QStringLiteral("fightid");
QString MapVisualiserOrder::text_randomoffset=QStringLiteral("random-offset");
QString MapVisualiserOrder::text_visible=QStringLiteral("visible");
QString MapVisualiserOrder::text_true=QStringLiteral("true");
QString MapVisualiserOrder::text_false=QStringLiteral("false");
QString MapVisualiserOrder::text_trigger=QStringLiteral("trigger");

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
                QList<Tiled::MapObject*> objects=objectGroup->objects();
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
                                const QString &animation=tile->property(MapVisualiserOrder::text_animation);
                                if(!animation.isEmpty())
                                {
                                    const QStringList &animationList=animation.split(MapVisualiserOrder::text_dotcomma);
                                    if(animationList.size()==2)
                                    {
                                        if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                        {
                                            QString msString=animationList.at(0);
                                            QString framesString=animationList.at(1);
                                            msString.remove(MapVisualiserOrder::text_ms);
                                            framesString.remove(MapVisualiserOrder::text_frames);
                                            uint16_t ms=msString.toUShort();
                                            uint8_t frames=static_cast<uint8_t>(framesString.toUShort());
                                            if(ms>0 && frames>1)
                                            {
                                                if(!tempMapObject->doors.contains(QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))))
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=door;
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
                QList<Tiled::MapObject*> objects=objectGroup->objects();
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

