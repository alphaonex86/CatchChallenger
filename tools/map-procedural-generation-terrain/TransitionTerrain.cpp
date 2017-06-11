#include "TransitionTerrain.h"

void TransitionTerrain::addTransitionOnMap(Tiled::Map &tiledMap,const std::vector<LoadMap::TerrainTransition> &terrainTransitionList)
{
    struct BufferRemplace
    {
        unsigned int x,y;
        Tiled::Tile * tile;
    };
    std::map<Tiled::TileLayer *,std::vector<BufferRemplace> > bufferRemplace;

    /*Tiled::Layer * walkableLayer=searchTileLayerByName(tiledMap,"Walkable");
    Tiled::Layer * waterLayer=searchTileLayerByName(tiledMap,"Water");*/
    Tiled::TileLayer * transitionLayer=LoadMap::searchTileLayerByName(tiledMap,"OnGrass");
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
                if(terrainTransition.from_type_layer->cellAt(x,y).tile==terrainTransition.from_type_tile)
                {
                    Tiled::Tile * transitionTileToTypeTmp=NULL;
                    Tiled::Tile * transitionTileToType=NULL;
                    //check the near tile and determine what transition use
                    uint8_t to_type_match=0;//9 bits used-1=8bit, the center bit is the current tile
                    if(x>0 && y>0)
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x-1,y-1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=1;
                        }
                    }
                    if(y>0)
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x,y-1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=2;
                        }
                    }
                    if(x<(w-1) && y>0)
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x+1,y-1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=4;
                        }
                    }
                    if(x>0)
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x-1,y,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=8;
                        }
                    }
                    /*if(the center tile)
                        to_type_match|=X;*/
                    if(x<(w-1))
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x+1,y,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=16;
                        }
                    }
                    if(x>0 && y<(h-1))
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x-1,y+1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=32;
                        }
                    }
                    if(y<(h-1))
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x,y+1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=64;
                        }
                    }
                    if(x<(w-1) && y<(h-1))
                    {
                        transitionTileToTypeTmp=LoadMap::haveTileAtReturnTileUniqueLayer(x+1,y+1,terrainTransition.to_type_layer,terrainTransition.to_type_tile);
                        if(transitionTileToTypeTmp!=NULL)
                        {
                            transitionTileToType=transitionTileToTypeTmp;
                            to_type_match|=128;
                        }
                    }

                    if(to_type_match!=0)
                    {
                        //remplace the tile
                        Tiled::Cell cellReplace;
                        cellReplace.tile=NULL;
                        cellReplace.flippedHorizontally=false;
                        cellReplace.flippedVertically=false;
                        cellReplace.flippedAntiDiagonally=false;
                        Tiled::Cell cellOver;
                        cellOver.tile=NULL;
                        cellOver.flippedHorizontally=false;
                        cellOver.flippedVertically=false;
                        cellOver.flippedAntiDiagonally=false;
                        Tiled::Cell cellCollision;
                        cellCollision.tile=NULL;
                        cellCollision.flippedHorizontally=false;
                        cellCollision.flippedVertically=false;
                        cellCollision.flippedAntiDiagonally=false;

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

                        cellOver.tile=terrainTransition.transition_tile.at(indexTile);
                        cellCollision.tile=terrainTransition.collision_tile.at(indexTile);
                        cellReplace.tile=transitionTileToType;

                        if(!terrainTransition.replace_tile)
                        {
                            Tiled::Cell cell;
                            cell.tile=cellOver.tile;
                            cell.flippedHorizontally=false;
                            cell.flippedVertically=false;
                            cell.flippedAntiDiagonally=false;
                            transitionLayer->setCell(x,y,cell);
                        }
                        else
                        {
                            //current tile
                            {
                                BufferRemplace bufferRemplaceUnit;
                                bufferRemplaceUnit.x=x;
                                bufferRemplaceUnit.y=y;
                                bufferRemplaceUnit.tile=transitionTileToType;
                                bufferRemplace[terrainTransition.from_type_layer].push_back(bufferRemplaceUnit);
                            }
                            //over layer
                            {
                                Tiled::Cell cell;
                                cell.tile=cellOver.tile;
                                cell.flippedHorizontally=false;
                                cell.flippedVertically=false;
                                cell.flippedAntiDiagonally=false;
                                transitionLayer->setCell(x,y,cell);
                            }
                        }
                        //collision
                        if(cellCollision.tile!=NULL)
                        {
                            Tiled::Cell cell;
                            cell.tile=cellCollision.tile;
                            cell.flippedHorizontally=false;
                            cell.flippedVertically=false;
                            cell.flippedAntiDiagonally=false;
                            collisionsLayer->setCell(x,y,cell);
                        }
                    }
                    break;
                }
                terrainTransitionIndex++;
            }
            x++;
        }
        y++;
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
