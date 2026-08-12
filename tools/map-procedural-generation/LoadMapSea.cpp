#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapPlants.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include <libtiled/tileset.h>
#include <libtiled/tile.h>
#include <libtiled/objectgroup.h>

#include <iostream>
#include <set>
#include <map>
#include <queue>
#include <vector>

#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

std::vector<LoadMapAll::WaterRoute> LoadMapAll::waterRoutes;
std::vector<unsigned char> LoadMapAll::seaAllowedCells;
std::vector<unsigned char> LoadMapAll::seaWallCells;
std::vector<LoadMapAll::DecorationVariant> LoadMapAll::seaDecorations;

//the mountain cliff ring of the terrain, shared with the road painter (islands)
unsigned int mountainBorderTileIndex(const uint8_t to_type_match);

//Cheap integer hash, the same mix as seedChunk: two neighbouring tiles must not
//come out correlated. Used instead of rand() because this pass runs on the WHOLE
//world at once — a per chunk stream would make the coast of one map depend on how
//many numbers the map before it drew.
static uint32_t seaHash(const unsigned int &x,const unsigned int &y,const unsigned int &seed)
{
    uint32_t h=(uint32_t)seed;
    h^=(uint32_t)x*0x9E3779B1u;
    h^=(uint32_t)y*0x85EBCA77u;
    h^=h>>16;
    h*=0x7FEB352Du;
    h^=h>>15;
    h*=0x846CA68Bu;
    h^=h>>16;
    return h;
}

//SMOOTH value noise, 0..255, on a grid of `step` tiles. This is what makes every
//rock line WANDER: a wall at a constant distance from the shore is a ruler stroke,
//and a coast never looks like that.
static unsigned int seaNoise(const unsigned int &x,const unsigned int &y,
                             const unsigned int &seed,const unsigned int &step)
{
    const unsigned int gridX=x/step;
    const unsigned int gridY=y/step;
    const unsigned int fracX=x%step;
    const unsigned int fracY=y%step;
    const unsigned int corner00=seaHash(gridX,gridY,seed)&0xFF;
    const unsigned int corner10=seaHash(gridX+1,gridY,seed)&0xFF;
    const unsigned int corner01=seaHash(gridX,gridY+1,seed)&0xFF;
    const unsigned int corner11=seaHash(gridX+1,gridY+1,seed)&0xFF;
    const unsigned int top=(corner00*(step-fracX)+corner10*fracX)/step;
    const unsigned int bottom=(corner01*(step-fracX)+corner11*fracX)/step;
    return (top*(step-fracY)+bottom*fracY)/step;
}

//how far from the shore the player may swim HERE, in tiles
static unsigned int seaBandLimit(const unsigned int &x,const unsigned int &y,
                                 const SettingsAll::SettingsExtra &setting)
{
    const unsigned int span=setting.waterBeachMax-setting.waterBeachMin+1;
    return setting.waterBeachMin+seaNoise(x,y,setting.seed+7717,16)*span/256;
}

//the ground a chunk stands on, to fill what a carved lane opens
static Tiled::Tile *seaChunkGroundTile(Tiled::TileLayer * const walkLayer,
                                       const unsigned int &chunkX,const unsigned int &chunkY,
                                       const unsigned int &mapWidth,const unsigned int &mapHeight)
{
    std::map<Tiled::Tile*,unsigned int> tileCount;
    unsigned int localY=0;
    while(localY<mapHeight)
    {
        unsigned int localX=0;
        while(localX<mapWidth)
        {
            Tiled::Tile * const tile=walkLayer->cellAt(chunkX*mapWidth+localX,chunkY*mapHeight+localY).tile();
            if(tile!=NULL)
                tileCount[tile]++;
            localX++;
        }
        localY++;
    }
    Tiled::Tile *best=NULL;
    unsigned int bestCount=0;
    std::map<Tiled::Tile*,unsigned int>::const_iterator tileIterator=tileCount.cbegin();
    while(tileIterator!=tileCount.cend())
    {
        if(tileIterator->second>bestCount)
        {
            bestCount=tileIterator->second;
            best=tileIterator->first;
        }
        ++tileIterator;
    }
    return best;
}

//template/sea/<name>/: the decorations of a swimmable route. Same shape as
//template/on-<terrain>/ — one tmx per variant plus its how-use.ini — but they are
//brushed on the WATER of a route and carry a fight bot.
void LoadMapAll::scanSeaTemplates(Tiled::Map &worldMap,const unsigned int mapWidth,const unsigned int mapHeight)
{
    seaDecorations.clear();
    const QDir groupDir(QCoreApplication::applicationDirPath()+"/template/sea");
    if(!groupDir.exists())
        return;
    const QStringList variantNames=groupDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    int variantIndex=0;
    while(variantIndex<variantNames.size())
    {
        const QDir variantDir(groupDir.absoluteFilePath(variantNames.at(variantIndex)));
        const QStringList tmxNames=variantDir.entryList(QStringList("*.tmx"),QDir::Files,QDir::Name);
        if(tmxNames.isEmpty())
            std::cerr << "template/sea/" << variantNames.at(variantIndex).toStdString()
                      << "/ holds no tmx, ignored" << std::endl;
        else
        {
            DecorationVariant variant;
            variant.folder=("sea/"+variantNames.at(variantIndex)).toStdString();
            variant.use=readTemplateUse(variantDir.absolutePath());
            if(!variant.use.valid)
                std::cerr << "template/" << variant.folder
                          << "/ has no how-use.ini, it would never be placed" << std::endl;
            else
            {
                const QString base=tmxNames.at(0).left(tmxNames.at(0).size()-4);
                loadMapTemplate(("sea/"+variantNames.at(variantIndex)+"/").toUtf8().constData(),
                                variant.mapTemplate,base,mapWidth,mapHeight,worldMap);
                seaDecorations.push_back(variant);
            }
        }
        variantIndex++;
    }
    if(!seaDecorations.empty())
        std::cout << "Sea templates: " << seaDecorations.size() << " variant(s)" << std::endl;
}

void LoadMapAll::addSeaContent(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting)
{
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    const unsigned int mapWidth=setting.mapWidth;
    const unsigned int mapHeight=setting.mapHeight;
    const unsigned int mapXCount=setting.mapXCount;
    const unsigned int mapYCount=setting.mapYCount;
    seaAllowedCells.assign(worldWidth*worldHeight,0);
    seaWallCells.assign(worldWidth*worldHeight,0);

    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::TileLayer * const walkLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer * const colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    if(waterLayer==NULL || walkLayer==NULL || colliLayer==NULL)
    {
        std::cerr << "the world map has no Water/Walkable/Collisions layer, the sea cannot be closed" << std::endl;
        return;
    }
    Tiled::Tile * const rockTile=setting.waterBorderTile.isEmpty()
            ?NULL:fetchTile(worldMap,setting.waterBorderTile);
    if(rockTile==NULL)
    {
        std::cerr << "[water] borderTile resolves to nothing, the sea would have no wall" << std::endl;
        return;
    }
    std::vector<Tiled::TileLayer*> collisionLayers;
    std::vector<Tiled::TileLayer*> abovePlayerLayers;
    {
        unsigned int layerIndex=0;
        while(layerIndex<(unsigned int)worldMap.layerCount())
        {
            Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
            if(layer->isTileLayer())
            {
                if(layer->name()=="Collisions")
                    collisionLayers.push_back(static_cast<Tiled::TileLayer *>(layer));
                if(layer->name()=="WalkBehind")
                    abovePlayerLayers.push_back(static_cast<Tiled::TileLayer *>(layer));
            }
            layerIndex++;
        }
    }

    //=== 1) THE WORLD AS THE PLAYER SEES IT ==================================
    //water is walkable once the town shop sold the item for it, so a cell is only
    //closed by a COLLISION — that is the engine's own rule (the Collisions layers
    //are OR-merged, Map_loaderMain.cpp).
    std::vector<unsigned char> water(worldWidth*worldHeight,0);
    std::vector<unsigned char> blocked(worldWidth*worldHeight,0);
    {
        unsigned int tileY=0;
        while(tileY<worldHeight)
        {
            unsigned int tileX=0;
            while(tileX<worldWidth)
            {
                const unsigned int cell=tileX+tileY*worldWidth;
                if(waterLayer->cellAt(tileX,tileY).tile()!=NULL)
                    water[cell]=1;
                unsigned int layerIndex=0;
                while(layerIndex<collisionLayers.size())
                {
                    if(collisionLayers.at(layerIndex)->cellAt(tileX,tileY).tile()!=NULL)
                        blocked[cell]=1;
                    layerIndex++;
                }
                tileX++;
            }
            tileY++;
        }
    }

    //=== 1b) WHAT IS THE OPEN SEA ============================================
    //The rock only ever closes the OPEN SEA. A pond in a field, a mountain lake,
    //a river mouth: the player swims it end to end, it is bounded by its own
    //shore and a rock chain across it would be a wall drawn for nothing. So the
    //water is split into connected bodies HERE, on the map as it was really
    //painted, and only a body of at least [water] seaMinTiles is bounded.
    std::vector<unsigned char> openSea(worldWidth*worldHeight,0);
    {
        std::vector<unsigned char> seen(worldWidth*worldHeight,0);
        std::vector<unsigned int> body;
        std::vector<unsigned int> queue;
        unsigned int startCell=0;
        while(startCell<worldWidth*worldHeight)
        {
            if(water.at(startCell)!=0 && seen.at(startCell)==0)
            {
                body.clear();
                queue.clear();
                seen[startCell]=1;
                queue.push_back(startCell);
                unsigned int queueIndex=0;
                while(queueIndex<queue.size())
                {
                    const unsigned int cell=queue.at(queueIndex);
                    queueIndex++;
                    body.push_back(cell);
                    const int cellX=(int)(cell%worldWidth);
                    const int cellY=(int)(cell/worldWidth);
                    const int stepX[4]={-1,1,0,0};
                    const int stepY[4]={0,0,-1,1};
                    unsigned int direction=0;
                    while(direction<4)
                    {
                        const int nextX=cellX+stepX[direction];
                        const int nextY=cellY+stepY[direction];
                        if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                        {
                            const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                            if(water.at(next)!=0 && seen.at(next)==0)
                            {
                                seen[next]=1;
                                queue.push_back(next);
                            }
                        }
                        direction++;
                    }
                }
                if(body.size()>=setting.waterSeaMinTiles)
                {
                    unsigned int bodyIndex=0;
                    while(bodyIndex<body.size())
                    {
                        openSea[body.at(bodyIndex)]=1;
                        bodyIndex++;
                    }
                }
            }
            startCell++;
        }
    }

    //=== 2) THE BEACH ========================================================
    //The player enters the sea where the shore is WALKABLE, and may then swim
    //beachMin..beachMax tiles from it. Everything further out is the open sea and
    //is closed. Nothing here knows about chunks: the coast does not stop at a map
    //border, so neither does the band — and where the beach dies against a cliff
    //the band simply stops there, which is what makes the wall end on the rock
    //instead of hanging in the water.
    std::vector<unsigned char> allowed(worldWidth*worldHeight,0);
    {
        std::vector<int> swimDistance(worldWidth*worldHeight,-1);
        std::vector<unsigned int> queue;
        unsigned int tileY=0;
        while(tileY<worldHeight)
        {
            unsigned int tileX=0;
            while(tileX<worldWidth)
            {
                const unsigned int cell=tileX+tileY*worldWidth;
                //anything that is not the open sea is swimmable end to end
                if(water.at(cell)!=0 && openSea.at(cell)==0)
                    allowed[cell]=1;
                if(water.at(cell)!=0 && openSea.at(cell)!=0 && blocked.at(cell)==0)
                {
                    const int stepX[4]={-1,1,0,0};
                    const int stepY[4]={0,0,-1,1};
                    bool fromTheShore=false;
                    unsigned int direction=0;
                    while(direction<4)
                    {
                        const int nextX=(int)tileX+stepX[direction];
                        const int nextY=(int)tileY+stepY[direction];
                        if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                        {
                            const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                            if(water.at(next)==0 && blocked.at(next)==0)
                                fromTheShore=true;
                        }
                        direction++;
                    }
                    if(fromTheShore)
                    {
                        swimDistance[cell]=1;
                        allowed[cell]=1;
                        queue.push_back(cell);
                    }
                }
                tileX++;
            }
            tileY++;
        }
        unsigned int queueIndex=0;
        while(queueIndex<queue.size())
        {
            const unsigned int cell=queue.at(queueIndex);
            queueIndex++;
            const int cellX=(int)(cell%worldWidth);
            const int cellY=(int)(cell/worldWidth);
            const int stepX[4]={-1,1,0,0};
            const int stepY[4]={0,0,-1,1};
            unsigned int direction=0;
            while(direction<4)
            {
                const int nextX=cellX+stepX[direction];
                const int nextY=cellY+stepY[direction];
                if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                {
                    const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                    if(water.at(next)!=0 && openSea.at(next)!=0
                            && blocked.at(next)==0 && swimDistance.at(next)<0)
                    {
                        const unsigned int limit=seaBandLimit((unsigned int)nextX,(unsigned int)nextY,setting);
                        if((unsigned int)(swimDistance.at(cell)+1)<=limit)
                        {
                            swimDistance[next]=swimDistance.at(cell)+1;
                            allowed[next]=1;
                            queue.push_back(next);
                        }
                    }
                }
                direction++;
            }
        }
    }

    //=== 3) THE LANES OF THE SWIMMABLE ROUTES ================================
    //From the beach of one end to the beach of the other, through the chunks the
    //route was planned on and NOTHING else — a lane through a chunk no map is
    //written for could never be swum. Water costs least, walkable shore a little
    //more, a cliff or a wood far more: the way follows the sea, walks the beach
    //when it must and only cuts through what really blocks it.
    unsigned int laneCount=0;
    unsigned int carvedLaneCells=0;
    {
        std::vector<int> routePosition(mapXCount*mapYCount,-1);
        unsigned int routeIndex=0;
        while(routeIndex<waterRoutes.size())
        {
            const WaterRoute &route=waterRoutes.at(routeIndex);
            if(!route.isBoat && route.chunks.size()>=2)
            {
                unsigned int chunkIndex=0;
                while(chunkIndex<route.chunks.size())
                {
                    routePosition[route.chunks.at(chunkIndex)]=(int)chunkIndex;
                    chunkIndex++;
                }
                const unsigned int startChunk=route.chunks.front();
                const unsigned int endChunk=route.chunks.back();
                //Dijkstra, cheapest way first
                static const unsigned int costWater=1;
                static const unsigned int costLand=8;
                static const unsigned int costBlocked=60;
                static const unsigned int costUnreachable=0xFFFFFFFF;
                std::vector<unsigned int> cost(worldWidth*worldHeight,costUnreachable);
                std::vector<int> parent(worldWidth*worldHeight,-1);
                std::priority_queue<std::pair<unsigned int,unsigned int>,
                        std::vector<std::pair<unsigned int,unsigned int> >,
                        std::greater<std::pair<unsigned int,unsigned int> > > walk;
                {
                    const unsigned int x0=(startChunk%mapXCount)*mapWidth;
                    const unsigned int y0=(startChunk/mapXCount)*mapHeight;
                    unsigned int localY=0;
                    while(localY<mapHeight)
                    {
                        unsigned int localX=0;
                        while(localX<mapWidth)
                        {
                            const unsigned int cell=(x0+localX)+(y0+localY)*worldWidth;
                            if(blocked.at(cell)==0)
                            {
                                cost[cell]=0;
                                walk.push(std::pair<unsigned int,unsigned int>(0,cell));
                            }
                            localX++;
                        }
                        localY++;
                    }
                }
                int reached=-1;
                while(!walk.empty() && reached<0)
                {
                    const std::pair<unsigned int,unsigned int> top=walk.top();
                    walk.pop();
                    const unsigned int cell=top.second;
                    if(top.first<=cost.at(cell))
                    {
                        const unsigned int cellX=cell%worldWidth;
                        const unsigned int cellY=cell/worldWidth;
                        if(cellX/mapWidth+(cellY/mapHeight)*mapXCount==endChunk && cost.at(cell)>0)
                            reached=(int)cell;
                        else
                        {
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
                                    const unsigned int fromChunk=(cellX/mapWidth)+(cellY/mapHeight)*mapXCount;
                                    const unsigned int toChunk=((unsigned int)nextX/mapWidth)
                                            +((unsigned int)nextY/mapHeight)*mapXCount;
                                    //inside the route, and a border is only crossed
                                    //between two chunks the route really links
                                    bool legal=(routePosition.at(toChunk)>=0);
                                    if(legal && toChunk!=fromChunk)
                                        if(abs(routePosition.at(toChunk)-routePosition.at(fromChunk))!=1)
                                            legal=false;
                                    if(legal)
                                    {
                                        unsigned int stepCost=costLand;
                                        if(blocked.at(next)!=0)
                                            stepCost=costBlocked;
                                        else if(water.at(next)!=0)
                                            stepCost=costWater;
                                        if(cost.at(cell)+stepCost<cost.at(next))
                                        {
                                            cost[next]=cost.at(cell)+stepCost;
                                            parent[next]=(int)cell;
                                            walk.push(std::pair<unsigned int,unsigned int>(cost.at(next),next));
                                        }
                                    }
                                }
                                direction++;
                            }
                        }
                    }
                }
                if(reached<0)
                    std::cerr << "no lane can be drawn from the map "
                              << startChunk%mapXCount << "," << startChunk/mapXCount << " to "
                              << endChunk%mapXCount << "," << endChunk/mapXCount << std::endl;
                else
                {
                    //the way itself: open what blocks it, and remember its water
                    std::vector<unsigned int> lane;
                    int walkCell=reached;
                    while(walkCell>=0)
                    {
                        const unsigned int cell=(unsigned int)walkCell;
                        lane.push_back(cell);
                        const unsigned int tileX=cell%worldWidth;
                        const unsigned int tileY=cell/worldWidth;
                        if(blocked.at(cell)!=0)
                        {
                            //A TREE GOES WHOLE OR NOT AT ALL, like a road corridor
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
                            blocked[cell]=0;
                            carvedLaneCells++;
                            //WATER NEEDS NO GROUND under it, the engine walks on it
                            if(water.at(cell)==0 && walkLayer->cellAt(tileX,tileY).tile()==NULL)
                            {
                                Tiled::Tile * const groundTile=seaChunkGroundTile(
                                            walkLayer,tileX/mapWidth,tileY/mapHeight,mapWidth,mapHeight);
                                if(groundTile!=NULL)
                                    walkLayer->setCell(tileX,tileY,Tiled::Cell(groundTile));
                            }
                        }
                        if(water.at(cell)!=0)
                            allowed[cell]=1;
                        //nothing grows back on a lane
                        maskVegetationAround(worldMap,tileX,tileY,1);
                        walkCell=parent.at(cell);
                    }
                    //...and the CHANNEL around it: the lane widened through water
                    //only, by a half width that wanders, so the wall reads as a
                    //reef and not as two drawn rails
                    {
                        std::vector<int> depth(worldWidth*worldHeight,-1);
                        std::vector<unsigned int> ring;
                        unsigned int laneIndex=0;
                        while(laneIndex<lane.size())
                        {
                            if(water.at(lane.at(laneIndex))!=0)
                            {
                                depth[lane.at(laneIndex)]=0;
                                ring.push_back(lane.at(laneIndex));
                            }
                            laneIndex++;
                        }
                        unsigned int ringIndex=0;
                        while(ringIndex<ring.size())
                        {
                            const unsigned int cell=ring.at(ringIndex);
                            ringIndex++;
                            const int cellX=(int)(cell%worldWidth);
                            const int cellY=(int)(cell/worldWidth);
                            const int stepX[4]={-1,1,0,0};
                            const int stepY[4]={0,0,-1,1};
                            unsigned int direction=0;
                            while(direction<4)
                            {
                                const int nextX=cellX+stepX[direction];
                                const int nextY=cellY+stepY[direction];
                                if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                                {
                                    const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                                    const unsigned int nextChunk=((unsigned int)nextX/mapWidth)
                                            +((unsigned int)nextY/mapHeight)*mapXCount;
                                    if(depth.at(next)<0 && water.at(next)!=0 && blocked.at(next)==0
                                            && routePosition.at(nextChunk)>=0)
                                    {
                                        int halfWidth=(int)setting.waterChannelHalfWidth;
                                        if(setting.waterWanderAmplitude>0)
                                        {
                                            const int amplitude=(int)setting.waterWanderAmplitude;
                                            halfWidth+=(int)(seaNoise((unsigned int)nextX,(unsigned int)nextY,
                                                                      setting.seed+91,12)*(2*amplitude+1)/256)
                                                    -amplitude;
                                        }
                                        if(halfWidth<2)
                                            halfWidth=2;
                                        if(depth.at(cell)+1<=halfWidth)
                                        {
                                            depth[next]=depth.at(cell)+1;
                                            allowed[next]=1;
                                            ring.push_back(next);
                                        }
                                    }
                                }
                                direction++;
                            }
                        }
                    }
                    laneCount++;
                }
                chunkIndex=0;
                while(chunkIndex<route.chunks.size())
                {
                    routePosition[route.chunks.at(chunkIndex)]=-1;
                    chunkIndex++;
                }
            }
            routeIndex++;
        }
    }

    //=== 4) THE HARBOUR OF EVERY BOAT CROSSING ===============================
    //A ferry needs an open basin, not a corridor: the whole water of the chunk is
    //free so the ship has room to moor and the player has somewhere to arrive.
    //The sides NO MAP lies behind are closed by a rock line boatBorderMin..Max
    //tiles inside the map border — its distance wanders too, else it is a ruler
    //stroke along the chunk edge, which is the one thing a border must never be.
    {
        unsigned int crossingIndex=0;
        while(crossingIndex<boatCrossings.size())
        {
            unsigned int side=0;
            while(side<2)
            {
                const BoatCrossing &crossing=boatCrossings.at(crossingIndex);
                const unsigned int chunkX=(side==0)?crossing.fromX:crossing.toX;
                const unsigned int chunkY=(side==0)?crossing.fromY:crossing.toY;
                const uint8_t links=mapPathDirection[chunkX+chunkY*mapXCount];
                const unsigned int x0=chunkX*mapWidth;
                const unsigned int y0=chunkY*mapHeight;
                const unsigned int span=setting.waterBoatBorderMax-setting.waterBoatBorderMin+1;
                unsigned int localY=0;
                while(localY<mapHeight)
                {
                    unsigned int localX=0;
                    while(localX<mapWidth)
                    {
                        const unsigned int tileX=x0+localX;
                        const unsigned int tileY=y0+localY;
                        const unsigned int cell=tileX+tileY*worldWidth;
                        if(water.at(cell)!=0 && blocked.at(cell)==0)
                        {
                            const unsigned int margin=setting.waterBoatBorderMin
                                    +seaNoise(tileX,tileY,setting.seed+331,10)*span/256;
                            bool sealed=false;
                            if((links&Orientation_left)==0 && localX<margin)
                                sealed=true;
                            if((links&Orientation_right)==0 && localX+margin>=mapWidth)
                                sealed=true;
                            if((links&Orientation_top)==0 && localY<margin)
                                sealed=true;
                            if((links&Orientation_bottom)==0 && localY+margin>=mapHeight)
                                sealed=true;
                            if(sealed)
                                allowed[cell]=0;
                            else
                                allowed[cell]=1;
                        }
                        localX++;
                    }
                    localY++;
                }
                side++;
            }
            crossingIndex++;
        }
    }

    //=== 5) AN ISLET NOW AND THEN, in the open part of a lane =================
    //It is BUILT ON the water, so it adds land instead of destroying any: mountain
    //texture inside, and the same cliff border tiles the terrain uses around it.
    unsigned int islandCount=0;
    if(setting.waterIslandPercent>0)
    {
        Tiled::Tile *mountainTile=NULL;
        std::vector<Tiled::Tile *> mountainBorderTiles;
        {
            int height=0;
            while(height<5 && mountainTile==NULL)
            {
                int moisure=0;
                while(moisure<6 && mountainTile==NULL)
                {
                    const LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
                    if(terrain.tile!=NULL && terrain.terrainName.compare(QString("mountain"),Qt::CaseInsensitive)==0)
                        mountainTile=terrain.tile;
                    moisure++;
                }
                height++;
            }
            const Tiled::Tileset * const mountainTsx=LoadMap::searchTilesetByName(worldMap,mountain.tsx);
            if(mountainTsx!=NULL)
            {
                const QStringList mountainTileList=mountain.tile.split(",");
                int mountainIndex=0;
                while(mountainIndex<mountainTileList.size())
                {
                    mountainBorderTiles.push_back(mountainTsx->tileAt(mountainTileList.at(mountainIndex).toInt()));
                    mountainIndex++;
                }
            }
        }
        unsigned int chunkY=0;
        while(chunkY<mapYCount && mountainTile!=NULL)
        {
            unsigned int chunkX=0;
            while(chunkX<mapXCount)
            {
                bool isLaneChunk=false;
                if(roadCoordToIndex.find((uint16_t)chunkX)!=roadCoordToIndex.cend()
                        && roadCoordToIndex.at((uint16_t)chunkX).find((uint16_t)chunkY)
                           !=roadCoordToIndex.at((uint16_t)chunkX).cend())
                {
                    const RoadIndex &roadIndex=roadCoordToIndex.at((uint16_t)chunkX).at((uint16_t)chunkY);
                    isLaneChunk=(roadIndex.isWater && !roadIndex.isBoat);
                }
                if(isLaneChunk)
                {
                    seedChunk(setting.seed,chunkX,chunkY,ChunkPass_sea);
                    if((unsigned int)(rand()%100)<setting.waterIslandPercent)
                    {
                        int radius=1;
                        while((unsigned int)(3*radius*radius)<setting.waterIslandMinTiles && radius<5)
                            radius++;
                        //somewhere in the open water of the lane, away from the border
                        int tries=0;
                        bool placed=false;
                        while(tries<40 && !placed)
                        {
                            tries++;
                            const int centerX=(int)(chunkX*mapWidth)+radius+2
                                    +(int)(rand()%(mapWidth-2*(unsigned int)radius-4));
                            const int centerY=(int)(chunkY*mapHeight)+radius+2
                                    +(int)(rand()%(mapHeight-2*(unsigned int)radius-4));
                            std::vector<unsigned int> islandCells;
                            bool fits=true;
                            int islandY=-radius;
                            while(islandY<=radius && fits)
                            {
                                int islandX=-radius;
                                while(islandX<=radius && fits)
                                {
                                    if(islandX*islandX+islandY*islandY<=radius*radius)
                                    {
                                        const unsigned int cell=(unsigned int)(centerX+islandX)
                                                +(unsigned int)(centerY+islandY)*worldWidth;
                                        if(water.at(cell)==0 || blocked.at(cell)!=0 || allowed.at(cell)==0)
                                            fits=false;
                                        else
                                            islandCells.push_back(cell);
                                    }
                                    islandX++;
                                }
                                islandY++;
                            }
                            if(fits && islandCells.size()>=setting.waterIslandMinTiles)
                            {
                                const bool landable=((unsigned int)(rand()%100)<setting.waterIslandLandablePercent);
                                std::set<unsigned int> islandSet(islandCells.cbegin(),islandCells.cend());
                                unsigned int cellIndex=0;
                                while(cellIndex<islandCells.size())
                                {
                                    const unsigned int cell=islandCells.at(cellIndex);
                                    const unsigned int tileX=cell%worldWidth;
                                    const unsigned int tileY=cell/worldWidth;
                                    waterLayer->setCell(tileX,tileY,Tiled::Cell());
                                    walkLayer->setCell(tileX,tileY,Tiled::Cell(mountainTile));
                                    water[cell]=0;
                                    allowed[cell]=0;
                                    uint8_t toTypeMatch=0;
                                    const int neighbourX[8]={-1,0,1,-1,1,-1,0,1};
                                    const int neighbourY[8]={-1,-1,-1,0,0,1,1,1};
                                    const uint8_t neighbourBit[8]={1,2,4,8,16,32,64,128};
                                    unsigned int neighbourIndex=0;
                                    while(neighbourIndex<8)
                                    {
                                        const unsigned int neighbour=(unsigned int)((int)tileX+neighbourX[neighbourIndex])
                                                +(unsigned int)((int)tileY+neighbourY[neighbourIndex])*worldWidth;
                                        if(islandSet.find(neighbour)==islandSet.cend())
                                            toTypeMatch|=neighbourBit[neighbourIndex];
                                        neighbourIndex++;
                                    }
                                    if(toTypeMatch!=0 && !mountainBorderTiles.empty())
                                    {
                                        const unsigned int borderIndex=mountainBorderTileIndex(toTypeMatch);
                                        if(borderIndex<mountainBorderTiles.size()
                                                && mountainBorderTiles.at(borderIndex)!=NULL)
                                        {
                                            colliLayer->setCell(tileX,tileY,
                                                                Tiled::Cell(mountainBorderTiles.at(borderIndex)));
                                            blocked[cell]=1;
                                        }
                                    }
                                    else if(!landable)
                                    {
                                        colliLayer->setCell(tileX,tileY,Tiled::Cell(mountainTile));
                                        blocked[cell]=1;
                                    }
                                    cellIndex++;
                                }
                                placed=true;
                                islandCount++;
                            }
                        }
                    }
                }
                chunkX++;
            }
            chunkY++;
        }
    }

    //=== 6) THE ROCK =========================================================
    //A LINE, never a sea filled with it: the water cells the player may NOT use
    //that touch one they may. The player walks on four directions, so that line is
    //closed by construction, and because the allowed side is the beach band it
    //follows the coast — it never runs along a chunk border and never cuts the
    //land, so the shore keeps its shape and can always be landed on.
    unsigned int wallCells=0;
    {
        unsigned int tileY=0;
        while(tileY<worldHeight)
        {
            unsigned int tileX=0;
            while(tileX<worldWidth)
            {
                const unsigned int cell=tileX+tileY*worldWidth;
                if(water.at(cell)!=0 && allowed.at(cell)==0 && blocked.at(cell)==0)
                {
                    //(only the open sea is ever closed: allowed already covers
                    //every lake and every pond, so none of them reaches here)
                    const int stepX[4]={-1,1,0,0};
                    const int stepY[4]={0,0,-1,1};
                    bool touchesAllowed=false;
                    unsigned int direction=0;
                    while(direction<4)
                    {
                        const int nextX=(int)tileX+stepX[direction];
                        const int nextY=(int)tileY+stepY[direction];
                        if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                        {
                            const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                            if(allowed.at(next)!=0 && water.at(next)!=0)
                                touchesAllowed=true;
                        }
                        direction++;
                    }
                    if(touchesAllowed)
                    {
                        colliLayer->setCell(tileX,tileY,Tiled::Cell(rockTile));
                        blocked[cell]=1;
                        seaWallCells[cell]=1;
                        wallCells++;
                    }
                }
                tileX++;
            }
            tileY++;
        }
    }
    seaAllowedCells=allowed;

    //=== 7) THE BOATS ========================================================
    //The ship lies HORIZONTALLY on the water, inside the rock line, and has to
    //touch the land by the SIDE of a tile (never a corner): the best berth is the
    //one with the most tiles of shore along its length. The push-teleport goes on
    //the ship tile that touches the land, the most centred one when there are
    //several — and the player arrives on the QUAY of the other side, never on the
    //boat tile itself.
    unsigned int mooredBoats=0;
    {
        unsigned int shipWidth=1,shipHeight=1;
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
        Tiled::ObjectGroup * const movingGroup=LoadMap::searchObjectGroupByName(worldMap,"Moving");
        const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
        unsigned int crossingIndex=0;
        while(crossingIndex<boatCrossings.size())
        {
            unsigned int side=0;
            while(side<2)
            {
                const BoatCrossing &crossing=boatCrossings.at(crossingIndex);
                const unsigned int chunkX=(side==0)?crossing.fromX:crossing.toX;
                const unsigned int chunkY=(side==0)?crossing.fromY:crossing.toY;
                const unsigned int otherX=(side==0)?crossing.toX:crossing.fromX;
                const unsigned int otherY=(side==0)?crossing.toY:crossing.fromY;
                const unsigned int x0=chunkX*mapWidth;
                const unsigned int y0=chunkY*mapHeight;
                int bestCell=-1;
                int bestScore=-1;
                unsigned int localY=0;
                while(localY+shipHeight<=mapHeight)
                {
                    unsigned int localX=0;
                    while(localX+shipWidth<=mapWidth)
                    {
                        bool berthFree=true;
                        unsigned int shipRow=0;
                        while(shipRow<shipHeight && berthFree)
                        {
                            unsigned int shipColumn=0;
                            while(shipColumn<shipWidth && berthFree)
                            {
                                const unsigned int cell=(x0+localX+shipColumn)
                                        +(y0+localY+shipRow)*worldWidth;
                                if(water.at(cell)==0 || blocked.at(cell)!=0 || allowed.at(cell)==0)
                                    berthFree=false;
                                shipColumn++;
                            }
                            shipRow++;
                        }
                        if(berthFree)
                        {
                            //the shore along the LENGTH of the ship (its horizontal
                            //sides) is what makes a berth: a quay it can be tied to
                            unsigned int alongShore=0;
                            unsigned int anyShore=0;
                            unsigned int shipColumn=0;
                            while(shipColumn<shipWidth)
                            {
                                const unsigned int aboveCell=(x0+localX+shipColumn)
                                        +(y0+localY-1)*worldWidth;
                                const unsigned int belowCell=(x0+localX+shipColumn)
                                        +(y0+localY+shipHeight)*worldWidth;
                                if(localY>0 && water.at(aboveCell)==0)
                                {
                                    anyShore++;
                                    if(blocked.at(aboveCell)==0)
                                        alongShore++;
                                }
                                if(localY+shipHeight<mapHeight && water.at(belowCell)==0)
                                {
                                    anyShore++;
                                    if(blocked.at(belowCell)==0)
                                        alongShore++;
                                }
                                shipColumn++;
                            }
                            //a walkable quay beats a cliff coast, a cliff coast
                            //beats nothing at all
                            const int score=(int)(alongShore*100+anyShore);
                            if(anyShore>0 && score>bestScore)
                            {
                                bestScore=score;
                                bestCell=(int)(localX+localY*mapWidth);
                            }
                        }
                        localX++;
                    }
                    localY++;
                }
                if(bestCell<0)
                    std::cerr << "no shore in the map " << chunkX << "," << chunkY
                              << " can moor a boat, the crossing to " << otherX << "," << otherY
                              << " has none" << std::endl;
                else
                {
                    const unsigned int shipX=x0+(unsigned int)bestCell%mapWidth;
                    const unsigned int shipY=y0+(unsigned int)bestCell/mapWidth;
                    stampTileRect(worldMap,colliLayer,setting.waterShipUsable,shipX,shipY,waterLayer);
                    //the ship is a collision now: the player pushes against it
                    {
                        unsigned int shipRow=0;
                        while(shipRow<shipHeight)
                        {
                            unsigned int shipColumn=0;
                            while(shipColumn<shipWidth)
                            {
                                blocked[(shipX+shipColumn)+(shipY+shipRow)*worldWidth]=1;
                                shipColumn++;
                            }
                            shipRow++;
                        }
                    }
                    //the ship tile the land touches, the most centred one, and the
                    //quay cell in front of it the far side will land on
                    int teleportX=-1,teleportY=-1,landX=-1,landY=-1;
                    {
                        const int middle=(int)shipX+(int)shipWidth/2;
                        int bestDistance=0;
                        unsigned int shipColumn=0;
                        while(shipColumn<shipWidth)
                        {
                            unsigned int shipRow=0;
                            while(shipRow<shipHeight)
                            {
                                const unsigned int tileX=shipX+shipColumn;
                                const unsigned int tileY=shipY+shipRow;
                                const int stepX[4]={0,0,-1,1};
                                const int stepY[4]={-1,1,0,0};
                                unsigned int direction=0;
                                while(direction<4)
                                {
                                    const int quayX=(int)tileX+stepX[direction];
                                    const int quayY=(int)tileY+stepY[direction];
                                    if(quayX>=0 && quayY>=0 && quayX<(int)worldWidth && quayY<(int)worldHeight)
                                    {
                                        const unsigned int quay=(unsigned int)quayX+(unsigned int)quayY*worldWidth;
                                        //the quay must be walkable and in the same
                                        //map, else the player lands in a wall
                                        if(blocked.at(quay)==0
                                                && (unsigned int)quayX/mapWidth==chunkX
                                                && (unsigned int)quayY/mapHeight==chunkY
                                                && (water.at(quay)==0 || allowed.at(quay)!=0))
                                        {
                                            const int distance=abs((int)tileX-middle);
                                            if(teleportX<0 || distance<bestDistance)
                                            {
                                                bestDistance=distance;
                                                teleportX=(int)tileX;
                                                teleportY=(int)tileY;
                                                landX=quayX;
                                                landY=quayY;
                                            }
                                        }
                                    }
                                    direction++;
                                }
                                shipRow++;
                            }
                            shipColumn++;
                        }
                    }
                    if(teleportX<0 || movingGroup==NULL)
                        std::cerr << "the boat of the map " << chunkX << "," << chunkY
                                  << " has no cell the player can push it from" << std::endl;
                    else
                    {
                        boatLandingCells[std::pair<uint16_t,uint16_t>((uint16_t)chunkX,(uint16_t)chunkY)]=
                                std::pair<uint8_t,uint8_t>((uint8_t)((unsigned int)landX%mapWidth),
                                                           (uint8_t)((unsigned int)landY%mapHeight));
                        Tiled::MapObject * const boat=new Tiled::MapObject("","teleport on push",
                            QPointF(teleportX*worldMap.tileWidth(),teleportY*worldMap.tileHeight()),
                            QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                        const QDir mapDir(QFileInfo(QString::fromStdString(getMapFile(chunkX,chunkY))).absoluteDir());
                        boat->setProperty("map",mapDir.relativeFilePath(
                                              QString::fromStdString(getMapFile(otherX,otherY))));
                        //filled in once BOTH shores moored (see wireBoatCrossings)
                        boat->setProperty("x","0");
                        boat->setProperty("y","0");
                        if(invisibleTileset!=NULL)
                        {
                            Tiled::Cell boatMarker;
                            boatMarker.setTile(invisibleTileset->tileAt(2));
                            boat->setCell(boatMarker);
                        }
                        movingGroup->addObject(boat);
                        boatTeleportObjects[std::pair<uint16_t,uint16_t>((uint16_t)chunkX,(uint16_t)chunkY)]=boat;
                        maskVegetationAround(worldMap,(unsigned int)landX,(unsigned int)landY,1);
                        mooredBoats++;
                    }
                }
                side++;
            }
            crossingIndex++;
        }
    }

    //=== 8) WHAT LIVES ON THE ROUTE ==========================================
    //template/sea decorations and their fight bot, then the lone swimmers. Both
    //only ever go on the open water of a swimmable lane, never in a harbour: a
    //ferry chunk is a quay, the player never swims there.
    unsigned int seaDecorationCount=0;
    unsigned int swimmerCount=0;
    {
        Tiled::ObjectGroup * const objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");
        const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
        static const char * const lookDirs[4]={"bottom","top","left","right"};
        unsigned int chunkY=0;
        while(chunkY<mapYCount)
        {
            unsigned int chunkX=0;
            while(chunkX<mapXCount)
            {
                bool isLaneChunk=false;
                if(roadCoordToIndex.find((uint16_t)chunkX)!=roadCoordToIndex.cend()
                        && roadCoordToIndex.at((uint16_t)chunkX).find((uint16_t)chunkY)
                           !=roadCoordToIndex.at((uint16_t)chunkX).cend())
                {
                    const RoadIndex &roadIndex=roadCoordToIndex.at((uint16_t)chunkX).at((uint16_t)chunkY);
                    isLaneChunk=(roadIndex.isWater && !roadIndex.isBoat);
                }
                if(isLaneChunk && objectLayer!=NULL && invisibleTileset!=NULL)
                {
                    const unsigned int x0=chunkX*mapWidth;
                    const unsigned int y0=chunkY*mapHeight;
                    //own stream for this chunk, second sea pass
                    seedChunk(setting.seed,chunkX,chunkY,ChunkPass_seaContent);
                    //--- the decorations of template/sea -------------------------
                    unsigned int variantIndex=0;
                    while(variantIndex<seaDecorations.size())
                    {
                        const DecorationVariant &variant=seaDecorations.at(variantIndex);
                        unsigned int wanted=templateUseCount(variant.use);
                        const unsigned int width=variant.mapTemplate.width;
                        const unsigned int height=variant.mapTemplate.height;
                        unsigned int tries=0;
                        while(wanted>0 && tries<80 && width+4<mapWidth && height+4<mapHeight)
                        {
                            tries++;
                            const unsigned int localX=2+(unsigned int)(rand()%(int)(mapWidth-4-width));
                            const unsigned int localY=2+(unsigned int)(rand()%(int)(mapHeight-4-height));
                            //room INSIDE the rock: every cell open water of the lane,
                            //and the way past it must stay open, so a ring of lane
                            //water is asked for around the whole footprint
                            bool valid=true;
                            int cellY=-1;
                            while(cellY<=(int)height && valid)
                            {
                                int cellX=-1;
                                while(cellX<=(int)width && valid)
                                {
                                    const unsigned int tileX=x0+localX+(unsigned int)cellX;
                                    const unsigned int tileY=y0+localY+(unsigned int)cellY;
                                    const unsigned int cell=tileX+tileY*worldWidth;
                                    if(water.at(cell)==0 || blocked.at(cell)!=0 || allowed.at(cell)==0)
                                        valid=false;
                                    cellX++;
                                }
                                cellY++;
                            }
                            if(valid)
                            {
                                MapBrush::brushTheMap(worldMap,variant.mapTemplate,
                                                      x0+localX,y0+localY,MapBrush::mapMask,true);
                                unsigned int maskY=0;
                                while(maskY<height)
                                {
                                    unsigned int maskX=0;
                                    while(maskX<width)
                                    {
                                        const unsigned int cell=(x0+localX+maskX)+(y0+localY+maskY)*worldWidth;
                                        if(colliLayer->cellAt(x0+localX+maskX,y0+localY+maskY).tile()!=NULL)
                                            blocked[cell]=1;
                                        maskVegetationAround(worldMap,x0+localX+maskX,y0+localY+maskY,0);
                                        maskX++;
                                    }
                                    maskY++;
                                }
                                //ITS FIGHT BOT, on the open water beside it: a
                                //trainer waiting by the reef
                                {
                                    const int stepX[4]={-1,1,0,0};
                                    const int stepY[4]={0,0,-1,1};
                                    bool botPlaced=false;
                                    unsigned int direction=0;
                                    while(direction<4 && !botPlaced)
                                    {
                                        const unsigned int botX=x0+localX+(unsigned int)((int)(width/2)+stepX[direction]*(int)(width/2+1));
                                        const unsigned int botY=y0+localY+(unsigned int)((int)(height/2)+stepY[direction]*(int)(height/2+1));
                                        const unsigned int cell=botX+botY*worldWidth;
                                        if(botX<worldWidth && botY<worldHeight
                                                && water.at(cell)!=0 && blocked.at(cell)==0
                                                && allowed.at(cell)!=0)
                                        {
                                            Tiled::MapObject * const bot=new Tiled::MapObject("","bot",
                                                QPointF(botX*worldMap.tileWidth(),(botY+1)*worldMap.tileHeight()),
                                                QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                                            bot->setProperty("id",QString::number(1));
                                            bot->setProperty("lookAt",QString::fromLatin1(lookDirs[rand()%4]));
                                            if(!setting.botSkins.empty())
                                                bot->setProperty("skin",QString::fromStdString(
                                                                     setting.botSkins.at(rand()%setting.botSkins.size())));
                                            Tiled::Cell botCell;
                                            botCell.setTile(invisibleTileset->tileAt(0));
                                            bot->setCell(botCell);
                                            objectLayer->addObject(bot);
                                            maskVegetationAround(worldMap,botX,botY,2);
                                            botPlaced=true;
                                        }
                                        direction++;
                                    }
                                }
                                seaDecorationCount++;
                                wanted--;
                            }
                        }
                        variantIndex++;
                    }
                    //--- the swimmers -------------------------------------------
                    if(!setting.botSkins.empty())
                    {
                        unsigned int wanted=setting.waterMinFighter;
                        if(setting.waterMaxFighter>setting.waterMinFighter)
                            wanted+=rand()%(setting.waterMaxFighter-setting.waterMinFighter+1);
                        std::set<unsigned int> usedCells;
                        unsigned int tries=0;
                        while(wanted>0 && tries<200)
                        {
                            tries++;
                            const unsigned int tileX=x0+2+(unsigned int)(rand()%(int)(mapWidth-4));
                            const unsigned int tileY=y0+2+(unsigned int)(rand()%(int)(mapHeight-4));
                            const unsigned int cell=tileX+tileY*worldWidth;
                            bool covered=false;
                            unsigned int aboveIndex=0;
                            while(aboveIndex<abovePlayerLayers.size())
                            {
                                if(abovePlayerLayers.at(aboveIndex)->cellAt(tileX,tileY).tile()!=NULL)
                                    covered=true;
                                aboveIndex++;
                            }
                            if(!covered && water.at(cell)!=0 && blocked.at(cell)==0 && allowed.at(cell)!=0
                                    && usedCells.find(cell)==usedCells.cend())
                            {
                                usedCells.insert(cell);
                                Tiled::MapObject * const bot=new Tiled::MapObject("","bot",
                                    QPointF(tileX*worldMap.tileWidth(),(tileY+1)*worldMap.tileHeight()),
                                    QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                                bot->setProperty("id",QString::number(wanted));
                                bot->setProperty("lookAt",QString::fromLatin1(lookDirs[rand()%4]));
                                bot->setProperty("skin",QString::fromStdString(
                                                     setting.botSkins.at(rand()%setting.botSkins.size())));
                                Tiled::Cell botCell;
                                botCell.setTile(invisibleTileset->tileAt(0));
                                bot->setCell(botCell);
                                objectLayer->addObject(bot);
                                maskVegetationAround(worldMap,tileX,tileY,2);
                                swimmerCount++;
                                wanted--;
                            }
                        }
                    }
                    //nothing grows on the sea of a route
                    {
                        unsigned int localY=0;
                        while(localY<mapHeight)
                        {
                            unsigned int localX=0;
                            while(localX<mapWidth)
                            {
                                if(water.at((x0+localX)+(y0+localY)*worldWidth)!=0)
                                    maskVegetationAround(worldMap,x0+localX,y0+localY,0);
                                localX++;
                            }
                            localY++;
                        }
                    }
                }
                chunkX++;
            }
            chunkY++;
        }
    }

    std::cout << "sea: " << laneCount << " lane(s) drawn (" << carvedLaneCells
              << " cell(s) opened on the way), " << mooredBoats << " boat(s) moored, "
              << wallCells << " rock cell(s), " << islandCount << " islet(s), "
              << seaDecorationCount << " decoration(s), " << swimmerCount << " swimmer(s)"
              << std::endl;
}

bool LoadMapAll::checkSeaClosed(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting,
                                std::vector<std::string> &errors)
{
    //THE OPEN SEA IS NEVER REACHABLE. The rock is drawn on every water cell that
    //touches the swimmable region from outside, so the region is closed by
    //construction — this re-measures it on the map as it was really WRITTEN (the
    //vegetation and the repairs both run after the sea pass), which is the only
    //thing worth trusting. Only the maps the player can be on count: a chunk no
    //map is written for cannot be walked into in the first place.
    if(seaAllowedCells.empty())
        return true;
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    if(waterLayer==NULL)
        return true;
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
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    //open water: the Water layer holds a tile and nothing blocks it
    std::vector<unsigned char> openWater(worldWidth*worldHeight,0);
    {
        unsigned int tileY=0;
        while(tileY<worldHeight)
        {
            unsigned int tileX=0;
            while(tileX<worldWidth)
            {
                if(waterLayer->cellAt(tileX,tileY).tile()!=NULL)
                {
                    bool cellBlocked=false;
                    unsigned int layerIndex=0;
                    while(layerIndex<collisionLayers.size())
                    {
                        if(collisionLayers.at(layerIndex)->cellAt(tileX,tileY).tile()!=NULL)
                            cellBlocked=true;
                        layerIndex++;
                    }
                    if(!cellBlocked)
                        openWater[tileX+tileY*worldWidth]=1;
                }
                tileX++;
            }
            tileY++;
        }
    }
    unsigned int leaks=0;
    unsigned int tileY=0;
    while(tileY<worldHeight)
    {
        unsigned int tileX=0;
        while(tileX<worldWidth)
        {
            const unsigned int cell=tileX+tileY*worldWidth;
            if(seaAllowedCells.at(cell)!=0 && openWater.at(cell)!=0
                    && mapPathDirection[(tileX/setting.mapWidth)
                                        +(tileY/setting.mapHeight)*setting.mapXCount]!=0)
            {
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=(int)tileX+stepX[direction];
                    const int nextY=(int)tileY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                        if(seaAllowedCells.at(next)==0 && openWater.at(next)!=0)
                        {
                            if(leaks<20)
                                errors.push_back("the open sea is reachable from "
                                                 +std::to_string(tileX)+","+std::to_string(tileY));
                            leaks++;
                        }
                    }
                    direction++;
                }
            }
            tileX++;
        }
        tileY++;
    }
    if(leaks>0)
        std::cerr << "sea: " << leaks << " way(s) out into the open sea" << std::endl;
    return leaks==0;
}
