#include <QApplication>
#include <QSettings>
#include <QTime>
#include <iostream>

#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_tile.h"

#include "znoise/headers/Simplex.hpp"
#include "VoronioForTiledMapTmx.h"
#include "LoadMap.h"

//#pragma clang optimize on
/*To do: Polygons, Graph Representation, Islands, Elevation, Rivers, Moisture, Biomes, Noisy Edges
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

void addTransitionOnMap(Tiled::Map &tiledMap,const std::vector<LoadMap::TerrainTransition> &terrainTransitionList)
{
    /*Tiled::Layer * walkableLayer=searchTileLayerByName(tiledMap,"Walkable");
    Tiled::Layer * waterLayer=searchTileLayerByName(tiledMap,"Water");*/
    Tiled::TileLayer * transitionLayer=LoadMap::searchTileLayerByName(tiledMap,"Transition");
    Tiled::TileLayer * collisionsLayer=LoadMap::searchTileLayerByName(tiledMap,"Collisions");
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();
    unsigned int y=0;
    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            unsigned int terrainTransitionIndex=0;
            while(terrainTransitionIndex<terrainTransitionList.size())
            {
                const LoadMap::TerrainTransition &terrainTransition=terrainTransitionList.at(terrainTransitionIndex);
                Tiled::TileLayer * const transitionLayerReturn=LoadMap::haveTileAt(tiledMap,x,y,terrainTransition.from_type);
                if(transitionLayerReturn!=NULL)
                {
                    Tiled::TileLayer * transitionLayerTmp=NULL;
                    if(terrainTransition.collision)
                        transitionLayerTmp=collisionsLayer;
                    else if(terrainTransition.replace_tile)
                        transitionLayerTmp=transitionLayerReturn;
                    else
                        transitionLayerTmp=transitionLayer;
                    //check the near tile and determine what transition use
                    uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                    if(x>0 && y>0 && LoadMap::haveTileAt(tiledMap,x-1,y-1,terrainTransition.to_type))
                        to_type_match|=1;
                    if(y>0 && LoadMap::haveTileAt(tiledMap,x,y-1,terrainTransition.to_type))
                        to_type_match|=2;
                    if(x<(w-1) && y>0 && LoadMap::haveTileAt(tiledMap,x+1,y-1,terrainTransition.to_type))
                        to_type_match|=4;
                    if(x>0 && LoadMap::haveTileAt(tiledMap,x-1,y,terrainTransition.to_type))
                        to_type_match|=8;
                    /*if(the center tile)
                        to_type_match|=X;*/
                    if(x<(w-1) && LoadMap::haveTileAt(tiledMap,x+1,y,terrainTransition.to_type))
                        to_type_match|=16;
                    if(x>0 && y<(h-1) && LoadMap::haveTileAt(tiledMap,x-1,y+1,terrainTransition.to_type))
                        to_type_match|=32;
                    if(y<(h-1) && LoadMap::haveTileAt(tiledMap,x,y+1,terrainTransition.to_type))
                        to_type_match|=64;
                    if(x<(w-1) && y<(h-1) && LoadMap::haveTileAt(tiledMap,x+1,y+1,terrainTransition.to_type))
                        to_type_match|=128;
                    //remplace the tile
                    Tiled::Cell cell;
                    cell.tile=NULL;
                    cell.flippedHorizontally=false;
                    cell.flippedVertically=false;
                    cell.flippedAntiDiagonally=false;
                    if(to_type_match&2)
                    {
                        if(to_type_match&8)
                            cell.tile=terrainTransition.transition_tile.at(0);
                        else if(to_type_match&16)
                            cell.tile=terrainTransition.transition_tile.at(2);
                        else
                            cell.tile=terrainTransition.transition_tile.at(1);
                    }
                    else if(to_type_match&64)
                    {
                        if(to_type_match&8)
                            cell.tile=terrainTransition.transition_tile.at(6);
                        else if(to_type_match&16)
                            cell.tile=terrainTransition.transition_tile.at(4);
                        else
                            cell.tile=terrainTransition.transition_tile.at(5);
                    }
                    else if(to_type_match&8)
                        cell.tile=terrainTransition.transition_tile.at(7);
                    else if(to_type_match&16)
                        cell.tile=terrainTransition.transition_tile.at(3);
                    else if(to_type_match&128)
                        cell.tile=terrainTransition.transition_tile.at(8);
                    else if(to_type_match&32)
                        cell.tile=terrainTransition.transition_tile.at(9);
                    else if(to_type_match&1)
                        cell.tile=terrainTransition.transition_tile.at(10);
                    else if(to_type_match&4)
                        cell.tile=terrainTransition.transition_tile.at(11);

                    if(cell.tile!=NULL && !cell.tile->image().isNull())
                        transitionLayerTmp->setCell(x,y,cell);
                    break;
                }
                terrainTransitionIndex++;
            }
            x++;
        }
        y++;
    }
}

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);
    QTime t;

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation-terrain"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    settings.beginGroup("terrain");
    /*do the settings table:
        altitude
        heat
        Give type

        For each type:
        tsx
        tile*/
        //do tile to zone converter
        settings.beginGroup("water");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","animation.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("montain");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",353);
        settings.endGroup();
        settings.beginGroup("transition");
            settings.beginGroup("waterborder");
                if(!settings.contains("from_type"))
                    settings.setValue("from_type","water");
                if(!settings.contains("to_type"))
                    settings.setValue("to_type","grass,grass2");
                if(!settings.contains("transition_tsx"))
                    settings.setValue("transition_tsx","terra.tsx");
                if(!settings.contains("transition_tile"))
                    settings.setValue("transition_tile","184,185,186,218,250,249,248,216,281,282,314,313");
                if(!settings.contains("collision"))
                    settings.setValue("collision",false);
                if(!settings.contains("replace_tile"))
                    settings.setValue("replace_tile",false);
            settings.endGroup();
        settings.endGroup();
    settings.endGroup();
    if(!settings.contains("resize_TerrainMap"))
        settings.setValue("resize_TerrainMap",4);
    if(!settings.contains("resize_TerrainHeat"))
        settings.setValue("resize_TerrainHeat",4);
    if(!settings.contains("scale_TerrainMap"))
        settings.setValue("scale_TerrainMap",(double)0.01f);
    if(!settings.contains("scale_TerrainHeat"))
        settings.setValue("scale_TerrainHeat",(double)0.01f);
    if(!settings.contains("mapWidth"))
        settings.setValue("mapWidth",44);
    if(!settings.contains("mapHeight"))
        settings.setValue("mapHeight",44);
    if(!settings.contains("mapXCount"))
        settings.setValue("mapXCount",3);
    if(!settings.contains("mapYCount"))
        settings.setValue("mapYCount",3);
    if(!settings.contains("seed"))
        settings.setValue("seed",0);
    if(!settings.contains("displayzone"))
        settings.setValue("displayzone",false);

    settings.sync();

    bool ok;
    LoadMap::Terrain grass,water,montain;
    std::vector<LoadMap::TerrainTransition> terrainTransitionList;
    settings.beginGroup("terrain");
        settings.beginGroup("grass");
            grass.tsx=settings.value("tsx").toString();
            grass.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            grass.baseX=1;
            grass.baseY=1;
        settings.endGroup();
        settings.beginGroup("water");
            water.tsx=settings.value("tsx").toString();
            water.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            water.baseX=4;
            water.baseY=4;
        settings.endGroup();
        settings.beginGroup("montain");
            montain.tsx=settings.value("tsx").toString();
            montain.tileId=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            montain.baseX=8;
            montain.baseY=8;
        settings.endGroup();
    settings.endGroup();
    const unsigned int mapWidth=settings.value("mapWidth").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapWidth not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapHeight=settings.value("mapHeight").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapHeight not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapXCount=settings.value("mapXCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapXCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapYCount=settings.value("mapYCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapYCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int seed=settings.value("seed").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "seed not number into the config file" << std::endl;
        abort();
    }
    srand(seed);
    const bool displayzone=settings.value("displayzone").toBool();
    settings.beginGroup("terrain");
    settings.beginGroup("transition");
        const QStringList &groupsNames=settings.childGroups();
        {
            unsigned int index=0;
            while(index<(unsigned int)groupsNames.size())
            {
                settings.beginGroup(groupsNames.at(index));
                    LoadMap::TerrainTransition terrainTransition;
                    //before map creation
                    terrainTransition.collision=settings.value("collision").toBool();
                    terrainTransition.replace_tile=settings.value("replace_tile").toBool();
                    //tempory value
                    terrainTransition.tmp_from_type=LoadMap::stringToTerrainInt(settings.value("from_type").toString());
                    {
                        const QStringList &to_type_list=settings.value("to_type").toString().split(',');
                        unsigned int to_type_index=0;
                        while(to_type_index<(unsigned int)to_type_list.size())
                        {
                            terrainTransition.tmp_to_type.push_back(LoadMap::stringToTerrainInt(to_type_list.at(to_type_index)));
                            to_type_index++;
                        }
                    }
                    terrainTransition.tmp_transition_tsx=settings.value("transition_tsx").toString();
                    {
                        bool ok;
                        const QString &tmp_transition_tile_settings=settings.value("transition_tile").toString();
                        const QStringList &tmp_transition_tile_list=tmp_transition_tile_settings.split(',');
                        unsigned int tmp_transition_tile_index=0;
                        while(tmp_transition_tile_index<(unsigned int)tmp_transition_tile_list.size())
                        {
                            const QString &s=tmp_transition_tile_list.at(tmp_transition_tile_index);
                            terrainTransition.tmp_transition_tile.push_back(s.toUInt(&ok));
                            if(!ok)
                            {
                                std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_transition_tile_settings.toStdString() << " (abort)" << std::endl;
                                abort();
                            }
                            tmp_transition_tile_index++;
                        }
                    }
                    terrainTransitionList.push_back(terrainTransition);
                settings.endGroup();
                index++;
            }
        }
    settings.endGroup();
    settings.endGroup();

    {
        const unsigned int totalWidth=mapWidth*mapXCount;
        const unsigned int totalHeight=mapHeight*mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,seed,1000);
        qDebug("generateGrid took %d ms", t.elapsed());
        t.start();

        /*Tiled::ObjectGroup *layerPoint=new Tiled::ObjectGroup("Point",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerPoint);
        layerPoint->setVisible(false);

        for (const auto &p : grid)
        {
            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(
                                                                ((float)p.x()/VoronioForTiledMapTmx::SCALE),
                                                                ((float)p.y()/VoronioForTiledMapTmx::SCALE)
                                                                ),
                                                            QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }
        qDebug("do point took %d ms", t.elapsed());*/

        const float noiseMapScale=0.005f/((mapXCount+mapYCount)/2);
        Simplex heighmap(seed+500);
        Simplex moisuremap(seed+5200);

        t.start();
        const VoronioForTiledMapTmx::PolygonZoneMap &vd = VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,2);
        if(vd.zones.size()!=grid.size())
            abort();
        qDebug("computVoronoi took %d ms", t.elapsed());

        {
            Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::loadTileset(water,cachedTileset,tiledMap);LoadMap::loadTileset(grass,cachedTileset,tiledMap);LoadMap::loadTileset(montain,cachedTileset,tiledMap);
            LoadMap::load_terrainTransitionList(grass,water,montain,cachedTileset,terrainTransitionList,tiledMap);
            std::vector<std::vector<Tiled::Tile *> > arrayTerrainTile;
            LoadMap::addTerrainTile(arrayTerrainTile,grass,montain);
            if(displayzone)
            {
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                Tiled::ObjectGroup *layerZoneWaterPolygon=LoadMap::addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                Tiled::ObjectGroup *layerZoneWaterTile=LoadMap::addDebugLayer(tiledMap,arrayTerrainTile,false);
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                std::vector<std::vector<Tiled::TileLayer *> > arrayTerrain;
                Tiled::TileLayer *layerZoneWater=LoadMap::addTerrainLayer(tiledMap,arrayTerrain);
                LoadMap::addTerrain(arrayTerrain,layerZoneWater,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height(),water,arrayTerrainTile);
                addTransitionOnMap(tiledMap,terrainTransitionList);
                qDebug("Transitions took %d ms", t.elapsed());
            }
            {
                Tiled::ObjectGroup *layerZoneChunk=new Tiled::ObjectGroup("Chunk",0,0,tiledMap.width(),tiledMap.height());
                layerZoneChunk->setColor(QColor("#ffe000"));
                tiledMap.addLayer(layerZoneChunk);

                unsigned int mapY=0;
                while(mapY<mapYCount)
                {
                    unsigned int mapX=0;
                    while(mapX<mapXCount)
                    {
                        Tiled::MapObject *object = new Tiled::MapObject(QString::number(mapX)+","+QString::number(mapY),"",QPointF(0,0), QSizeF(0.0,0.0));
                        object->setPolygon(QPolygonF(QRectF(mapX*mapWidth,mapY*mapHeight,mapWidth,mapHeight)));
                        object->setShape(Tiled::MapObject::Polygon);
                        layerZoneChunk->addObject(object);
                        mapX++;
                    }
                    mapY++;
                }
                layerZoneChunk->setVisible(false);
            }
            Tiled::MapWriter maprwriter;maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
        }
/* rewrite a real all.tmx split        {
            unsigned int mapY=0;
            while(mapY<mapYCount)
            {
                unsigned int mapX=0;
                while(mapX<mapXCount)
                {
                    Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,mapWidth,mapHeight,16,16);
                    QHash<QString,Tiled::Tileset *> cachedTileset;
                    loadTileset(water,cachedTileset,tiledMap);loadTileset(grass,cachedTileset,tiledMap);loadTileset(montain,cachedTileset,tiledMap);
                    load_terrainTransitionList(grass,water,montain,cachedTileset,terrainTransitionList,tiledMap);
                    if(displayzone)
                    {
                        std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                        Tiled::ObjectGroup *layerZoneWaterPolygon=addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                        std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                        Tiled::ObjectGroup *layerZoneWaterTile=addDebugLayer(tiledMap,arrayTerrainTile,false);
                        addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height(),-(mapWidth*mapX),-(mapHeight*mapY));
                    }
                    Tiled::MapWriter maprwriter;
                    maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/"+QString::number(mapX)+"."+QString::number(mapY)+".tmx");

                    mapX++;
                }
                mapY++;
            }
        }*/
    }

    return 0;
}
