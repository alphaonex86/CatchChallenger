#include "TransitionTerrain.h"

void TransitionTerrain::addTransitionOnMap(Tiled::Map &tiledMap)
{
    struct BufferRemplace
    {
        unsigned int x,y;
        Tiled::Tile * tile;
    };
    std::map<Tiled::TileLayer *,std::vector<BufferRemplace> > bufferRemplace;

    /*Tiled::Layer * walkableLayer=searchTileLayerByName(tiledMap,"Walkable");
    Tiled::Layer * waterLayer=searchTileLayerByName(tiledMap,"Water");*/
    Tiled::TileLayer * collisionsLayer=LoadMap::searchTileLayerByName(tiledMap,"Collisions");
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();

    //list the layer name to parse
    QHash<QString,LoadMap::Terrain> innerTransition;
    QHash<QString,LoadMap::Terrain> outsideTransition;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            if(!terrain.tmp_layerString.isEmpty())
            {
                if(terrain.outsideBorder)
                    outsideTransition["[T]"+terrain.terrainName]=terrain;
                else
                    innerTransition[terrain.tmp_layerString]=terrain;
            }
        }

    //innerTerrain
    {
        Tiled::TileLayer * const transitionLayer=LoadMap::searchTileLayerByName(tiledMap,"OnGrass");
        QHashIterator<QString,LoadMap::Terrain> i(innerTransition);
        while (i.hasNext()) {
            i.next();
            const LoadMap::Terrain &terrain=i.value();
            const Tiled::TileLayer * const terrainLayer=terrain.tileLayer;
            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    //check the near tile and determine what transition use
                    uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                    if(x>0 && y>0)
                    {
                        if(terrainLayer->cellAt(x-1,y-1).tile!=terrain.tile)
                            to_type_match|=1;
                    }
                    if(y>0)
                    {
                        if(terrainLayer->cellAt(x,y-1).tile!=terrain.tile)
                            to_type_match|=2;
                    }
                    if(x<(w-1) && y>0)
                    {
                        if(terrainLayer->cellAt(x+1,y-1).tile!=terrain.tile)
                            to_type_match|=4;
                    }
                    if(x>0)
                    {
                        if(terrainLayer->cellAt(x-1,y).tile!=terrain.tile)
                            to_type_match|=8;
                    }
                    /*if(the center tile)
                        to_type_match|=X;*/
                    if(x<(w-1))
                    {
                        if(terrainLayer->cellAt(x+1,y).tile!=terrain.tile)
                            to_type_match|=16;
                    }
                    if(x>0 && y<(h-1))
                    {
                        if(terrainLayer->cellAt(x-1,y+1).tile!=terrain.tile)
                            to_type_match|=32;
                    }
                    if(y<(h-1))
                    {
                        if(terrainLayer->cellAt(x,y+1).tile!=terrain.tile)
                            to_type_match|=64;
                    }
                    if(x<(w-1) && y<(h-1))
                    {
                        if(terrainLayer->cellAt(x+1,y+1).tile!=terrain.tile)
                            to_type_match|=128;
                    }

                    if(to_type_match!=0)
                    {
                        unsigned int indexTile=0;
                        if(to_type_match!=0)
                        {
                            if(to_type_match&2)
                            {
                                if(to_type_match&8)
                                    indexTile=0;
                                else if(to_type_match&16)
                                    indexTile=2;
                                else
                                    indexTile=1;
                            }
                            else if(to_type_match&64)
                            {
                                if(to_type_match&8)
                                    indexTile=6;
                                else if(to_type_match&16)
                                    indexTile=4;
                                else
                                    indexTile=5;
                            }
                            else if(to_type_match&8)
                                indexTile=7;
                            else if(to_type_match&16)
                                indexTile=3;
                            else if(to_type_match&128)
                                indexTile=8;
                            else if(to_type_match&32)
                                indexTile=9;
                            else if(to_type_match&1)
                                indexTile=10;
                            else if(to_type_match&4)
                                indexTile=11;
                        }

                        Tiled::Cell cell;
                        cell.tile=terrain.transition_tile.at(indexTile);
                        cell.flippedHorizontally=false;
                        cell.flippedVertically=false;
                        cell.flippedAntiDiagonally=false;
                        transitionLayer->setCell(x,y,cell);
                    }
                    x++;
                }
                y++;
            }
        }
    }
    //outsideTerrain
    {
        QHashIterator<QString,LoadMap::Terrain> i(innerTransition);
        while (i.hasNext()) {
            i.next();
            const LoadMap::Terrain &terrain=i.value();
            Tiled::TileLayer * const transitionLayer=terrain.tileLayer;
            const Tiled::TileLayer * const terrainLayer=transitionLayer;
            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    //check the near tile and determine what transition use
                    uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                    if(x>0 && y>0)
                    {
                        if(terrainLayer->cellAt(x-1,y-1).tile!=terrain.tile)
                            to_type_match|=1;
                    }
                    if(y>0)
                    {
                        if(terrainLayer->cellAt(x,y-1).tile!=terrain.tile)
                            to_type_match|=2;
                    }
                    if(x<(w-1) && y>0)
                    {
                        if(terrainLayer->cellAt(x+1,y-1).tile!=terrain.tile)
                            to_type_match|=4;
                    }
                    if(x>0)
                    {
                        if(terrainLayer->cellAt(x-1,y).tile!=terrain.tile)
                            to_type_match|=8;
                    }
                    /*if(the center tile)
                        to_type_match|=X;*/
                    if(x<(w-1))
                    {
                        if(terrainLayer->cellAt(x+1,y).tile!=terrain.tile)
                            to_type_match|=16;
                    }
                    if(x>0 && y<(h-1))
                    {
                        if(terrainLayer->cellAt(x-1,y+1).tile!=terrain.tile)
                            to_type_match|=32;
                    }
                    if(y<(h-1))
                    {
                        if(terrainLayer->cellAt(x,y+1).tile!=terrain.tile)
                            to_type_match|=64;
                    }
                    if(x<(w-1) && y<(h-1))
                    {
                        if(terrainLayer->cellAt(x+1,y+1).tile!=terrain.tile)
                            to_type_match|=128;
                    }

                    if(to_type_match!=0)
                    {
                        unsigned int indexTile=0;
                        if(to_type_match!=0)
                        {
                            if(to_type_match&2)
                            {
                                if(to_type_match&8)
                                    indexTile=0;
                                else if(to_type_match&16)
                                    indexTile=2;
                                else
                                    indexTile=1;
                            }
                            else if(to_type_match&64)
                            {
                                if(to_type_match&8)
                                    indexTile=6;
                                else if(to_type_match&16)
                                    indexTile=4;
                                else
                                    indexTile=5;
                            }
                            else if(to_type_match&8)
                                indexTile=7;
                            else if(to_type_match&16)
                                indexTile=3;
                            else if(to_type_match&128)
                                indexTile=8;
                            else if(to_type_match&32)
                                indexTile=9;
                            else if(to_type_match&1)
                                indexTile=10;
                            else if(to_type_match&4)
                                indexTile=11;
                        }

                        Tiled::Cell cell;
                        cell.tile=terrain.transition_tile.at(indexTile);
                        cell.flippedHorizontally=false;
                        cell.flippedVertically=false;
                        cell.flippedAntiDiagonally=false;
                        transitionLayer->setCell(x,y,cell);
                    }
                    x++;
                }
                y++;
            }
        }
    }

    //flush the replace buffer
    for (auto it = bufferRemplace.begin(); it != bufferRemplace.cend(); ++it) {
        Tiled::TileLayer * layer=it->first;
        const std::vector<BufferRemplace> &entries=it->second;
        unsigned int index=0;
        while(index<entries.size())
        {
            const BufferRemplace &bufferRemplace=entries.at(index);
            Tiled::Cell cell;
            cell.tile=bufferRemplace.tile;
            cell.flippedHorizontally=false;
            cell.flippedVertically=false;
            cell.flippedAntiDiagonally=false;
            layer->setCell(bufferRemplace.x,bufferRemplace.y,cell);
            index++;
        }
      }
}
