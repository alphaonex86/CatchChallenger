#include "LoadMapAll.h"

#include <libtiled/mapobject.h>
#include <libtiled/objectgroup.h>
#include "../../general/base/cpp11addition.hpp"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>

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
static bool carveChunkCorridor(Tiled::Map &worldMap,const std::vector<Tiled::TileLayer*> &collisionLayers,
                               const std::vector<uint16_t> &component,const uint16_t fromComponent,
                               const unsigned int toCell,
                               const unsigned int chunkX,const unsigned int chunkY,
                               const unsigned int mapWidth,const unsigned int mapHeight)
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
                        const unsigned int stepCost=(component.at(next)==walkNoComponent)?costBlocked:costOpen;
                        if(cost.at(cell)+stepCost<cost.at(next))
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
        if(groundTile!=NULL)
        {
            if(walkLayer->cellAt(tileX,tileY).tile()==NULL)
                walkLayer->setCell(tileX,tileY,Tiled::Cell(groundTile));
        }
        else if(waterLayer->cellAt(tileX,tileY).tile()==NULL)
            waterLayer->setCell(tileX,tileY,Tiled::Cell(waterTile));
        walkCell=parent.at((unsigned int)walkCell);
    }
    return true;
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

    //the cells the player crosses a border on, and the doorsteps of the
    //buildings, both read from the Moving group the generator itself filled
    std::map<std::pair<unsigned int,unsigned int>,std::vector<std::pair<unsigned int,unsigned int> > > borderCells;
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
                    //border-bottom sits on the FIRST row of the next chunk: the cell
                    //the player actually walks from is the last row of this one
                    if(type=="border-bottom")
                        tileY--;
                    if(tileY>=0)
                    {
                        const std::pair<unsigned int,unsigned int> chunk((unsigned int)tileX/mapWidth,
                                                                        (unsigned int)tileY/mapHeight);
                        borderCells[chunk].push_back(std::pair<unsigned int,unsigned int>(
                                                         (unsigned int)tileX%mapWidth,(unsigned int)tileY%mapHeight));
                    }
                }
                else if(type=="door" || type=="teleport on it" || type=="teleport on push")
                {
                    const std::pair<unsigned int,unsigned int> chunk((unsigned int)tileX/mapWidth,
                                                                    (unsigned int)tileY/mapHeight);
                    doorCells[chunk].push_back(std::pair<unsigned int,unsigned int>(
                                                   (unsigned int)tileX%mapWidth,(unsigned int)tileY%mapHeight));
                }
            }
            objectIndex++;
        }
    }

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
                //1) the border openings
                if(borderCells.find(chunk)!=borderCells.cend())
                {
                    const std::vector<std::pair<unsigned int,unsigned int> > &cells=borderCells.at(chunk);
                    uint16_t reference=walkNoComponent;
                    unsigned int cellIndex=0;
                    while(cellIndex<cells.size())
                    {
                        const unsigned int cell=cells.at(cellIndex).first+cells.at(cellIndex).second*mapWidth;
                        const uint16_t cellComponent=component.at(cell);
                        const std::string where=std::to_string(cells.at(cellIndex).first)+","+
                                std::to_string(cells.at(cellIndex).second);
                        if(cellComponent==walkNoComponent)
                        {
                            //the opening itself is walled: open it and everything
                            //between it and the biggest component
                            if(!reportPass && biggestComponent!=walkNoComponent)
                                repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                             biggestComponent,cell,chunkX,chunkY,
                                                             mapWidth,mapHeight);
                            else if(reportPass)
                            {
                                errors.push_back(chunkName+": the border opening "+where+" is a collision");
                                brokenBorders++;
                            }
                        }
                        else if(chunkIsCave)
                        {
                            //this side must open on a cave mouth, else the player
                            //walks in and is stuck in a dead-end pocket
                            int nearestMouth=-1;
                            bool reachesAMouth=false;
                            if(doorCells.find(chunk)!=doorCells.cend())
                            {
                                const std::vector<std::pair<unsigned int,unsigned int> > &doors=doorCells.at(chunk);
                                unsigned int doorIndex=0;
                                while(doorIndex<doors.size())
                                {
                                    const unsigned int doorCell=doors.at(doorIndex).first+doors.at(doorIndex).second*mapWidth;
                                    if(component.at(doorCell)==cellComponent)
                                        reachesAMouth=true;
                                    //nearest by chunk distance, the corridor cost does the rest
                                    if(nearestMouth<0
                                            || (abs((int)(doorCell%mapWidth)-(int)(cell%mapWidth))
                                                +abs((int)(doorCell/mapWidth)-(int)(cell/mapWidth)))
                                               <(abs((int)((unsigned int)nearestMouth%mapWidth)-(int)(cell%mapWidth))
                                                 +abs((int)((unsigned int)nearestMouth/mapWidth)-(int)(cell/mapWidth))))
                                        nearestMouth=(int)doorCell;
                                    doorIndex++;
                                }
                            }
                            if(!reachesAMouth)
                            {
                                //join this border to its own mouth ONLY: joining the
                                //two borders would let the player walk around the
                                //cave, which is the whole point of the chunk
                                if(!reportPass && nearestMouth>=0)
                                    repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                                 cellComponent,(unsigned int)nearestMouth,
                                                                 chunkX,chunkY,mapWidth,mapHeight);
                                else if(reportPass)
                                {
                                    errors.push_back(chunkName+": the border opening "+where+
                                                     " opens on a dead end, no cave mouth is reachable from it");
                                    brokenBorders++;
                                }
                            }
                        }
                        else if(reference==walkNoComponent)
                            reference=cellComponent;
                        else if(cellComponent!=reference)
                        {
                            if(!reportPass)
                                repaired|=carveChunkCorridor(worldMap,collisionLayers,component,
                                                             reference,cell,chunkX,chunkY,
                                                             mapWidth,mapHeight);
                            else
                            {
                                errors.push_back(chunkName+": the border opening "+where+
                                                 " cannot be walked to from the other borders of the chunk");
                                brokenBorders++;
                            }
                        }
                        cellIndex++;
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
                if(repaired)
                    repairedChunks++;
                repairPass++;
                }
            }
            chunkX++;
        }
        chunkY++;
    }
    std::cout << "walkability: " << repairedChunks << " chunk(s) repaired, "
              << brokenBorders << " broken border link(s) left, "
              << unreachableDoors << " unreachable door(s) left" << std::endl;
    return errors.empty();
}

std::vector<LoadMapAll::BoatCrossing> LoadMapAll::boatCrossings;
std::map<std::pair<uint16_t,uint16_t>,std::pair<uint8_t,uint8_t> > LoadMapAll::boatLandingCells;
std::map<std::pair<uint16_t,uint16_t>,Tiled::MapObject*> LoadMapAll::boatTeleportObjects;

void LoadMapAll::wireBoatCrossings()
{
    unsigned int crossingIndex=0;
    unsigned int wired=0;
    while(crossingIndex<boatCrossings.size())
    {
        const BoatCrossing &crossing=boatCrossings.at(crossingIndex);
        const std::pair<uint16_t,uint16_t> from(crossing.fromX,crossing.fromY);
        const std::pair<uint16_t,uint16_t> to(crossing.toX,crossing.toY);
        //each side lands NEXT TO the other side's boat, on the shore cell it
        //touches — the boat tile itself is the teleport, standing on it would
        //bounce the player straight back
        unsigned int side=0;
        while(side<2)
        {
            const std::pair<uint16_t,uint16_t> &here=(side==0)?from:to;
            const std::pair<uint16_t,uint16_t> &there=(side==0)?to:from;
            if(boatTeleportObjects.find(here)!=boatTeleportObjects.cend()
                    && boatLandingCells.find(there)!=boatLandingCells.cend())
            {
                const std::pair<uint8_t,uint8_t> &landing=boatLandingCells.at(there);
                Tiled::MapObject * const object=boatTeleportObjects.at(here);
                object->setProperty("x",QString::number(landing.first));
                object->setProperty("y",QString::number(landing.second));
                wired++;
            }
            side++;
        }
        crossingIndex++;
    }
    if(wired<boatCrossings.size()*2)
        std::cerr << "only " << wired << " of " << (boatCrossings.size()*2)
                  << " boat teleport(s) could be wired: a shore had no boat" << std::endl;
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
            body.isSea=(body.size>=setting.waterSeaMinTiles);
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
              << " sea(s) of at least " << setting.waterSeaMinTiles << " tiles, biggest "
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

//one candidate sea route between two coastal towns
struct WaterCandidate
{
    unsigned int cityA,cityB;
    unsigned int distance;
};
static bool waterCandidateCloser(const WaterCandidate &a,const WaterCandidate &b)
{
    if(a.distance!=b.distance)
        return a.distance<b.distance;
    //stable whatever the order the pairs were found in
    if(a.cityA!=b.cityA)
        return a.cityA<b.cityA;
    return a.cityB<b.cityB;
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
    //NOT gated on pathPercentOfLand: that setting only buys the EXTRA routes.
    //Joining the land masses is what the sea is for and always runs.
    if(cities.size()<2 || waterBodies.empty())
        return;
    //how much SEA each chunk holds, and which seas it touches
    std::vector<unsigned int> chunkSeaTiles(mapXCount*mapYCount,0);
    std::vector<std::set<uint16_t> > chunkSeas(mapXCount*mapYCount);
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
                        if(body!=waterNoBody && waterBodies.at(body).isSea)
                        {
                            chunkSeaTiles[chunkX+chunkY*mapXCount]++;
                            chunkSeas[chunkX+chunkY*mapXCount].insert(body);
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
                    //THE MIDPOINT of the shared border has to be sea on both sides:
                    //the border teleport sits exactly there, so a route that
                    //crosses somewhere else along the line ends on dry land. The
                    //chunk search and the route validation now ask the same
                    //question, which is why they used to disagree and every
                    //swimmable route was thrown away.
                    const unsigned int lineLength=(direction<2)?singleMapHeight:singleMapWidth;
                    unsigned int step=lineLength/2;
                    while(step<lineLength/2+1 && seaAtBorder.at(chunk*4+direction)==0)
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
    //the seas every town can put to sea on: its chunk plus one chunk of margin
    std::vector<std::set<uint16_t> > citySeas(cities.size());
    {
        unsigned int cityIndex=0;
        while(cityIndex<cities.size())
        {
            const City &city=cities.at(cityIndex);
            //how far from a town the coast may be for it to count as a PORT. One
            //chunk left most towns with no sea at all and two whole land masses
            //unjoinable; the shore does not have to be at the town gate.
            const int harbourRadius=(int)setting.waterHarbourChunkRadius;
            int neighborY=-harbourRadius;
            while(neighborY<=harbourRadius)
            {
                int neighborX=-harbourRadius;
                while(neighborX<=harbourRadius)
                {
                    const int chunkX=(int)city.x+neighborX;
                    const int chunkY=(int)city.y+neighborY;
                    if(chunkX>=0 && chunkY>=0 && chunkX<(int)mapXCount && chunkY<(int)mapYCount)
                    {
                        const std::set<uint16_t> &seas=chunkSeas.at((unsigned int)chunkX+(unsigned int)chunkY*mapXCount);
                        citySeas[cityIndex].insert(seas.cbegin(),seas.cend());
                    }
                    neighborX++;
                }
                neighborY++;
            }
            cityIndex++;
        }
    }
    //which LAND MASS each town is on, before any sea route is added
    std::vector<unsigned int> componentOfCity(cities.size(),0xFFFFFFFF);
    {
        std::vector<unsigned int> chunkComponentEarly(mapXCount*mapYCount,0xFFFFFFFF);
        unsigned int componentCount=0;
        unsigned int startChunk=0;
        while(startChunk<mapXCount*mapYCount)
        {
            if(mapPathDirection[startChunk]!=0 && chunkComponentEarly.at(startChunk)==0xFFFFFFFF)
            {
                std::vector<unsigned int> queue;
                chunkComponentEarly[startChunk]=componentCount;
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
                                if(mapPathDirection[next]!=0 && chunkComponentEarly.at(next)==0xFFFFFFFF)
                                {
                                    chunkComponentEarly[next]=componentCount;
                                    queue.push_back(next);
                                }
                            }
                        }
                        direction++;
                    }
                }
                componentCount++;
            }
            startChunk++;
        }
        unsigned int cityIndex=0;
        while(cityIndex<cities.size())
        {
            componentOfCity[cityIndex]=chunkComponentEarly.at(cities.at(cityIndex).x+cities.at(cityIndex).y*mapXCount);
            cityIndex++;
        }
        std::cout << "land masses before the sea routes: " << componentCount << std::endl;
    }
    //every pair of towns that can reach each other on the SAME sea, nearest first
    std::vector<WaterCandidate> candidates;
    {
        unsigned int firstCity=0;
        while(firstCity<cities.size())
        {
            unsigned int secondCity=firstCity+1;
            while(secondCity<cities.size())
            {
                bool shareASea=false;
                std::set<uint16_t>::const_iterator seaIterator=citySeas.at(firstCity).cbegin();
                while(seaIterator!=citySeas.at(firstCity).cend() && !shareASea)
                {
                    if(citySeas.at(secondCity).find(*seaIterator)!=citySeas.at(secondCity).cend())
                        shareASea=true;
                    ++seaIterator;
                }
                //Different LAND MASSES also qualify without a shared sea: a boat
                //crossing is a teleport, so a shore on each side is enough. That
                //is the whole point of the sea here — a route between two towns of
                //the SAME continent is useless, the road already joins them.
                const bool differentLandMass=(componentOfCity.at(firstCity)!=componentOfCity.at(secondCity)
                                              && componentOfCity.at(firstCity)!=0xFFFFFFFF
                                              && componentOfCity.at(secondCity)!=0xFFFFFFFF);
                if(shareASea || (differentLandMass && !citySeas.at(firstCity).empty()
                                 && !citySeas.at(secondCity).empty()))
                {
                    WaterCandidate candidate;
                    candidate.cityA=firstCity;
                    candidate.cityB=secondCity;
                    const int deltaX=(int)cities.at(firstCity).x-(int)cities.at(secondCity).x;
                    const int deltaY=(int)cities.at(firstCity).y-(int)cities.at(secondCity).y;
                    candidate.distance=(unsigned int)(abs(deltaX)+abs(deltaY));
                    candidates.push_back(candidate);
                }
                secondCity++;
            }
            firstCity++;
        }
    }
    //union-find over the land masses, so each route only counts when it really
    //merges two of them
    std::vector<unsigned int> componentOf(componentOfCity);
    std::sort(candidates.begin(),candidates.end(),waterCandidateCloser);
    //"a water path should be X fewer than land": a share of the land road count
    unsigned int landRoads=roads.size();
    if(landRoads==0)
        landRoads=cities.size();
    unsigned int wanted=landRoads*setting.waterPathPercentOfLand/100;
    if(wanted==0 && !candidates.empty())
        wanted=1;
    //a town is joined by sea ONCE: the nearest partner, so the routes spread over
    //the coast instead of all leaving the same harbour
    std::vector<unsigned char> cityUsed(cities.size(),0);
    //the land mass pairs already joined by a crossing
    std::set<std::pair<unsigned int,unsigned int> > joinedMasses;
    unsigned int built=0;
    unsigned int candidateIndex=0;
    //two passes over the same sorted list: the routes that JOIN two land masses
    //first, then the quota on the rest
    unsigned int pass=0;
    while(pass<2)
    {
    candidateIndex=0;
    while(candidateIndex<candidates.size() && (pass==0 || built<wanted))
    {
        const WaterCandidate &candidate=candidates.at(candidateIndex);
        candidateIndex++;
        //ONE crossing per PAIR of land masses, not a spanning tree: a tree leaves a
        //single path, so going from the bottom-left group to the bottom-right one
        //meant sailing around the whole world. The pair is remembered by the
        //ORIGINAL land mass ids, so each pair is joined exactly once.
        const unsigned int massA=componentOfCity.at(candidate.cityA);
        const unsigned int massB=componentOfCity.at(candidate.cityB);
        const bool joinsLandMasses=(massA!=massB && massA!=0xFFFFFFFF && massB!=0xFFFFFFFF
                                    && joinedMasses.find(std::pair<unsigned int,unsigned int>(
                                           (massA<massB)?massA:massB,(massA<massB)?massB:massA))==joinedMasses.cend());
        if(pass==0)
        {
            //only what merges two land masses, whatever the quota
            if(!joinsLandMasses)
                continue;
        }
        else
        {
            //EXTRA routes, off by default: a sea route between two towns of the
            //SAME continent is useless, the road already joins them (owner). The
            //quota only buys scenery, so pathPercentOfLand ships at 0.
            if(joinsLandMasses)
                continue;
            if(cityUsed.at(candidate.cityA)!=0 || cityUsed.at(candidate.cityB)!=0)
                continue;
        }
        //BOAT crossing: no corridor at all, one closed chunk on each shore and a
        //push-teleport between them. Decided before the route is even searched.
        const bool byBoat=((unsigned int)(rand()%100)<setting.waterBoatPercent);
        //cheapest sail from A to B over the sailable chunks; the two town chunks
        //are the ends and are never painted as water themselves
        std::vector<int> parent(mapXCount*mapYCount,-2);
        const unsigned int startChunk=cities.at(candidate.cityA).x+cities.at(candidate.cityA).y*mapXCount;
        const unsigned int endChunk=cities.at(candidate.cityB).x+cities.at(candidate.cityB).y*mapXCount;
        std::vector<unsigned int> queue;
        parent[startChunk]=-1;
        queue.push_back(startChunk);
        unsigned int queueIndex=0;
        bool found=false;
        while(queueIndex<queue.size() && !found)
        {
            const unsigned int chunk=queue.at(queueIndex);
            queueIndex++;
            const int chunkX=(int)(chunk%mapXCount);
            const int chunkY=(int)(chunk/mapXCount);
            const int stepX[4]={-1,1,0,0};
            const int stepY[4]={0,0,-1,1};
            unsigned int direction=0;
            while(direction<4 && !found)
            {
                const int nextX=chunkX+stepX[direction];
                const int nextY=chunkY+stepY[direction];
                if(nextX>=0 && nextY>=0 && nextX<(int)mapXCount && nextY<(int)mapYCount)
                {
                    const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*mapXCount;
                    //the sea has to cross the border between the two, except on
                    //the two town chunks where the lane simply reaches the shore
                    const bool townStep=(chunk==startChunk || next==endChunk);
                    //A HARBOUR chunk only has to HOLD sea: the chunks next to a
                    //town are coastal by nature, and demanding chunkSeaPercent of
                    //them meant no route could ever leave the harbour — every
                    //join fell back to a boat. Out at sea the full share applies.
                    const bool usable=(sailable.at(next)!=0 || next==endChunk
                                       || (chunk==startChunk && chunkSeaTiles.at(next)>0
                                           && mapPathDirection[next]==0
                                           && !haveCityEntry(citiesCoordToIndex,next%mapXCount,next/mapXCount)));
                    if(parent.at(next)==-2 && usable
                            && (townStep || seaAtBorder.at(chunk*4+direction)!=0))
                    {
                        parent[next]=(int)chunk;
                        if(next==endChunk)
                            found=true;
                        else
                            queue.push_back(next);
                    }
                }
                direction++;
            }
        }
        //A BOAT CROSSING needs NO continuous water at all — it is a teleport. So
        //when two LAND MASSES cannot be joined by a swimmable route, they are
        //joined by boat anyway: a chunk with sea next to each town, and the
        //crossing between them. "Just connect city to city on the other side."
        if(!found && pass==0)
        {
            int shoreA=-1,shoreB=-1;
            unsigned int side=0;
            while(side<2)
            {
                const City &city=cities.at(side==0?candidate.cityA:candidate.cityB);
                //The harbour is the nearest chunk with REAL sea within the harbour
                //radius — a chunk with a corner of water and the rest forest is no
                //harbour, the boat had nowhere to moor and a corridor ended up cut
                //through the trees. It does NOT have to touch the town: the chunks
                //in between are marked as ordinary ROAD chunks below, so the way to
                //the boat is drawn by the road generator like any other road.
                int best=-1;
                int bestDistance=0;
                const int harbourRadius=(int)setting.waterHarbourChunkRadius;
                int neighborY=-harbourRadius;
                while(neighborY<=harbourRadius)
                {
                    int neighborX=-harbourRadius;
                    while(neighborX<=harbourRadius)
                    {
                        const int chunkX=(int)city.x+neighborX;
                        const int chunkY=(int)city.y+neighborY;
                        //straight line of chunks: the road to the harbour follows
                        //one axis, so it is a short branch off the town
                        if((neighborX==0)!=(neighborY==0)
                                && chunkX>=0 && chunkY>=0 && chunkX<(int)mapXCount && chunkY<(int)mapYCount)
                        {
                            const unsigned int chunk=(unsigned int)chunkX+(unsigned int)chunkY*mapXCount;
                            const int distance=abs(neighborX)+abs(neighborY);
                            if(chunkSeaTiles.at(chunk)*100>=chunkTiles*setting.waterChunkSeaPercent
                                    && mapPathDirection[chunk]==0
                                    && !haveCityEntry(citiesCoordToIndex,(unsigned int)chunkX,(unsigned int)chunkY)
                                    && (best<0 || distance<bestDistance))
                            {
                                //every chunk between the town and it must be free
                                bool wayClear=true;
                                int stepIndex=1;
                                while(stepIndex<distance && wayClear)
                                {
                                    const int wayX=(int)city.x+((neighborX>0)?stepIndex:((neighborX<0)?-stepIndex:0));
                                    const int wayY=(int)city.y+((neighborY>0)?stepIndex:((neighborY<0)?-stepIndex:0));
                                    const unsigned int wayChunk=(unsigned int)wayX+(unsigned int)wayY*mapXCount;
                                    if(mapPathDirection[wayChunk]!=0
                                            || haveCityEntry(citiesCoordToIndex,(unsigned int)wayX,(unsigned int)wayY))
                                        wayClear=false;
                                    stepIndex++;
                                }
                                if(wayClear)
                                {
                                    best=(int)chunk;
                                    bestDistance=distance;
                                }
                            }
                        }
                        neighborX++;
                    }
                    neighborY++;
                }
                if(side==0)
                    shoreA=best;
                else
                    shoreB=best;
                side++;
            }
            if(shoreA>=0 && shoreB>=0 && shoreA!=shoreB)
            {
                //Link the whole CHAIN town -> ... -> harbour, both ways (a border
                //teleport only exists for a bit that is SET, so linking one way let
                //the player leave the harbour but never enter it). The chunks in
                //between stay ordinary ROAD chunks, so the way to the boat is drawn
                //by the road generator — not a line carved through the forest.
                unsigned int shoreIndex=0;
                while(shoreIndex<2)
                {
                    const unsigned int townChunk=(shoreIndex==0)?startChunk:endChunk;
                    const unsigned int harbourChunk=(unsigned int)((shoreIndex==0)?shoreA:shoreB);
                    const int townX=(int)(townChunk%mapXCount);
                    const int townY=(int)(townChunk/mapXCount);
                    const int harbourX=(int)(harbourChunk%mapXCount);
                    const int harbourY=(int)(harbourChunk/mapXCount);
                    const int stepX=(harbourX>townX)?1:((harbourX<townX)?-1:0);
                    const int stepY=(harbourY>townY)?1:((harbourY<townY)?-1:0);
                    int walkX=townX;
                    int walkY=townY;
                    while(walkX!=harbourX || walkY!=harbourY)
                    {
                        const unsigned int here=(unsigned int)walkX+(unsigned int)walkY*mapXCount;
                        const unsigned int there=(unsigned int)(walkX+stepX)+(unsigned int)(walkY+stepY)*mapXCount;
                        linkChunkToNeighbour(here,there,mapXCount);
                        linkChunkToNeighbour(there,here,mapXCount);
                        walkX+=stepX;
                        walkY+=stepY;
                    }
                    shoreIndex++;
                }
                boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)((unsigned int)shoreA%mapXCount),(uint16_t)((unsigned int)shoreA/mapXCount)));
                boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)((unsigned int)shoreB%mapXCount),(uint16_t)((unsigned int)shoreB/mapXCount)));
                waterChunks.push_back(boatChunks.at(boatChunks.size()-2));
                waterChunks.push_back(boatChunks.back());
                BoatCrossing crossing;
                crossing.fromX=(uint16_t)((unsigned int)shoreA%mapXCount);
                crossing.fromY=(uint16_t)((unsigned int)shoreA/mapXCount);
                crossing.toX=(uint16_t)((unsigned int)shoreB%mapXCount);
                crossing.toY=(uint16_t)((unsigned int)shoreB/mapXCount);
                boatCrossings.push_back(crossing);
                cityUsed[candidate.cityA]=1;
                cityUsed[candidate.cityB]=1;
                built++;
                joinedMasses.insert(std::pair<unsigned int,unsigned int>(
                                        (massA<massB)?massA:massB,(massA<massB)?massB:massA));
                const unsigned int merged=componentOf.at(candidate.cityB);
                const unsigned int into=componentOf.at(candidate.cityA);
                unsigned int cityIndex=0;
                while(cityIndex<componentOf.size())
                {
                    if(componentOf.at(cityIndex)==merged)
                        componentOf[cityIndex]=into;
                    cityIndex++;
                }
            }
        }
        if(found)
        {
            //the chunks between the two towns, town chunks excluded
            std::vector<unsigned int> route;
            int walkChunk=(int)endChunk;
            while(walkChunk>=0)
            {
                route.push_back((unsigned int)walkChunk);
                walkChunk=parent.at((unsigned int)walkChunk);
            }
            //Every chunk of the route must let the sea CROSS IT: its own water has
            //to join the side it is entered by to the side it is left by. The
            //chunk-level search only knows that sea touches each border, so a
            //chunk whose sea is in two pieces would be entered and never left.
            //When one fails the route is dropped and the crossing goes by boat.
            bool sailableRoute=true;
            {
                unsigned int routeIndex=1;
                while(routeIndex+1<route.size() && sailableRoute)
                {
                    const unsigned int chunk=route.at(routeIndex);
                    const unsigned int chunkX=chunk%mapXCount;
                    const unsigned int chunkY=chunk/mapXCount;
                    //the two sides this chunk is used by
                    const unsigned int previous=route.at(routeIndex-1);
                    const unsigned int nextChunk=route.at(routeIndex+1);
                    std::vector<unsigned int> borderCells;
                    unsigned int neighbourIndex=0;
                    while(neighbourIndex<2)
                    {
                        const unsigned int neighbour=(neighbourIndex==0)?previous:nextChunk;
                        const int deltaX=(int)(neighbour%mapXCount)-(int)chunkX;
                        const int deltaY=(int)(neighbour/mapXCount)-(int)chunkY;
                        //THE MIDPOINT of the shared border, and it has to be sea.
                        //The border teleport sits exactly there (addMapChange), so
                        //accepting "the nearest sea cell of that border" let through
                        //chunks whose teleport ended up on dry land, walled off from
                        //the lane.
                        const unsigned int lineLength=(deltaX!=0)?singleMapHeight:singleMapWidth;
                        const unsigned int middleStep=lineLength/2;
                        const unsigned int localX=(deltaX<0)?0:((deltaX>0)?singleMapWidth-1:middleStep);
                        const unsigned int localY=(deltaY<0)?0:((deltaY>0)?singleMapHeight-1:middleStep);
                        const unsigned int tile=(chunkX*singleMapWidth+localX)
                                +(chunkY*singleMapHeight+localY)*worldWidth;
                        int best=-1;
                        if(waterBodyOfTile.at(tile)!=waterNoBody)
                            best=(int)(localX+localY*singleMapWidth);
                        if(best<0)
                            sailableRoute=false;
                        else
                            borderCells.push_back((unsigned int)best);
                        neighbourIndex++;
                    }
                    //do the two join THROUGH THIS CHUNK's own water?
                    if(sailableRoute)
                    {
                        std::vector<unsigned char> seen(singleMapWidth*singleMapHeight,0);
                        std::vector<unsigned int> localQueue;
                        seen[borderCells.at(0)]=1;
                        localQueue.push_back(borderCells.at(0));
                        unsigned int localIndex=0;
                        bool joined=false;
                        while(localIndex<localQueue.size() && !joined)
                        {
                            const unsigned int cell=localQueue.at(localIndex);
                            localIndex++;
                            if(cell==borderCells.at(1))
                                joined=true;
                            else
                            {
                                const int cellX=(int)(cell%singleMapWidth);
                                const int cellY=(int)(cell/singleMapWidth);
                                const int stepX[4]={-1,1,0,0};
                                const int stepY[4]={0,0,-1,1};
                                unsigned int direction=0;
                                while(direction<4)
                                {
                                    const int nextX=cellX+stepX[direction];
                                    const int nextY=cellY+stepY[direction];
                                    if(nextX>=0 && nextY>=0 && nextX<(int)singleMapWidth && nextY<(int)singleMapHeight)
                                    {
                                        const unsigned int localCell=(unsigned int)nextX+(unsigned int)nextY*singleMapWidth;
                                        const unsigned int tile=(chunkX*singleMapWidth+(unsigned int)nextX)
                                                +(chunkY*singleMapHeight+(unsigned int)nextY)*worldWidth;
                                        if(seen.at(localCell)==0 && waterBodyOfTile.at(tile)!=waterNoBody)
                                        {
                                            seen[localCell]=1;
                                            localQueue.push_back(localCell);
                                        }
                                    }
                                    direction++;
                                }
                            }
                        }
                        if(!joined)
                            sailableRoute=false;
                    }
                    routeIndex++;
                }
            }
            //route is end..start; it always holds both town chunks
            if(route.size()>=3 && sailableRoute)
            {
                cityUsed[candidate.cityA]=1;
                cityUsed[candidate.cityB]=1;
                built++;
                joinedMasses.insert(std::pair<unsigned int,unsigned int>(
                                        (massA<massB)?massA:massB,(massA<massB)?massB:massA));
                //the two land masses are now one
                {
                    const unsigned int merged=componentOf.at(candidate.cityB);
                    const unsigned int into=componentOf.at(candidate.cityA);
                    unsigned int cityIndex=0;
                    while(cityIndex<componentOf.size())
                    {
                        if(componentOf.at(cityIndex)==merged)
                            componentOf[cityIndex]=into;
                        cityIndex++;
                    }
                }
                if(byBoat)
                {
                    //only the chunk next to each town, closed, with the teleport
                    const unsigned int nearB=route.at(1);
                    const unsigned int nearA=route.at(route.size()-2);
                    //link each closed chunk to ITS town, both ways
                    linkChunkToNeighbour(nearB,endChunk,mapXCount);
                    linkChunkToNeighbour(endChunk,nearB,mapXCount);
                    linkChunkToNeighbour(nearA,startChunk,mapXCount);
                    linkChunkToNeighbour(startChunk,nearA,mapXCount);
                    boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)(nearA%mapXCount),(uint16_t)(nearA/mapXCount)));
                    boatChunks.push_back(std::pair<uint16_t,uint16_t>((uint16_t)(nearB%mapXCount),(uint16_t)(nearB/mapXCount)));
                    waterChunks.push_back(boatChunks.at(boatChunks.size()-2));
                    waterChunks.push_back(boatChunks.back());
                    BoatCrossing crossing;
                    crossing.fromX=(uint16_t)(nearA%mapXCount);
                    crossing.fromY=(uint16_t)(nearA/mapXCount);
                    crossing.toX=(uint16_t)(nearB%mapXCount);
                    crossing.toY=(uint16_t)(nearB/mapXCount);
                    boatCrossings.push_back(crossing);
                }
                else
                {
                    //a swimmable channel: every chunk of the route joined to the
                    //next, so the border teleports chain all the way across
                    unsigned int routeIndex=0;
                    while(routeIndex+1<route.size())
                    {
                        linkChunkToNeighbour(route.at(routeIndex),route.at(routeIndex+1),mapXCount);
                        linkChunkToNeighbour(route.at(routeIndex+1),route.at(routeIndex),mapXCount);
                        routeIndex++;
                    }
                    routeIndex=1;
                    while(routeIndex+1<route.size())
                    {
                        waterChunks.push_back(std::pair<uint16_t,uint16_t>(
                                                  (uint16_t)(route.at(routeIndex)%mapXCount),
                                                  (uint16_t)(route.at(routeIndex)/mapXCount)));
                        routeIndex++;
                    }
                }
            }
        }
    }
    pass++;
    }
    std::cout << "water paths: " << built << " route(s) of the " << wanted << " asked ("
              << candidates.size() << " coastal town pairs), " << boatCrossings.size()
              << " by boat, " << waterChunks.size() << " chunk(s)" << std::endl;
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
