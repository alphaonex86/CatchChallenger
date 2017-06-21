#include "TransitionTerrain.h"

void TransitionTerrain::addTransitionOnMap(Tiled::Map &tiledMap)
{
    const unsigned int w=tiledMap.width();
    const unsigned int h=tiledMap.height();

    //list the layer name to parse
    QHash<QString,LoadMap::Terrain> transition;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            if(!terrain.tmp_layerString.isEmpty())
                transition[terrain.terrainName]=terrain;
        }

    {
        QHashIterator<QString,LoadMap::Terrain> i(transition);
        while (i.hasNext()) {
            i.next();
            Tiled::TileLayer * const transitionLayerMask=LoadMap::searchTileLayerByName(tiledMap,"OnGrass");
            const LoadMap::Terrain &terrain=i.value();
            Tiled::TileLayer * terrainLayer=terrain.tileLayer;
            Tiled::TileLayer * transitionLayer=NULL;
            bool XORop;
            if(terrain.outsideBorder)
            {
                transitionLayer=terrainLayer;
                XORop=true;
            }
            else
            {
                transitionLayer=LoadMap::searchTileLayerByName(tiledMap,"OnGrass");
                XORop=false;
            }
            unsigned int y=0;
            while(y<h)
            {
                unsigned int x=0;
                while(x<w)
                {
                    if((terrainLayer->cellAt(x,y).tile==terrain.tile) ^ XORop && (!XORop || transitionLayerMask->cellAt(x,y).tile==NULL))
                    {
                        //check the near tile and determine what transition use
                        uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                        if(x>0 && y>0)
                        {
                            if((terrainLayer->cellAt(x-1,y-1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=1;
                        }
                        if(y>0)
                        {
                            if((terrainLayer->cellAt(x,y-1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=2;
                        }
                        if(x<(w-1) && y>0)
                        {
                            if((terrainLayer->cellAt(x+1,y-1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=4;
                        }
                        if(x>0)
                        {
                            if((terrainLayer->cellAt(x-1,y).tile!=terrain.tile) ^ XORop)
                                to_type_match|=8;
                        }
                        /*if(the center tile)
                            to_type_match|=X;*/
                        if(x<(w-1))
                        {
                            if((terrainLayer->cellAt(x+1,y).tile!=terrain.tile) ^ XORop)
                                to_type_match|=16;
                        }
                        if(x>0 && y<(h-1))
                        {
                            if((terrainLayer->cellAt(x-1,y+1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=32;
                        }
                        if(y<(h-1))
                        {
                            if((terrainLayer->cellAt(x,y+1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=64;
                        }
                        if(x<(w-1) && y<(h-1))
                        {
                            if((terrainLayer->cellAt(x+1,y+1).tile!=terrain.tile) ^ XORop)
                                to_type_match|=128;
                        }

                        if(to_type_match!=0)
                        {
                            unsigned int indexTile=0;
                            if(to_type_match!=0)
                            {
                                if(XORop)
                                {
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
                                }
                                else
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
                            }

                            Tiled::Cell cell;
                            cell.tile=terrain.transition_tile.at(indexTile);
                            cell.flippedHorizontally=false;
                            cell.flippedVertically=false;
                            cell.flippedAntiDiagonally=false;
                            transitionLayer->setCell(x,y,cell);
                        }
                    }
                    x++;
                }
                y++;
            }
        }
    }
}
