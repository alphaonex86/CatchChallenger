#include "LoadMapAll.h"

#include <libtiled/mapobject.h>
#include <libtiled/objectgroup.h>
#include "../../general/base/cpp11addition.hpp"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"
#include "../map-procedural-generation-terrain/MapPlants.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <QFile>
#include <QXmlStreamReader>

std::vector<LoadMapAll::City> LoadMapAll::cities;
std::unordered_map<uint16_t,std::unordered_map<uint16_t,unsigned int> > LoadMapAll::citiesCoordToIndex;
uint8_t * LoadMapAll::mapPathDirection=NULL;
std::vector<LoadMapAll::Road> LoadMapAll::roads;
std::unordered_map<uint16_t,std::unordered_map<uint16_t,LoadMapAll::RoadIndex> > LoadMapAll::roadCoordToIndex;
std::unordered_map<std::string,LoadMapAll::Zone> LoadMapAll::zones;
std::vector<unsigned int> LoadMapAll::cityBuildingCount;
std::vector<unsigned int> LoadMapAll::cityBuildingArea;

void LoadMapAll::seedChunk(const unsigned int &seed, const unsigned int &chunkX, const unsigned int &chunkY,
                           const ChunkPass &pass)
{
    //Mix the four inputs, then avalanche: neighbouring chunks must not get
    //correlated streams (a plain seed+x+y*w would give chunk (1,0) and (0,1) the
    //same value on a square world, and consecutive chunks near-identical ones).
    //Cheap integer hash, no dependency, deterministic on every platform because
    //everything is unsigned 32 bit.
    uint32_t h=(uint32_t)seed;
    h^=(uint32_t)chunkX*0x9E3779B1u;
    h^=(uint32_t)chunkY*0x85EBCA77u;
    h^=(uint32_t)pass*0xC2B2AE3Du;
    h^=h>>16;
    h*=0x7FEB352Du;
    h^=h>>15;
    h*=0x846CA68Bu;
    h^=h>>16;
    srand((unsigned int)h);
}

LoadMapAll::CityHole LoadMapAll::cityHole(const CityType &type,const unsigned int &mapWidth,const unsigned int &mapHeight,
                                          const SettingsAll::SettingsExtra &setting)
{
    const SettingsAll::SettingsExtra::CitySize &citySize=setting.citySize[(unsigned int)type];
    CityHole hole;
    //never eat the vegetation border ring, even at holePercent=100: a town whose
    //buildings touch the chunk border has no frame and reads as a field
    unsigned int maxWidth=0;
    unsigned int maxHeight=0;
    if(mapWidth>2*cityHoleBorderRing)
        maxWidth=mapWidth-2*cityHoleBorderRing;
    if(mapHeight>2*cityHoleBorderRing)
        maxHeight=mapHeight-2*cityHoleBorderRing;
    hole.width=mapWidth*citySize.holePercent/100;
    hole.height=mapHeight*citySize.holePercent/100;
    if(hole.width>maxWidth)
        hole.width=maxWidth;
    if(hole.height>maxHeight)
        hole.height=maxHeight;
    //centered, and on an EVEN offset so the hole aligns with the scale-2 grid the
    //placement works on (an odd offset made a footprint spill one cell further)
    hole.x=((mapWidth-hole.width)/2)&~1u;
    hole.y=((mapHeight-hole.height)/2)&~1u;
    return hole;
}

void LoadMapAll::maskCityHoles(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting)
{
    if(MapBrush::mapMask==NULL)
    {
        std::cerr << "maskCityHoles called before MapBrush::initialiseMapMask" << std::endl;
        return;
    }
    const unsigned int mapWidth=setting.mapWidth;
    const unsigned int mapHeight=setting.mapHeight;
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);
        const CityHole hole=cityHole(city.type,mapWidth,mapHeight,setting);
        unsigned int localY=0;
        while(localY<hole.height)
        {
            unsigned int localX=0;
            while(localX<hole.width)
            {
                const unsigned int tileX=city.x*mapWidth+hole.x+localX;
                const unsigned int tileY=city.y*mapHeight+hole.y+localY;
                if(tileX<worldWidth && tileY<worldHeight)
                {
                    const unsigned int bit=tileX+tileY*worldWidth;
                    MapBrush::mapMask[bit/8]|=(1<<(7-bit%8));
                }
                localX++;
            }
            localY++;
        }
        index++;
    }
}

void LoadMapAll::addDebugCityLimits(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting)
{
    Tiled::ObjectGroup * const layerCity=new Tiled::ObjectGroup("City",0,0);
    layerCity->setColor(QColor("#ff3860"));
    worldMap.addLayer(layerCity);
    const unsigned int mapWidth=worldMap.width()/setting.mapXCount;
    const unsigned int mapHeight=worldMap.height()/setting.mapYCount;
    const int tileWidth=worldMap.tileWidth();
    const int tileHeight=worldMap.tileHeight();
    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);
        const SettingsAll::SettingsExtra::CitySize &citySize=setting.citySize[(unsigned int)city.type];
        const CityHole hole=cityHole(city.type,mapWidth,mapHeight,setting);
        const char *sizeName="small";
        if(city.type==CityType_medium)
            sizeName="medium";
        else if(city.type==CityType_big)
            sizeName="big";
        const unsigned int holeArea=hole.width*hole.height;
        const unsigned int placedCount=(index<cityBuildingCount.size())?cityBuildingCount.at(index):0;
        const unsigned int placedArea=(index<cityBuildingArea.size())?cityBuildingArea.at(index):0;
        //ONE short line: name, size, level, element type, house style, chunk, the
        //hole it was laid out in, the density reached over the density limit, and
        //the buildings placed over the minimum asked for
        const QString label=QString::fromStdString(city.name)+
                " "+QString::fromLatin1(sizeName)+
                " lvl"+QString::number(city.level)+
                " "+QString::fromStdString(city.elementType.empty()?std::string("-"):city.elementType)+
                " "+QString::fromStdString(city.style.empty()?std::string("-"):city.style)+
                " chunk"+QString::number(city.x)+","+QString::number(city.y)+
                " hole"+QString::number(hole.width)+"x"+QString::number(hole.height)+
                " dens"+QString::number(holeArea==0?0:placedArea*100/holeArea)+"/"+QString::number(citySize.densityPercent)+"%"+
                " bld"+QString::number(placedCount)+"/"+QString::number(citySize.minBuilding);
        const int pixelX=(int)(city.x*mapWidth+hole.x)*tileWidth;
        const int pixelY=(int)(city.y*mapHeight+hole.y)*tileHeight;
        Tiled::MapObject * const objectPolygon=new Tiled::MapObject(label,"",QPointF(pixelX,pixelY),QSizeF(0.0,0.0));
        objectPolygon->setPolygon(QPolygonF(QRectF(0,0,hole.width*tileWidth,hole.height*tileHeight)));
        objectPolygon->setShape(Tiled::MapObject::Polygon);
        layerCity->addObject(objectPolygon);
        index++;
    }
    layerCity->setVisible(false);
}

//flood the walkable cells of ONE chunk and label each with its component index;
//returns the number of components. blocked cells keep the label 0xFFFF.
static const uint16_t walkNoComponent=0xFFFF;
static unsigned int floodChunkComponents(const std::vector<unsigned char> &blocked,
                                         const unsigned int width, const unsigned int height,
                                         std::vector<uint16_t> &component)
{
    component.assign(width*height,walkNoComponent);
    unsigned int componentCount=0;
    std::vector<unsigned int> queue;
    unsigned int startCell=0;
    while(startCell<width*height)
    {
        if(blocked.at(startCell)==0 && component.at(startCell)==walkNoComponent)
        {
            const uint16_t label=(uint16_t)componentCount;
            componentCount++;
            component[startCell]=label;
            queue.clear();
            queue.push_back(startCell);
            unsigned int queueIndex=0;
            while(queueIndex<queue.size())
            {
                const unsigned int cell=queue.at(queueIndex);
                queueIndex++;
                const unsigned int cellX=cell%width;
                const unsigned int cellY=cell/width;
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=(int)cellX+stepX[direction];
                    const int nextY=(int)cellY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)width && nextY<(int)height)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*width;
                        if(blocked.at(next)==0 && component.at(next)==walkNoComponent)
                        {
                            component[next]=label;
                            queue.push_back(next);
                        }
                    }
                    direction++;
                }
            }
        }
        startCell++;
    }
    return componentCount;
}

//Carve a walkable corridor from any cell of component `fromComponent` to the cell
//`toCell`, and open it for real on the map: the Collisions of the crossed cells
//are cleared, whatever hangs above the player with them, and the ground is filled
//with the tile the chunk already uses. Blocked cells cost far more than open ones,
//so the corridor follows the existing ground and only cuts the cliff where it has
//to — a narrow notch, not a trench. Returns false when even that is impossible.
//how many whole plants the repairs took out, reported with the walkability summary
static unsigned int carvedPlants=0;

static bool carveChunkCorridor(Tiled::Map &worldMap,const std::vector<Tiled::TileLayer*> &collisionLayers,
                               const std::vector<uint16_t> &component,const uint16_t fromComponent,
                               const unsigned int toCell,
                               const unsigned int chunkX,const unsigned int chunkY,
                               const unsigned int mapWidth,const unsigned int mapHeight,
                               const std::vector<unsigned char> *forbidden=NULL)
{
    Tiled::TileLayer * const walkLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    if(walkLayer==NULL)
        return false;
    std::vector<Tiled::TileLayer*> abovePlayerLayers;
    {
        unsigned int layerIndex=0;
        while(layerIndex<(unsigned int)worldMap.layerCount())
        {
            Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
            if(layer->isTileLayer() && layer->name()=="WalkBehind")
                abovePlayerLayers.push_back(static_cast<Tiled::TileLayer *>(layer));
            layerIndex++;
        }
    }
    const unsigned int x0=chunkX*mapWidth;
    const unsigned int y0=chunkY*mapHeight;
    //the ground this chunk stands on, to fill what the notch opens
    Tiled::Tile *groundTile=NULL;
    {
        std::map<Tiled::Tile*,unsigned int> tileCount;
        unsigned int cell=0;
        while(cell<mapWidth*mapHeight)
        {
            Tiled::Tile * const tile=walkLayer->cellAt(x0+cell%mapWidth,y0+cell/mapWidth).tile();
            if(tile!=NULL)
                tileCount[tile]++;
            cell++;
        }
        unsigned int bestCount=0;
        std::map<Tiled::Tile*,unsigned int>::const_iterator tileIterator=tileCount.cbegin();
        while(tileIterator!=tileCount.cend())
        {
            if(tileIterator->second>bestCount)
            {
                bestCount=tileIterator->second;
                groundTile=tileIterator->first;
            }
            ++tileIterator;
        }
    }
    //An open-sea chunk has NO walkable ground at all, so the repair used to give
    //up on exactly the chunks that needed it most. There the corridor is opened
    //as WATER, which the engine walks on just the same.
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::Tile *waterTile=NULL;
    if(groundTile==NULL)
    {
        int height=0;
        while(height<5 && waterTile==NULL)
        {
            int moisure=0;
            while(moisure<6 && waterTile==NULL)
            {
                const LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
                if(terrain.tile!=NULL && terrain.terrainName.compare(QString("water"),Qt::CaseInsensitive)==0)
                    waterTile=terrain.tile;
                moisure++;
            }
            height++;
        }
        if(waterTile==NULL || waterLayer==NULL)
            return false;
    }
    //cheapest path from toCell back to the target component (walking the search
    //backwards means the parent chain already points the right way)
    static const unsigned int costOpen=1;
    static const unsigned int costBlocked=60;
    static const unsigned int costUnreachable=0xFFFFFFFF;
    std::vector<unsigned int> cost(mapWidth*mapHeight,costUnreachable);
    std::vector<int> parent(mapWidth*mapHeight,-1);
    //plain Dijkstra with two buckets is overkill here; the grid is 44x44, a simple
    //repeated relaxation is small and obviously correct
    cost[toCell]=0;
    bool changed=true;
    while(changed)
    {
        changed=false;
        unsigned int cell=0;
        while(cell<mapWidth*mapHeight)
        {
            if(cost.at(cell)!=costUnreachable)
            {
                const int cellX=(int)(cell%mapWidth);
                const int cellY=(int)(cell/mapWidth);
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=cellX+stepX[direction];
                    const int nextY=cellY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)mapWidth && nextY<(int)mapHeight)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapWidth;
                        //THE SEA WALL IS NEVER OPENED. A repair that cut through it
                        //would hand the player the open sea, which is exactly what
                        //the rock is there to stop: the corridor goes round it.
                        //what the caller forbids, chunk-local: the ON FOOT repair
                        //of a boat quay forbids the water. Without it the corridor
                        //happily "opened" a way through the harbour — clearing
                        //nothing, filling nothing (water needs no ground), so the
                        //quay stayed walled off and the pass ate a tile of the boat.
                        bool wallCell=false;
                        if(forbidden!=NULL)
                            if(forbidden->at(next)!=0)
                                wallCell=true;
                        if(!wallCell && !LoadMapAll::seaWallCells.empty())
                        {
                            const unsigned int worldCell=(x0+(unsigned int)nextX)
                                    +(y0+(unsigned int)nextY)*(unsigned int)worldMap.width();
                            if(worldCell<LoadMapAll::seaWallCells.size())
                                wallCell=(LoadMapAll::seaWallCells.at(worldCell)!=0);
                        }
                        const unsigned int stepCost=(component.at(next)==walkNoComponent)?costBlocked:costOpen;
                        if(!wallCell && cost.at(cell)+stepCost<cost.at(next))
                        {
                            cost[next]=cost.at(cell)+stepCost;
                            parent[next]=(int)cell;
                            changed=true;
                        }
                    }
                    direction++;
                }
            }
            cell++;
        }
    }
    //the cheapest cell of the target component
    int bestCell=-1;
    {
        unsigned int cell=0;
        while(cell<mapWidth*mapHeight)
        {
            if(component.at(cell)==fromComponent && cost.at(cell)!=costUnreachable)
                if(bestCell<0 || cost.at(cell)<cost.at((unsigned int)bestCell))
                    bestCell=(int)cell;
            cell++;
        }
    }
    if(bestCell<0)
        return false;
    //walk the parent chain back to toCell, opening every cell on the way
    int walkCell=bestCell;
    while(walkCell>=0)
    {
        const unsigned int tileX=x0+(unsigned int)walkCell%mapWidth;
        const unsigned int tileY=y0+(unsigned int)walkCell/mapWidth;
        //A TREE GOES WHOLE OR NOT AT ALL. Clearing the one cell that blocked the
        //way took the trunk and left the canopy hanging in the air; the road
        //generator never shows that because it masks its way before anything
        //grows on it. Here the vegetation is already down, so the plant the cell
        //belongs to is removed entirely, every cell of it on every layer.
        if(MapPlants::removePlantAt(worldMap,tileX,tileY))
            carvedPlants++;
        unsigned int layerIndex=0;
        while(layerIndex<collisionLayers.size())
        {
            if(collisionLayers.at(layerIndex)->cellAt(tileX,tileY).tile()!=NULL)
            {
                collisionLayers.at(layerIndex)->setCell(tileX,tileY,Tiled::Cell());
                unsigned int aboveIndex=0;
                while(aboveIndex<abovePlayerLayers.size())
                {
                    abovePlayerLayers.at(aboveIndex)->setCell(tileX,tileY,Tiled::Cell());
                    aboveIndex++;
                }
            }
            layerIndex++;
        }
        //WATER IS ALREADY WALKABLE and needs no ground under it. Filling it put
        //two tiles of land against the moored boat of a sea chunk — the corridor
        //to the boat "door" ran over the water and paved it with the mountain of
        //the islet, the only walkable tile that chunk had.
        const bool onWater=(waterLayer!=NULL && waterLayer->cellAt(tileX,tileY).tile()!=NULL);
        if(!onWater)
        {
            if(groundTile!=NULL)
            {
                if(walkLayer->cellAt(tileX,tileY).tile()==NULL)
                    walkLayer->setCell(tileX,tileY,Tiled::Cell(groundTile));
            }
            else if(waterLayer!=NULL && waterTile!=NULL)
                waterLayer->setCell(tileX,tileY,Tiled::Cell(waterTile));
        }
        walkCell=parent.at((unsigned int)walkCell);
    }
    return true;
}

//A BORDER IS CROSSED ON A PAIR OF CELLS, one on each map. The engine keeps the
//neighbour plus an offset (Map_loaderMain.cpp) and the offset the generator writes
//is 0, so the player leaving row i comes out on row i of the other map: a row that
//is open here and a wall there is no crossing at all. Per-chunk walkability cannot
//see that — it only knows its own side — so the pair is opened here, on the world,
//before the per-chunk repair makes it reachable from inside.
//Returns how many pairs had to be opened. The sea wall is never one of them.
static unsigned int openBorderPairs(Tiled::Map &worldMap,
                                    const std::vector<Tiled::TileLayer*> &collisionLayers,
                                    const SettingsAll::SettingsExtra &setting)
{
    Tiled::TileLayer * const walkLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    if(walkLayer==NULL)
        return 0;
    std::vector<Tiled::TileLayer*> abovePlayerLayers;
    {
        unsigned int layerIndex=0;
        while(layerIndex<(unsigned int)worldMap.layerCount())
        {
            Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
            if(layer->isTileLayer() && layer->name()=="WalkBehind")
                abovePlayerLayers.push_back(static_cast<Tiled::TileLayer *>(layer));
            layerIndex++;
        }
    }
    const unsigned int mapWidth=setting.mapWidth;
    const unsigned int mapHeight=setting.mapHeight;
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    unsigned int opened=0;
    unsigned int chunkY=0;
    while(chunkY<setting.mapYCount)
    {
        unsigned int chunkX=0;
        while(chunkX<setting.mapXCount)
        {
            const uint8_t links=LoadMapAll::mapPathDirection[chunkX+chunkY*setting.mapXCount];
            //only the right and bottom links, so each pair is looked at once
            unsigned int direction=0;
            while(direction<2)
            {
                const bool toTheRight=(direction==0);
                const bool linked=toTheRight?((links&LoadMapAll::Orientation_right)!=0)
                                            :((links&LoadMapAll::Orientation_bottom)!=0);
                const unsigned int nextChunkX=chunkX+(toTheRight?1:0);
                const unsigned int nextChunkY=chunkY+(toTheRight?0:1);
                if(linked && nextChunkX<setting.mapXCount && nextChunkY<setting.mapYCount)
                {
                    const unsigned int lineLength=toTheRight?mapHeight:mapWidth;
                    int bestIndex=-1;
                    int bestScore=-1;
                    unsigned int step=0;
                    while(step<lineLength)
                    {
                        unsigned int hereX=0,hereY=0,thereX=0,thereY=0;
                        if(toTheRight)
                        {
                            hereX=chunkX*mapWidth+mapWidth-1;
                            hereY=chunkY*mapHeight+step;
                            thereX=nextChunkX*mapWidth;
                            thereY=nextChunkY*mapHeight+step;
                        }
                        else
                        {
                            hereX=chunkX*mapWidth+step;
                            hereY=chunkY*mapHeight+mapHeight-1;
                            thereX=nextChunkX*mapWidth+step;
                            thereY=nextChunkY*mapHeight;
                        }
                        bool hereBlocked=false;
                        bool thereBlocked=false;
                        unsigned int layerIndex=0;
                        while(layerIndex<collisionLayers.size())
                        {
                            if(collisionLayers.at(layerIndex)->cellAt(hereX,hereY).tile()!=NULL)
                                hereBlocked=true;
                            if(collisionLayers.at(layerIndex)->cellAt(thereX,thereY).tile()!=NULL)
                                thereBlocked=true;
                            layerIndex++;
                        }
                        //the sea wall is not a candidate: opening it is opening the
                        //open sea, which is the one thing it exists to stop
                        bool wall=false;
                        if(!LoadMapAll::seaWallCells.empty())
                            if(LoadMapAll::seaWallCells.at(hereX+hereY*worldWidth)!=0
                                    || LoadMapAll::seaWallCells.at(thereX+thereY*worldWidth)!=0)
                                wall=true;
                        if(!wall)
                        {
                            //an open pair wins outright; else the fewer walls to cut
                            //the better, and near the middle of the side
                            int score=1000;
                            if(hereBlocked)
                                score-=300;
                            if(thereBlocked)
                                score-=300;
                            score-=abs((int)step-(int)lineLength/2);
                            if(score>bestScore)
                            {
                                bestScore=score;
                                bestIndex=(int)step;
                            }
                            if(!hereBlocked && !thereBlocked)
                            {
                                bestIndex=-1;//nothing to do on this side
                                step=lineLength;
                            }
                        }
                        if(step<lineLength)
                            step++;
                    }
                    if(bestIndex>=0)
                    {
                        //open that one pair, on both maps
                        unsigned int side=0;
                        while(side<2)
                        {
                            unsigned int tileX=0,tileY=0;
                            if(toTheRight)
                            {
                                tileX=(side==0)?(chunkX*mapWidth+mapWidth-1):(nextChunkX*mapWidth);
                                tileY=((side==0)?chunkY:nextChunkY)*mapHeight+(unsigned int)bestIndex;
                            }
                            else
                            {
                                tileX=((side==0)?chunkX:nextChunkX)*mapWidth+(unsigned int)bestIndex;
                                tileY=(side==0)?(chunkY*mapHeight+mapHeight-1):(nextChunkY*mapHeight);
                            }
                            MapPlants::removePlantAt(worldMap,tileX,tileY);
                            unsigned int layerIndex=0;
                            while(layerIndex<collisionLayers.size())
                            {
                                collisionLayers.at(layerIndex)->setCell(tileX,tileY,Tiled::Cell());
                                layerIndex++;
                            }
                            unsigned int aboveIndex=0;
                            while(aboveIndex<abovePlayerLayers.size())
                            {
                                abovePlayerLayers.at(aboveIndex)->setCell(tileX,tileY,Tiled::Cell());
                                aboveIndex++;
                            }
                            //water needs no ground under it, the engine walks on it
                            const bool onWater=(waterLayer!=NULL
                                                && waterLayer->cellAt(tileX,tileY).tile()!=NULL);
                            if(!onWater && walkLayer->cellAt(tileX,tileY).tile()==NULL)
                            {
                                Tiled::Tile *groundTile=NULL;
                                if(tileX>0)
                                    groundTile=walkLayer->cellAt(tileX-1,tileY).tile();
                                if(groundTile==NULL && tileY>0)
                                    groundTile=walkLayer->cellAt(tileX,tileY-1).tile();
                                if(groundTile!=NULL)
                                    walkLayer->setCell(tileX,tileY,Tiled::Cell(groundTile));
                            }
                            side++;
                        }
                        opened++;
                    }
                }
                direction++;
            }
            chunkX++;
        }
        chunkY++;
    }
    return opened;
}

bool LoadMapAll::checkWalkability(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting,
                                  std::vector<std::string> &errors)
{
    //every layer named Collisions: the engine OR-merges them, so a cell is
    //blocked as soon as ONE of them holds a tile
    std::vector<Tiled::TileLayer*> collisionLayers;
    {
        unsigned int layerIndex=0;
        while(layerIndex<(unsigned int)worldMap.layerCount())
        {
            Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
            if(layer->isTileLayer() && layer->name()=="Collisions")
                collisionLayers.push_back(static_cast<Tiled::TileLayer *>(layer));
            layerIndex++;
        }
    }
    if(collisionLayers.empty())
    {
        errors.push_back("no Collisions layer in the world map, walkability cannot be checked");
        return false;
    }
    const unsigned int mapWidth=setting.mapWidth;
    const unsigned int mapHeight=setting.mapHeight;
    const int tileWidth=worldMap.tileWidth();
    const int tileHeight=worldMap.tileHeight();
    //the sea, for the ON FOOT rule of the boat quays below
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");

    //the cells the player crosses a border on, and the doorsteps of the
    //buildings, both read from the Moving group the generator itself filled
    //one entry per connected SIDE: every cell of that side of the chunk
    std::map<std::pair<unsigned int,unsigned int>,std::vector<std::vector<std::pair<unsigned int,unsigned int> > > > borderLines;
    std::map<std::pair<unsigned int,unsigned int>,std::vector<std::pair<unsigned int,unsigned int> > > doorCells;
    {
        Tiled::ObjectGroup * const movingGroup=LoadMap::searchObjectGroupByName(worldMap,"Moving");
        if(movingGroup==NULL)
        {
            errors.push_back("no Moving object group in the world map");
            return false;
        }
        const QList<Tiled::MapObject*> &objects=movingGroup->objects();
        unsigned int objectIndex=0;
        while(objectIndex<(unsigned int)objects.size())
        {
            const Tiled::MapObject * const object=objects.at(objectIndex);
            //Door / teleport / border objects are stored ON the cell the player
            //stands on (wireBuildingDoors and addMapChange both write
            //doorY*tileHeight). The -1 row offset is a BOT rule, not a general
            //one — applying it here pointed one row up, onto the collision door
            //tile above every doorstep, and reported 340 good doors as broken.
            int tileX=(int)(object->x()/tileWidth);
            int tileY=(int)(object->y()/tileHeight);
            const QString type=object->type();
            if(tileX>=0 && tileY>=0 && tileX<worldMap.width() && tileY<worldMap.height())
            {
                if(type.startsWith("border-"))
                {
                    //The engine reads a border-* object as the WHOLE SIDE of the
                    //map with an offset (Map_loaderMain.cpp), NOT as a teleport on
                    //the cell the object sits on: the player crosses wherever that
                    //side is walkable. So the whole line is recorded, and the rule
                    //below only asks that SOME cell of it is reachable.
                    //Every border object is written ONE ROW DOWN (addMapChange: the
                    //-1 object convention, which is what makes the offset 0), so
                    //the line of a TOP border is the row above the object, exactly
                    //like a bottom one. Reading the object row as the line made the
                    //guard check row 1 and repair row 1, while the player crosses
                    //on row 0 — a top border could be a wall from end to end and
                    //the guard reported nothing.
                    if(type=="border-bottom" || type=="border-top")
                        tileY--;
                    if(tileY>=0)
                    {
                        const std::pair<unsigned int,unsigned int> chunk((unsigned int)tileX/mapWidth,
                                                                        (unsigned int)tileY/mapHeight);
                        const unsigned int localX=(unsigned int)tileX%mapWidth;
                        const unsigned int localY=(unsigned int)tileY%mapHeight;
                        std::vector<std::pair<unsigned int,unsigned int> > line;
                        if(type=="border-left" || type=="border-right")
                        {
                            unsigned int step=0;
                            while(step<mapHeight)
                            {
                                line.push_back(std::pair<unsigned int,unsigned int>(localX,step));
                                step++;
                            }
                        }
                        else
                        {
                            unsigned int step=0;
                            while(step<mapWidth)
                            {
                                line.push_back(std::pair<unsigned int,unsigned int>(step,localY));
                                step++;
                            }
                        }
                        borderLines[chunk].push_back(line);
                    }
                }
                else if(type=="door" || type=="teleport on it" || type=="teleport on push")
                {
                    //THE BOAT OF A CROSSING IS THE ONE DOOR THAT IS A WALL: its
                    //push-teleport sits ON the ship, which the player never stands
                    //on — they push against it from the quay. What has to be
                    //reachable is that quay, and the sea pass recorded it, so the
                    //teleport object itself is skipped here.
                    bool boatTeleport=false;
                    {
                        std::map<std::pair<uint16_t,uint16_t>,Tiled::MapObject*>::const_iterator boatIterator=
                                boatTeleportObjects.cbegin();
                        while(boatIterator!=boatTeleportObjects.cend() && !boatTeleport)
                        {
                            if(boatIterator->second==object)
                                boatTeleport=true;
                            ++boatIterator;
                        }
                    }
                    if(!boatTeleport)
                    {
                        const std::pair<unsigned int,unsigned int> chunk((unsigned int)tileX/mapWidth,
                                                                        (unsigned int)tileY/mapHeight);
                        doorCells[chunk].push_back(std::pair<unsigned int,unsigned int>(
                                                       (unsigned int)tileX%mapWidth,(unsigned int)tileY%mapHeight));
                    }
                }
            }
            objectIndex++;
        }
    }

    //BEFORE anything else: a crossable pair of cells on every border link, which
    //is what the engine really needs; the per-chunk repair below then makes that
    //pair reachable from inside its own map.
    const unsigned int openedPairs=openBorderPairs(worldMap,collisionLayers,setting);

    std::vector<unsigned char> blocked(mapWidth*mapHeight,0);
    std::vector<uint16_t> component;
    unsigned int brokenBorders=0;
    unsigned int unreachableDoors=0;
    unsigned int repairedChunks=0;
    unsigned int chunkY=0;
    while(chunkY<setting.mapYCount)
    {
        unsigned int chunkX=0;
        while(chunkX<setting.mapXCount)
        {
            const std::pair<unsigned int,unsigned int> chunk(chunkX,chunkY);
            //only the chunks a map is written for
            if(mapPathDirection[chunkX+chunkY*setting.mapXCount]!=0)
            {
                //CHECK AND REPAIR: pass 0 carves a corridor through whatever cuts
                //the chunk in two (the zone growth gives up after its retry budget
                //and leaves a cliff across the road), pass 1 re-measures and only
                //then reports. A problem that survives the repair is a real one.
                unsigned int repairPass=0;
                bool repaired=false;
                while(repairPass<2)
                {
                    const bool reportPass=(repairPass==1);
                {
                    unsigned int localY=0;
                    while(localY<mapHeight)
                    {
                        unsigned int localX=0;
                        while(localX<mapWidth)
                        {
                            const unsigned int tileX=chunkX*mapWidth+localX;
                            const unsigned int tileY=chunkY*mapHeight+localY;
                            unsigned char cellBlocked=0;
                            unsigned int layerIndex=0;
                            while(layerIndex<collisionLayers.size())
                            {
                                if(collisionLayers.at(layerIndex)->cellAt(tileX,tileY).tile()!=NULL)
                                    cellBlocked=1;
                                layerIndex++;
                            }
                            blocked[localX+localY*mapWidth]=cellBlocked;
                            localX++;
                        }
                        localY++;
                    }
                }
                floodChunkComponents(blocked,mapWidth,mapHeight,component);
                //the BIGGEST walkable component: the one the town square, the road
                //and the borders live in
                uint16_t biggestComponent=walkNoComponent;
                {
                    std::map<uint16_t,unsigned int> componentSize;
                    unsigned int cell=0;
                    while(cell<mapWidth*mapHeight)
                    {
                        if(component.at(cell)!=walkNoComponent)
                            componentSize[component.at(cell)]++;
                        cell++;
                    }
                    unsigned int bestSize=0;
                    std::map<uint16_t,unsigned int>::const_iterator sizeIterator=componentSize.cbegin();
                    while(sizeIterator!=componentSize.cend())
                    {
                        if(sizeIterator->second>bestSize)
                        {
                            bestSize=sizeIterator->second;
                            biggestComponent=sizeIterator->first;
                        }
                        ++sizeIterator;
                    }
                }
                const std::string chunkName=chunkDebugName(chunkX,chunkY);
                //A CAVE chunk is the one case where the borders are MEANT to be
                //separated: that is what forces the player through the corridor
                //instead of walking around it. There the rule is per side — each
                //border opening must reach the cave mouth on its own side.
                const bool chunkIsCave=isCaveChunk(chunkX,chunkY);
                //1) the border SIDES. A side is crossable as soon as ONE of its
                //cells is walkable (the engine crosses on the whole side with an
                //offset), and the sides of a chunk must all reach one another.
                if(borderLines.find(chunk)!=borderLines.cend())
                {
                    const std::vector<std::vector<std::pair<unsigned int,unsigned int> > > &lines=borderLines.at(chunk);
                    //the components each side opens on
                    std::vector<std::set<uint16_t> > sideComponents;
                    unsigned int lineIndex=0;
                    while(lineIndex<lines.size())
                    {
                        std::set<uint16_t> reached;
                        unsigned int cellIndex=0;
                        while(cellIndex<lines.at(lineIndex).size())
                        {
                            const unsigned int cell=lines.at(lineIndex).at(cellIndex).first
                                    +lines.at(lineIndex).at(cellIndex).second*mapWidth;
                            if(component.at(cell)!=walkNoComponent)
                                reached.insert(component.at(cell));
                            cellIndex++;
                        }
                        sideComponents.push_back(reached);
                        lineIndex++;
                    }
                    //a side with nothing walkable at all cannot be crossed
                    lineIndex=0;
                    while(lineIndex<lines.size())
                    {
                        if(sideComponents.at(lineIndex).empty())
                        {
                            const std::pair<unsigned int,unsigned int> &middle=
                                    lines.at(lineIndex).at(lines.at(lineIndex).size()/2);
                            const unsigned int cell=middle.first+middle.second*mapWidth;
                            if(!reportPass && biggestComponent!=walkNoComponent)
                                repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                             biggestComponent,cell,chunkX,chunkY,
                                                             mapWidth,mapHeight);
                            else if(reportPass)
                            {
                                errors.push_back(chunkName+": the border side through "+
                                                 std::to_string(middle.first)+","+std::to_string(middle.second)+
                                                 " is a wall from end to end");
                                brokenBorders++;
                            }
                        }
                        lineIndex++;
                    }
                    if(chunkIsCave)
                    {
                        //a cave chunk is the one case where the sides are MEANT to
                        //be separated: each must reach the cave mouth on its own
                        //side, and nothing more
                        lineIndex=0;
                        while(lineIndex<lines.size())
                        {
                            if(!sideComponents.at(lineIndex).empty())
                            {
                                //In a CAVE chunk the side is the cliff: it is enough
                                //that ANY of the components it opens on reaches a
                                //mouth, the player walks along the approach to it.
                                const std::pair<unsigned int,unsigned int> &middleCell=
                                        lines.at(lineIndex).at(lines.at(lineIndex).size()/2);
                                bool reachesAMouth=false;
                                if(doorCells.find(chunk)!=doorCells.cend())
                                {
                                    const std::vector<std::pair<unsigned int,unsigned int> > &doors=doorCells.at(chunk);
                                    unsigned int doorIndex=0;
                                    while(doorIndex<doors.size())
                                    {
                                        const unsigned int doorCell=doors.at(doorIndex).first+doors.at(doorIndex).second*mapWidth;
                                        if(sideComponents.at(lineIndex).find(component.at(doorCell))
                                                !=sideComponents.at(lineIndex).cend())
                                            reachesAMouth=true;
                                        doorIndex++;
                                    }
                                }
                                if(!reachesAMouth)
                                {
                                    //join this side to its own mouth ONLY: joining
                                    //the two sides would let the player walk around
                                    //the cave, which is the point of the chunk
                                    int nearestMouth=-1;
                                    if(doorCells.find(chunk)!=doorCells.cend())
                                    {
                                        const std::vector<std::pair<unsigned int,unsigned int> > &doors=doorCells.at(chunk);
                                        unsigned int doorIndex=0;
                                        while(doorIndex<doors.size())
                                        {
                                            const unsigned int doorCell=doors.at(doorIndex).first
                                                    +doors.at(doorIndex).second*mapWidth;
                                            if(nearestMouth<0
                                                    || (abs((int)(doorCell%mapWidth)-(int)middleCell.first)
                                                        +abs((int)(doorCell/mapWidth)-(int)middleCell.second))
                                                       <(abs((int)((unsigned int)nearestMouth%mapWidth)-(int)middleCell.first)
                                                         +abs((int)((unsigned int)nearestMouth/mapWidth)-(int)middleCell.second)))
                                                nearestMouth=(int)doorCell;
                                            doorIndex++;
                                        }
                                    }
                                    if(!reportPass && nearestMouth>=0 && !sideComponents.at(lineIndex).empty())
                                        repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                                     *sideComponents.at(lineIndex).cbegin(),
                                                                     (unsigned int)nearestMouth,
                                                                     chunkX,chunkY,mapWidth,mapHeight);
                                    else if(reportPass)
                                    {
                                        const std::pair<unsigned int,unsigned int> &middle=
                                                lines.at(lineIndex).at(lines.at(lineIndex).size()/2);
                                        errors.push_back(chunkName+": the border side through "+
                                                         std::to_string(middle.first)+","+std::to_string(middle.second)+
                                                         " opens on a dead end, no cave mouth is reachable from it");
                                        brokenBorders++;
                                    }
                                }
                            }
                            lineIndex++;
                        }
                    }
                    else
                    {
                        //every side has to share a component with the first one
                        lineIndex=1;
                        while(lineIndex<lines.size())
                        {
                            bool shared=false;
                            std::set<uint16_t>::const_iterator componentIterator=sideComponents.at(lineIndex).cbegin();
                            while(componentIterator!=sideComponents.at(lineIndex).cend() && !shared)
                            {
                                if(sideComponents.at(0).find(*componentIterator)!=sideComponents.at(0).cend())
                                    shared=true;
                                ++componentIterator;
                            }
                            if(!shared && !sideComponents.at(lineIndex).empty() && !sideComponents.at(0).empty())
                            {
                                const std::pair<unsigned int,unsigned int> &middle=
                                        lines.at(lineIndex).at(lines.at(lineIndex).size()/2);
                                const unsigned int cell=middle.first+middle.second*mapWidth;
                                if(!reportPass)
                                    repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                                 *sideComponents.at(0).cbegin(),cell,
                                                                 chunkX,chunkY,mapWidth,mapHeight);
                                else
                                {
                                    errors.push_back(chunkName+": the border side through "+
                                                     std::to_string(middle.first)+","+std::to_string(middle.second)+
                                                     " cannot be walked to from the other sides of the chunk");
                                    brokenBorders++;
                                }
                            }
                            lineIndex++;
                        }
                    }
                }
                //2) every door of this chunk in the BIGGEST component. A cave chunk
                //is exempt: its mouths sit in the separate pockets of rule 1, which
                //already checked that each is reachable from its own border.
                if(!chunkIsCave && doorCells.find(chunk)!=doorCells.cend() && biggestComponent!=walkNoComponent)
                {
                    const std::vector<std::pair<unsigned int,unsigned int> > &cells=doorCells.at(chunk);
                    unsigned int cellIndex=0;
                    while(cellIndex<cells.size())
                    {
                        const unsigned int cell=cells.at(cellIndex).first+cells.at(cellIndex).second*mapWidth;
                        if(component.at(cell)!=biggestComponent)
                        {
                            if(!reportPass)
                                repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                             biggestComponent,cell,chunkX,chunkY,
                                                             mapWidth,mapHeight);
                            else
                            {
                                errors.push_back(chunkName+": the door at "+
                                                 std::to_string(cells.at(cellIndex).first)+","+
                                                 std::to_string(cells.at(cellIndex).second)+
                                                 (component.at(cell)==walkNoComponent
                                                  ? " stands on a collision"
                                                  : " is walled off from the rest of the chunk"));
                                unreachableDoors++;
                            }
                        }
                        cellIndex++;
                    }
                }
                //3) THE QUAY OF A MOORED BOAT, reached ON FOOT from a border of its
                //map. Rule 2 cannot answer this: a ferry map is mostly sea, so its
                //BIGGEST component is the water basin and a quay sitting in it
                //passes while the player — who takes the boat precisely because
                //they cannot swim — never reaches the shore at all. Here the water
                //is a wall, and the quay must share its ground with a border.
                if(boatLandingCells.find(std::pair<uint16_t,uint16_t>((uint16_t)chunkX,(uint16_t)chunkY))
                        !=boatLandingCells.cend()
                        && borderLines.find(chunk)!=borderLines.cend())
                {
                    std::vector<unsigned char> footBlocked(mapWidth*mapHeight,0);
                    std::vector<unsigned char> footWater(mapWidth*mapHeight,0);
                    {
                        unsigned int cell=0;
                        while(cell<mapWidth*mapHeight)
                        {
                            const unsigned int tileX=chunkX*mapWidth+cell%mapWidth;
                            const unsigned int tileY=chunkY*mapHeight+cell/mapWidth;
                            if(waterLayer!=NULL && waterLayer->cellAt(tileX,tileY).tile()!=NULL)
                                footWater[cell]=1;
                            if(blocked.at(cell)!=0 || footWater.at(cell)!=0)
                                footBlocked[cell]=1;
                            cell++;
                        }
                    }
                    std::vector<uint16_t> footComponent;
                    floodChunkComponents(footBlocked,mapWidth,mapHeight,footComponent);
                    //the ground the linked borders of this map open on
                    std::set<uint16_t> arrival;
                    {
                        const std::vector<std::vector<std::pair<unsigned int,unsigned int> > > &lines=
                                borderLines.at(chunk);
                        unsigned int lineIndex=0;
                        while(lineIndex<lines.size())
                        {
                            unsigned int cellIndex=0;
                            while(cellIndex<lines.at(lineIndex).size())
                            {
                                const unsigned int cell=lines.at(lineIndex).at(cellIndex).first
                                        +lines.at(lineIndex).at(cellIndex).second*mapWidth;
                                if(footComponent.at(cell)!=walkNoComponent)
                                    arrival.insert(footComponent.at(cell));
                                cellIndex++;
                            }
                            lineIndex++;
                        }
                    }
                    const std::pair<uint8_t,uint8_t> &landing=boatLandingCells.at(
                                std::pair<uint16_t,uint16_t>((uint16_t)chunkX,(uint16_t)chunkY));
                    const unsigned int landingCell=(unsigned int)landing.first
                            +(unsigned int)landing.second*mapWidth;
                    //a map whose borders open on no dry ground at all is entered by
                    //water anyway: there is nothing better to ask for there
                    if(!arrival.empty() && arrival.find(footComponent.at(landingCell))==arrival.cend())
                    {
                        if(!reportPass)
                            repaired|=carveChunkCorridor(worldMap,collisionLayers,footComponent,
                                                         *arrival.cbegin(),landingCell,chunkX,chunkY,
                                                         mapWidth,mapHeight,&footWater);
                        else
                        {
                            errors.push_back(chunkName+": the boat quay at "+
                                             std::to_string(landing.first)+","+std::to_string(landing.second)+
                                             " cannot be walked to from the border of the map");
                            unreachableDoors++;
                        }
                    }
                }
                if(repaired)
                    repairedChunks++;
                repairPass++;
                }
            }
            chunkX++;
        }
        chunkY++;
    }
    std::cout << "walkability: " << openedPairs << " border pair(s) opened, "
              << repairedChunks << " chunk(s) repaired, "
              << carvedPlants << " whole plant(s) taken out, "
              << brokenBorders << " broken border link(s) left, "
              << unreachableDoors << " unreachable door(s) left" << std::endl;
    return errors.empty();
}

std::vector<unsigned char> LoadMapAll::portCity;
std::vector<LoadMapAll::BoatCrossing> LoadMapAll::boatCrossings;
std::map<std::pair<uint16_t,uint16_t>,std::pair<uint8_t,uint8_t> > LoadMapAll::boatLandingCells;
std::map<std::pair<uint16_t,uint16_t>,Tiled::MapObject*> LoadMapAll::boatTeleportObjects;

void LoadMapAll::wireBoatCrossings()
{
    unsigned int crossingIndex=0;
    unsigned int wired=0;
    std::vector<BoatCrossing> kept;
    while(crossingIndex<boatCrossings.size())
    {
        const BoatCrossing &crossing=boatCrossings.at(crossingIndex);
        const std::pair<uint16_t,uint16_t> from(crossing.fromX,crossing.fromY);
        const std::pair<uint16_t,uint16_t> to(crossing.toX,crossing.toY);
        //A crossing is only real when BOTH shores moored a boat: a chunk whose sea
        //was flattened away by a nearby town has nowhere to put one. Half a
        //crossing is worse than none — the teleport of the other side would point
        //at 0,0 — so it is dropped whole, objects included, and the connectivity
        //check that runs next then measures the world as it really is.
        const bool complete=(boatLandingCells.find(from)!=boatLandingCells.cend()
                             && boatLandingCells.find(to)!=boatLandingCells.cend()
                             && boatTeleportObjects.find(from)!=boatTeleportObjects.cend()
                             && boatTeleportObjects.find(to)!=boatTeleportObjects.cend());
        unsigned int side=0;
        while(side<2)
        {
            const std::pair<uint16_t,uint16_t> &here=(side==0)?from:to;
            const std::pair<uint16_t,uint16_t> &there=(side==0)?to:from;
            if(boatTeleportObjects.find(here)!=boatTeleportObjects.cend())
            {
                Tiled::MapObject * const object=boatTeleportObjects.at(here);
                if(complete)
                {
                    //each side lands NEXT TO the other side's boat, on the shore
                    //cell it touches — the boat tile itself is the teleport,
                    //standing on it would bounce the player straight back
                    const std::pair<uint8_t,uint8_t> &landing=boatLandingCells.at(there);
                    object->setProperty("x",QString::number(landing.first));
                    object->setProperty("y",QString::number(landing.second));
                    wired++;
                }
                else
                {
                    Tiled::ObjectGroup * const group=object->objectGroup();
                    if(group!=NULL)
                        group->removeObject(object);
                    boatTeleportObjects.erase(here);
                    delete object;
                }
            }
            side++;
        }
        if(complete)
            kept.push_back(crossing);
        else
        {
            //no landing either: nothing lands on the quay of a crossing that does
            //not exist, so the walkability guard must not ask for it
            boatLandingCells.erase(from);
            boatLandingCells.erase(to);
            std::cerr << "the boat crossing " << crossing.fromX << "," << crossing.fromY
                      << " <-> " << crossing.toX << "," << crossing.toY
                      << " has no boat on one of its shores and is dropped" << std::endl;
        }
        crossingIndex++;
    }
    if(kept.size()<boatCrossings.size())
        boatCrossings=kept;
    std::cout << "boat crossings: " << boatCrossings.size() << ", " << wired
              << " teleport(s) wired" << std::endl;
}

std::vector<LoadMapAll::WaterBody> LoadMapAll::waterBodies;
std::vector<uint16_t> LoadMapAll::waterBodyOfTile;

void LoadMapAll::detectWaterBodies(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting)
{
    waterBodies.clear();
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    waterBodyOfTile.assign(worldWidth*worldHeight,waterNoBody);
    //Read the VORONOI zones, not the drawn Water layer: the water paths are
    //decided together with the land roads, before a single tile is painted, and
    //that is the only moment where the whole road graph can still be changed.
    //The zone heights are set by the first addTerrain pass, so this is the very
    //terrain that will be drawn.
    if(VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex==NULL)
    {
        std::cerr << "detectWaterBodies called before the voronoi map is computed" << std::endl;
        return;
    }
    std::vector<unsigned char> isWater(worldWidth*worldHeight,0);
    {
        unsigned int cell=0;
        while(cell<worldWidth*worldHeight)
        {
            const VoronioForTiledMapTmx::PolygonZone &zone=
                    VoronioForTiledMapTmx::voronoiMap.zones.at(
                        VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[cell].index);
            if(zone.height<5 && zone.moisure>=1 && zone.moisure<=6)
                if(LoadMap::terrainList[zone.height][zone.moisure-1].terrainName
                        .compare(QString("water"),Qt::CaseInsensitive)==0)
                    isWater[cell]=1;
            cell++;
        }
    }
    std::vector<unsigned int> queue;
    unsigned int startCell=0;
    while(startCell<worldWidth*worldHeight)
    {
        if(isWater.at(startCell)!=0 && waterBodyOfTile.at(startCell)==waterNoBody)
        {
            //0xFFFF is the "land" marker, so that many bodies is the limit; the
            //ones after it are simply not tracked (they are tiny by then)
            if(waterBodies.size()>=waterNoBody)
                break;
            const uint16_t label=(uint16_t)waterBodies.size();
            WaterBody body;
            body.size=0;
            body.isSea=false;
            body.minX=startCell%worldWidth;
            body.maxX=body.minX;
            body.minY=startCell/worldWidth;
            body.maxY=body.minY;
            body.seedX=body.minX;
            body.seedY=body.minY;
            waterBodyOfTile[startCell]=label;
            queue.clear();
            queue.push_back(startCell);
            unsigned int queueIndex=0;
            while(queueIndex<queue.size())
            {
                const unsigned int cell=queue.at(queueIndex);
                queueIndex++;
                const unsigned int cellX=cell%worldWidth;
                const unsigned int cellY=cell/worldWidth;
                body.size++;
                if(cellX<body.minX)
                    body.minX=cellX;
                if(cellX>body.maxX)
                    body.maxX=cellX;
                if(cellY<body.minY)
                    body.minY=cellY;
                if(cellY>body.maxY)
                    body.maxY=cellY;
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=(int)cellX+stepX[direction];
                    const int nextY=(int)cellY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                        if(isWater.at(next)!=0 && waterBodyOfTile.at(next)==waterNoBody)
                        {
                            waterBodyOfTile[next]=label;
                            queue.push_back(next);
                        }
                    }
                    direction++;
                }
            }
            //A SEA is big AND LONG: the size alone made a round inland lake a
            //shipping lane. The longest side of its bounding box has to reach
            //seaMinSpan tiles, which is what tells a coast from a pond.
            {
                const unsigned int spanX=body.maxX-body.minX+1;
                const unsigned int spanY=body.maxY-body.minY+1;
                const unsigned int span=(spanX>spanY)?spanX:spanY;
                body.isSea=(body.size>=setting.waterSeaMinTiles
                            && span>=setting.waterSeaMinSpan);
            }
            waterBodies.push_back(body);
        }
        startCell++;
    }
    unsigned int seaCount=0;
    unsigned int biggest=0;
    unsigned int bodyIndex=0;
    while(bodyIndex<waterBodies.size())
    {
        if(waterBodies.at(bodyIndex).isSea)
            seaCount++;
        if(waterBodies.at(bodyIndex).size>biggest)
            biggest=waterBodies.at(bodyIndex).size;
        bodyIndex++;
    }
    std::cout << "water: " << waterBodies.size() << " bod(y|ies), " << seaCount
              << " sea(s) of at least " << setting.waterSeaMinTiles << " tiles and "
              << setting.waterSeaMinSpan << " tiles long, biggest "
              << biggest << " tiles" << std::endl;
}

//Outline of a mask as a closed polygon of CELL CORNERS, by following the boundary
//edges. A bounding box would say nothing about a coastline; this is the shape.
static QPolygonF traceMaskOutline(const std::vector<unsigned char> &mask,
                                  const unsigned int worldWidth,const unsigned int worldHeight,
                                  const unsigned int seedX,const unsigned int seedY)
{
    QPolygonF outline;
    //Walk the boundary edges with the body always on the RIGHT hand, clockwise.
    //Direction 0 right, 1 down, 2 left, 3 up; the position is a CORNER of the
    //tile grid, (cx,cy) being the top-left corner of tile (cx,cy).
    //The seed is the FIRST body cell in scan order, so the cell above it and the
    //one left of it are not in the body: starting at its top-left corner heading
    //RIGHT puts the body below, that is on the right hand. (With the body on the
    //LEFT the very first step is already invalid and the walk never closes.)
    const int stepX[4]={1,0,-1,0};
    const int stepY[4]={0,1,0,-1};
    int cornerX=(int)seedX;
    int cornerY=(int)seedY;
    int direction=0;
    const int startX=cornerX;
    const int startY=cornerY;
    const int startDirection=direction;
    unsigned int guard=0;
    //one step per boundary edge; a coastline cannot be longer than every edge of
    //the grid, and the polygon is capped well below that anyway
    const unsigned int guardLimit=4*worldWidth*worldHeight;
    do
    {
        //only a CORNER is worth a point: a straight run of edges would add one
        //point per tile and bury the debug layer under a million of them
        bool turned=false;
        //turn RIGHT first to hug the coast, then straight, then left, then back
        int tryDirection=(direction+1)%4;
        unsigned int attempt=0;
        while(attempt<4 && !turned)
        {
            const int nextX=cornerX+stepX[tryDirection];
            const int nextY=cornerY+stepY[tryDirection];
            //the two tiles the edge separates, left and right of the move
            int leftX=0,leftY=0,rightX=0,rightY=0;
            if(tryDirection==0){leftX=cornerX;leftY=cornerY-1;rightX=cornerX;rightY=cornerY;}
            else if(tryDirection==1){leftX=cornerX;leftY=cornerY;rightX=cornerX-1;rightY=cornerY;}
            else if(tryDirection==2){leftX=cornerX-1;leftY=cornerY;rightX=cornerX-1;rightY=cornerY-1;}
            else {leftX=cornerX-1;leftY=cornerY-1;rightX=cornerX;rightY=cornerY-1;}
            const bool leftIn=(leftX>=0 && leftY>=0 && leftX<(int)worldWidth && leftY<(int)worldHeight
                               && mask.at((unsigned int)leftX+(unsigned int)leftY*worldWidth)!=0);
            const bool rightIn=(rightX>=0 && rightY>=0 && rightX<(int)worldWidth && rightY<(int)worldHeight
                                && mask.at((unsigned int)rightX+(unsigned int)rightY*worldWidth)!=0);
            if(rightIn && !leftIn)
            {
                //the point is emitted where the walk TURNS
                if(tryDirection!=direction || outline.isEmpty())
                    outline << QPointF(cornerX,cornerY);
                cornerX=nextX;
                cornerY=nextY;
                direction=tryDirection;
                turned=true;
            }
            else
            {
                tryDirection=(tryDirection+3)%4;
                attempt++;
            }
        }
        if(!turned)
            break;//a single isolated cell, or a shape the walk cannot follow
        guard++;
    }
    while((cornerX!=startX || cornerY!=startY || direction!=startDirection) && guard<guardLimit);
    return outline;
}

void LoadMapAll::addDebugWaterBodies(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting)
{
    Tiled::ObjectGroup * const layerTerrain=new Tiled::ObjectGroup("Terrain",0,0);
    layerTerrain->setColor(QColor("#2E9EF3"));
    worldMap.addLayer(layerTerrain);
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    const int tileWidth=worldMap.tileWidth();
    const int tileHeight=worldMap.tileHeight();
    unsigned int bodyIndex=0;
    unsigned int drawn=0;
    while(bodyIndex<waterBodies.size())
    {
        const WaterBody &body=waterBodies.at(bodyIndex);
        //a one-pond puddle would add thousands of useless objects; only what is
        //worth naming on a map of the world
        if(body.size>=setting.waterLakeMinTiles)
        {
            //A GENERAL shape, not the per-tile coastline: traced at tile
            //resolution a noisy 668k-tile sea gives over a MILLION corners, which
            //is useless to look at and doubles the size of all.tmx. The body is
            //downsampled first — a block belongs to it as soon as one of its
            //tiles does, so nothing is lost from the silhouette.
            const unsigned int step=setting.waterBodyDebugStep;
            const unsigned int coarseWidth=(worldWidth+step-1)/step;
            const unsigned int coarseHeight=(worldHeight+step-1)/step;
            std::vector<unsigned char> coarse(coarseWidth*coarseHeight,0);
            {
                unsigned int tileY=body.minY;
                while(tileY<=body.maxY)
                {
                    unsigned int tileX=body.minX;
                    while(tileX<=body.maxX)
                    {
                        if(waterBodyOfTile.at(tileX+tileY*worldWidth)==(uint16_t)bodyIndex)
                            coarse[(tileX/step)+(tileY/step)*coarseWidth]=1;
                        tileX++;
                    }
                    tileY++;
                }
            }
            //the seed must be the FIRST block in scan order, as the tracer expects
            unsigned int coarseSeed=0;
            while(coarseSeed<coarse.size() && coarse.at(coarseSeed)==0)
                coarseSeed++;
            QPolygonF outline=traceMaskOutline(coarse,coarseWidth,coarseHeight,
                                               coarseSeed%coarseWidth,coarseSeed/coarseWidth);
            //scale the block corners back to world tiles
            {
                unsigned int pointIndex=0;
                while(pointIndex<(unsigned int)outline.size())
                {
                    outline[pointIndex]=QPointF(outline.at(pointIndex).x()*step,
                                                outline.at(pointIndex).y()*step);
                    pointIndex++;
                }
            }
            if(outline.size()>=3)
            {
                const QString label=QString::fromLatin1(body.isSea?"sea ":"lake ")+
                        QString::number(bodyIndex)+" "+QString::number(body.size)+" tiles";
                //the polygon points are relative to the object position
                const QPointF origin=outline.first();
                unsigned int pointIndex=0;
                while(pointIndex<(unsigned int)outline.size())
                {
                    outline[pointIndex]=QPointF((outline.at(pointIndex).x()-origin.x())*tileWidth,
                                                (outline.at(pointIndex).y()-origin.y())*tileHeight);
                    pointIndex++;
                }
                Tiled::MapObject * const object=new Tiled::MapObject(label,"",
                    QPointF(origin.x()*tileWidth,origin.y()*tileHeight),QSizeF(0.0,0.0));
                object->setPolygon(outline);
                object->setShape(Tiled::MapObject::Polygon);
                layerTerrain->addObject(object);
                drawn++;
            }
        }
        bodyIndex++;
    }
    layerTerrain->setVisible(false);
    std::cout << "terrainDebug: " << drawn << " water body outline(s)" << std::endl;
}

//THE ITEMS A DATAPACK ASKS FOR TO WALK ON WATER. map/layers.xml says it:
//  <monstersCollision item="31" tile="swim" layer="Water" type="walkOn" .../>
//Without that item the engine refuses to step on the Water layer, so a sea route
//is unusable and the coastal shops have to sell it.
bool LoadMapAll::readWaterWalkItems(const QString &datapackPath,std::vector<unsigned int> &items)
{
    const QString path=datapackPath+"/map/layers.xml";
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        std::cerr << "No " << path.toStdString()
                  << ", the water walk items stay the ones of the settings" << std::endl;
        return false;
    }
    QXmlStreamReader reader(&file);
    while(!reader.atEnd())
    {
        if(reader.readNext()==QXmlStreamReader::StartElement)
        {
            if(reader.name().toString()==QString("monstersCollision"))
            {
                const QXmlStreamAttributes attributes=reader.attributes();
                if(attributes.value("layer").toString()==QString("Water")
                        && attributes.value("type").toString()==QString("walkOn")
                        && attributes.hasAttribute("item"))
                {
                    bool ok=false;
                    const unsigned int item=attributes.value("item").toString().toUInt(&ok);
                    if(ok)
                    {
                        bool already=false;
                        unsigned int itemIndex=0;
                        while(itemIndex<items.size())
                        {
                            if(items.at(itemIndex)==item)
                                already=true;
                            itemIndex++;
                        }
                        if(!already)
                            items.push_back(item);
                    }
                }
            }
        }
    }
    file.close();
    if(reader.hasError())
    {
        std::cerr << "Broken " << path.toStdString() << ": "
                  << reader.errorString().toStdString() << std::endl;
        return false;
    }
    return true;
}

//one link between two GROUPS of maps: the closest pair of maps of each
struct GroupLink
{
    unsigned int groupA,groupB;
    unsigned int chunkA,chunkB;
    unsigned int distance;
};

//do the two links CROSS? Two sea routes may not: they are drawn on the same sea
//and would run through one another. Segment intersection on the chunk centres,
//touching at an end excluded — two links may leave from the same map.
static bool segmentsCross(const unsigned int firstA,const unsigned int firstB,
                          const unsigned int secondA,const unsigned int secondB,
                          const unsigned int mapXCount)
{
    if(firstA==secondA || firstA==secondB || firstB==secondA || firstB==secondB)
        return false;
    const long firstAx=(long)(firstA%mapXCount),firstAy=(long)(firstA/mapXCount);
    const long firstBx=(long)(firstB%mapXCount),firstBy=(long)(firstB/mapXCount);
    const long secondAx=(long)(secondA%mapXCount),secondAy=(long)(secondA/mapXCount);
    const long secondBx=(long)(secondB%mapXCount),secondBy=(long)(secondB/mapXCount);
    //sign of the cross product: which side of a segment a point falls on
    const long firstToSecondA=(firstBx-firstAx)*(secondAy-firstAy)-(firstBy-firstAy)*(secondAx-firstAx);
    const long firstToSecondB=(firstBx-firstAx)*(secondBy-firstAy)-(firstBy-firstAy)*(secondBx-firstAx);
    const long secondToFirstA=(secondBx-secondAx)*(firstAy-secondAy)-(secondBy-secondAy)*(firstAx-secondAx);
    const long secondToFirstB=(secondBx-secondAx)*(firstBy-secondAy)-(secondBy-secondAy)*(firstBx-secondAx);
    if(firstToSecondA==0 || firstToSecondB==0 || secondToFirstA==0 || secondToFirstB==0)
        return false;
    return ((firstToSecondA>0)!=(firstToSecondB>0)) && ((secondToFirstA>0)!=(secondToFirstB>0));
}

//CAN A SHIP BE MOORED IN THAT CHUNK? The boat lies horizontally on the water and
//has to touch the land by the SIDE of a tile, inside the rock line the sea pass
//will draw (so never against the map border). Measured here, on the terrain as
//the router sees it, because a crossing whose shore cannot hold a boat has to be
//moved to another water path BEFORE anything is painted — the far side would
//otherwise keep a teleport pointing at nothing.
static bool chunkCanMoorShip(const unsigned int &chunk,const unsigned int &mapXCount,
                             const unsigned int &singleMapWidth,const unsigned int &singleMapHeight,
                             const unsigned int &worldWidth,
                             const unsigned int &shipWidth,const unsigned int &shipHeight,
                             const unsigned int &margin)
{
    const unsigned int x0=(chunk%mapXCount)*singleMapWidth;
    const unsigned int y0=(chunk/mapXCount)*singleMapHeight;
    if(singleMapWidth<2*margin+shipWidth || singleMapHeight<2*margin+shipHeight)
        return false;
    unsigned int localY=margin;
    while(localY+shipHeight+margin<=singleMapHeight)
    {
        unsigned int localX=margin;
        while(localX+shipWidth+margin<=singleMapWidth)
        {
            bool allWater=true;
            bool touchesLand=false;
            unsigned int shipRow=0;
            while(shipRow<shipHeight && allWater)
            {
                unsigned int shipColumn=0;
                while(shipColumn<shipWidth && allWater)
                {
                    const unsigned int tileX=x0+localX+shipColumn;
                    const unsigned int tileY=y0+localY+shipRow;
                    if(LoadMapAll::waterBodyOfTile.at(tileX+tileY*worldWidth)==LoadMapAll::waterNoBody)
                        allWater=false;
                    else
                    {
                        //a SIDE against the land, never a corner
                        const int stepX[4]={-1,1,0,0};
                        const int stepY[4]={0,0,-1,1};
                        unsigned int direction=0;
                        while(direction<4)
                        {
                            const unsigned int neighbourX=(unsigned int)((int)tileX+stepX[direction]);
                            const unsigned int neighbourY=(unsigned int)((int)tileY+stepY[direction]);
                            if(neighbourX<worldWidth
                                    && neighbourY<LoadMapAll::waterBodyOfTile.size()/worldWidth)
                                if(LoadMapAll::waterBodyOfTile.at(neighbourX+neighbourY*worldWidth)
                                        ==LoadMapAll::waterNoBody)
                                    touchesLand=true;
                            direction++;
                        }
                    }
                    shipColumn++;
                }
                shipRow++;
            }
            if(allWater && touchesLand)
                return true;
            localX++;
        }
        localY++;
    }
    return false;
}

void LoadMapAll::addWaterPaths(const unsigned int mapXCount,const unsigned int mapYCount,
                               const unsigned int singleMapWidth,const unsigned int singleMapHeight,
                               const unsigned int worldWidth,
                               const SettingsAll::SettingsExtra &setting,
                               std::vector<std::pair<uint16_t,uint16_t> > &waterChunks,
                               std::vector<std::pair<uint16_t,uint16_t> > &boatChunks)
{
    waterChunks.clear();
    boatChunks.clear();
    boatCrossings.clear();
    waterRoutes.clear();
    //the ship of a crossing, as a block of its sheet: its size decides where a
    //boat can be moored at all, so it is needed here, at planning time
    unsigned int shipWidth=1,shipHeight=1;
    {
        const QStringList shipParts=setting.waterShipUsable.split(",");
        if(shipParts.size()==3)
        {
            shipWidth=(unsigned int)shipParts.at(1).trimmed().toInt();
            shipHeight=(unsigned int)shipParts.at(2).trimmed().toInt();
        }
        if(shipWidth<1)
            shipWidth=1;
        if(shipHeight<1)
            shipHeight=1;
    }
    //NOT gated on pathPercentOfLand: that setting only buys the EXTRA routes.
    //Joining the land masses is what the sea is for and always runs.
    if(cities.size()<2 || waterBodies.empty())
        return;
    //how much SEA each chunk holds, which seas it touches, and whether it has a
    //SHORE at all — land against the sea, the only place a boat can be moored
    std::vector<unsigned int> chunkSeaTiles(mapXCount*mapYCount,0);
    std::vector<std::set<uint16_t> > chunkSeas(mapXCount*mapYCount);
    std::vector<unsigned char> chunkShore(mapXCount*mapYCount,0);
    {
        unsigned int chunkY=0;
        while(chunkY<mapYCount)
        {
            unsigned int chunkX=0;
            while(chunkX<mapXCount)
            {
                unsigned int localY=0;
                while(localY<singleMapHeight)
                {
                    unsigned int localX=0;
                    while(localX<singleMapWidth)
                    {
                        const unsigned int tileX=chunkX*singleMapWidth+localX;
                        const unsigned int tileY=chunkY*singleMapHeight+localY;
                        const uint16_t body=waterBodyOfTile.at(tileX+tileY*worldWidth);
                        //A ROUTE MAY SAIL A SEA OR A LAKE, never a puddle. The sea
                        //is what a shipping lane is drawn on, but a whole group of
                        //maps can sit by a big inland lake and nothing else — 44
                        //maps of the reference world are landlocked that way, and
                        //without the lake they could never be joined at all.
                        const bool namedBody=(body!=waterNoBody
                                              && (waterBodies.at(body).isSea
                                                  || waterBodies.at(body).size>=setting.waterLakeMinTiles));
                        if(namedBody)
                        {
                            chunkSeaTiles[chunkX+chunkY*mapXCount]++;
                            chunkSeas[chunkX+chunkY*mapXCount].insert(body);
                        }
                        else if(body==waterNoBody)
                        {
                            //land: is the sea right next to it? then this chunk has
                            //a coast, and a ship can be tied to it
                            const int stepX[4]={-1,1,0,0};
                            const int stepY[4]={0,0,-1,1};
                            unsigned int direction=0;
                            while(direction<4)
                            {
                                const int nextX=(int)tileX+stepX[direction];
                                const int nextY=(int)tileY+stepY[direction];
                                if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth
                                        && (unsigned int)nextY<waterBodyOfTile.size()/worldWidth)
                                {
                                    const uint16_t nextBody=waterBodyOfTile.at((unsigned int)nextX
                                                                               +(unsigned int)nextY*worldWidth);
                                    if(nextBody!=waterNoBody
                                            && (waterBodies.at(nextBody).isSea
                                                || waterBodies.at(nextBody).size>=setting.waterLakeMinTiles))
                                        chunkShore[chunkX+chunkY*mapXCount]=1;
                                }
                                direction++;
                            }
                        }
                        localX++;
                    }
                    localY++;
                }
                chunkX++;
            }
            chunkY++;
        }
    }
    //A chunk a route may sail through: it holds sea, carries no map yet and is no
    //town. chunkSeaPercent is only a MINIMUM of open water, not "mostly sea" —
    //the lane follows the water inside the chunk, so a coastal chunk carries a
    //route perfectly well, and demanding 80% of sea was why almost every join
    //fell back to a boat.
    const unsigned int chunkTiles=singleMapWidth*singleMapHeight;
    std::vector<unsigned char> sailable(mapXCount*mapYCount,0);
    {
        unsigned int chunk=0;
        while(chunk<mapXCount*mapYCount)
        {
            if(chunkSeaTiles.at(chunk)*100>=chunkTiles*setting.waterChunkSeaPercent
                    && mapPathDirection[chunk]==0
                    && !haveCityEntry(citiesCoordToIndex,chunk%mapXCount,chunk/mapXCount))
                sailable[chunk]=1;
            chunk++;
        }
    }
    //...and the step from one chunk to the next is only possible where the SEA
    //really crosses their shared border, else the two lanes meet a coast and the
    //route is drawn but cannot be sailed
    std::vector<unsigned char> seaAtBorder(mapXCount*mapYCount*4,0);
    {
        const int stepX[4]={-1,1,0,0};
        const int stepY[4]={0,0,-1,1};
        unsigned int chunk=0;
        while(chunk<mapXCount*mapYCount)
        {
            const int chunkX=(int)(chunk%mapXCount);
            const int chunkY=(int)(chunk/mapXCount);
            unsigned int direction=0;
            while(direction<4)
            {
                const int nextX=chunkX+stepX[direction];
                const int nextY=chunkY+stepY[direction];
                if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                {
                    //ANY pair of sea cells facing each other across the shared
                    //border will do. The engine reads a border-* object as the
                    //WHOLE SIDE of the map with an offset (Map_loaderMain.cpp), not
                    //as a teleport on one cell, so the crossing is not tied to the
                    //midpoint — demanding sea there threw away every lane.
                    const unsigned int lineLength=(direction<2)?singleMapHeight:singleMapWidth;
                    unsigned int step=0;
                    while(step<lineLength && seaAtBorder.at(chunk*4+direction)==0)
                    {
                        unsigned int ownX=0,ownY=0,otherX=0,otherY=0;
                        if(direction==0){ownX=0;ownY=step;otherX=singleMapWidth-1;otherY=step;}
                        else if(direction==1){ownX=singleMapWidth-1;ownY=step;otherX=0;otherY=step;}
                        else if(direction==2){ownX=step;ownY=0;otherX=step;otherY=singleMapHeight-1;}
                        else {ownX=step;ownY=singleMapHeight-1;otherX=step;otherY=0;}
                        const unsigned int ownTile=((unsigned int)chunkX*singleMapWidth+ownX)
                                +((unsigned int)chunkY*singleMapHeight+ownY)*worldWidth;
                        const unsigned int otherTile=((unsigned int)nextX*singleMapWidth+otherX)
                                +((unsigned int)nextY*singleMapHeight+otherY)*worldWidth;
                        const uint16_t ownBody=waterBodyOfTile.at(ownTile);
                        const uint16_t otherBody=waterBodyOfTile.at(otherTile);
                        if(ownBody!=waterNoBody && waterBodies.at(ownBody).isSea
                                && otherBody!=waterNoBody && waterBodies.at(otherBody).isSea)
                            seaAtBorder[chunk*4+direction]=1;
                        step++;
                    }
                }
                direction++;
            }
            chunk++;
        }
    }
    //=== THE LINKS, decided on the MAP GRAPH, not on the towns ===============
    //1) the maps interconnected by ROADS ONLY are one GROUP
    std::vector<unsigned int> groupOfChunk(mapXCount*mapYCount,0xFFFFFFFF);
    unsigned int groupCount=0;
    {
        unsigned int startChunk=0;
        while(startChunk<mapXCount*mapYCount)
        {
            if(mapPathDirection[startChunk]!=0 && groupOfChunk.at(startChunk)==0xFFFFFFFF)
            {
                std::vector<unsigned int> queue;
                groupOfChunk[startChunk]=groupCount;
                queue.push_back(startChunk);
                unsigned int queueIndex=0;
                while(queueIndex<queue.size())
                {
                    const unsigned int chunk=queue.at(queueIndex);
                    queueIndex++;
                    const int chunkX=(int)(chunk%mapXCount);
                    const int chunkY=(int)(chunk/mapXCount);
                    const int stepX[4]={-1,1,0,0};
                    const int stepY[4]={0,0,-1,1};
                    const Orientation bit[4]={Orientation_left,Orientation_right,Orientation_top,Orientation_bottom};
                    unsigned int direction=0;
                    while(direction<4)
                    {
                        if((mapPathDirection[chunk]&bit[direction])!=0)
                        {
                            const int nextX=chunkX+stepX[direction];
                            const int nextY=chunkY+stepY[direction];
                            if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                            {
                                const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                                if(mapPathDirection[next]!=0 && groupOfChunk.at(next)==0xFFFFFFFF)
                                {
                                    groupOfChunk[next]=groupCount;
                                    queue.push_back(next);
                                }
                            }
                        }
                        direction++;
                    }
                }
                groupCount++;
            }
            startChunk++;
        }
    }
    //TWO GROUPS THE LAND CAN JOIN ARE JOINED BY LAND, never by sea. A ferry
    //between two maps side by side is absurd, and a water path may not cross the
    //land to reach one: a group sitting by a LAKE and nothing else (44 maps of
    //the reference world) has no sea to be reached by, and the only honest answer
    //is a ROAD. The chunks the join walks through become ordinary road maps —
    //generateRoadContent paints any chunk whose orientation bits are set — so
    //what comes out is a road between the two, not a boat over a continent.
    {
        unsigned int landJoins=0;
        unsigned int joinedChunks=0;
        bool joined=true;
        while(joined)
        {
            joined=false;
            //the cheapest way from one group to another over EMPTY LAND: chunks
            //no map is written for and the sea does not cover
            int bestFrom=-1,bestTo=-1;
            std::vector<int> bestParent;
            unsigned int bestLength=0xFFFFFFFF;
            unsigned int startGroup=0;
            while(startGroup<groupCount)
            {
                std::vector<int> parent(mapXCount*mapYCount,-2);
                std::vector<unsigned int> queue;
                unsigned int chunk=0;
                while(chunk<mapXCount*mapYCount)
                {
                    if(groupOfChunk.at(chunk)==startGroup)
                    {
                        parent[chunk]=-1;
                        queue.push_back(chunk);
                    }
                    chunk++;
                }
                unsigned int queueIndex=0;
                while(queueIndex<queue.size())
                {
                    const unsigned int current=queue.at(queueIndex);
                    queueIndex++;
                    //another group reached: how long is the way?
                    if(groupOfChunk.at(current)!=0xFFFFFFFF && groupOfChunk.at(current)!=startGroup)
                    {
                        unsigned int length=0;
                        int walk=(int)current;
                        while(parent.at((unsigned int)walk)>=0)
                        {
                            length++;
                            walk=parent.at((unsigned int)walk);
                        }
                        if(length<bestLength)
                        {
                            bestLength=length;
                            bestFrom=walk;
                            bestTo=(int)current;
                            bestParent=parent;
                        }
                    }
                    else
                    {
                        const int currentX=(int)(current%mapXCount);
                        const int currentY=(int)(current/mapXCount);
                        const int stepX[4]={-1,1,0,0};
                        const int stepY[4]={0,0,-1,1};
                        unsigned int direction=0;
                        while(direction<4)
                        {
                            const int nextX=currentX+stepX[direction];
                            const int nextY=currentY+stepY[direction];
                            if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                            {
                                const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                                //empty land, or a map of another group (the arrival)
                                const bool emptyLand=(mapPathDirection[next]==0
                                                      && chunkSeaTiles.at(next)*4<chunkTiles);
                                const bool otherGroup=(groupOfChunk.at(next)!=0xFFFFFFFF
                                                       && groupOfChunk.at(next)!=startGroup);
                                if(parent.at(next)==-2 && (emptyLand || otherGroup))
                                {
                                    parent[next]=(int)current;
                                    queue.push_back(next);
                                }
                            }
                            direction++;
                        }
                    }
                }
                startGroup++;
            }
            if(bestTo>=0 && bestFrom>=0)
            {
                //link the whole chain, both ways
                int walk=bestTo;
                while(bestParent.at((unsigned int)walk)>=0)
                {
                    const unsigned int previous=(unsigned int)bestParent.at((unsigned int)walk);
                    linkChunkToNeighbour((unsigned int)walk,previous,mapXCount);
                    linkChunkToNeighbour(previous,(unsigned int)walk,mapXCount);
                    if(mapPathDirection[(unsigned int)walk]!=0 && groupOfChunk.at((unsigned int)walk)==0xFFFFFFFF)
                        joinedChunks++;
                    walk=(int)previous;
                }
                landJoins++;
                joined=true;
                //re-group: the flood is the truth, the labels are not
                groupCount=0;
                groupOfChunk.assign(mapXCount*mapYCount,0xFFFFFFFF);
                unsigned int floodStart=0;
                while(floodStart<mapXCount*mapYCount)
                {
                    if(mapPathDirection[floodStart]!=0 && groupOfChunk.at(floodStart)==0xFFFFFFFF)
                    {
                        std::vector<unsigned int> floodQueue;
                        groupOfChunk[floodStart]=groupCount;
                        floodQueue.push_back(floodStart);
                        unsigned int floodIndex=0;
                        while(floodIndex<floodQueue.size())
                        {
                            const unsigned int chunk=floodQueue.at(floodIndex);
                            floodIndex++;
                            const int chunkX=(int)(chunk%mapXCount);
                            const int chunkY=(int)(chunk/mapXCount);
                            const int stepX[4]={-1,1,0,0};
                            const int stepY[4]={0,0,-1,1};
                            const Orientation bit[4]={Orientation_left,Orientation_right,
                                                      Orientation_top,Orientation_bottom};
                            unsigned int direction=0;
                            while(direction<4)
                            {
                                if((mapPathDirection[chunk]&bit[direction])!=0)
                                {
                                    const int nextX=chunkX+stepX[direction];
                                    const int nextY=chunkY+stepY[direction];
                                    if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                                    {
                                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                                        if(mapPathDirection[next]!=0 && groupOfChunk.at(next)==0xFFFFFFFF)
                                        {
                                            groupOfChunk[next]=groupCount;
                                            floodQueue.push_back(next);
                                        }
                                    }
                                }
                                direction++;
                            }
                        }
                        groupCount++;
                    }
                    floodStart++;
                }
            }
        }
        if(landJoins>0)
            std::cout << "sea: " << landJoins << " group(s) of maps joined BY LAND ("
                      << joinedChunks << " new road map(s)): the land can reach them, "
                      << "so no boat has to" << std::endl;
    }
    std::cout << "sea: " << groupCount << " group(s) of maps joined by road" << std::endl;
    std::vector<unsigned int> groupSize(groupCount,0);
    {
        unsigned int chunk=0;
        while(chunk<mapXCount*mapYCount)
        {
            if(groupOfChunk.at(chunk)!=0xFFFFFFFF)
                groupSize[groupOfChunk.at(chunk)]++;
            chunk++;
        }
    }
    //one group of maps only: the land roads already join everything, there is
    //nothing for the sea to link
    if(groupCount<2)
        return;
    //2) EMBARKATION points: a map of a group that TOUCHES a sailable sea chunk. A
    //link between two groups always runs from one of those to another.
    std::vector<std::vector<unsigned int> > groupShore(groupCount);
    std::map<unsigned int,std::set<uint16_t> > shoreWater;
    {
        unsigned int chunk=0;
        while(chunk<mapXCount*mapYCount)
        {
            if(groupOfChunk.at(chunk)!=0xFFFFFFFF)
            {
                const int chunkX=(int)(chunk%mapXCount);
                const int chunkY=(int)(chunk/mapXCount);
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                bool touchesSea=false;
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=chunkX+stepX[direction];
                    const int nextY=chunkY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                        if(sailable.at((unsigned int)nextX+(unsigned int)nextY*mapXCount)!=0)
                            touchesSea=true;
                    direction++;
                }
                if(touchesSea)
                {
                    groupShore[groupOfChunk.at(chunk)].push_back(chunk);
                    //...and WHICH WATER it can put to sea on: a link between two
                    //maps only exists when both sail the SAME body. A water path
                    //never crosses land, so two shores on two different seas are
                    //not a pair, however close they look on the grid.
                    unsigned int bodyDirection=0;
                    while(bodyDirection<4)
                    {
                        const int nextX=chunkX+stepX[bodyDirection];
                        const int nextY=chunkY+stepY[bodyDirection];
                        if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                        {
                            const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                            if(sailable.at(next)!=0)
                                shoreWater[chunk].insert(chunkSeas.at(next).cbegin(),chunkSeas.at(next).cend());
                        }
                        bodyDirection++;
                    }
                }
            }
            chunk++;
        }
    }
    {
        unsigned int groupIndex=0;
        while(groupIndex<groupCount)
        {
            std::set<uint16_t> groupBodies;
            {
                unsigned int shoreIndex=0;
                while(shoreIndex<groupShore.at(groupIndex).size())
                {
                    const std::set<uint16_t> &bodies=shoreWater[groupShore.at(groupIndex).at(shoreIndex)];
                    groupBodies.insert(bodies.cbegin(),bodies.cend());
                    shoreIndex++;
                }
            }
            std::cout << "sea: group " << groupIndex << " holds " << groupSize.at(groupIndex)
                      << " map(s) and touches the sea on " << groupShore.at(groupIndex).size()
                      << " of them, on water bod(y|ies)";
            {
                std::set<uint16_t>::const_iterator bodyIterator=groupBodies.cbegin();
                while(bodyIterator!=groupBodies.cend())
                {
                    std::cout << " " << *bodyIterator;
                    ++bodyIterator;
                }
            }
            std::cout << std::endl;
            groupIndex++;
        }
    }
    //3) the CLOSEST pair of maps between each pair of groups
    std::vector<GroupLink> groupLinks;
    {
        unsigned int firstGroup=0;
        while(firstGroup<groupCount)
        {
            unsigned int secondGroup=firstGroup+1;
            while(secondGroup<groupCount)
            {
                GroupLink link;
                link.groupA=firstGroup;
                link.groupB=secondGroup;
                link.chunkA=0;
                link.chunkB=0;
                link.distance=0xFFFFFFFF;
                unsigned int firstIndex=0;
                while(firstIndex<groupShore.at(firstGroup).size())
                {
                    const unsigned int chunkA=groupShore.at(firstGroup).at(firstIndex);
                    unsigned int secondIndex=0;
                    while(secondIndex<groupShore.at(secondGroup).size())
                    {
                        const unsigned int chunkB=groupShore.at(secondGroup).at(secondIndex);
                        //THE SAME WATER, or it is no pair at all
                        bool sameWater=false;
                        {
                            std::set<uint16_t>::const_iterator bodyIterator=shoreWater[chunkA].cbegin();
                            while(bodyIterator!=shoreWater[chunkA].cend() && !sameWater)
                            {
                                if(shoreWater[chunkB].find(*bodyIterator)!=shoreWater[chunkB].cend())
                                    sameWater=true;
                                ++bodyIterator;
                            }
                        }
                        const unsigned int distance=(unsigned int)(
                                    abs((int)(chunkA%mapXCount)-(int)(chunkB%mapXCount))
                                    +abs((int)(chunkA/mapXCount)-(int)(chunkB/mapXCount)));
                        if(sameWater && distance<link.distance)
                        {
                            link.distance=distance;
                            link.chunkA=chunkA;
                            link.chunkB=chunkB;
                        }
                        secondIndex++;
                    }
                    firstIndex++;
                }
                if(link.distance!=0xFFFFFFFF)
                    groupLinks.push_back(link);
                secondGroup++;
            }
            firstGroup++;
        }
    }
    //4) THE MESH. A group is joined to its DIRECT neighbours only: a link that
    //would go AROUND a third group is not a neighbour link, and two sea links
    //never cross. The groups with the FEWEST links are served first, so an
    //isolated one is joined before a well connected one gets a second route.
    //WHAT IS A DIRECT NEIGHBOUR, without a magic number: A and B are neighbours
    //when NO third group C is closer to BOTH of them than they are to each other
    //(the relative neighbourhood graph). Testing only "the straight line crosses
    //no map" is not enough — a link happily ran 32 maps down the coast over open
    //water between two groups their common neighbour already joined in 6.
    std::vector<unsigned int> groupDistance(groupCount*groupCount,0xFFFFFFFF);
    {
        unsigned int linkIndex=0;
        while(linkIndex<groupLinks.size())
        {
            const GroupLink &link=groupLinks.at(linkIndex);
            groupDistance[link.groupA+link.groupB*groupCount]=link.distance;
            groupDistance[link.groupB+link.groupA*groupCount]=link.distance;
            linkIndex++;
        }
    }
    std::vector<unsigned int> linkCount(groupCount,0);
    std::vector<GroupLink> accepted;
    std::vector<unsigned char> linkDone(groupLinks.size(),0);
    //union-find over the groups: NO GROUP MAY STAY ISOLATED, so when the mesh runs
    //out of neighbour links a group still on its own takes the best link it can
    //get, tests relaxed one at a time (going around another group first, crossing
    //last) — being cut off the world is worse than either.
    std::vector<unsigned int> groupJoined(groupCount,0);
    {
        unsigned int groupIndex=0;
        while(groupIndex<groupCount)
        {
            groupJoined[groupIndex]=groupIndex;
            groupIndex++;
        }
    }
    unsigned int relaxed=0;
    while(true)
    {
        int best=-1;
        unsigned int bestDegree=0;
        unsigned int bestDistance=0;
        unsigned int linkIndex=0;
        while(linkIndex<groupLinks.size())
        {
            if(linkDone.at(linkIndex)==0)
            {
                const GroupLink &link=groupLinks.at(linkIndex);
                const unsigned int degree=(linkCount.at(link.groupA)<linkCount.at(link.groupB))
                        ?linkCount.at(link.groupA):linkCount.at(link.groupB);
                //the mesh is served least-connected first; once the rules are
                //relaxed the only goal left is to join what is still on its own,
                //and there the SHORTEST link is always the right one
                const bool better=(best<0)
                        || (relaxed==0 && (degree<bestDegree
                                           || (degree==bestDegree && link.distance<bestDistance)))
                        || (relaxed>0 && link.distance<bestDistance);
                if(better)
                {
                    //A DIRECT NEIGHBOUR: no third group is closer to both ends
                    //than they are to one another
                    bool throughAnother=false;
                    {
                        unsigned int otherGroup=0;
                        while(otherGroup<groupCount && !throughAnother)
                        {
                            if(otherGroup!=link.groupA && otherGroup!=link.groupB)
                            {
                                const unsigned int toA=groupDistance.at(link.groupA+otherGroup*groupCount);
                                const unsigned int toB=groupDistance.at(link.groupB+otherGroup*groupCount);
                                if(toA<link.distance && toB<link.distance)
                                    throughAnother=true;
                            }
                            otherGroup++;
                        }
                    }
                    //ONLY A DIRECT NEIGHBOUR: the straight line between the two
                    //maps must cross NO MAP AT ALL — not a third group's, and not
                    //one of their own either. Testing only for a third group let a
                    //link run the whole length of its own coast: a ferry 24 maps
                    //long down the east side of the world, drawn straight across
                    //the continent on the minimap, between two groups a short hop
                    //already joined through their neighbour.
                    bool aroundAnother=false;
                    {
                        int walkX=(int)(link.chunkA%mapXCount);
                        int walkY=(int)(link.chunkA/mapXCount);
                        const int endX=(int)(link.chunkB%mapXCount);
                        const int endY=(int)(link.chunkB/mapXCount);
                        while((walkX!=endX || walkY!=endY) && !aroundAnother)
                        {
                            if(walkX!=endX)
                                walkX+=(endX>walkX)?1:-1;
                            if(walkY!=endY)
                                walkY+=(endY>walkY)?1:-1;
                            const unsigned int walkChunk=(unsigned int)walkX+(unsigned int)walkY*mapXCount;
                            if(walkChunk!=link.chunkB && mapPathDirection[walkChunk]!=0)
                                aroundAnother=true;
                        }
                    }
                    //...and TWO SEA ROUTES NEVER CROSS
                    bool crosses=false;
                    {
                        unsigned int acceptedIndex=0;
                        while(acceptedIndex<accepted.size() && !crosses)
                        {
                            if(segmentsCross(link.chunkA,link.chunkB,
                                             accepted.at(acceptedIndex).chunkA,
                                             accepted.at(acceptedIndex).chunkB,mapXCount))
                                crosses=true;
                            acceptedIndex++;
                        }
                    }
                    //relaxed 0: a DIRECT NEIGHBOUR whose line crosses no map and
                    //            no other route — the mesh proper.
                    //relaxed 1: a group the mesh could not reach may take a link
                    //            that goes around another group.
                    //relaxed 2: ...and one whose line runs over a map.
                    //relaxed 3: ...and one that crosses another route.
                    //A relaxed link is only ever taken for a group that would
                    //otherwise stay isolated, which is what the loop below asks.
                    const bool acceptable=(!throughAnother && !aroundAnother && !crosses)
                            || (relaxed>=1 && !aroundAnother && !crosses)
                            || (relaxed>=2 && !crosses)
                            || (relaxed>=3);
                    const bool joinsTwoGroups=(groupJoined.at(link.groupA)!=groupJoined.at(link.groupB));
                    if(acceptable && (relaxed==0 || joinsTwoGroups))
                    {
                        best=(int)linkIndex;
                        bestDegree=degree;
                        bestDistance=link.distance;
                    }
                    else if(relaxed==0 && !acceptable)
                        linkDone[linkIndex]=1;
                }
            }
            linkIndex++;
        }
        if(best<0)
        {
            //is anything still on its own? then relax and try again
            bool allJoined=true;
            {
                unsigned int groupIndex=1;
                while(groupIndex<groupCount)
                {
                    if(groupJoined.at(groupIndex)!=groupJoined.at(0))
                        allJoined=false;
                    groupIndex++;
                }
            }
            if(allJoined || relaxed>=3)
                break;
            relaxed++;
            std::cerr << "sea: a group of maps is still on its own, "
                      << ((relaxed==1)?"allowing a route around another group"
                                      :((relaxed==2)?"allowing a route over a map"
                                                    :"allowing two routes to cross")) << std::endl;
            unsigned int linkReset=0;
            while(linkReset<linkDone.size())
            {
                linkDone[linkReset]=0;
                linkReset++;
            }
            unsigned int acceptedIndex=0;
            while(acceptedIndex<accepted.size())
            {
                //the ones already built are not candidates any more
                unsigned int candidateIndex=0;
                while(candidateIndex<groupLinks.size())
                {
                    if(groupLinks.at(candidateIndex).groupA==accepted.at(acceptedIndex).groupA
                            && groupLinks.at(candidateIndex).groupB==accepted.at(acceptedIndex).groupB)
                        linkDone[candidateIndex]=1;
                    candidateIndex++;
                }
                acceptedIndex++;
            }
        }
        else
        {
            linkDone[(unsigned int)best]=1;
            const GroupLink &chosen=groupLinks.at((unsigned int)best);
            accepted.push_back(chosen);
            linkCount[chosen.groupA]++;
            linkCount[chosen.groupB]++;
            const unsigned int merged=groupJoined.at(chosen.groupB);
            const unsigned int into=groupJoined.at(chosen.groupA);
            unsigned int groupIndex=0;
            while(groupIndex<groupCount)
            {
                if(groupJoined.at(groupIndex)==merged)
                    groupJoined[groupIndex]=into;
                groupIndex++;
            }
        }
    }
    //5) BOAT OR SWIMMABLE, decided HERE for the whole mesh at once. A link the
    //water cannot be swum along is a boat whatever the quota says; the quota then
    //picks the rest, shortest route first — a dice per link gave four ferries out
    //of four on a world that has room for lanes.
    std::vector<std::vector<unsigned int> > linkRoute(accepted.size());
    std::vector<unsigned char> linkIsBoat(accepted.size(),0);
    std::vector<unsigned char> linkResolved(accepted.size(),0);
    //ONE CROSSING PER HARBOUR CHUNK. A chunk serving two ferries needs two boats,
    //and the teleport of the second one silently replaced the first: the far shore
    //of the crossing that lost kept an x/y of 0,0 and dropped the player in a wall.
    std::set<unsigned int> harbourTaken;
    unsigned int built=0;
    unsigned int byBoatCount=0;
    unsigned int linkIndex=0;
    while(linkIndex<accepted.size())
    {
        const GroupLink &link=accepted.at(linkIndex);
        //the sea chunks between the two maps: a BFS over the sailable chunks that
        //starts and ends on a sea chunk touching each end
        std::vector<int> parent(mapXCount*mapYCount,-2);
        std::vector<unsigned char> isTarget(mapXCount*mapYCount,0);
        std::vector<unsigned int> queue;
        {
            const int stepX[4]={-1,1,0,0};
            const int stepY[4]={0,0,-1,1};
            unsigned int endIndex=0;
            while(endIndex<2)
            {
                const unsigned int endChunk=(endIndex==0)?link.chunkA:link.chunkB;
                const int chunkX=(int)(endChunk%mapXCount);
                const int chunkY=(int)(endChunk/mapXCount);
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=chunkX+stepX[direction];
                    const int nextY=chunkY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                        if(sailable.at(next)!=0 && mapPathDirection[next]==0)
                        {
                            if(endIndex==0)
                            {
                                if(parent.at(next)==-2)
                                {
                                    parent[next]=-1;
                                    queue.push_back(next);
                                }
                            }
                            else
                                isTarget[next]=1;
                        }
                    }
                    direction++;
                }
                endIndex++;
            }
        }
        bool found=false;
        int reachedTarget=-1;
        {
            unsigned int queueIndex=0;
            while(queueIndex<queue.size() && !found)
            {
                const unsigned int chunk=queue.at(queueIndex);
                queueIndex++;
                if(isTarget.at(chunk)!=0)
                {
                    found=true;
                    reachedTarget=(int)chunk;
                }
                else
                {
                    const int chunkX=(int)(chunk%mapXCount);
                    const int chunkY=(int)(chunk/mapXCount);
                    const int stepX[4]={-1,1,0,0};
                    const int stepY[4]={0,0,-1,1};
                    unsigned int direction=0;
                    while(direction<4)
                    {
                        const int nextX=chunkX+stepX[direction];
                        const int nextY=chunkY+stepY[direction];
                        if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                        {
                            const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                            if(parent.at(next)==-2 && sailable.at(next)!=0
                                    && mapPathDirection[next]==0
                                    && seaAtBorder.at(chunk*4+direction)!=0)
                            {
                                parent[next]=(int)chunk;
                                queue.push_back(next);
                            }
                        }
                        direction++;
                    }
                }
            }
            //a source that is already a target is the whole route
            if(!found)
            {
                unsigned int sourceIndex=0;
                while(sourceIndex<queue.size() && !found)
                {
                    if(isTarget.at(queue.at(sourceIndex))!=0)
                    {
                        found=true;
                        reachedTarget=(int)queue.at(sourceIndex);
                    }
                    sourceIndex++;
                }
            }
        }
        //A BOAT SAILS, IT DOES NOT FLY OVER THE LAND. When no lane can be swum
        //the link may still be a ferry, but only between two shores that sit on
        //the SAME body of water: a crossing from a lake to the sea is a water
        //path drawn straight over a continent, which is exactly what a water path
        //never is. Every possible shore of each end is collected first — picking
        //the first one and then looking for a partner threw away pairs that did
        //share their water.
        std::vector<unsigned int> route;
        bool forcedBoat=false;
        if(!found)
        {
            const int stepX[4]={-1,1,0,0};
            const int stepY[4]={0,0,-1,1};
            std::vector<unsigned int> shoreCandidate[2];
            unsigned int endIndex=0;
            while(endIndex<2)
            {
                const unsigned int endChunk=(endIndex==0)?link.chunkA:link.chunkB;
                const int chunkX=(int)(endChunk%mapXCount);
                const int chunkY=(int)(endChunk/mapXCount);
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=chunkX+stepX[direction];
                    const int nextY=chunkY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                        //a chunk with a SHORE the ship really fits against, and no
                        //other crossing already moored there
                        if(sailable.at(next)!=0 && mapPathDirection[next]==0
                                && chunkShore.at(next)!=0
                                && harbourTaken.find(next)==harbourTaken.cend()
                                && chunkCanMoorShip(next,mapXCount,singleMapWidth,singleMapHeight,
                                                    worldWidth,shipWidth,shipHeight,
                                                    setting.waterBoatBorderMin))
                            shoreCandidate[endIndex].push_back(next);
                    }
                    direction++;
                }
                endIndex++;
            }
            int shoreA=-1,shoreB=-1;
            {
                unsigned int indexA=0;
                while(indexA<shoreCandidate[0].size() && shoreB<0)
                {
                    unsigned int indexB=0;
                    while(indexB<shoreCandidate[1].size() && shoreB<0)
                    {
                        const unsigned int candidateA=shoreCandidate[0].at(indexA);
                        const unsigned int candidateB=shoreCandidate[1].at(indexB);
                        if(candidateA!=candidateB)
                        {
                            bool sameWater=false;
                            std::set<uint16_t>::const_iterator bodyIterator=chunkSeas.at(candidateA).cbegin();
                            while(bodyIterator!=chunkSeas.at(candidateA).cend() && !sameWater)
                            {
                                if(chunkSeas.at(candidateB).find(*bodyIterator)!=chunkSeas.at(candidateB).cend())
                                    sameWater=true;
                                ++bodyIterator;
                            }
                            if(sameWater)
                            {
                                shoreA=(int)candidateA;
                                shoreB=(int)candidateB;
                            }
                        }
                        indexB++;
                    }
                    indexA++;
                }
            }
            if(shoreA>=0 && shoreB>=0)
            {
                found=true;
                forcedBoat=true;
                harbourTaken.insert((unsigned int)shoreA);
                harbourTaken.insert((unsigned int)shoreB);
                route.push_back((unsigned int)shoreB);
                route.push_back((unsigned int)shoreA);
            }
        }
        if(!found)
        {
            std::cerr << "no water joins the map " << link.chunkA%mapXCount << "," << link.chunkA/mapXCount
                      << " to " << link.chunkB%mapXCount << "," << link.chunkB/mapXCount
                      << ": the groups " << link.groupA << " and " << link.groupB
                      << " stay apart" << std::endl;
            linkIndex++;
        }
        else
        {
            if(route.empty())
            {
                int walkChunk=reachedTarget;
                while(walkChunk>=0)
                {
                    route.push_back((unsigned int)walkChunk);
                    walkChunk=parent.at((unsigned int)walkChunk);
                }
            }
            linkRoute[linkIndex]=route;
            linkIsBoat[linkIndex]=forcedBoat?1:0;
            linkResolved[linkIndex]=1;
        }
        linkIndex++;
    }
    //THE QUOTA, on the links that can be swum: the shortest route becomes a ferry
    //first, a long channel is the one worth swimming. A link whose two shores
    //cannot hold a boat is NEVER picked — the quota moves on to the next water
    //path instead, which is the only way the far side does not end up with a
    //teleport pointing at nothing.
    {
        unsigned int resolvedCount=0;
        unsigned int boatCount=0;
        unsigned int index=0;
        while(index<accepted.size())
        {
            if(linkResolved.at(index)!=0)
            {
                resolvedCount++;
                if(linkIsBoat.at(index)!=0)
                    boatCount++;
            }
            index++;
        }
        const unsigned int wantedBoats=(resolvedCount*setting.waterBoatPercent+50)/100;
        //boat IMPOSSIBLE: measured once per link, on its two end water chunks
        std::vector<unsigned char> boatImpossible(accepted.size(),0);
        index=0;
        while(index<accepted.size())
        {
            if(linkResolved.at(index)!=0 && linkIsBoat.at(index)==0)
            {
                const std::vector<unsigned int> &route=linkRoute.at(index);
                bool sameWater=false;
                if(!route.empty())
                {
                    std::set<uint16_t>::const_iterator bodyIterator=chunkSeas.at(route.front()).cbegin();
                    while(bodyIterator!=chunkSeas.at(route.front()).cend() && !sameWater)
                    {
                        if(chunkSeas.at(route.back()).find(*bodyIterator)!=chunkSeas.at(route.back()).cend())
                            sameWater=true;
                        ++bodyIterator;
                    }
                }
                //a ferry sails, it does not fly over the land: same water at both ends
                if(route.empty() || route.front()==route.back() || !sameWater
                        || harbourTaken.find(route.front())!=harbourTaken.cend()
                        || harbourTaken.find(route.back())!=harbourTaken.cend()
                        || !chunkCanMoorShip(route.front(),mapXCount,singleMapWidth,singleMapHeight,
                                             worldWidth,shipWidth,shipHeight,setting.waterBoatBorderMin)
                        || !chunkCanMoorShip(route.back(),mapXCount,singleMapWidth,singleMapHeight,
                                             worldWidth,shipWidth,shipHeight,setting.waterBoatBorderMin))
                    boatImpossible[index]=1;
            }
            index++;
        }
        while(boatCount<wantedBoats)
        {
            int shortest=-1;
            unsigned int shortestSize=0;
            index=0;
            while(index<accepted.size())
            {
                if(linkResolved.at(index)!=0 && linkIsBoat.at(index)==0 && boatImpossible.at(index)==0
                        && harbourTaken.find(linkRoute.at(index).front())==harbourTaken.cend()
                        && harbourTaken.find(linkRoute.at(index).back())==harbourTaken.cend())
                    if(shortest<0 || linkRoute.at(index).size()<shortestSize)
                    {
                        shortest=(int)index;
                        shortestSize=(unsigned int)linkRoute.at(index).size();
                    }
                index++;
            }
            if(shortest<0)
            {
                std::cout << "sea: only " << boatCount << " of the " << wantedBoats
                          << " ferries asked for could be moored, the rest stay swimmable lanes"
                          << std::endl;
                break;
            }
            linkIsBoat[(unsigned int)shortest]=1;
            harbourTaken.insert(linkRoute.at((unsigned int)shortest).front());
            harbourTaken.insert(linkRoute.at((unsigned int)shortest).back());
            boatCount++;
        }
    }
    //6) BUILD what was decided
    linkIndex=0;
    while(linkIndex<accepted.size())
    {
        const GroupLink &link=accepted.at(linkIndex);
        if(linkResolved.at(linkIndex)!=0)
        {
            const std::vector<unsigned int> route=linkRoute.at(linkIndex);
            const bool byBoat=(linkIsBoat.at(linkIndex)!=0);
            const unsigned int harbourA=route.back();
            const unsigned int harbourB=route.front();
            linkChunkToNeighbour(link.chunkA,harbourA,mapXCount);
            linkChunkToNeighbour(harbourA,link.chunkA,mapXCount);
            linkChunkToNeighbour(link.chunkB,harbourB,mapXCount);
            linkChunkToNeighbour(harbourB,link.chunkB,mapXCount);
            //THE ROUTE AS THE SEA PASS WILL DRAW IT: land end, the water chunks in
            //travel order, land end. The lane is drawn from beach to beach, so
            //both land ends belong to it.
            {
                WaterRoute waterRoute;
                waterRoute.isBoat=byBoat;
                waterRoute.chunks.push_back(link.chunkA);
                unsigned int routeIndex=(unsigned int)route.size();
                while(routeIndex>0)
                {
                    routeIndex--;
                    waterRoute.chunks.push_back(route.at(routeIndex));
                }
                waterRoute.chunks.push_back(link.chunkB);
                waterRoutes.push_back(waterRoute);
            }
            if(byBoat && harbourA!=harbourB)
            {
                boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)(harbourA%mapXCount),(uint16_t)(harbourA/mapXCount)));
                boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)(harbourB%mapXCount),(uint16_t)(harbourB/mapXCount)));
                waterChunks.push_back(boatChunks.at(boatChunks.size()-2));
                waterChunks.push_back(boatChunks.back());
                BoatCrossing crossing;
                crossing.fromX=(uint16_t)(harbourA%mapXCount);
                crossing.fromY=(uint16_t)(harbourA/mapXCount);
                crossing.toX=(uint16_t)(harbourB%mapXCount);
                crossing.toY=(uint16_t)(harbourB/mapXCount);
                boatCrossings.push_back(crossing);
                byBoatCount++;
            }
            else
            {
                unsigned int routeIndex=0;
                while(routeIndex+1<route.size())
                {
                    linkChunkToNeighbour(route.at(routeIndex),route.at(routeIndex+1),mapXCount);
                    linkChunkToNeighbour(route.at(routeIndex+1),route.at(routeIndex),mapXCount);
                    routeIndex++;
                }
                routeIndex=0;
                while(routeIndex<route.size())
                {
                    waterChunks.push_back(std::pair<uint16_t,uint16_t>(
                                              (uint16_t)(route.at(routeIndex)%mapXCount),
                                              (uint16_t)(route.at(routeIndex)/mapXCount)));
                    routeIndex++;
                }
            }
            std::cout << "sea: link group " << link.groupA << " <-> " << link.groupB
                      << " from " << link.chunkA%mapXCount << "," << link.chunkA/mapXCount
                      << " to " << link.chunkB%mapXCount << "," << link.chunkB/mapXCount
                      << " (" << link.distance << " map(s) apart, "
                      << (byBoat?"boat":"lane") << ", " << route.size() << " sea chunk(s))" << std::endl;
            //the towns of both ends sell what it takes to swim
            unsigned int cityIndex=0;
            while(cityIndex<cities.size())
            {
                const unsigned int cityChunk=cities.at(cityIndex).x+cities.at(cityIndex).y*mapXCount;
                if(groupOfChunk.at(cityChunk)==link.groupA || groupOfChunk.at(cityChunk)==link.groupB)
                {
                    const unsigned int endChunk=(groupOfChunk.at(cityChunk)==link.groupA)?link.chunkA:link.chunkB;
                    const unsigned int distance=(unsigned int)(
                                abs((int)(cityChunk%mapXCount)-(int)(endChunk%mapXCount))
                                +abs((int)(cityChunk/mapXCount)-(int)(endChunk/mapXCount)));
                    if(distance<=setting.waterHarbourChunkRadius)
                        cities[cityIndex].coastal=true;
                }
                cityIndex++;
            }
            built++;
        }
        linkIndex++;
    }
    std::cout << "water paths: " << built << " link(s) of the " << accepted.size()
              << " the mesh asked for, " << byBoatCount << " by boat, "
              << waterChunks.size() << " chunk(s)" << std::endl;
}

//OR the orientation bit of `from` toward its NEIGHBOUR `to` into mapPathDirection
void LoadMapAll::linkChunkToNeighbour(const unsigned int &from,const unsigned int &to,
                                      const unsigned int &mapXCount)
{
    const int fromX=(int)(from%mapXCount);
    const int fromY=(int)(from/mapXCount);
    const int toX=(int)(to%mapXCount);
    const int toY=(int)(to/mapXCount);
    if(toX==fromX-1 && toY==fromY)
        mapPathDirection[from]|=Orientation_left;
    else if(toX==fromX+1 && toY==fromY)
        mapPathDirection[from]|=Orientation_right;
    else if(toX==fromX && toY==fromY-1)
        mapPathDirection[from]|=Orientation_top;
    else if(toX==fromX && toY==fromY+1)
        mapPathDirection[from]|=Orientation_bottom;
    else
        std::cerr << "linkChunkToNeighbour on chunks that do not touch" << std::endl;
}

bool LoadMapAll::checkNoIsolatedMap(const SettingsAll::SettingsExtra &setting,
                                    std::vector<std::string> &errors)
{
    const unsigned int mapXCount=setting.mapXCount;
    const unsigned int mapYCount=setting.mapYCount;
    if(mapPathDirection==NULL || cities.empty())
        return true;
    //start where the player does: the lowest level town
    unsigned int startCity=0;
    {
        unsigned int cityIndex=1;
        while(cityIndex<cities.size())
        {
            if(cities.at(cityIndex).level<cities.at(startCity).level)
                startCity=cityIndex;
            cityIndex++;
        }
    }
    const unsigned int startChunk=cities.at(startCity).x+cities.at(startCity).y*mapXCount;
    std::vector<unsigned char> reached(mapXCount*mapYCount,0);
    std::vector<unsigned int> queue;
    reached[startChunk]=1;
    queue.push_back(startChunk);
    unsigned int queueIndex=0;
    while(queueIndex<queue.size())
    {
        const unsigned int chunk=queue.at(queueIndex);
        queueIndex++;
        const int chunkX=(int)(chunk%mapXCount);
        const int chunkY=(int)(chunk/mapXCount);
        const uint8_t orientation=mapPathDirection[chunk];
        //a border teleport exists in BOTH directions, so following the bits of
        //this chunk is enough
        const int stepX[4]={-1,1,0,0};
        const int stepY[4]={0,0,-1,1};
        const Orientation bit[4]={Orientation_left,Orientation_right,Orientation_top,Orientation_bottom};
        unsigned int direction=0;
        while(direction<4)
        {
            if((orientation&bit[direction])!=0)
            {
                const int nextX=chunkX+stepX[direction];
                const int nextY=chunkY+stepY[direction];
                if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                {
                    const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                    if(reached.at(next)==0 && mapPathDirection[next]!=0)
                    {
                        reached[next]=1;
                        queue.push_back(next);
                    }
                }
            }
            direction++;
        }
        //a boat crossing joins two chunks that do NOT touch
        unsigned int crossingIndex=0;
        while(crossingIndex<boatCrossings.size())
        {
            const BoatCrossing &crossing=boatCrossings.at(crossingIndex);
            const unsigned int fromChunk=crossing.fromX+crossing.fromY*mapXCount;
            const unsigned int toChunk=crossing.toX+crossing.toY*mapXCount;
            unsigned int other=mapXCount*mapYCount;
            if(fromChunk==chunk)
                other=toChunk;
            else if(toChunk==chunk)
                other=fromChunk;
            if(other<mapXCount*mapYCount && reached.at(other)==0)
            {
                reached[other]=1;
                queue.push_back(other);
            }
            crossingIndex++;
        }
    }
    unsigned int isolated=0;
    unsigned int chunk=0;
    while(chunk<mapXCount*mapYCount)
    {
        if(mapPathDirection[chunk]!=0 && reached.at(chunk)==0)
        {
            isolated++;
            if(errors.size()<40)
                errors.push_back(chunkDebugName(chunk%mapXCount,chunk/mapXCount)+
                                 ": no way to reach this map from "+cities.at(startCity).name);
        }
        chunk++;
    }
    std::cout << "connectivity: " << queue.size() << " map(s) reachable from "
              << cities.at(startCity).name << ", " << isolated << " isolated" << std::endl;
    return isolated==0;
}

std::string LoadMapAll::chunkDebugName(const unsigned int &x, const unsigned int &y)
{
    if(haveCityEntry(citiesCoordToIndex,x,y))
    {
        const City &city=cities.at(citiesCoordToIndex.at(x).at(y));
        const char *sizeName="small";
        if(city.type==CityType_medium)
            sizeName="medium";
        else if(city.type==CityType_big)
            sizeName="big";
        return city.name+" ("+std::string(sizeName)+" city)";
    }
    if(roadCoordToIndex.find((uint16_t)x)==roadCoordToIndex.cend()
            || roadCoordToIndex.at((uint16_t)x).find((uint16_t)y)==roadCoordToIndex.at((uint16_t)x).cend())
        return std::string();//no map written for this chunk
    const RoadIndex &roadIndex=roadCoordToIndex.at((uint16_t)x).at((uint16_t)y);
    const Road &road=roads.at(roadIndex.roadIndex);
    std::string name="Road "+std::to_string(roadIndex.roadIndex+1);
    if(road.haveOnlySegmentNearCity)
    {
        //a one-chunk road: it is written INSIDE the city folder it touches
        if(!roadIndex.cityIndex.empty())
            name+=" of "+cities.at(roadIndex.cityIndex.front().cityIndex).name;
    }
    else
        name+=" #"+std::to_string(vectorindexOf(road.coords,std::pair<uint16_t,uint16_t>(x,y))+1);
    if(roadIndex.isCave)
        name+=" (cave)";
    if(roadIndex.isBoat)
        name+=" (boat)";
    else if(roadIndex.isWater)
        name+=" (sea)";
    return name;
}

void LoadMapAll::addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight)
{
    Tiled::ObjectGroup *layerCity=new Tiled::ObjectGroup("City",0,0); // ObjectGroup contructor no longer accept width and height 
    layerCity->setColor(QColor("#bb5555"));
    worldMap.addLayer(layerCity);

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        std::string citySize;
        switch(city.type) {
        case CityType_small:
            citySize="small";
            break;
        case CityType_medium:
            citySize="medium";
            break;
        default:
            citySize="big";
            break;
        }
        const uint32_t x=city.x;
        const uint32_t y=city.y;
        QPolygonF poly(QRectF(0,0,mapWidth-4,mapHeight-4));
        // TODO: Position of MapObject() may need conversion of units from tiles to pixels (API change introduced in v0.10.0)
        Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(city.name+" ("+citySize+", level: "+std::to_string(city.level)+")"),"",QPointF(mapWidth*x+2,mapHeight*y+2), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);
        layerCity->addObject(objectPolygon);

        index++;
    }

    Tiled::ObjectGroup *layerRoad=new Tiled::ObjectGroup("Road",0,0); // ObjectGroup contructor no longer accept width and height 
    layerRoad->setColor(QColor("#ffcc22"));
    worldMap.addLayer(layerRoad);
    const unsigned int w=worldMap.width()/mapWidth;
    //const unsigned int h=worldMap.height()/mapHeight;
    {
        unsigned int indexIntRoad=0;
        while(indexIntRoad<roads.size())
        {
            const Road &road=roads.at(indexIntRoad);
            unsigned int indexCoord=0;
            while(indexCoord<road.coords.size())
            {
                const std::pair<uint16_t,uint16_t> &coord=road.coords.at(indexCoord);
                const unsigned int &x=coord.first;
                const unsigned int &y=coord.second;

                const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
                if(zoneOrientation!=0)
                {
                    //orientation
                    std::vector<std::string> orientationList;
                    if(zoneOrientation&Orientation_bottom)
                        orientationList.push_back("bottom");
                    if(zoneOrientation&Orientation_top)
                        orientationList.push_back("top");
                    if(zoneOrientation&Orientation_left)
                        orientationList.push_back("left");
                    if(zoneOrientation&Orientation_right)
                        orientationList.push_back("right");

                    const RoadIndex &indexRoad=roadCoordToIndex[x][y];

                    //compose string
                    std::string str="Road "+std::to_string(indexRoad.roadIndex+1)+" (";
                    if(road.haveOnlySegmentNearCity)
                    {
                        if(indexRoad.cityIndex.empty())
                        {
                            std::cerr << "road.haveOnlySegmentNearCity and indexRoad.cityIndex.empty()" << std::endl;
                            abort();
                        }
                        const RoadToCity &cityIndex=indexRoad.cityIndex.front();
                        str+=stringimplode(orientationList,",")+","+cities.at(cityIndex.cityIndex).name;
                    }
                    else
                        str+=stringimplode(orientationList,",");
                    str+=", level: "+std::to_string(indexRoad.level)+")";

                    //paint it
                    QPolygonF poly(QRectF(0,0,mapWidth,mapHeight));
                    // TODO: Position of MapObject() may need conversion of units from tiles to pixels (API change introduced in v0.10.0)
                    Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(str),"",QPointF(mapWidth*x,mapHeight*y), QSizeF(0.0,0.0));
                    objectPolygon->setPolygon(poly);
                    objectPolygon->setShape(Tiled::MapObject::Polygon);
                    layerRoad->addObject(objectPolygon);
                }

                indexCoord++;
            }
            indexIntRoad++;
        }
    }
}

bool LoadMapAll::haveCityEntryInternal(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y)
{
    if(positionsAndIndex.find(x)==positionsAndIndex.cend())
        return false;
    const std::unordered_map<uint32_t,CityInternal *> &subEntry=positionsAndIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return true;
}

bool LoadMapAll::haveCityEntry(const std::unordered_map<uint16_t,std::unordered_map<uint16_t,unsigned int> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y)
{
    if(positionsAndIndex.find(x)==positionsAndIndex.cend())
        return false;
    const std::unordered_map<uint16_t,unsigned int> &subEntry=positionsAndIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return true;
}

bool LoadMapAll::haveCityPath(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > &resolvedPath,
                              const unsigned int &x1,const unsigned int &y1,
                              const unsigned int &x2,const unsigned int &y2)
{
    if(resolvedPath.find(x1)!=resolvedPath.cend())
        if(resolvedPath.at(x1).find(y1)!=resolvedPath.at(x1).cend())
            if(resolvedPath.at(x1).at(y1).find(x2)!=resolvedPath.at(x1).at(y1).cend())
                if(resolvedPath.at(x1).at(y1).at(x2).find(y2)!=resolvedPath.at(x1).at(y1).at(x2).cend())
                    return true;
    if(resolvedPath.find(x2)!=resolvedPath.cend())
        if(resolvedPath.at(x2).find(y2)!=resolvedPath.at(x2).cend())
            if(resolvedPath.at(x2).at(y2).find(x1)!=resolvedPath.at(x2).at(y2).cend())
                if(resolvedPath.at(x2).at(y2).at(x1).find(y1)!=resolvedPath.at(x2).at(y2).at(x1).cend())
                    return true;
    return false;
}

LoadMapAll::Orientation LoadMapAll::reverseOrientation(const Orientation &orientation)
{
    switch (orientation) {
    case Orientation_bottom:
            return Orientation_top;
    case Orientation_top:
            return Orientation_bottom;
    case Orientation_left:
            return Orientation_right;
    case Orientation_right:
            return Orientation_left;
    default:
        return orientation;
    }
}

std::string LoadMapAll::orientationToString(const Orientation &orientation)
{
    switch (orientation) {
    case Orientation_bottom:
            return "bottom";
    case Orientation_top:
            return "top";
    case Orientation_left:
            return "left";
    case Orientation_right:
            return "right";
    default:
        return "unknown";
    }
}

void LoadMapAll::addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames,
                         const unsigned int &mapXCount, const unsigned int &mapYCount,
                         const unsigned int &maxCityLinks, const unsigned int &cityRadius, const Simplex &levelmap,
                         const float &levelmapscale, const unsigned int &levelmapmin, const unsigned int &levelmapmax,
                         const SettingsAll::SettingsExtra &setting)
{
    if(grid.empty())
        return;
    std::vector<CityInternal *> citiesInternal;
    std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > resolvedPath;
    const unsigned int singleMapWitdh=worldMap.width()/mapXCount;
    const unsigned int singleMapHeight=worldMap.height()/mapYCount;
    const unsigned int sWH=singleMapWitdh*singleMapHeight;

    std::vector<uint16_t> mapWalkable;
    mapWalkable.resize(mapXCount*mapYCount);
    std::fill(mapWalkable.begin(),mapWalkable.end(),0);
    if(LoadMapAll::mapPathDirection!=NULL)
        delete LoadMapAll::mapPathDirection;
    LoadMapAll::mapPathDirection=new uint8_t[mapXCount*mapYCount];
    for(unsigned int i = 0; i < mapYCount; i++)
        for(unsigned int j = 0; j < mapXCount; j++)
            mapPathDirection[j+i*mapXCount]=0;
    {
        unsigned int y=0;
        while(y<mapYCount)
        {
            unsigned int x=0;
            while(x<mapXCount)
            {
                unsigned int countWalkable=0;
                {
                    //Tiled::TileLayer * tileLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
                    unsigned int yMap=y*singleMapHeight;
                    while(yMap<(y*singleMapHeight+singleMapHeight))
                    {
                        unsigned int xMap=x*singleMapWitdh;
                        while(xMap<(x*singleMapWitdh+singleMapWitdh))
                        {
                            const unsigned int zoneIndex=VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[xMap+yMap*worldMap.width()].index;
                            const VoronioForTiledMapTmx::PolygonZone &polygonZone=VoronioForTiledMapTmx::voronoiMap.zones.at(zoneIndex);
                            /*
                            if(tileLayer->cellAt(xMap,yMap).tile!=NULL)
                                countWalkable++;walkable not fill at this call*/
                            if(polygonZone.height>0)
                                countWalkable++;
                            xMap++;
                        }
                        yMap++;
                    }
                }
                if(x+y*mapXCount>=mapWalkable.size())
                    abort();
                mapWalkable[x+y*mapXCount]=countWalkable;
                x++;
            }
            y++;
        }
    }
    if(grid.size()>citiesNames.size())
    {
        std::cerr << "Need more cities name, have: " << citiesNames.size() << " but need " << grid.size() << std::endl;
        abort();
    }
    std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > positionsAndIndex;
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);

        const uint32_t x=centroid.x();
        const uint32_t y=centroid.y();
        if(!haveCityEntryInternal(positionsAndIndex,x,y))
        {
            CityInternal *city=new CityInternal();
            //do the random value
            city->name=citiesNames.at(index);
            switch(rand()%3) {
            case 0:
                city->type=CityType_small;
                break;
            case 1:
                city->type=CityType_medium;
                break;
            default:
                city->type=CityType_big;
                break;
            }
            city->x=x;
            city->y=y;
            //count walkable tile
            unsigned int countWalkable=mapWalkable.at(x+y*mapXCount);
            //add
            if(countWalkable*100/(sWH)>75)
            {
                positionsAndIndex[x][y]=city;
                citiesInternal.push_back(city);
            }
        }

        index++;
    }

    //do top list of map with number of direct neighbor
    std::map<uint32_t,std::vector<CityInternal *> > citiesByNeighbor;
    index=0;
    while(index<citiesInternal.size())
    {
        CityInternal *city=citiesInternal.at(index);
        if(city->x>0)
        {
            if(city->y>0)
                if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y-1));
            if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y));
            if(city->y<(mapYCount-1))
                if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y+1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y+1));
        }
        if(city->y>0)
            if(haveCityEntryInternal(positionsAndIndex,city->x,city->y-1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y-1));
        if(city->y<(mapYCount-1))
            if(haveCityEntryInternal(positionsAndIndex,city->x,city->y+1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y+1));
        if(city->x<(mapXCount-1))
        {
            if(city->y>0)
                if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y-1));
            if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y));
            if(city->y<(mapYCount-1))
                if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y+1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y+1));
        }
        citiesByNeighbor[city->citiesNeighbor.size()].push_back(city);
        index++;
    }
    //drop the first and decremente their neighbor
    while(citiesByNeighbor.size()>1 || (citiesByNeighbor.find(0)==citiesByNeighbor.cend() && citiesByNeighbor.size()==1))
    {
        std::map<uint32_t,std::vector<CityInternal *> >::reverse_iterator rit;
        for(rit=citiesByNeighbor.rbegin(); rit!=citiesByNeighbor.rend(); ++rit) {
            uint32_t indexCity=rit->first;
            std::vector<CityInternal *> citiesList=rit->second;
            if(!citiesList.empty())
            {
                CityInternal * city=citiesList.front();
                if(city->citiesNeighbor.empty())
                    abort();
                unsigned int index=0;
                while(index<city->citiesNeighbor.size())
                {
                    CityInternal * cityNeighbor=city->citiesNeighbor.at(index);
                    const unsigned int oldCount=cityNeighbor->citiesNeighbor.size();
                    //remove from local
                    if(!vectorremoveOne(cityNeighbor->citiesNeighbor,city))
                        abort();
                    //remove into the global list
                    if(!vectorremoveOne(citiesByNeighbor[oldCount],cityNeighbor))
                        abort();
                    if(citiesByNeighbor[oldCount].empty())
                        citiesByNeighbor.erase(oldCount);
                    const unsigned int newCount=cityNeighbor->citiesNeighbor.size();
                    citiesByNeighbor[newCount].push_back(cityNeighbor);
                    index++;
                }
                if(!vectorremoveOne(citiesByNeighbor[indexCity],city))
                    abort();
                if(citiesByNeighbor[indexCity].empty())
                    citiesByNeighbor.erase(indexCity);
            }
            else
                citiesByNeighbor.erase(indexCity);
            break;
        }
    }
    //detect the min and max level
    unsigned int maxLevel=0;
    unsigned int minLevel=999999;
    //happen
    citiesCoordToIndex.clear();
    cities.clear();
    for(const auto& n:citiesByNeighbor) {
        const std::vector<CityInternal *> &citiesList=n.second;
        unsigned int index=0;
        while(index<citiesList.size())
        {
            const CityInternal *cityInternal=citiesList.at(index);
            City city;
            city.coastal=false;
            city.name=cityInternal->name;
            city.type=cityInternal->type;
            city.x=cityInternal->x;
            city.y=cityInternal->y;
            city.level=(levelmap.Get({(float)city.x,(float)city.y},levelmapscale)+1.0)/2.0*(levelmapmax-levelmapmin)+levelmapmin;
            if(city.level<minLevel)
                minLevel=city.level;
            if(city.level>maxLevel)
                maxLevel=city.level;
            citiesCoordToIndex[city.x][city.y]=cities.size();
            cities.push_back(city);
            delete cityInternal;
            index++;
        }
        break;
    }
    //calculate the zone
    {
        unsigned int indexCities=0;
        while(indexCities<cities.size())
        {
            City &city=cities[indexCities];
            //Calibration
            city.level=city.level-(minLevel-levelmapmin);
            if(city.level<levelmapmin)
                city.level=levelmapmin;

            int minX=(int)city.x-cityRadius;
            int maxX=(int)city.x+cityRadius;
            int minY=(int)city.y-cityRadius;
            int maxY=(int)city.y+cityRadius;
            if(minX<0)
                minX=0;
            if(maxX>(int)mapXCount)
                maxX=mapXCount;
            if(minY<0)
                minY=0;
            if(maxY>(int)mapYCount)
                maxY=mapYCount;

            //resolv the path
            std::vector<MapPointToParse> mapPointToParseList;
            SimplifiedMapForPathFinding tempMap;

            //init the first case
            {
                MapPointToParse tempPoint;
                tempPoint.x=city.x;
                tempPoint.y=city.y;
                mapPointToParseList.push_back(tempPoint);

                std::pair<uint16_t,uint16_t> coord(tempPoint.x,tempPoint.y);
                tempMap.pathToGo[coord].left.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1)
                            );
                tempMap.pathToGo[coord].right.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1)
                            );
                tempMap.pathToGo[coord].bottom.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1)
                            );
                tempMap.pathToGo[coord].top.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1)
                            );
            }

            if(maxCityLinks<2)
            {
                std::cerr << "maxCityLinks<2 abort" << std::endl;
                abort();
            }
            uint8_t citycount=0;
            std::pair<uint16_t,uint16_t> coord;
            while(!mapPointToParseList.empty() && citycount<maxCityLinks)
            {
                const MapPointToParse tempPoint=mapPointToParseList.at(0);
                mapPointToParseList.erase(mapPointToParseList.begin());
                SimplifiedMapForPathFinding::PathToGo pathToGo;
                //resolv the own point
                {
                    //if the right case have been parsed
                    if(tempPoint.x+1<maxX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x+1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                        {
                            const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                            if(pathToGo.left.empty() || pathToGo.left.size()>nearPathToGo.left.size())
                            {
                                pathToGo.left=nearPathToGo.left;
                                pathToGo.left.back().second++;
                            }
                            if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                            {
                                pathToGo.top=nearPathToGo.left;
                                pathToGo.top.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1));
                            }
                            if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                            {
                                pathToGo.bottom=nearPathToGo.left;
                                pathToGo.bottom.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1));
                            }
                        }
                    }
                    //if the left case have been parsed
                    if(tempPoint.x>minX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x-1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                        {
                            const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                            if(pathToGo.right.empty() || pathToGo.right.size()>nearPathToGo.right.size())
                            {
                                pathToGo.right=nearPathToGo.right;
                                pathToGo.right.back().second++;
                            }
                            if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                            {
                                pathToGo.top=nearPathToGo.right;
                                pathToGo.top.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1));
                            }
                            if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                            {
                                pathToGo.bottom=nearPathToGo.right;
                                pathToGo.bottom.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1));
                            }
                        }
                    }
                    //if the top case have been parsed
                    if(tempPoint.y+1<maxY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y+1);
                        if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                        {
                            const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                            if(pathToGo.top.empty() || pathToGo.top.size()>nearPathToGo.top.size())
                            {
                                pathToGo.top=nearPathToGo.top;
                                pathToGo.top.back().second++;
                            }
                            if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                            {
                                pathToGo.left=nearPathToGo.top;
                                pathToGo.left.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1));
                            }
                            if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                            {
                                pathToGo.right=nearPathToGo.top;
                                pathToGo.right.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1));
                            }
                        }
                    }
                    //if the bottom case have been parsed
                    if(tempPoint.y>minY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y-1);
                        if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                        {
                            const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                            if(pathToGo.bottom.empty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                            {
                                pathToGo.bottom=nearPathToGo.bottom;
                                pathToGo.bottom.back().second++;
                            }
                            if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                            {
                                pathToGo.left=nearPathToGo.bottom;
                                pathToGo.left.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1));
                            }
                            if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                            {
                                pathToGo.right=nearPathToGo.bottom;
                                pathToGo.right.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1));
                            }
                        }
                    }
                }
                coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y);
                if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                {
                    // extraControlOnData() calls used to live here under
                    // CATCHCHALLENGER_HARDENED, but the function was never
                    // defined — dead code that compiled only because the
                    // HARDENED guard was off by default on non-test builds
                    // and accidentally became reachable when testing*.py
                    // started forcing HARDENED=ON.
                    tempMap.pathToGo[coord]=pathToGo;
                }
                if(haveCityEntry(citiesCoordToIndex,tempPoint.x,tempPoint.y) && (tempPoint.x!=city.x || tempPoint.y!=city.y))
                {
                    citycount++;
                    std::vector<std::pair<Orientation,uint8_t/*step number*/> > returnedVar;
                    if(returnedVar.empty() || pathToGo.bottom.size()<returnedVar.size())
                        if(!pathToGo.bottom.empty())
                            returnedVar=pathToGo.bottom;
                    if(returnedVar.empty() || pathToGo.top.size()<returnedVar.size())
                        if(!pathToGo.top.empty())
                            returnedVar=pathToGo.top;
                    if(returnedVar.empty() || pathToGo.right.size()<returnedVar.size())
                        if(!pathToGo.right.empty())
                            returnedVar=pathToGo.right;
                    if(returnedVar.empty() || pathToGo.left.size()<returnedVar.size())
                        if(!pathToGo.left.empty())
                            returnedVar=pathToGo.left;
                    //just display
                    const bool displayPath=false;
                    if(displayPath)
                    {
                        std::cout << "city from " << city.x << "," << city.y << " to " << tempPoint.x << "," << tempPoint.y << ": ";
                        unsigned int index=0;
                        while(index<returnedVar.size())
                        {
                            std::pair<Orientation,uint8_t/*step number*/> &returnedLine=returnedVar[index];
                            unsigned int indexSecond=0;
                            while(indexSecond<returnedLine.second)
                            {
                                //change tile
                                if(returnedLine.first!=Orientation_none)
                                {
                                    switch(returnedLine.first)
                                    {
                                        case Orientation_bottom:std::cout << "b";break;
                                        case Orientation_top:std::cout << "t";break;
                                        case Orientation_right:std::cout << "r";break;
                                        case Orientation_left:std::cout << "l";break;
                                        default:abort();
                                    }
                                }
                                indexSecond++;
                            }
                            index++;
                        }
                        std::cout << std::endl;
                    }
                    if(!returnedVar.empty())
                    {
                        if(returnedVar.back().second<=1)
                        {
                            std::cerr << "Bug due for last step" << std::endl;
                            abort();
                        }
                        else
                        {
                            returnedVar.back().second--;
                            if(!haveCityPath(resolvedPath,city.x,city.y,tempPoint.x,tempPoint.y))
                            {
                                resolvedPath[city.x][city.y][tempPoint.x].insert(tempPoint.y);
                                if(haveCityEntry(citiesCoordToIndex,tempPoint.x,tempPoint.y) && (tempPoint.x!=city.x || tempPoint.y!=city.y))
                                {
                                    uint16_t x=city.x;
                                    uint16_t y=city.y;
                                    unsigned int index=0;
                                    while(index<returnedVar.size())
                                    {
                                        std::pair<Orientation,uint8_t/*step number*/> &returnedLine=returnedVar[index];
                                        while(returnedLine.second>0)
                                        {
                                            mapPathDirection[x+y*mapXCount]|=returnedLine.first;
                                            //change tile
                                            if(returnedLine.first!=Orientation_none)
                                            {
                                                switch(returnedLine.first)
                                                {
                                                    case Orientation_bottom:y++;break;
                                                    case Orientation_top:y--;break;
                                                    case Orientation_right:x++;break;
                                                    case Orientation_left:x--;break;
                                                    default:abort();
                                                }
                                                mapPathDirection[x+y*mapXCount]|=LoadMapAll::reverseOrientation(returnedLine.first);
                                                returnedLine.second--;
                                            }
                                        }
                                        index++;
                                    }
                                }
                            }
                            /// \todo the city path
                        }
                    }
                    else
                    {
                        returnedVar.clear();
                        std::cerr << "Bug due to resolved path is empty" << std::endl;
                        abort();
                    }
                }
                //revers resolv
                //add to point to parse
                {
                    //if the right case have been parsed
                    if(tempPoint.x+1<maxX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x+1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            newPoint.x++;
                            if(newPoint.x<maxX)
                            {
                                if(newPoint.x+newPoint.y*mapXCount>=mapWalkable.size())
                                    abort();
                                if(mapWalkable.at(newPoint.x+newPoint.y*mapXCount)*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                            }
                        }
                    }
                    //if the left case have been parsed
                    if(tempPoint.x>minX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x-1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.x>minX)
                            {
                                newPoint.x--;
                                if(newPoint.x+newPoint.y*mapXCount>=mapWalkable.size())
                                    abort();
                                if(mapWalkable.at(newPoint.x+newPoint.y*mapXCount)*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                            }
                        }
                    }
                    //if the bottom case have been parsed
                    if(tempPoint.y+1<maxY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y+1);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            newPoint.y++;
                            if(newPoint.y<maxY)
                            {
                                if(newPoint.x+newPoint.y*mapXCount>=mapWalkable.size())
                                    abort();
                                if(mapWalkable.at(newPoint.x+newPoint.y*mapXCount)*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                            }
                        }
                    }
                    //if the top case have been parsed
                    if(tempPoint.y>minY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y-1);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.y>minY)
                            {
                                newPoint.y--;
                                if(newPoint.x+newPoint.y*mapXCount>=mapWalkable.size())
                                    abort();
                                if(mapWalkable.at(newPoint.x+newPoint.y*mapXCount)*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                            }
                        }
                    }
                }


            }

            indexCities++;
        }
    }
    //SEA ROUTES, before the grouping: they ride the same graph as the land roads,
    //so everything below (chunk grouping, city links, level, wild monsters, and
    //later the border teleports and the minimap) applies to them unchanged.
    std::vector<std::pair<uint16_t,uint16_t> > waterChunks;
    std::vector<std::pair<uint16_t,uint16_t> > boatChunks;
    addWaterPaths(mapXCount,mapYCount,singleMapWitdh,singleMapHeight,
                  (unsigned int)worldMap.width(),setting,waterChunks,boatChunks);

    //Do the road group
    {
        unsigned int mapY=0;
        while(mapY<mapYCount)
        {
            unsigned int mapX=0;
            while(mapX<mapXCount)
            {
                if(mapPathDirection[mapX+mapY*mapXCount]!=0 &&
                        !haveCityEntry(citiesCoordToIndex,mapX,mapY) &&
                        (roadCoordToIndex.find(mapX)==roadCoordToIndex.cend() || roadCoordToIndex.at(mapX).find(mapY)==roadCoordToIndex.at(mapX).cend())
                        )
                {
                    unsigned int roadIntIndex=roads.size();;
                    {
                        Road road;
                        road.haveOnlySegmentNearCity=true;
                        roads.push_back(road);
                    }
                    Road &road=roads.back();
                    std::vector<uint32_t> pointToScan;
                    std::vector<uint32_t> pointDone;
                    pointToScan.push_back(mapX+mapY*mapXCount);
                    while(!pointToScan.empty())
                    {
                        const uint16_t x=pointToScan.front()%mapXCount;
                        const uint16_t y=(pointToScan.front()-x)/mapXCount;
                        pointDone.push_back(pointToScan.front());
                        pointToScan.erase(pointToScan.cbegin());

                        RoadIndex roadIndex;
                        roadIndex.roadIndex=roadIntIndex;
                        roadIndex.level=0;
                        roadIndex.isCave=false;
                        roadIndex.isWater=false;
                        roadIndex.isBoat=false;

                        //left tile
                        if(x>0)
                        {
                            const uint16_t newX=x-1;
                            const uint16_t newY=y;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_left;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_right);
                            }
                        }
                        //right tile
                        if(x<(mapXCount-1))
                        {
                            const uint16_t newX=x+1;
                            const uint16_t newY=y;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_right;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_left);
                            }
                        }
                        //top tile
                        if(y>0)
                        {
                            const uint16_t newX=x;
                            const uint16_t newY=y-1;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_top;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_bottom);
                            }
                        }
                        //bottom tile
                        if(y<(mapYCount-1))
                        {
                            const uint16_t newX=x;
                            const uint16_t newY=y+1;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_bottom;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_top);
                            }
                        }

                        road.coords.push_back(std::pair<uint16_t,uint16_t>(x,y));
                        roadCoordToIndex[x][y]=roadIndex;
                        if(roadIndex.cityIndex.empty())
                            road.haveOnlySegmentNearCity=false;
                    }
                }
                mapX++;
            }
            mapY++;
        }
    }

    //mark which of the freshly grouped chunks are SEA, and which of those are the
    //closed ends of a boat crossing. Done after the grouping so a water chunk is
    //an ordinary road everywhere except where its content is painted.
    {
        unsigned int waterIndex=0;
        while(waterIndex<waterChunks.size())
        {
            const std::pair<uint16_t,uint16_t> &chunk=waterChunks.at(waterIndex);
            if(roadCoordToIndex.find(chunk.first)!=roadCoordToIndex.cend()
                    && roadCoordToIndex.at(chunk.first).find(chunk.second)!=roadCoordToIndex.at(chunk.first).cend())
                roadCoordToIndex[chunk.first][chunk.second].isWater=true;
            waterIndex++;
        }
        unsigned int boatIndex=0;
        while(boatIndex<boatChunks.size())
        {
            const std::pair<uint16_t,uint16_t> &chunk=boatChunks.at(boatIndex);
            if(roadCoordToIndex.find(chunk.first)!=roadCoordToIndex.cend()
                    && roadCoordToIndex.at(chunk.first).find(chunk.second)!=roadCoordToIndex.at(chunk.first).cend())
                roadCoordToIndex[chunk.first][chunk.second].isBoat=true;
            boatIndex++;
        }
    }

    //set the road level
    std::unordered_map<unsigned int,unsigned int> monsterRoadSpawnCount;
    unsigned int roadParsed=0;
    for(auto& p:LoadMapAll::roadCoordToIndex)
    {
        const unsigned int &x=p.first;
        for(auto& q:p.second)
        {
            const unsigned int &y=q.first;
            LoadMapAll::RoadIndex &roadIndex=q.second;
            unsigned int tempLevel=(levelmap.Get({(float)x,(float)y},levelmapscale)+1.0)/2.0*(levelmapmax-levelmapmin)+levelmapmin;
            int temp_roadIndex=tempLevel-(minLevel-levelmapmin);
            //clamp the calibrated level (was reading the not-yet-set roadIndex.level)
            if(temp_roadIndex<(int)levelmapmin)
                temp_roadIndex=levelmapmin;
            if(temp_roadIndex>255)
            {
                std::cerr << "roadIndexLevel>255, WARN!" << std::endl;
                abort();
            }
            roadIndex.level=temp_roadIndex;
            float roadIndexLevel=roadIndex.level;
            uint8_t levelDiff=roadIndexLevel*0.1;
            if(levelDiff<2)
                levelDiff=2;
            if(levelDiff>25)
            {
                std::cerr << "levelDiff>25" << std::endl;
                abort();
            }
            std::vector<uint8_t> levelRange;
            uint8_t inferiorLevel=roadIndex.level-levelDiff;
            if(inferiorLevel<2)
                inferiorLevel=2;
            while(inferiorLevel<=(roadIndex.level+levelDiff))
            {
                levelRange.push_back(inferiorLevel);
                if(inferiorLevel>=255)//work around for roadIndex.level==255
                    break;
                inferiorLevel++;
            }
            if(levelRange.size()<2)
            {
                std::cerr << "levelRange.size()<2" << std::endl;
                abort();
            }
            std::vector<std::pair<uint8_t,uint8_t> > minMaxLevel;
            uint8_t minMaxLevelIndex=0;
            while(minMaxLevelIndex<8)
            {
                //fixed level
                if(minMaxLevelIndex%2==0)
                {
                    uint8_t randomIndex=rand()%levelRange.size();
                    minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndex),levelRange.at(randomIndex)));
                }
                else
                {
                    uint8_t randomIndexL=rand()%levelRange.size();
                    uint8_t randomIndexT=0;
                    do {
                        randomIndexT=rand()%levelRange.size();
                    } while(randomIndexT==randomIndexL);
                    if(randomIndexL<randomIndexT)
                        minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndexL),levelRange.at(randomIndexT)));
                    else
                        minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndexT),levelRange.at(randomIndexL)));
                }
                minMaxLevelIndex++;
            }

            //for now fixed number of monster
            const unsigned int numberOfMonster=5;
            //The wild monsters of a road must match the terrain the player walks on,
            //so take the terrain of the voronoi zone at the CENTRE of that chunk:
            //the very zone the chunk is painted from. Re-sampling the noise here is
            //a trap and was wrong: x and y are CHUNK indices while the terrain
            //samples world tiles scaled by VoronioForTiledMapTmx::SCALE, so it read
            //a completely different place of the noise - and the moisure sample was
            //missing its /100 on x on top of that.
            const unsigned int centerX=x*singleMapWitdh+singleMapWitdh/2;
            const unsigned int centerY=y*singleMapHeight+singleMapHeight/2;
            const VoronioForTiledMapTmx::PolygonZone &centerZone=
                    VoronioForTiledMapTmx::voronoiMap.zones.at(
                        VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[centerX+centerY*worldMap.width()].index);
            const unsigned int height=centerZone.height;
            //moisure is 1-6, terrainList is indexed 0-5 like everywhere else
            const unsigned int moisureIndex=centerZone.moisure-1;
            //take the monster list and clean it
            std::map<unsigned int,std::vector<LoadMap::TerrainMonster> > terrainMonsterMap;
            std::map<unsigned int,std::vector<LoadMap::TerrainMonster> > terrainMonsterMapBack;
            //keep the higthest number with the percent at more than 30%
            unsigned int hightestLuck=0;
            for(const auto& z:LoadMap::terrainList[height][moisureIndex].terrainMonsters)
            {
                const unsigned int luckWeight=z.first;
                if(hightestLuck<luckWeight)
                {
                    hightestLuck=luckWeight;
                    std::vector<LoadMap::TerrainMonster> monsters=z.second;
                    unsigned int monsterIndex=0;
                    while(monsterIndex<monsters.size())
                    {
                        const LoadMap::TerrainMonster &monster=monsters.at(monsterIndex);
                        if(monster.mapweight>30)
                            monsters.erase(monsters.cbegin()+monsterIndex);
                        else
                            monsterIndex++;
                    }
                    if(!monsters.empty())
                    {
                        terrainMonsterMapBack.clear();
                        terrainMonsterMapBack[luckWeight]=monsters;
                    }
                    else if(terrainMonsterMapBack.empty())
                        terrainMonsterMapBack[luckWeight]=z.second;
                }
            }
            //drop if current spawn rate on populated map is > rate
            for(const auto& z:LoadMap::terrainList[height][moisureIndex].terrainMonsters)
            {
                const unsigned int luckWeight=z.first;
                std::vector<LoadMap::TerrainMonster> monsters=z.second;
                unsigned int monsterIndex=0;
                while(monsterIndex<monsters.size())
                {
                    const LoadMap::TerrainMonster &monster=monsters.at(monsterIndex);
                    if(monsterRoadSpawnCount.find(monster.monster)!=monsterRoadSpawnCount.cend())
                    {
                        if(roadParsed>0)
                        {
                            unsigned int spawnCount=monsterRoadSpawnCount.at(monster.monster);
                            if(spawnCount*100/roadParsed>monster.mapweight)
                                monsters.erase(monsters.cbegin()+monsterIndex);
                            else
                                monsterIndex++;
                        }
                        else
                            monsterIndex++;
                    }
                    else
                        monsterIndex++;
                }
                if(!monsters.empty())
                    terrainMonsterMap[luckWeight]=monsters;
            }
            if(terrainMonsterMap.empty())
                terrainMonsterMap=terrainMonsterMapBack;

            if(!terrainMonsterMap.empty())
            {
                //take proportional  random index into terrainMonsters
                std::vector<unsigned int> indexesProportional;
                for(auto const &it : terrainMonsterMap)
                    indexesProportional.insert(indexesProportional.cend(),it.first,it.first);

                float luckSum=0;
                unsigned int numberOfMonsterIndex=0;
                while(numberOfMonsterIndex<numberOfMonster && !terrainMonsterMap.empty())
                {
                    //take proportional  random index into terrainMonsters
                    unsigned int indexGroupMonster=indexesProportional.at(rand()%indexesProportional.size());
                    //take random monster
                    const uint8_t randomLevelIndex=rand()%minMaxLevel.size();
                    if(terrainMonsterMap.find(indexGroupMonster)==terrainMonsterMap.cend())
                    {
                        std::cerr << "terrainMonsterMap.find(indexGroupMonster)==terrainMonsterMap.cend()" << std::endl;
                        abort();
                    }
                    std::vector<LoadMap::TerrainMonster> &localLuckMonster=terrainMonsterMap[indexGroupMonster];
                    const uint8_t randomMonsterIndex=rand()%localLuckMonster.size();
                    const LoadMap::TerrainMonster &terrainMonster=localLuckMonster.at(randomMonsterIndex);
                    LoadMapAll::RoadMonster roadMonster;
                    roadMonster.luck=indexGroupMonster;
                    luckSum+=roadMonster.luck;
                    roadMonster.minLevel=minMaxLevel.at(randomLevelIndex).first;
                    roadMonster.maxLevel=minMaxLevel.at(randomLevelIndex).second;
                    roadMonster.monsterId=terrainMonster.monster;
                    if(monsterRoadSpawnCount.find(roadMonster.monsterId)!=monsterRoadSpawnCount.cend())
                        monsterRoadSpawnCount[roadMonster.monsterId]++;
                    else
                        monsterRoadSpawnCount[roadMonster.monsterId]=1;
                    roadIndex.roadMonsters.push_back(roadMonster);

                    //remove the entry to drop duplicate
                    localLuckMonster.erase(localLuckMonster.cbegin()+randomMonsterIndex);
                    if(localLuckMonster.empty())
                    {
                        unsigned int index=0;
                        while(index<indexesProportional.size())
                        {
                            if(indexesProportional.at(index)==indexGroupMonster)
                                indexesProportional.erase(indexesProportional.cbegin()+index);
                            else
                                index++;
                        }
                        terrainMonsterMap.erase(terrainMonsterMap.find(indexGroupMonster));
                    }

                    numberOfMonsterIndex++;
                }
                //normalise the luck
                {
                    const float &ratioLuck=(float)100.0/luckSum;
                    numberOfMonsterIndex=0;
                    unsigned int newLuckSum=0;
                    while(numberOfMonsterIndex<roadIndex.roadMonsters.size())
                    {
                        LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[numberOfMonsterIndex];
                        roadMonster.luck*=ratioLuck;
                        if(roadMonster.luck<1)
                            roadMonster.luck=1;
                        newLuckSum+=roadMonster.luck;
                        numberOfMonsterIndex++;
                    }
                    while(newLuckSum<100)
                    {
                        LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[rand()%roadIndex.roadMonsters.size()];
                        roadMonster.luck++;
                        newLuckSum++;
                    }
                    while(newLuckSum>100)
                    {
                        LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[rand()%roadIndex.roadMonsters.size()];
                        if(roadMonster.luck>1)
                        {
                            roadMonster.luck--;
                            newLuckSum--;
                        }
                    }
                }
                //to drop random list and improve the compression ratio
                std::sort(roadIndex.roadMonsters.begin(),roadIndex.roadMonsters.end(),[](LoadMapAll::RoadMonster a, LoadMapAll::RoadMonster b) {
                    return b.monsterId < a.monsterId;
                });
                if(roadIndex.roadMonsters.empty())
                {
                    std::cerr << "!terrainMonsterMap.empty() && roadIndex.roadMonsters.empty() (abort)" << std::endl;
                    abort();
                }
            }
        }
        roadParsed++;
    }
}
