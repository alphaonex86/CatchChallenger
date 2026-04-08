#include "MapVisualiserOrder.hpp"
#include "QMap_client.hpp"
#include <QDebug>
#include <cmath>

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

/* QMap_client constructor is now in QMap_client.cpp */

void MapVisualiserOrder::layerChangeLevelAndTagsChange(QMap_client *tempMapObject, bool hideTheDoors)
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
                            const Tiled::Tile *tile=objects.at(index2)->cell().tile();
                            if(tile!=NULL)
                            {
                                //animation only for door
                                const QString &animation=tile->property("animation").toString();
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
                                                const float TILE_SIZE = 16.0f;
                                                uint32_t xTile = std::floor(x/TILE_SIZE);
                                                uint32_t yTile = std::floor(y/TILE_SIZE);
                                                if(tempMapObject->doors.find(
                                                            std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(xTile),static_cast<uint8_t>(yTile))
                                                            )==
                                                        tempMapObject->doors.cend())
                                                {
                                                    MapDoor *door=new MapDoor(objects.at(index2),frames,ms);
                                                    tempMapObject->doors[std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(xTile),static_cast<uint8_t>(yTile))]=door;
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
                qDebug() << QStringLiteral("Unable to locate the \"Collisions\" layer on the map");
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

