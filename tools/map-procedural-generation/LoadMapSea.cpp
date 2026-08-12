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

//THE CHUNK AS THE PLAYER WALKS IT ON FOOT — water is a wall here ON PURPOSE.
//A ferry map is mostly sea, so its biggest walkable component is the basin, and
//a quay that can only be reached by swimming is reached by nobody: the boat is
//what the player takes when they CANNOT swim. -1 = not walkable on foot.
static void seaFootComponents(const std::vector<unsigned char> &water,
                              const std::vector<unsigned char> &blocked,
                              const unsigned int &worldWidth,
                              const unsigned int &chunkX,const unsigned int &chunkY,
                              const unsigned int &mapWidth,const unsigned int &mapHeight,
                              std::vector<int> &component)
{
    component.assign(mapWidth*mapHeight,-1);
    const unsigned int x0=chunkX*mapWidth;
    const unsigned int y0=chunkY*mapHeight;
    int componentCount=0;
    std::vector<unsigned int> queue;
    unsigned int startCell=0;
    while(startCell<mapWidth*mapHeight)
    {
        const unsigned int startWorld=(x0+startCell%mapWidth)+(y0+startCell/mapWidth)*worldWidth;
        if(component.at(startCell)<0 && blocked.at(startWorld)==0 && water.at(startWorld)==0)
        {
            component[startCell]=componentCount;
            queue.clear();
            queue.push_back(startCell);
            unsigned int queueIndex=0;
            while(queueIndex<queue.size())
            {
                const unsigned int cell=queue.at(queueIndex);
                queueIndex++;
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
                        const unsigned int nextWorld=(x0+(unsigned int)nextX)+(y0+(unsigned int)nextY)*worldWidth;
                        if(component.at(next)<0 && blocked.at(nextWorld)==0 && water.at(nextWorld)==0)
                        {
                            component[next]=componentCount;
                            queue.push_back(next);
                        }
                    }
                    direction++;
                }
            }
            componentCount++;
        }
        startCell++;
    }
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

    //=== 2) WHAT IS SWIMMABLE AT ALL =========================================
    //Anything that is NOT the open sea — a lake, a pond, a river mouth — is swum
    //end to end. The open sea starts closed: it is opened only where a route
    //needs it (the pools below and the lanes), because THE ROCK IS THERE TO GUIDE
    //THE PLAYER, not to draw a ring around the continent.
    std::vector<unsigned char> allowed(worldWidth*worldHeight,0);
    {
        unsigned int cell=0;
        while(cell<worldWidth*worldHeight)
        {
            if(water.at(cell)!=0 && openSea.at(cell)==0)
                allowed[cell]=1;
            cell++;
        }
    }

    //=== 3) THE LANES OF THE SWIMMABLE ROUTES ================================
    //From the beach of one end to the beach of the other, through the chunks the
    //route was planned on and NOTHING else — a lane through a chunk no map is
    //written for could never be swum. Water costs least, walkable shore a little
    //more, a cliff or a wood far more: the way follows the sea, walks the beach
    //when it must and only cuts through what really blocks it.
    unsigned int laneCount=0;
    //the ground beside every beach: where the walk to the town starts
    std::vector<unsigned int> shoreLandings;
    //every water cell of a lane that touches walkable ground: the beaches of the
    //world, and the only places the open sea is ever opened
    std::vector<unsigned int> poolSeeds;
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
                //A WATER PATH NEVER CROSSES LAND — hard rule. The lane is a way
                //through WATER ONLY, from the beach of one end to the beach of the
                //other: a BEACH is a water cell of the first (or last) two chunks
                //of the route that touches ground the player can stand on. What
                //happens on land is the SHORE PATH's business, not the lane's.
                std::vector<unsigned char> isTarget(worldWidth*worldHeight,0);
                std::vector<unsigned int> cost(worldWidth*worldHeight,0xFFFFFFFF);
                std::vector<int> parent(worldWidth*worldHeight,-1);
                std::priority_queue<std::pair<unsigned int,unsigned int>,
                        std::vector<std::pair<unsigned int,unsigned int> >,
                        std::greater<std::pair<unsigned int,unsigned int> > > walk;
                {
                    unsigned int endIndex=0;
                    while(endIndex<2)
                    {
                        //the land end and the first sea chunk next to it
                        const unsigned int landChunk=(endIndex==0)?startChunk:endChunk;
                        const unsigned int seaChunk=(endIndex==0)?route.chunks.at(1)
                                                                :route.chunks.at(route.chunks.size()-2);
                        unsigned int side=0;
                        while(side<2)
                        {
                            const unsigned int chunk=(side==0)?landChunk:seaChunk;
                            const unsigned int x0=(chunk%mapXCount)*mapWidth;
                            const unsigned int y0=(chunk/mapXCount)*mapHeight;
                            unsigned int localY=0;
                            while(localY<mapHeight)
                            {
                                unsigned int localX=0;
                                while(localX<mapWidth)
                                {
                                    const unsigned int cell=(x0+localX)+(y0+localY)*worldWidth;
                                    if(water.at(cell)!=0 && blocked.at(cell)==0)
                                    {
                                        //a BEACH: ground beside it the player stands on
                                        const int stepX[4]={-1,1,0,0};
                                        const int stepY[4]={0,0,-1,1};
                                        bool beach=false;
                                        unsigned int direction=0;
                                        while(direction<4)
                                        {
                                            const int shoreX=(int)(x0+localX)+stepX[direction];
                                            const int shoreY=(int)(y0+localY)+stepY[direction];
                                            if(shoreX>=0 && shoreY>=0 && shoreX<(int)worldWidth
                                                    && shoreY<(int)worldHeight)
                                            {
                                                const unsigned int shore=(unsigned int)shoreX
                                                        +(unsigned int)shoreY*worldWidth;
                                                if(water.at(shore)==0 && blocked.at(shore)==0)
                                                    beach=true;
                                            }
                                            direction++;
                                        }
                                        if(beach)
                                        {
                                            if(endIndex==0)
                                            {
                                                if(cost.at(cell)!=0)
                                                {
                                                    cost[cell]=0;
                                                    walk.push(std::pair<unsigned int,unsigned int>(0,cell));
                                                }
                                            }
                                            else
                                                isTarget[cell]=1;
                                        }
                                    }
                                    localX++;
                                }
                                localY++;
                            }
                            side++;
                        }
                        endIndex++;
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
                        if(isTarget.at(cell)!=0 && cost.at(cell)>0)
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
                                    //WATER ONLY, inside the route, and a border is
                                    //only crossed between two chunks the route links
                                    bool legal=(routePosition.at(toChunk)>=0
                                                && water.at(next)!=0 && blocked.at(next)==0);
                                    if(legal && toChunk!=fromChunk)
                                        if(abs(routePosition.at(toChunk)-routePosition.at(fromChunk))!=1)
                                            legal=false;
                                    if(legal && cost.at(cell)+1<cost.at(next))
                                    {
                                        cost[next]=cost.at(cell)+1;
                                        parent[next]=(int)cell;
                                        walk.push(std::pair<unsigned int,unsigned int>(cost.at(next),next));
                                    }
                                }
                                direction++;
                            }
                        }
                    }
                }
                if(reached<0)
                {
                    //NO WATER JOINS THE TWO BEACHES: the link becomes a FERRY
                    //rather than a lane drawn over the land. The harbours are the
                    //sea chunks of each end, and the pass below moors both boats.
                    const unsigned int harbourA=route.chunks.at(1);
                    const unsigned int harbourB=route.chunks.at(route.chunks.size()-2);
                    std::cerr << "no water joins the beaches of the map "
                              << startChunk%mapXCount << "," << startChunk/mapXCount << " and "
                              << endChunk%mapXCount << "," << endChunk/mapXCount
                              << ": the route becomes a boat crossing" << std::endl;
                    if(harbourA!=harbourB)
                    {
                        BoatCrossing crossing;
                        crossing.fromX=(uint16_t)(harbourA%mapXCount);
                        crossing.fromY=(uint16_t)(harbourA/mapXCount);
                        crossing.toX=(uint16_t)(harbourB%mapXCount);
                        crossing.toY=(uint16_t)(harbourB/mapXCount);
                        boatCrossings.push_back(crossing);
                        roadCoordToIndex[(uint16_t)(harbourA%mapXCount)][(uint16_t)(harbourA/mapXCount)].isBoat=true;
                        roadCoordToIndex[(uint16_t)(harbourB%mapXCount)][(uint16_t)(harbourB/mapXCount)].isBoat=true;
                    }
                }
                else
                {
                    //the way itself: water from end to end. Its two extremities
                    //ARE the beaches — the pool of open water the player enters
                    //the sea by is grown from them, and the SHORE PATH to the town
                    //starts from the ground beside them.
                    std::vector<unsigned int> lane;
                    int walkCell=reached;
                    while(walkCell>=0)
                    {
                        const unsigned int cell=(unsigned int)walkCell;
                        lane.push_back(cell);
                        const unsigned int tileX=cell%worldWidth;
                        const unsigned int tileY=cell/worldWidth;
                        allowed[cell]=1;
                        //where the lane touches ground the player stands on, it is
                        //a beach: the pool is grown from there and nowhere else
                        {
                            const int stepX[4]={-1,1,0,0};
                            const int stepY[4]={0,0,-1,1};
                            unsigned int direction=0;
                            while(direction<4)
                            {
                                const int shoreX=(int)tileX+stepX[direction];
                                const int shoreY=(int)tileY+stepY[direction];
                                if(shoreX>=0 && shoreY>=0 && shoreX<(int)worldWidth && shoreY<(int)worldHeight)
                                {
                                    const unsigned int shore=(unsigned int)shoreX+(unsigned int)shoreY*worldWidth;
                                    if(water.at(shore)==0 && blocked.at(shore)==0)
                                    {
                                        poolSeeds.push_back(cell);
                                        shoreLandings.push_back(shore);
                                    }
                                }
                                direction++;
                            }
                        }
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

    //=== 3b) THE POOLS: the framed water of every beach =======================
    //A pool is grown from ONE beach — the cells where a lane meets walkable
    //ground — until it holds at least [water] poolMinTiles cells. Its rock is
    //therefore a FRAME: it runs out into the sea, comes back, and ENDS ON THE
    //LAND at both ends of the beach, with the way in left open. Every other
    //stretch of coast stays closed (see the rim, below), so the rock guides the
    //player to the water instead of ringing the whole continent.
    unsigned int poolCount=0;
    {
        std::vector<unsigned char> seedUsed(worldWidth*worldHeight,0);
        std::vector<unsigned char> isSeed(worldWidth*worldHeight,0);
        {
            unsigned int seedIndex=0;
            while(seedIndex<poolSeeds.size())
            {
                isSeed[poolSeeds.at(seedIndex)]=1;
                seedIndex++;
            }
        }
        unsigned int seedIndex=0;
        while(seedIndex<poolSeeds.size())
        {
            if(seedUsed.at(poolSeeds.at(seedIndex))==0)
            {
                //ONE BEACH: the seeds that touch one another are the same way in
                std::vector<unsigned int> beach;
                {
                    std::vector<unsigned int> queue;
                    queue.push_back(poolSeeds.at(seedIndex));
                    seedUsed[poolSeeds.at(seedIndex)]=1;
                    unsigned int queueIndex=0;
                    while(queueIndex<queue.size())
                    {
                        const unsigned int cell=queue.at(queueIndex);
                        queueIndex++;
                        beach.push_back(cell);
                        const int cellX=(int)(cell%worldWidth);
                        const int cellY=(int)(cell/worldWidth);
                        int stepY=-1;
                        while(stepY<=1)
                        {
                            int stepX=-1;
                            while(stepX<=1)
                            {
                                const int nextX=cellX+stepX;
                                const int nextY=cellY+stepY;
                                if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                                {
                                    const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                                    if(isSeed.at(next)!=0 && seedUsed.at(next)==0)
                                    {
                                        seedUsed[next]=1;
                                        queue.push_back(next);
                                    }
                                }
                                stepX++;
                            }
                            stepY++;
                        }
                    }
                }
                //...and its POOL, grown until it is big enough to be a bay
                unsigned int extra=0;
                bool done=false;
                while(!done)
                {
                    std::vector<int> distance(worldWidth*worldHeight,-1);
                    std::vector<unsigned int> queue;
                    unsigned int beachIndex=0;
                    while(beachIndex<beach.size())
                    {
                        distance[beach.at(beachIndex)]=0;
                        queue.push_back(beach.at(beachIndex));
                        beachIndex++;
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
                                if(distance.at(next)<0 && water.at(next)!=0 && blocked.at(next)==0)
                                {
                                    const unsigned int limit=seaBandLimit((unsigned int)nextX,
                                                                          (unsigned int)nextY,setting)+extra;
                                    if((unsigned int)(distance.at(cell)+1)<=limit)
                                    {
                                        distance[next]=distance.at(cell)+1;
                                        queue.push_back(next);
                                    }
                                }
                            }
                            direction++;
                        }
                    }
                    //big enough, or as big as this bay will ever get
                    if(queue.size()>=setting.waterPoolMinTiles || extra>=4*setting.waterBeachMax)
                    {
                        unsigned int poolIndex=0;
                        while(poolIndex<queue.size())
                        {
                            allowed[queue.at(poolIndex)]=1;
                            poolIndex++;
                        }
                        done=true;
                        poolCount++;
                    }
                    else
                        extra+=4;
                }
            }
            seedIndex++;
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

    //=== 6b) THE EDGE IS RAGGED ==============================================
    //A wall drawn along a straight edge reads as a wall, and one straight rock
    //line with a single rock nudged out every N tiles reads as WORSE: a pattern.
    //So the EDGE of the water the player may use is roughened here, before any
    //rock is drawn: every cell just outside it is given to the water or not, on a
    //hash of its own position, twice over. What comes out is a coast of one and
    //two tile bites — the shape the rock then follows — and no run of it repeats.
    {
        unsigned int roughened=0;
        unsigned int pass=0;
        while(pass<2)
        {
            std::vector<unsigned int> edge;
            unsigned int cell=0;
            while(cell<worldWidth*worldHeight)
            {
                if(water.at(cell)!=0 && blocked.at(cell)==0 && allowed.at(cell)==0)
                {
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
                            if(allowed.at(next)!=0 && water.at(next)!=0)
                            {
                                edge.push_back(cell);
                                direction=4;
                            }
                        }
                        direction++;
                    }
                }
                cell++;
            }
            unsigned int edgeIndex=0;
            while(edgeIndex<edge.size())
            {
                const unsigned int edgeCell=edge.at(edgeIndex);
                if(seaHash(edgeCell%worldWidth,edgeCell/worldWidth,setting.seed+4242+pass)%100<40)
                {
                    allowed[edgeCell]=1;
                    roughened++;
                }
                edgeIndex++;
            }
            pass++;
        }
        std::cout << "sea: " << roughened << " cell(s) of edge given to the water, "
                  << "so the rock never runs straight" << std::endl;
    }

    //WHAT THE PLAYER MAY SWIM IN, for the rest of the run. The rock itself is
    //NOT drawn here: it is drawn once the vegetation is down and the walkability
    //repairs are over (closeSeaAccess), because a shore under a tree is no shore
    //at all and needs no wall in front of it.
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
                //WHERE THE PLAYER ARRIVES ON FOOT: the ground the linked borders
                //of this map open on. The boat has to be moored against THAT, not
                //against any coast — a ship tied to the far side of a wood is a
                //ferry nobody can take.
                std::vector<int> footComponent;
                std::set<int> arrivalComponent;
                seaFootComponents(water,blocked,worldWidth,chunkX,chunkY,mapWidth,mapHeight,footComponent);
                {
                    const uint8_t links=mapPathDirection[chunkX+chunkY*mapXCount];
                    unsigned int step=0;
                    while(step<mapHeight)
                    {
                        if((links&Orientation_left)!=0 && footComponent.at(step*mapWidth)>=0)
                            arrivalComponent.insert(footComponent.at(step*mapWidth));
                        if((links&Orientation_right)!=0 && footComponent.at(mapWidth-1+step*mapWidth)>=0)
                            arrivalComponent.insert(footComponent.at(mapWidth-1+step*mapWidth));
                        step++;
                    }
                    step=0;
                    while(step<mapWidth)
                    {
                        if((links&Orientation_top)!=0 && footComponent.at(step)>=0)
                            arrivalComponent.insert(footComponent.at(step));
                        if((links&Orientation_bottom)!=0 && footComponent.at(step+(mapHeight-1)*mapWidth)>=0)
                            arrivalComponent.insert(footComponent.at(step+(mapHeight-1)*mapWidth));
                        step++;
                    }
                }
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
                            //sides) is what makes a berth: a quay it can be tied to.
                            //A quay the player REACHES ON FOOT from the border of
                            //the map is worth far more than any other, and a bare
                            //cliff coast is the last resort.
                            unsigned int connectedShore=0;
                            unsigned int walkableShore=0;
                            unsigned int anyShore=0;
                            unsigned int shipColumn=0;
                            while(shipColumn<shipWidth)
                            {
                                unsigned int quaySide=0;
                                while(quaySide<2)
                                {
                                    const bool above=(quaySide==0);
                                    if((above && localY>0) || (!above && localY+shipHeight<mapHeight))
                                    {
                                        const unsigned int quayLocalX=localX+shipColumn;
                                        const unsigned int quayLocalY=above?(localY-1):(localY+shipHeight);
                                        const unsigned int quayCell=(x0+quayLocalX)+(y0+quayLocalY)*worldWidth;
                                        if(water.at(quayCell)==0)
                                        {
                                            anyShore++;
                                            if(blocked.at(quayCell)==0)
                                            {
                                                walkableShore++;
                                                if(arrivalComponent.find(footComponent.at(
                                                       quayLocalX+quayLocalY*mapWidth))!=arrivalComponent.cend())
                                                    connectedShore++;
                                            }
                                        }
                                    }
                                    quaySide++;
                                }
                                shipColumn++;
                            }
                            const int score=(int)(connectedShore*10000+walkableShore*100+anyShore);
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
                    //THE CELL THE PLAYER PUSHES, and the QUAY they land on. The
                    //teleport goes on the ship tile the LAND touches, the most
                    //centred one; the quay is that land cell. Ranked: ground the
                    //border of the map really reaches on foot, then any open
                    //ground, then a coast that has to be opened — and only if the
                    //ship touches no land at all does it fall back to the water,
                    //which needs the swim item and is no ferry berth.
                    int teleportX=-1,teleportY=-1,landX=-1,landY=-1;
                    int bestQuayRank=0;
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
                                    if(quayX>=0 && quayY>=0 && quayX<(int)worldWidth && quayY<(int)worldHeight
                                            && (unsigned int)quayX/mapWidth==chunkX
                                            && (unsigned int)quayY/mapHeight==chunkY)
                                    {
                                        const unsigned int quay=(unsigned int)quayX+(unsigned int)quayY*worldWidth;
                                        const unsigned int quayLocal=((unsigned int)quayX%mapWidth)
                                                +((unsigned int)quayY%mapHeight)*mapWidth;
                                        int rank=0;
                                        if(water.at(quay)==0)
                                        {
                                            if(blocked.at(quay)==0)
                                                rank=(arrivalComponent.find(footComponent.at(quayLocal))
                                                      !=arrivalComponent.cend())?4:3;
                                            else
                                                rank=2;//a coast to open, like a road corridor
                                        }
                                        else if(allowed.at(quay)!=0 && blocked.at(quay)==0)
                                            rank=1;//open water: the last resort
                                        const int distance=abs((int)tileX-middle);
                                        if(rank>0 && (rank>bestQuayRank
                                                      || (rank==bestQuayRank && distance<bestDistance)))
                                        {
                                            bestQuayRank=rank;
                                            bestDistance=distance;
                                            teleportX=(int)tileX;
                                            teleportY=(int)tileY;
                                            landX=quayX;
                                            landY=quayY;
                                        }
                                    }
                                    direction++;
                                }
                                shipRow++;
                            }
                            shipColumn++;
                        }
                    }
                    //a quay under a tree or on the cliff edge is OPENED: whole
                    //plants only, and whatever hung above the player with them
                    if(bestQuayRank==2 && landX>=0)
                    {
                        const unsigned int quay=(unsigned int)landX+(unsigned int)landY*worldWidth;
                        MapPlants::removePlantAt(worldMap,(unsigned int)landX,(unsigned int)landY);
                        unsigned int layerIndex=0;
                        while(layerIndex<collisionLayers.size())
                        {
                            collisionLayers.at(layerIndex)->setCell((unsigned int)landX,(unsigned int)landY,
                                                                    Tiled::Cell());
                            layerIndex++;
                        }
                        unsigned int aboveIndex=0;
                        while(aboveIndex<abovePlayerLayers.size())
                        {
                            abovePlayerLayers.at(aboveIndex)->setCell((unsigned int)landX,(unsigned int)landY,
                                                                      Tiled::Cell());
                            aboveIndex++;
                        }
                        blocked[quay]=0;
                        if(walkLayer->cellAt((unsigned int)landX,(unsigned int)landY).tile()==NULL)
                        {
                            Tiled::Tile * const groundTile=seaChunkGroundTile(walkLayer,chunkX,chunkY,
                                                                             mapWidth,mapHeight);
                            if(groundTile!=NULL)
                                walkLayer->setCell((unsigned int)landX,(unsigned int)landY,
                                                   Tiled::Cell(groundTile));
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
                        //THE WALK FROM THE QUAY TO THE TOWN is opened below, on
                        //the world (section 9): the vegetation is brushed after
                        //this pass and grew a wood right across the shore, and a
                        //way opened only inside this one chunk stops at its border.
                        shoreLandings.push_back((unsigned int)landX+(unsigned int)landY*worldWidth);
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
                    //THE ENGINE KEEPS ONE BOT PER CELL: two characters on the same
                    //tile and the second is dropped. Every bot already on this map
                    //counts — the road trainers, and the sea bots placed just now.
                    std::set<unsigned int> botCells;
                    {
                        const QList<Tiled::MapObject*> &objects=objectLayer->objects();
                        unsigned int objectIndex=0;
                        while(objectIndex<(unsigned int)objects.size())
                        {
                            const Tiled::MapObject * const object=objects.at(objectIndex);
                            if(object->type()=="bot")
                            {
                                const unsigned int botX=(unsigned int)(object->x()/worldMap.tileWidth());
                                const int botY=(int)(object->y()/worldMap.tileHeight())-1;
                                if(botY>=0)
                                    botCells.insert(botX+(unsigned int)botY*worldWidth);
                            }
                            objectIndex++;
                        }
                    }
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
                                                && allowed.at(cell)!=0
                                                && botCells.find(cell)==botCells.cend())
                                        {
                                            botCells.insert(cell);
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
                                    && botCells.find(cell)==botCells.cend())
                            {
                                botCells.insert(cell);
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

    //=== 9) THE WALK FROM THE WATER TO THE TOWN ==============================
    //A route that lands on a beach nobody can walk away from is no route. The way
    //from every landing — a lane beach, a ferry quay — to the NEAREST TOWN is
    //opened here, ON THE WORLD: it crosses as many maps as it needs to, because
    //the walk no more stops at a chunk border than the coast does. Open ground
    //costs 1 and a wood or a cliff 60, so it follows the road that is already
    //there and only cuts where it must — whole plants, never half a tree — and
    //the whole way is masked, so the vegetation brushed next does not close it.
    unsigned int shorePathCells=0;
    unsigned int shorePathOpened=0;
    {
        unsigned int landingIndex=0;
        while(landingIndex<shoreLandings.size())
        {
            const unsigned int landing=shoreLandings.at(landingIndex);
            //the nearest town
            int targetCell=-1;
            {
                unsigned int bestDistance=0;
                unsigned int cityIndex=0;
                while(cityIndex<cities.size())
                {
                    const unsigned int cityCell=(cities.at(cityIndex).x*mapWidth+mapWidth/2)
                            +(cities.at(cityIndex).y*mapHeight+mapHeight/2)*worldWidth;
                    const unsigned int distance=(unsigned int)(
                                abs((int)(cityCell%worldWidth)-(int)(landing%worldWidth))
                                +abs((int)(cityCell/worldWidth)-(int)(landing/worldWidth)));
                    if(targetCell<0 || distance<bestDistance)
                    {
                        bestDistance=distance;
                        targetCell=(int)cityCell;
                    }
                    cityIndex++;
                }
            }
            if(targetCell>=0 && blocked.at((unsigned int)targetCell)==0)
            {
                static const unsigned int costOpen=1;
                static const unsigned int costBlocked=60;
                static const unsigned int costUnreachable=0xFFFFFFFF;
                std::vector<unsigned int> cost(worldWidth*worldHeight,costUnreachable);
                std::vector<int> parent(worldWidth*worldHeight,-1);
                std::priority_queue<std::pair<unsigned int,unsigned int>,
                        std::vector<std::pair<unsigned int,unsigned int> >,
                        std::greater<std::pair<unsigned int,unsigned int> > > walk;
                cost[landing]=0;
                walk.push(std::pair<unsigned int,unsigned int>(0,landing));
                bool found=false;
                while(!walk.empty() && !found)
                {
                    const std::pair<unsigned int,unsigned int> top=walk.top();
                    walk.pop();
                    const unsigned int cell=top.second;
                    if(top.first<=cost.at(cell))
                    {
                        if(cell==(unsigned int)targetCell)
                            found=true;
                        else
                        {
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
                                    //ON FOOT, and on the maps that were written: the
                                    //water is not a way to the town, and a chunk with
                                    //no map is not a place the player can ever be
                                    if(water.at(next)==0 && mapPathDirection[nextChunk]!=0)
                                    {
                                        const unsigned int stepCost=(blocked.at(next)!=0)?costBlocked:costOpen;
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
                if(!found)
                    std::cerr << "no way on foot from the shore at " << landing%worldWidth
                              << "," << landing/worldWidth << " to a town" << std::endl;
                else
                {
                    int walkCell=targetCell;
                    while(walkCell>=0)
                    {
                        const unsigned int cell=(unsigned int)walkCell;
                        const unsigned int tileX=cell%worldWidth;
                        const unsigned int tileY=cell/worldWidth;
                        if(blocked.at(cell)!=0)
                        {
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
                            if(walkLayer->cellAt(tileX,tileY).tile()==NULL)
                            {
                                Tiled::Tile * const groundTile=seaChunkGroundTile(
                                            walkLayer,tileX/mapWidth,tileY/mapHeight,mapWidth,mapHeight);
                                if(groundTile!=NULL)
                                    walkLayer->setCell(tileX,tileY,Tiled::Cell(groundTile));
                            }
                            shorePathOpened++;
                        }
                        maskVegetationAround(worldMap,tileX,tileY,1);
                        shorePathCells++;
                        walkCell=parent.at(cell);
                    }
                }
            }
            landingIndex++;
        }
    }

    std::cout << "sea: " << laneCount << " lane(s) drawn, "
              << poolCount << " beach pool(s), "
              << shorePathCells << " cell(s) of shore path to a town ("
              << shorePathOpened << " opened), "
              << mooredBoats << " boat(s) moored, "
              << seaDecorationCount << " decoration(s), " << swimmerCount << " swimmer(s)"
              << std::endl;
}

//WHAT THE PLAYER CAN REACH, on the world, from the towns. Land and the water
//`swimmable` says they may use; a chunk no map is written for is not part of it.
//Crossing a map border is allowed wherever both sides carry a map — an over
//estimate on purpose: a rim of rock too many is a rock, a rim too few is a hole
//into the open sea.
static void seaReachableCells(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting,
                              const std::vector<unsigned char> &water,
                              const std::vector<unsigned char> &blocked,
                              const std::vector<unsigned char> *swimmable,
                              std::vector<unsigned char> &reached)
{
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
    reached.assign(worldWidth*worldHeight,0);
    std::vector<unsigned int> queue;
    //start where the player does: every town, so a world cut in two by a broken
    //link still gets its rim on both halves
    unsigned int cityIndex=0;
    while(cityIndex<LoadMapAll::cities.size())
    {
        const unsigned int cell=(LoadMapAll::cities.at(cityIndex).x*setting.mapWidth+setting.mapWidth/2)
                +(LoadMapAll::cities.at(cityIndex).y*setting.mapHeight+setting.mapHeight/2)*worldWidth;
        if(cell<worldWidth*worldHeight && blocked.at(cell)==0 && reached.at(cell)==0)
        {
            reached[cell]=1;
            queue.push_back(cell);
        }
        cityIndex++;
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
                const unsigned int nextChunk=((unsigned int)nextX/setting.mapWidth)
                        +((unsigned int)nextY/setting.mapHeight)*setting.mapXCount;
                if(reached.at(next)==0 && blocked.at(next)==0
                        && LoadMapAll::mapPathDirection[nextChunk]!=0)
                {
                    //water only where the caller says the player may swim
                    if(water.at(next)==0 || swimmable==NULL || swimmable->at(next)!=0)
                    {
                        reached[next]=1;
                        queue.push_back(next);
                    }
                }
            }
            direction++;
        }
    }
}

void LoadMapAll::closeSeaAccess(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting)
{
    //THE ROCK, drawn LAST — once the vegetation is down and the walkability
    //repairs are over. Two rules and nothing else:
    // 1) THE RIM: one tile of rock in the water against a shore THE PLAYER CAN
    //    REALLY STAND ON. A cell of the Walkable layer that also carries a
    //    Collisions tile (a tree, a cliff) is not a shore — the player is never
    //    there, so protecting the sea in front of it is a rock drawn for nobody.
    //    Neither is a patch of ground no way leads to. That is why this runs at
    //    the very end: the vegetation is brushed AFTER the sea is shaped, and it
    //    turns whole stretches of open coast into a wall of trees.
    // 2) THE FRAME: the water the player may NOT use that touches water they may.
    if(seaAllowedCells.empty())
        return;
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::TileLayer * const colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::Tile * const rockTile=setting.waterBorderTile.isEmpty()
            ?NULL:fetchTile(worldMap,setting.waterBorderTile);
    if(waterLayer==NULL || colliLayer==NULL || rockTile==NULL)
        return;
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
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
    //the ground and the pools the player really gets to
    std::vector<unsigned char> reached;
    seaReachableCells(worldMap,setting,water,blocked,&seaAllowedCells,reached);
    seaWallCells.assign(worldWidth*worldHeight,0);
    unsigned int rimCells=0;
    unsigned int frameCells=0;
    unsigned int tileY=0;
    while(tileY<worldHeight)
    {
        unsigned int tileX=0;
        while(tileX<worldWidth)
        {
            const unsigned int cell=tileX+tileY*worldWidth;
            if(water.at(cell)!=0 && blocked.at(cell)==0 && seaAllowedCells.at(cell)==0)
            {
                const int stepX[4]={-1,1,0,0};
                const int stepY[4]={0,0,-1,1};
                bool touchesTheShore=false;
                bool touchesAllowed=false;
                unsigned int direction=0;
                while(direction<4)
                {
                    const int nextX=(int)tileX+stepX[direction];
                    const int nextY=(int)tileY+stepY[direction];
                    if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth && nextY<(int)worldHeight)
                    {
                        const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidth;
                        //a shore the player STANDS ON: no collision, and a way
                        //really leads there
                        if(water.at(next)==0 && blocked.at(next)==0 && reached.at(next)!=0)
                            touchesTheShore=true;
                        //the FRAME never asks whether the water beside it is
                        //reached: what the generator opened on purpose is one
                        //connected piece (a pool, its lane, the next pool), so as
                        //soon as ONE of its beaches is walkable the whole of it is
                        if(water.at(next)!=0 && seaAllowedCells.at(next)!=0)
                            touchesAllowed=true;
                    }
                    direction++;
                }
                if(touchesTheShore || touchesAllowed)
                {
                    colliLayer->setCell(tileX,tileY,Tiled::Cell(rockTile));
                    seaWallCells[cell]=1;
                    if(touchesAllowed)
                        frameCells++;
                    else
                        rimCells++;
                }
            }
            tileX++;
        }
        tileY++;
    }
    //NO RULER STROKE. Sixteen rock in a row, across or down, reads as a wall
    //somebody drew, not as a reef: where the coast or a lane runs straight the
    //wall is SHIFTED ONE TILE at the middle of the run — the cell is given to the
    //water and the rock closes around it — which breaks the line and leaves a
    //nick in the reef. Bounded: every shift opens one cell, and what it opens is
    //re-closed at once, so the wall is never left with a hole.
    unsigned int shifted=0;
    {
        static const unsigned int rockRunMax=15;
        unsigned int pass=0;
        bool again=true;
        while(again && pass<12)
        {
            again=false;
            pass++;
            unsigned int axis=0;
            while(axis<2)
            {
                const unsigned int lineCount=(axis==0)?worldHeight:worldWidth;
                const unsigned int lineLength=(axis==0)?worldWidth:worldHeight;
                unsigned int lineIndex=0;
                while(lineIndex<lineCount)
                {
                    unsigned int step=0;
                    unsigned int runStart=0;
                    unsigned int runLength=0;
                    while(step<=lineLength)
                    {
                        const bool isRock=(step<lineLength)
                                && (seaWallCells.at((axis==0)?(step+lineIndex*worldWidth)
                                                             :(lineIndex+step*worldWidth))!=0);
                        if(isRock)
                        {
                            if(runLength==0)
                                runStart=step;
                            runLength++;
                        }
                        if(!isRock || step==lineLength)
                        {
                            if(runLength>rockRunMax)
                            {
                                //break it where a HASH of the run says, never on a
                                //fixed step: one rock moved out every N tiles is a
                                //pattern, and a pattern is worse than a straight line
                                unsigned int breakIndex=runStart
                                        +seaHash(runStart,lineIndex,setting.seed+77+axis)
                                         %(rockRunMax-3)+2;
                                while(breakIndex<runStart+runLength)
                                {
                                    const unsigned int cell=(axis==0)?(breakIndex+lineIndex*worldWidth)
                                                                     :(lineIndex+breakIndex*worldWidth);
                                    //give the cell to the water...
                                    seaAllowedCells[cell]=1;
                                    seaWallCells[cell]=0;
                                    colliLayer->setCell(cell%worldWidth,cell/worldWidth,Tiled::Cell());
                                    blocked[cell]=0;
                                    //...and close what that opens
                                    const int stepX[4]={-1,1,0,0};
                                    const int stepY[4]={0,0,-1,1};
                                    unsigned int direction=0;
                                    while(direction<4)
                                    {
                                        const int nextX=(int)(cell%worldWidth)+stepX[direction];
                                        const int nextY=(int)(cell/worldWidth)+stepY[direction];
                                        if(nextX>=0 && nextY>=0 && nextX<(int)worldWidth
                                                && nextY<(int)worldHeight)
                                        {
                                            const unsigned int next=(unsigned int)nextX
                                                    +(unsigned int)nextY*worldWidth;
                                            if(water.at(next)!=0 && blocked.at(next)==0
                                                    && seaAllowedCells.at(next)==0)
                                            {
                                                colliLayer->setCell((unsigned int)nextX,(unsigned int)nextY,
                                                                    Tiled::Cell(rockTile));
                                                blocked[next]=1;
                                                seaWallCells[next]=1;
                                            }
                                        }
                                        direction++;
                                    }
                                    shifted++;
                                    again=true;
                                    breakIndex+=2+seaHash(breakIndex,lineIndex,setting.seed+91+axis)
                                                 %(rockRunMax-3);
                                }
                            }
                            runLength=0;
                        }
                        step++;
                    }
                    lineIndex++;
                }
                axis++;
            }
        }
    }
    std::cout << "sea: " << frameCells << " rock cell(s) framing the swimmable water, "
              << rimCells << " closing a shore the player can stand on, "
              << shifted << " shifted so no rock line runs straight for 16" << std::endl;
}

bool LoadMapAll::checkSeaClosed(Tiled::Map &worldMap,const SettingsAll::SettingsExtra &setting,
                                std::vector<std::string> &errors)
{
    //THE OPEN SEA IS NEVER REACHABLE — measured the way the player moves, not by
    //looking at the wall. Flood the world from the towns over everything that is
    //not a collision, water included (the swim item makes it walkable), and only
    //over the maps that were written: whatever water that reaches must be water
    //the generator opened on purpose. This is the check that catches a rim the
    //vegetation moved, a repair that opened a coast, or a pool that leaked.
    if(seaAllowedCells.empty())
        return true;
    Tiled::TileLayer * const waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    if(waterLayer==NULL)
        return true;
    const unsigned int worldWidth=(unsigned int)worldMap.width();
    const unsigned int worldHeight=(unsigned int)worldMap.height();
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
    //NULL: every water cell is swimmable for this flood — that is the point
    std::vector<unsigned char> reached;
    seaReachableCells(worldMap,setting,water,blocked,NULL,reached);
    unsigned int leaks=0;
    unsigned int cell=0;
    while(cell<worldWidth*worldHeight)
    {
        if(reached.at(cell)!=0 && water.at(cell)!=0 && seaAllowedCells.at(cell)==0)
        {
            if(leaks<20)
                errors.push_back("the open sea is reachable at "
                                 +std::to_string(cell%worldWidth)+","+std::to_string(cell/worldWidth));
            leaks++;
        }
        cell++;
    }
    //...AND NO RULER STROKE: 16 rock in a row, across or down, is a wall somebody
    //drew. closeSeaAccess shifts the wall where that would happen; this measures
    //the map that was really written, the repairs included.
    unsigned int straightRuns=0;
    if(!seaWallCells.empty())
    {
        unsigned int axis=0;
        while(axis<2)
        {
            const unsigned int lineCount=(axis==0)?worldHeight:worldWidth;
            const unsigned int lineLength=(axis==0)?worldWidth:worldHeight;
            unsigned int lineIndex=0;
            while(lineIndex<lineCount)
            {
                unsigned int step=0;
                unsigned int runStart=0;
                unsigned int runLength=0;
                while(step<=lineLength)
                {
                    const bool isRock=(step<lineLength)
                            && (seaWallCells.at((axis==0)?(step+lineIndex*worldWidth)
                                                         :(lineIndex+step*worldWidth))!=0);
                    if(isRock)
                    {
                        if(runLength==0)
                            runStart=step;
                        runLength++;
                    }
                    if(!isRock || step==lineLength)
                    {
                        if(runLength>=16)
                        {
                            if(straightRuns<20)
                            {
                                const unsigned int startCell=(axis==0)?(runStart+lineIndex*worldWidth)
                                                                      :(lineIndex+runStart*worldWidth);
                                errors.push_back("a rock line of "+std::to_string(runLength)+
                                                 ((axis==0)?" runs across from ":" runs down from ")+
                                                 std::to_string(startCell%worldWidth)+","+
                                                 std::to_string(startCell/worldWidth));
                            }
                            straightRuns++;
                        }
                        runLength=0;
                    }
                    step++;
                }
                lineIndex++;
            }
            axis++;
        }
    }
    if(straightRuns>0)
        std::cerr << "sea: " << straightRuns << " rock line(s) of 16 or more in a row" << std::endl;
    if(leaks>0)
    {
        std::cerr << "sea: " << leaks << " cell(s) of open sea the player can swim to" << std::endl;
    }
    return leaks==0;
}
