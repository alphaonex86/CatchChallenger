#include "PartialMap.h"

#include <libtiled/mapwriter.h>
#include <libtiled/mapreader.h>
#include <libtiled/mapobject.h>
#include <libtiled/objectgroup.h>
#include <libtiled/tileset.h>
#include <libtiled/tile.h>
#include "../../general/base/cpp11addition.hpp"

#include <QDir>
#include <QCoreApplication>
#include <iostream>
#include <unordered_map>
#include <cmath>

extern std::vector<Tiled::SharedTileset> PartialMap_tilesets_hack;

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
    std::unordered_map<const Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset; // TODO: Need to convert to SharedTileset
    unsigned int indexTileset=0;
    while(indexTileset<(unsigned int)world.tilesetCount())
    {
        const Tiled::SharedTileset tileset=world.tilesetAt(indexTileset);
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
        Tiled::SharedTileset tilesetBase=reader.readTileset(tilesetPath);
        if(tilesetBase==NULL)
        {
            std::cerr << "File not found: " << tilesetPath.toStdString() << std::endl;
            abort();
        }
        tiledMap.addTileset(tilesetBase);
        tilesetBase->setFileName(mapDir.relativeFilePath(tilesetPath));

        PartialMap_tilesets_hack.push_back(tileset);
        PartialMap_tilesets_hack.push_back(tilesetBase);

        templateTilesetToMapTileset[tileset.get()]=tilesetBase.get(); // TODO: Need to convert to SharedTileset
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
                        newCell.setFlippedAntiDiagonally(oldCell.flippedAntiDiagonally());
                        newCell.setFlippedHorizontally(oldCell.flippedHorizontally());
                        newCell.setFlippedVertically(oldCell.flippedVertically());
                        const unsigned int &oldTileId=oldCell.tile()->id();
                        Tiled::Tileset *oldTileset=oldCell.tile()->tileset();
                        Tiled::Tileset *newTileset=templateTilesetToMapTileset.at(oldTileset);
                        newCell.setTile(newTileset->tileAt(oldTileId));
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
            Tiled::ObjectGroup * objectGroup=new Tiled::ObjectGroup(castedLayer->name(),0,0); // ObjectGroup no longer accepts mapWidth,mapHeight
            bool haveContent=false;
            unsigned int objectIndex=0;
            while(objectIndex<(unsigned int)objects.size())
            {
                const Tiled::MapObject* oldobject=objects.at(objectIndex);
                const unsigned int objectx=oldobject->x();
                const unsigned int objecty=oldobject->y() -1;  // FIX: Without this some objects end up in wrong chunks

        #ifdef GENERATEMAPBORDER
                        -1//less the y offset, why?
        #endif
                        ;
                // Convert to pixel units when creating a new Tiled::MapObject
                // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                unsigned int pixels_minX = minX * world.tileWidth();
                unsigned int pixels_minY = minY * world.tileHeight();
                unsigned int pixels_maxX = maxX * world.tileWidth();
                unsigned int pixels_maxY = maxY * world.tileHeight();

                if(objectx>=pixels_minX && objectx<pixels_maxX && objecty>=pixels_minY && objecty<pixels_maxY)
                {
                    const Tiled::Cell &oldCell=oldobject->cell();
                    if(!oldCell.isEmpty())
                    {
                        haveContent=true;
                        QPointF position=oldobject->position();
                        position.setX(position.x()-pixels_minX); // pixels_minX to fix v0.10.0 API breakage
                        position.setY(position.y()-pixels_minY); // pixels_minY to fix v0.10.0 API breakage
                        Tiled::MapObject* newobject=new Tiled::MapObject(oldobject->name(),oldobject->type(),position,oldobject->size());
                        Tiled::Cell newCell;
                        newCell.setFlippedAntiDiagonally(oldCell.flippedAntiDiagonally());
                        newCell.setFlippedHorizontally(oldCell.flippedHorizontally());
                        newCell.setFlippedVertically(oldCell.flippedVertically());
                        const unsigned int &oldTileId=oldCell.tile()->id();
                        Tiled::Tileset *oldTileset=oldCell.tile()->tileset();
                        Tiled::Tileset *newTileset=templateTilesetToMapTileset.at(oldTileset);
                        newCell.setTile(newTileset->tileAt(oldTileId));
                        newobject->setCell(newCell);
                        newobject->setProperties(oldobject->properties());

                        objectGroup->addObject(newobject);

                        if(objectGroup->name()=="Moving" && newobject->type()=="rescue")
                        {
                            PartialMap::RecuesPoint recuesPoint;

                            // Convert to pixel units when creating a new Tiled::MapObject
                            // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                            recuesPoint.x=floor(position.x() / world.tileWidth());
                            recuesPoint.y=floor((position.y()-1) / world.tileHeight());//I don't know why it have this offset
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

#ifdef TILED_CSV
    tiledMap.setLayerDataFormat(Tiled::Map::CSV);  // DEBUG
#endif

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
        // It was commented out in the commit removing the embeded libs 42df8825b7d1dc939264d4145aeabf4ea2f1bdf6
        /*
        QHashIterator<QString,Tiled::Tileset *> i(Tiled::Tileset::preloadedTileset);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        Tiled::Tileset::preloadedTileset.clear();
        */
    }
    {
        unsigned int index=0;
        while(index<(unsigned int)tiledMap.tilesetCount())
        {
            Tiled::SharedTileset tileset=tiledMap.tilesetAt(index);
            PartialMap_tilesets_hack.push_back(tileset);

            if(!tileset->isExternal())
            {
                QString tsxPath=tileset->imageSource().toString();
                tsxPath.replace(".png",".tsx");
                if(!QFile::exists(tsxPath))
                {
                    Tiled::MapWriter writer;

                    //tileset->setLayerDataFormat(Tiled::Map::CSV);  // DEBUG

                    //write external tileset
                    writer.writeTileset(*tileset,tsxPath);
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
