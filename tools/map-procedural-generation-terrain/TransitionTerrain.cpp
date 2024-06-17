#include "TransitionTerrain.h"
#include "MapBrush.h"

#include <iostream>

int findLayerIndexByName(const Tiled::Map *map, const QString &targetLayerName) {
    const auto &layers = map->layers();
    for (int i = 0; i < layers.size(); ++i) {
        const Tiled::Layer *layer = layers.at(i);

        // Check if the layer name matches the target layer name
        if (layer->name() == targetLayerName) {
            return i;  // Found, return the index
        }
    }

    return -1;  // Not found
}

typedef VoronioForTiledMapTmx vd;

uint16_t TransitionTerrain::layerMask(const Tiled::TileLayer * const terrainLayer,const unsigned int &x,const unsigned int &y,Tiled::Tile *tile,const bool XORop)
{
    const unsigned int &w=terrainLayer->width();
    const unsigned int &h=terrainLayer->height();
    uint16_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
    if(x>0 && y>0)
    {
        if((terrainLayer->cellAt(x-1,y-1).tile()!=tile) ^ XORop)
            to_type_match|=1;
    }
    if(y>0)
    {
        if((terrainLayer->cellAt(x,y-1).tile()!=tile) ^ XORop)
            to_type_match|=2;
    }
    if(x<(w-1) && y>0)
    {
        if((terrainLayer->cellAt(x+1,y-1).tile()!=tile) ^ XORop)
            to_type_match|=4;
    }
    if(x>0)
    {
        if((terrainLayer->cellAt(x-1,y).tile()!=tile) ^ XORop)
            to_type_match|=8;
    }
    if((terrainLayer->cellAt(x,y).tile()!=tile) ^ XORop)
        to_type_match|=16;
    if(x<(w-1))
    {
        if((terrainLayer->cellAt(x+1,y).tile()!=tile) ^ XORop)
            to_type_match|=32;
    }
    if(x>0 && y<(h-1))
    {
        if((terrainLayer->cellAt(x-1,y+1).tile()!=tile) ^ XORop)
            to_type_match|=64;
    }
    if(y<(h-1))
    {
        if((terrainLayer->cellAt(x,y+1).tile()!=tile) ^ XORop)
            to_type_match|=128;
    }
    if(x<(w-1) && y<(h-1))
    {
        if((terrainLayer->cellAt(x+1,y+1).tile()!=tile) ^ XORop)
            to_type_match|=256;
    }
    return to_type_match;
}

void TransitionTerrain::addTransitionOnMap(Tiled::Map &tiledMap)
{
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();

    //list the layer name to parse
    QHash<QString,LoadMap::Terrain> transition;
    QStringList transitionList;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            if(!terrain.tmp_layerString.isEmpty())
            {
                if(!transitionList.contains(terrain.terrainName))
                {
                    transition[terrain.terrainName]=terrain;
                    transitionList << terrain.terrainName;
                }
            }
        }

    {
        Tiled::TileLayer * transitionLayerMask=LoadMap::searchTileLayerByName(tiledMap,"OnGrass");
        Tiled::TileLayer * const colisionLayerMask=LoadMap::searchTileLayerByName(tiledMap,"Collisions");
        unsigned int index=0;
        while(index<(unsigned int)transitionList.size())
        {
            const LoadMap::Terrain &terrain=transition.value(transitionList.at(index));
            Tiled::TileLayer * terrainLayer=terrain.tileLayer;
            Tiled::TileLayer * transitionLayer=NULL;
            uint16_t XORop;
            if(terrain.outsideBorder)
            {
                transitionLayer=terrainLayer;
                XORop=0x01ff;
            }
            else
            {
                const unsigned int index=LoadMap::searchTileIndexByName(tiledMap,terrain.tmp_layerString);
                Tiled::Layer * rawLayer=tiledMap.layerAt(index+1);
                if(!rawLayer->isTileLayer())
                {
                    std::cerr << "Next layer after " << terrain.tmp_layerString.toStdString() << "is not tile layer (abort)" << std::endl;
                    abort();
                }
                transitionLayer=static_cast<Tiled::TileLayer *>(rawLayer);
                transitionLayerMask=transitionLayer;
                XORop=0x0000;
            }
            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    const bool innerBool=!XORop && (terrainLayer->cellAt(x,y).tile()==terrain.tile);
                    const unsigned int &bitMask=x+y*w;
                    const bool outerBool=XORop && terrainLayer->cellAt(x,y).tile()!=terrain.tile && !(MapBrush::mapMask[bitMask/8] & (1<<(7-bitMask%8)));
                    if(
                            //inner
                            innerBool ||
                            //outer
                            outerBool
                            )
                    {
                        //check the near tile and determine what transition use
                        const uint16_t to_type_match=layerMask(terrainLayer,x,y,terrain.tile,XORop);
                        if(to_type_match>512)
                            abort();

                        if(to_type_match!=0 && to_type_match!=16)
                        {
                            unsigned int indexTile=0;
                            if(XORop)
                            {
                                const uint16_t to_type_match_collision=layerMask(colisionLayerMask,x,y,NULL,false);
                                const bool onGrass=transitionLayerMask->cellAt(x,y).tile()!=NULL;
                                bool forceDisplay=false;
                                //outer border
                                if(to_type_match&2)
                                {
                                    if(to_type_match&8)
                                    {
                                        indexTile=8;
                                    }
                                    else if(to_type_match&32)
                                    {
                                        indexTile=9;
                                    }
                                    else
                                    {
                                        indexTile=5;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                    }
                                }
                                else if(to_type_match&128)
                                {
                                    if(to_type_match&8)
                                    {
                                        indexTile=11;
                                    }
                                    else if(to_type_match&32)
                                    {
                                        indexTile=10;
                                    }
                                    else
                                    {
                                        indexTile=1;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                    }
                                }
                                else if(to_type_match&8)
                                {
                                    indexTile=3;
                                    if(to_type_match_collision&2 && to_type_match_collision&128)
                                        forceDisplay=true;
                                }
                                else if(to_type_match&32)
                                {
                                    indexTile=7;
                                    if(to_type_match_collision&2 && to_type_match_collision&128)
                                        forceDisplay=true;
                                }
                                else if(to_type_match&256)
                                {
                                    indexTile=0;
                                    if(!(to_type_match_collision&256))
                                    {
                                        if(to_type_match_collision&32 && to_type_match_collision&128)
                                            forceDisplay=true;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                        if(to_type_match_collision&2 && to_type_match_collision&128)
                                            forceDisplay=true;
                                    }
                                }
                                else if(to_type_match&64)
                                {
                                    indexTile=2;
                                    if(!(to_type_match_collision&64))
                                    {
                                        if(to_type_match_collision&8 && to_type_match_collision&128)
                                            forceDisplay=true;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                        if(to_type_match_collision&2 && to_type_match_collision&128)
                                            forceDisplay=true;
                                    }
                                }
                                else if(to_type_match&1)
                                {
                                    indexTile=4;
                                    if(!(to_type_match_collision&1))
                                    {
                                        if(to_type_match_collision&2 && to_type_match_collision&8)
                                            forceDisplay=true;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                        if(to_type_match_collision&2 && to_type_match_collision&128)
                                            forceDisplay=true;
                                    }
                                }
                                else if(to_type_match&4)
                                {
                                    indexTile=6;
                                    if(!(to_type_match_collision&4))
                                    {
                                        if(to_type_match_collision&2 && to_type_match_collision&32)
                                            forceDisplay=true;
                                        if(to_type_match_collision&8 && to_type_match_collision&32)
                                            forceDisplay=true;
                                        if(to_type_match_collision&2 && to_type_match_collision&128)
                                            forceDisplay=true;
                                    }
                                }

                                if(!onGrass || forceDisplay)
                                {
                                    Tiled::Cell cell;
                                    cell.setTile(terrain.transition_tile.at(indexTile));
                                    cell.setFlippedHorizontally(false);
                                    cell.setFlippedVertically(false);
                                    cell.setFlippedAntiDiagonally(false);
                                    transitionLayer->setCell(x,y,cell);
                                }
                            }
                            else
                            {
                                //inner border
                                if(to_type_match&2)
                                {
                                    if(to_type_match&8)
                                        indexTile=0;
                                    else if(to_type_match&32)
                                        indexTile=2;
                                    else
                                        indexTile=1;
                                }
                                else if(to_type_match&128)
                                {
                                    if(to_type_match&8)
                                        indexTile=6;
                                    else if(to_type_match&32)
                                        indexTile=4;
                                    else
                                        indexTile=5;
                                }
                                else if(to_type_match&8)
                                    indexTile=7;
                                else if(to_type_match&32)
                                    indexTile=3;
                                else if(to_type_match&256)
                                    indexTile=8;
                                else if(to_type_match&64)
                                    indexTile=9;
                                else if(to_type_match&1)
                                    indexTile=10;
                                else if(to_type_match&4)
                                    indexTile=11;

                                Tiled::Cell cell;
                                cell.setTile(terrain.transition_tile.at(indexTile));
                                cell.setFlippedHorizontally(false);
                                cell.setFlippedVertically(false);
                                cell.setFlippedAntiDiagonally(false);
                                transitionLayer->setCell(x,y,cell);
                            }
                        }
                    }
                    x++;
                }
                y++;
            }
            index++;
        }
    }
}


void TransitionTerrain::mergeDown(Tiled::Map &tiledMap)
{
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();

    std::vector<Tiled::TileLayer *> layerToDelete;
    unsigned int terrainIndex=0;
    while(terrainIndex<(unsigned int)LoadMap::terrainFlatList.size())
    {
        const QString &terrainName=LoadMap::terrainFlatList.at(terrainIndex);
        LoadMap::Terrain * terrain=LoadMap::terrainNameToObject.value(terrainName);
        if(terrain->outsideBorder)
        {
            Tiled::TileLayer * const transitionLayerMask=LoadMap::searchTileLayerByName(tiledMap,"[T]"+terrainName);
            layerToDelete.push_back(transitionLayerMask);

            //search the layer
            int tileLayerIndex=0;
            while(true)
            {
                if(tiledMap.layerCount()<tileLayerIndex)
                {
                    std::cerr << "tiledMap.layerCount()<tileLayerIndexTemp (abort)" << std::endl;
                    abort();
                }
                Tiled::Layer * tempLayer=tiledMap.layerAt(tileLayerIndex);
                if(tempLayer->isTileLayer() &&
                        //tempLayer==transitionLayerMask
                        tempLayer->name()==terrain->tmp_layerString
                        )
                    break;
                else
                    tileLayerIndex++;
            }

            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    const Tiled::Cell &cell=transitionLayerMask->cellAt(x,y);
                    if(cell.tile()!=NULL)
                    {
                        int tileLayerIndexTemp=tileLayerIndex;
                        while(true)
                        {
                            if(tiledMap.layerCount()<tileLayerIndexTemp)
                            {
                                std::cerr << "tiledMap.layerCount()<tileLayerIndexTemp (abort)" << std::endl;
                                abort();
                            }
                            Tiled::Layer * tempLayer=tiledMap.layerAt(tileLayerIndexTemp);
                            if(!tempLayer->isTileLayer())
                            {
                                std::cerr << "!tempLayer->isTileLayer() (abort)" << std::endl;
                                abort();
                            }
                            if(static_cast<Tiled::TileLayer *>(tempLayer)->cellAt(x,y).tile()!=NULL)
                            {
                                if(cell.tile()!=terrain->tile)
                                    tileLayerIndexTemp++;
                                else
                                {
                                    tileLayerIndexTemp++;
                                    //clean the upper layer
                                    while(tileLayerIndexTemp<(tileLayerIndex+3))
                                    {
                                        Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(tiledMap.layerAt(tileLayerIndexTemp));
                                        Tiled::Cell cell;
                                        cell.setTile(nullptr);
                                        cell.setFlippedHorizontally(false);
                                        cell.setFlippedVertically(false);
                                        cell.setFlippedAntiDiagonally(false);
                                        tileLayer->setCell(x,y,cell);
                                        tileLayerIndexTemp++;
                                    }
                                    tileLayerIndexTemp=tileLayerIndex;
                                    break;
                                }
                            }
                            else
                                break;
                        }
                        Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(tiledMap.layerAt(tileLayerIndexTemp));
                        tileLayer->setCell(x,y,cell);
                    }
                    x++;
                }
                y++;
            }
        }
        terrainIndex++;
    }
    unsigned int index=0;
    while(index<layerToDelete.size())
    {
        Tiled::TileLayer * layer=layerToDelete.at(index);
        //layer->setVisible(false);

        // FIX Libtiled v1.8.0 - Tiled::Map.indexOfLayer() no longer exist
        //const int indexOfLayer=tiledMap.indexOfLayer(layer->name());
        const int indexOfLayer = findLayerIndexByName(&tiledMap, layer->name());

        if(indexOfLayer>=0)
            delete tiledMap.takeLayerAt(indexOfLayer);
        index++;
    }
}

void TransitionTerrain::addTransitionGroupOnMap(Tiled::Map &tiledMap)
{
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();

    //list the layer name to parse
    QHash<QString,LoadMap::Terrain> transition;
    QStringList transitionList;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            if(!terrain.tmp_layerString.isEmpty())
            {
                if(!transitionList.contains(terrain.terrainName))
                {
                    transition[terrain.terrainName]=terrain;
                    transitionList << terrain.terrainName;
                }
            }
        }

    //outer border
    {
        unsigned int index=0;
        while(index<(unsigned int)LoadMap::groupedTerrainList.size())
        {
            const LoadMap::GroupedTerrain &groupedTerrain=LoadMap::groupedTerrainList.at(index);
            Tiled::TileLayer * transitionLayer=groupedTerrain.tileLayer;
            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+y*w].index);
                    const unsigned int &bitMask=x+y*w;
                    if(!(MapBrush::mapMask[bitMask/8] & (1<<(7-bitMask%8))))
                    {
                        if(zone.height<groupedTerrain.height)
                        {
                            //check the near tile and determine what transition use
                            uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                            if(x>0 && y>0)
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x-1+(y-1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=1;
                            }
                            if(y>0)
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+(y-1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=2;
                            }
                            if(x<(w-1) && y>0)
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+1+(y-1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=4;
                            }
                            if(x>0)
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x-1+y*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=8;
                            }
                            if(x<(w-1))
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+1+y*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=16;
                            }
                            if(x>0 && y<(h-1))
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x-1+(y+1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=32;
                            }
                            if(y<(h-1))
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+(y+1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=64;
                            }
                            if(x<(w-1) && y<(h-1))
                            {
                                const vd::PolygonZone &zone=vd::voronoiMap.zones.at(vd::voronoiMap.tileToPolygonZoneIndex[x+1+(y+1)*w].index);
                                if(zone.height>=groupedTerrain.height)
                                    to_type_match|=128;
                            }

                            if(to_type_match!=0)
                            {
                                unsigned int indexTile=0;

                                if(to_type_match&2)
                                {
                                    if(to_type_match&8)
                                        indexTile=8;
                                    else if(to_type_match&16)
                                        indexTile=9;
                                    else
                                        indexTile=5;
                                }
                                else if(to_type_match&64)
                                {
                                    if(to_type_match&8)
                                        indexTile=11;
                                    else if(to_type_match&16)
                                        indexTile=10;
                                    else
                                        indexTile=1;
                                }
                                else if(to_type_match&8)
                                    indexTile=3;
                                else if(to_type_match&16)
                                    indexTile=7;
                                else if(to_type_match&128)
                                    indexTile=0;
                                else if(to_type_match&32)
                                    indexTile=2;
                                else if(to_type_match&1)
                                    indexTile=4;
                                else if(to_type_match&4)
                                    indexTile=6;

                                Tiled::Cell cell;
                                cell.setTile(groupedTerrain.transition_tile.at(indexTile));
                                cell.setFlippedHorizontally(false);
                                cell.setFlippedVertically(false);
                                cell.setFlippedAntiDiagonally(false);
                                transitionLayer->setCell(x,y,cell);
                            }
                        }
                    }
                    x++;
                }
                y++;
            }
            index++;
        }
    }
}

void TransitionTerrain::changeTileLayerOrder(Tiled::Map &tiledMap)
{
     // FIX Libtiled v1.8.0 - Tiled::Map.indexOfLayer() no longer exist
    //const int indexOfLayerWalkable=tiledMap.indexOfLayer("Walkable");
    const int indexOfLayerWalkable = findLayerIndexByName(&tiledMap, "Walkable");
    if(indexOfLayerWalkable<0)
        return;

     // FIX Libtiled v1.8.0 - Tiled::Map.indexOfLayer() no longer exist
    //const int indexOfLayerGrass=tiledMap.indexOfLayer("Grass");
    const int indexOfLayerGrass = findLayerIndexByName(&tiledMap, "Grass");
    if(indexOfLayerGrass<0)
        return;

    Tiled::Layer *layerZoneGrass=tiledMap.takeLayerAt(indexOfLayerGrass);
    tiledMap.insertLayer(indexOfLayerWalkable+1,layerZoneGrass);
}
