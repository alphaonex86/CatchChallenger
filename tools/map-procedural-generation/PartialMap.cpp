#include "PartialMap.h"

#include "../../client/tiled/tiled_mapwriter.hpp"
#include "../../client/tiled/tiled_mapreader.hpp"
#include "../../client/tiled/tiled_mapobject.hpp"
#include "../../client/tiled/tiled_objectgroup.hpp"
#include "../../client/tiled/tiled_tileset.hpp"
#include "../../client/tiled/tiled_tile.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <QDir>
#include <QCoreApplication>
#include <iostream>
#include <unordered_map>
#include <cmath>

bool PartialMap::save(const Tiled::Map &world, const unsigned int &minX, const unsigned int &minY,
                      const unsigned int &maxX, const unsigned int &maxY, const std::string &file, std::vector<RecuesPoint> &recuesPoints,
                      const std::string &type,const std::string &zone,const std::string &name,
                      const std::string &additionalXmlInfo,const bool writeAdditionalData,const bool appendPath)
{
    const unsigned int mapWidth=maxX-minX;
    const unsigned int mapHeight=maxY-minY;
    Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,mapWidth,mapHeight,16,16);
    QFileInfo fileInfo;
    if(appendPath)
        fileInfo=QFileInfo(QString::fromStdString(QCoreApplication::applicationDirPath().toStdString()+"/dest/map/main/official/"+file));
    else
        fileInfo=QFileInfo(QString::fromStdString(file));
    QDir mapDir(fileInfo.absolutePath());
    if(appendPath)
        if(!mapDir.mkpath(fileInfo.absolutePath()))
        {
            std::cerr << "Unable to create path: " << fileInfo.absolutePath().toStdString() << std::endl;
            abort();
        }

    //add external tileset
    std::unordered_map<const Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset;
    unsigned int indexTileset=0;
    while(indexTileset<(unsigned int)world.tilesetCount())
    {
        const Tiled::Tileset * const tileset=world.tilesetAt(indexTileset);
        QString tilesetFileName=tileset->fileName();
        if(tilesetFileName.isEmpty())
        {
            std::cerr << "tileset->fileName() is empty, internal tileset not supported" << std::endl;
            abort();
        }
        if(appendPath)
            if(!QFile::exists(tilesetFileName))
            {
                QString pathAppend=QCoreApplication::applicationDirPath()+"/dest/map/main/official/";
                if(!tilesetFileName.startsWith("/"))
                    tilesetFileName=pathAppend+tilesetFileName;
            }
        QString tilesetPath(QFileInfo(tilesetFileName).absoluteFilePath());

        Tiled::MapReader reader;
        Tiled::Tileset *tilesetBase=reader.readTileset(tilesetPath);
        if(tilesetBase==NULL)
        {
            std::cerr << "File not found: " << tilesetPath.toStdString() << std::endl;
            abort();
        }
        tiledMap.addTileset(tilesetBase);
        tilesetBase->setFileName(mapDir.relativeFilePath(tilesetPath));
        templateTilesetToMapTileset[tileset]=tilesetBase;
        indexTileset++;
    }

    unsigned int indexLayer=0;
    while(indexLayer<(unsigned int)world.layerCount())
    {
        const Tiled::Layer * const layer=world.layerAt(indexLayer);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            //add zone of layer
            Tiled::TileLayer * tileLayer=new Tiled::TileLayer(castedLayer->name(),0,0,mapWidth,mapHeight);
            bool haveContent=false;
            unsigned int y=0;
            while(y<mapHeight)
            {
                unsigned int x=0;
                while(x<mapWidth)
                {
                    const Tiled::Cell &oldCell=castedLayer->cellAt(minX+x,minY+y);
                    if(!oldCell.isEmpty())
                    {
                        haveContent=true;
                        Tiled::Cell newCell;
                        newCell.flippedAntiDiagonally=oldCell.flippedAntiDiagonally;
                        newCell.flippedHorizontally=oldCell.flippedHorizontally;
                        newCell.flippedVertically=oldCell.flippedVertically;
                        const unsigned int &oldTileId=oldCell.tile->id();
                        Tiled::Tileset *oldTileset=oldCell.tile->tileset();
                        Tiled::Tileset *newTileset=templateTilesetToMapTileset.at(oldTileset);
                        newCell.tile=newTileset->tileAt(oldTileId);
                        tileLayer->setCell(x,y,newCell);
                    }
                    x++;
                }
                y++;
            }
            //if zone of the layer is not empty
            if(haveContent)
                tiledMap.addLayer(tileLayer);
            else
                delete tileLayer;
        }
        else if(layer->isObjectGroup())
        {
            const Tiled::ObjectGroup * const castedLayer=static_cast<const Tiled::ObjectGroup * const>(layer);
            const QList<Tiled::MapObject*> &objects=castedLayer->objects();
            Tiled::ObjectGroup * objectGroup=new Tiled::ObjectGroup(castedLayer->name(),0,0,mapWidth,mapHeight);
            bool haveContent=false;
            unsigned int objectIndex=0;
            while(objectIndex<(unsigned int)objects.size())
            {
                const Tiled::MapObject* oldobject=objects.at(objectIndex);
                const unsigned int objectx=oldobject->x();
                const unsigned int objecty=oldobject->y()
        #ifdef GENERATEMAPBORDER
                        -1//less the y offset, why?
        #endif
                        ;
                if(objectx>=minX && objectx<maxX && objecty>=minY && objecty<maxY)
                {
                    const Tiled::Cell &oldCell=oldobject->cell();
                    if(!oldCell.isEmpty())
                    {
                        haveContent=true;
                        QPointF position=oldobject->position();
                        position.setX(position.x()-minX);
                        position.setY(position.y()-minY);
                        Tiled::MapObject* newobject=new Tiled::MapObject(oldobject->name(),oldobject->type(),position,oldobject->size());
                        Tiled::Cell newCell;
                        newCell.flippedAntiDiagonally=oldCell.flippedAntiDiagonally;
                        newCell.flippedHorizontally=oldCell.flippedHorizontally;
                        newCell.flippedVertically=oldCell.flippedVertically;
                        const unsigned int &oldTileId=oldCell.tile->id();
                        Tiled::Tileset *oldTileset=oldCell.tile->tileset();
                        Tiled::Tileset *newTileset=templateTilesetToMapTileset.at(oldTileset);
                        newCell.tile=newTileset->tileAt(oldTileId);
                        newobject->setCell(newCell);
                        newobject->setProperties(oldobject->properties());

                        objectGroup->addObject(newobject);

                        if(objectGroup->name()=="Moving" && newobject->type()=="rescue")
                        {
                            PartialMap::RecuesPoint recuesPoint;
                            recuesPoint.x=floor(position.x());
                            recuesPoint.y=floor(position.y())-1;//I don't know why it have this offset
                            recuesPoint.map=file;
                            if(stringEndsWith(recuesPoint.map,".tmx"))
                                recuesPoint.map.erase(recuesPoint.map.size()-4);
                            recuesPoints.push_back(recuesPoint);
                        }
                    }
                }
                objectIndex++;
            }
            //if zone of the layer is not empty
            if(haveContent)
                tiledMap.addLayer(objectGroup);
            else
                delete objectGroup;
        }
        else
            abort();
        indexLayer++;
    }

    Tiled::MapWriter maprwriter;
    bool returnVar=maprwriter.writeMap(&tiledMap,fileInfo.absoluteFilePath());
    if(!returnVar)
        std::cerr << maprwriter.errorString().toStdString() << std::endl;

    if(writeAdditionalData)
    {
        QString xmlPath(fileInfo.absoluteFilePath());
        xmlPath.remove(xmlPath.size()-4,4);
        xmlPath+=".xml";
        QFile xmlinfo(xmlPath);
        if(xmlinfo.open(QFile::WriteOnly))
        {
            QString content("<map type=\""+QString::fromStdString(type)+"\"");
            if(!zone.empty())
                content+=" zone=\""+QString::fromStdString(zone)+"\"";
            content+=">\n"
            "  <name>"+QString::fromStdString(name)+"</name>\n"+
            QString::fromStdString(additionalXmlInfo)+
            "</map>";
            QByteArray contentData(content.toUtf8());
            xmlinfo.write(contentData.constData(),contentData.size());
            xmlinfo.close();
        }
        else
        {
            std::cerr << "Unable to write " << xmlPath.toStdString() << std::endl;
            returnVar=false;
        }
    }

    //delete map
    {
        QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        Tiled::Tileset::preloadedTileset.clear();
    }
    {
        unsigned int index=0;
        while(index<(unsigned int)tiledMap.tilesetCount())
        {
            Tiled::Tileset * tileset=tiledMap.tilesetAt(index);
            if(!tileset->isExternal())
            {
                QString tsxPath=tileset->imageSource();
                tsxPath.replace(".png",".tsx");
                if(!QFile::exists(tsxPath))
                {
                    Tiled::MapWriter writer;
                    //write external tileset
                    writer.writeTileset(tileset,tsxPath);
                    std::cout << "Write new tileset: " << tsxPath.toStdString() << std::endl;
                }
                //use the external tileset
                tileset->setFileName(tsxPath);
            }
            index++;
        }
    }
    return returnVar;
}
