#include "MapVisualiserOrder.hpp"
#include <QDebug>

std::string MapVisualiserOrder::text_blockedtext="blockedtext";
std::string MapVisualiserOrder::text_en="en";
std::string MapVisualiserOrder::text_lang="lang";
std::string MapVisualiserOrder::text_Dyna_management="Dyna management";
std::string MapVisualiserOrder::text_Moving="Moving";
std::string MapVisualiserOrder::text_door="door";
std::string MapVisualiserOrder::text_Object="Object";
std::string MapVisualiserOrder::text_bot="bot";
std::string MapVisualiserOrder::text_bots="bots";
std::string MapVisualiserOrder::text_WalkBehind="WalkBehind";
std::string MapVisualiserOrder::text_Collisions="Collisions";
std::string MapVisualiserOrder::text_Grass="Grass";
std::string MapVisualiserOrder::text_animation="animation";
std::string MapVisualiserOrder::text_dotcomma=";";
std::string MapVisualiserOrder::text_ms="ms";
std::string MapVisualiserOrder::text_frames="frames";
std::string MapVisualiserOrder::text_map="map";
std::string MapVisualiserOrder::text_objectgroup="objectgroup";
std::string MapVisualiserOrder::text_name="name";
std::string MapVisualiserOrder::text_object="object";
std::string MapVisualiserOrder::text_type="type";
std::string MapVisualiserOrder::text_x="x";
std::string MapVisualiserOrder::text_y="y";
std::string MapVisualiserOrder::text_botfight="botfight";
std::string MapVisualiserOrder::text_property="property";
std::string MapVisualiserOrder::text_value="value";
std::string MapVisualiserOrder::text_file="file";
std::string MapVisualiserOrder::text_id="id";
std::string MapVisualiserOrder::text_slash="/";
std::string MapVisualiserOrder::text_dotxml=".xml";
std::string MapVisualiserOrder::text_dottmx=".tmx";
std::string MapVisualiserOrder::text_properties="properties";
std::string MapVisualiserOrder::text_shop="shop";
std::string MapVisualiserOrder::text_learn="learn";
std::string MapVisualiserOrder::text_heal="heal";
std::string MapVisualiserOrder::text_market="market";
std::string MapVisualiserOrder::text_zonecapture="zonecapture";
std::string MapVisualiserOrder::text_fight="fight";
std::string MapVisualiserOrder::text_zone="zone";
std::string MapVisualiserOrder::text_fightid="fightid";
std::string MapVisualiserOrder::text_randomoffset="random-offset";
std::string MapVisualiserOrder::text_visible="visible";
std::string MapVisualiserOrder::text_true="true";
std::string MapVisualiserOrder::text_false="false";
std::string MapVisualiserOrder::text_trigger="trigger";

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

Map_full::Map_full()
{
    logicalMap.xmlRoot=NULL;
    logicalMap.border.bottom.map=NULL;
    logicalMap.border.bottom.x_offset=0;
    logicalMap.border.top.map=NULL;
    logicalMap.border.top.x_offset=0;
    logicalMap.border.right.map=NULL;
    logicalMap.border.right.y_offset=0;
    logicalMap.border.left.map=NULL;
    logicalMap.border.left.y_offset=0;
    logicalMap.teleporter=NULL;
    logicalMap.teleporter_list_size=0;
    logicalMap.width=0;
    logicalMap.height=0;
    logicalMap.group=0;
    logicalMap.id=0;
    tiledMap=NULL;
    tiledRender=NULL;
    objectGroup=NULL;
    objectGroupIndex=0;
    relative_x=0;
    relative_y=0;
    relative_x_pixel=0;
    relative_y_pixel=0;
    displayed=0;
}

void MapVisualiserOrder::layerChangeLevelAndTagsChange(Map_full *tempMapObject,bool hideTheDoors)
{
    //remove the hidden tags, and unknown layer
    int index=0;
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
                    const uint32_t &x=objects.at(index2)->x();
                    const uint32_t &y=objects.at(index2)->y()-1;
                    //remove the unknow object
                    if(objects.at(index2)->type()=="door")
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
                                const QString &animation=tile->property("animation");
                                if(!animation.isEmpty())
                                {
                                    const QStringList &animationList=animation.split(";");
                                    if(animationList.size()==2)
                                    {
                                        if(animationList.at(0).contains(regexMs) && animationList.at(1).contains(regexFrames))
                                        {
                                            QString msString=animationList.at(0);
                                            QString framesString=animationList.at(1);
                                            msString.remove("ms");
                                            framesString.remove("frames");
                                            uint16_t ms=msString.toUShort();
                                            uint8_t frames=static_cast<uint8_t>(framesString.toUShort());
                                            if(ms>0 && frames>1)
                                            {
                                                if(tempMapObject->doors.find(
                                                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))
                                                            )==
                                                        tempMapObject->doors.cend())
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))]=door;
                                                }
                                            }
                                            else
                                                qDebug() << "ms is 0 or frame is <=1" << animation;
                                        }
                                        else
                                            qDebug() << "Wrong animation tile args regex match" << animation;
                                    }
                                    else
                                        qDebug() << "Wrong animation tile args count (Order): " << animation;
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
            else if(objectGroup->name()=="Object")
            {
                QList<Tiled::MapObject*> objects=objectGroup->objects();
                int index2=0;
                while(index2<objects.size())
                {
                    //remove the bot
                    if(objects.at(index2)->type()=="bot")
                    {
                        /// \see MapController::loadBotOnTheMap()
                        #ifndef ONLYMAPRENDER
                        objectGroup->removeObject(objects.at(index2));
                        delete objects.at(index2);
                        #endif
                    }
                    else if(objects.at(index2)->type()=="object")
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
            if(layer->name()=="WalkBehind")
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
                if(tempMapObject->tiledMap->layerAt(index)->name()=="Collisions")
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
            index++;
        }
    }
}

