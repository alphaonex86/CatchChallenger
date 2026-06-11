#include "LoadMapAll.h"
#include "PartialMap.h"

#include "../map-procedural-generation-terrain/LoadMap.h"

#include <libtiled/tileset.h>
#include <libtiled/tile.h>
#include <libtiled/objectgroup.h>
#include <libtiled/mapwriter.h>

#include <iostream>
#include <algorithm>
#include <random>
#include <queue>
#include <unordered_map>

#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QImage>
#include <QColor>

unsigned int ** LoadMapAll::roadData = NULL;
int LoadMapAll::botId = 0;
LoadMapAll::RoadMountain LoadMapAll::mountain;
LoadMapAll::CityGround LoadMapAll::cityGroundBig;
LoadMapAll::CityGround LoadMapAll::cityGroundMedium;
std::vector<LoadMapAll::CitySign> LoadMapAll::citySigns;
std::vector<LoadMapAll::BuildingRect> LoadMapAll::cityBuildingRects;
std::map<std::pair<uint16_t,uint16_t>,LoadMapAll::CavePlan> LoadMapAll::cavePlans;

//re-assert the city signs AFTER vegetation: a tree canopy lands on WalkBehind
//and could hide a sign placed near the town border (tree-wall band)
void LoadMapAll::reassertCitySigns(Tiled::Map &worldMap)
{
    Tiled::TileLayer * const colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::TileLayer * const walkBehindLayer=LoadMap::searchTileLayerByName(worldMap,"WalkBehind");
    Tiled::TileLayer * const onGrassLayer=LoadMap::searchTileLayerByName(worldMap,"OnGrass");
    Tiled::TileLayer * const grassLayer=LoadMap::searchTileLayerByName(worldMap,"Grass");
    unsigned int index=0;
    while(index<citySigns.size())
    {
        const CitySign &sign=citySigns.at(index);
        if(colliLayer!=NULL)
            colliLayer->setCell(sign.x,sign.y,sign.cell);
        if(walkBehindLayer!=NULL)
            walkBehindLayer->setCell(sign.x,sign.y,Tiled::Cell());
        if(onGrassLayer!=NULL)
            onGrassLayer->setCell(sign.x,sign.y,Tiled::Cell());
        if(grassLayer!=NULL)
            grassLayer->setCell(sign.x,sign.y,Tiled::Cell());
        index++;
    }
}

//scale-map codes for the city avenue: walkable (bit 0 set) so the entrance
//pathing checks still pass, but not 1 so no building/bot lands on the path
#define CITY_AVENUE_CODE 0x0B

static void preloadTilesetOfTileRef(Tiled::Map &worldMap, const QString &tileRef);
static bool worldHasTilesetNamed(Tiled::Map &worldMap, const QString &name);
static unsigned int mountainBorderTileIndex(const uint8_t to_type_match);

//walkable corridor test of a cave FLOOR at tile resolution, matching the
//interior floorGrid exactly (mirror per floor + sealed border line)
static bool caveFloorAtTile(const unsigned int *map, const unsigned int scale, const unsigned int scaleWidth,
                            const int mapWidth, const int mapHeight, const bool mirrored,
                            const int lx, const int ly)
{
    if(lx<1 || ly<1 || lx>=mapWidth-1 || ly>=mapHeight-1)
        return false;
    const int baseX=mirrored ? mapWidth-1-lx : lx;
    const unsigned int type=map[(baseX/(int)scale)+(ly/(int)scale)*scaleWidth];
    return type!=0 && type!=0x9;
}

QString LoadMapAll::monsterRef(const uint16_t &monsterId, const SettingsAll::SettingsExtra &setting)
{
    const std::map<uint16_t,std::string>::const_iterator it=setting.monsterNames.find(monsterId);
    if(it!=setting.monsterNames.cend())
        return QString::fromStdString(it->second);
    return QString::number(monsterId);
}

bool LoadMapAll::isCaveChunk(const unsigned int &x, const unsigned int &y)
{
    if(roadCoordToIndex.find(x)==roadCoordToIndex.cend())
        return false;
    const std::unordered_map<uint16_t,RoadIndex> &subEntry=roadCoordToIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return subEntry.at(y).isCave;
}

//mirror of the main.cpp split naming, with the "-cave" suffix (same folder)
std::string LoadMapAll::caveInteriorBaseName(const unsigned int &x, const unsigned int &y)
{
    if(roadCoordToIndex.find(x)==roadCoordToIndex.cend())
        return std::string();
    const std::unordered_map<uint16_t,RoadIndex> &subEntry=roadCoordToIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return std::string();
    const RoadIndex &roadIndex=subEntry.at(y);
    const Road &road=roads.at(roadIndex.roadIndex);
    if(road.haveOnlySegmentNearCity)
    {
        if(roadIndex.cityIndex.empty())
            return std::string();
        const RoadToCity &cityIndex=roadIndex.cityIndex.front();
        return "road-"+std::to_string(roadIndex.roadIndex+1)+
                "-"+orientationToString(reverseOrientation(cityIndex.orientation))+"-cave";
    }
    const unsigned int indexCoord=vectorindexOf(road.coords,std::pair<uint16_t,uint16_t>(x,y));
    return std::to_string(indexCoord+1)+"-cave";
}

bool LoadMapAll::writeCaveInterior(Tiled::Map &worldMap,
                                   const unsigned int &chunkX, const unsigned int &chunkY,
                                   const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                   const RoadIndex &roadIndex, const SettingsAll::SettingsExtra &setting,
                                   const std::string &overworldFile, const std::string &zoneName)
{
    //the no-go zone may be a 3x3 repeating rock block (9-tile comma list)
    std::vector<Tiled::Tile *> wallTiles;
    {
        const QStringList wallTileList=setting.caveWallTile.split(",");
        int wallTileIndex=0;
        while(wallTileIndex<wallTileList.size())
        {
            Tiled::Tile * const tile=fetchTile(worldMap,wallTileList.at(wallTileIndex).trimmed());
            if(tile!=NULL)
                wallTiles.push_back(tile);
            wallTileIndex++;
        }
    }
    Tiled::Tile * const wallTile=wallTiles.empty() ? NULL : wallTiles.front();
    Tiled::Tile * const floorTile=fetchTile(worldMap,setting.caveFloorTile);
    Tiled::TileLayer * const walkLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer * const colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::ObjectGroup * const movingGroup=LoadMap::searchObjectGroupByName(worldMap,"Moving");
    Tiled::ObjectGroup * const objectGroup=LoadMap::searchObjectGroupByName(worldMap,"Object");
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
    unsigned int * const map=roadData[chunkX+chunkY*setting.mapXCount];
    if(wallTile==NULL || floorTile==NULL || walkLayer==NULL || colliLayer==NULL
            || movingGroup==NULL || objectGroup==NULL || invisibleTileset==NULL || map==NULL)
        return false;
    const unsigned int scale=2;
    const unsigned int scaleWidth=singleMapWidth/scale;
    const unsigned int x0=chunkX*singleMapWidth;
    const unsigned int y0=chunkY*singleMapHeight;
    const int pixelX0=x0*worldMap.tileWidth();
    const int pixelY0=y0*worldMap.tileHeight();
    const int pixelX1=(x0+singleMapWidth)*worldMap.tileWidth();
    const int pixelY1=(y0+singleMapHeight)*worldMap.tileHeight();

    //the natural overworld chunk is already saved: drop its border travel and
    //cave-mouth objects from the region so they don't leak into the interior
    {
        unsigned int layerIndex=0;
        while(layerIndex<(unsigned int)worldMap.layerCount())
        {
            Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
            if(layer->isObjectGroup())
            {
                Tiled::ObjectGroup * const group=static_cast<Tiled::ObjectGroup *>(layer);
                const QList<Tiled::MapObject*> objects=group->objects();
                unsigned int objectIndex=0;
                while(objectIndex<(unsigned int)objects.size())
                {
                    Tiled::MapObject * const object=objects.at(objectIndex);
                    const int objectX=(int)object->x();
                    const int objectY=(int)object->y()-1;//same region convention as PartialMap
                    if(objectX>=pixelX0 && objectX<pixelX1 && objectY>=pixelY0 && objectY<pixelY1)
                    {
                        bool drop=false;
                        if(object->type().startsWith("border-"))
                            drop=true;
                        else if(object->type()=="teleport on push"
                                && object->properties().value("map").toString().contains("-cave"))
                            drop=true;
                        if(drop)
                        {
                            group->removeObject(object);
                            delete object;
                        }
                    }
                    objectIndex++;
                }
            }
            layerIndex++;
        }
    }

    Tiled::Tile * const stairDownTile=setting.caveStairDownTile.isEmpty() ? NULL : fetchTile(worldMap,setting.caveStairDownTile);
    Tiled::Tile * const stairUpTile=setting.caveStairUpTile.isEmpty() ? NULL : fetchTile(worldMap,setting.caveStairUpTile);
    Tiled::Tile * const itemTile=setting.caveItemTile.isEmpty() ? NULL : fetchTile(worldMap,setting.caveItemTile);
    //the blocked zone is edged like the overworld mountains: the same Terra
    //ring tiles ([road] region\mountain_tile/mountain_tsx) face the corridor
    std::vector<Tiled::Tile *> mountainRingTiles;
    {
        const QStringList mountainTileList=mountain.tile.split(",");
        if(mountainTileList.size()>=12 && worldHasTilesetNamed(worldMap,mountain.tsx))
        {
            const Tiled::Tileset * const mountainTsx=LoadMap::searchTilesetByName(worldMap,mountain.tsx);
            int mountainTileIndex=0;
            while(mountainTileIndex<mountainTileList.size())
            {
                mountainRingTiles.push_back(mountainTsx->tileAt(mountainTileList.at(mountainTileIndex).toInt()));
                mountainTileIndex++;
            }
        }
    }

    std::string overworldBase=overworldFile;
    {
        const size_t slash=overworldBase.rfind('/');
        if(slash!=std::string::npos)
            overworldBase=overworldBase.substr(slash+1);
        if(stringEndsWith(overworldBase,".tmx"))
            overworldBase.erase(overworldBase.size()-4);
    }
    std::string interiorPathBase=overworldFile;
    if(stringEndsWith(interiorPathBase,".tmx"))
        interiorPathBase.erase(interiorPathBase.size()-4);
    interiorPathBase+="-cave";

    //the plan (depth, stair links, per-side floor and exit cells) was decided
    //at selection time in generateRoadContent; the interior executes it
    if(cavePlans.find(std::pair<uint16_t,uint16_t>(chunkX,chunkY))==cavePlans.cend())
        return false;
    const CavePlan &plan=cavePlans.at(std::pair<uint16_t,uint16_t>(chunkX,chunkY));
    const unsigned int depth=plan.depth;
    const std::vector<std::pair<uint8_t,uint8_t> > &stairCells=plan.stairCells;
    Tiled::Tile * const exitBottomTile=fetchTile(worldMap,setting.caveExitBottomTile);
    Tiled::Tile * const exitTopTile=fetchTile(worldMap,setting.caveExitTopTile);
    if(exitBottomTile==NULL || exitTopTile==NULL)
        return false;
    //a deeper floor reuses the corridor MIRRORED horizontally so the layouts
    //differ while the stair cells stay corridor on both floors
    //level file names: base, base-2, base-3...
    std::vector<std::string> levelBaseName;
    {
        std::string interiorNameOnly=interiorPathBase;
        const size_t slash=interiorNameOnly.rfind('/');
        if(slash!=std::string::npos)
            interiorNameOnly=interiorNameOnly.substr(slash+1);
        unsigned int level=0;
        while(level<depth)
        {
            if(level==0)
                levelBaseName.push_back(interiorNameOnly);
            else
                levelBaseName.push_back(interiorNameOnly+"-"+std::to_string(level+1));
            level++;
        }
    }

    unsigned int level=0;
    while(level<depth)
    {
        const bool mirrored=(level%2!=0);
        //the actual floor of this level: the (mirrored) corridor. The map
        //border line is ALWAYS sealed (never walkable: there is a collision or
        //a teleporter before the border), and a top-exit cell becomes a
        //walkable gap in the ring (its surroundings then ring up around it)
        std::vector<unsigned char> floorGrid(singleMapWidth*singleMapHeight,0);
        {
            unsigned int ly=1;
            while(ly<singleMapHeight-1)
            {
                unsigned int lx=1;
                while(lx<singleMapWidth-1)
                {
                    unsigned int baseX=lx;
                    if(mirrored)
                        baseX=singleMapWidth-1-lx;
                    const unsigned int type=map[(baseX/scale)+(ly/scale)*scaleWidth];
                    if(type!=0 && type!=0x9)
                        floorGrid[lx+ly*singleMapWidth]=1;
                    lx++;
                }
                ly++;
            }
            int side=0;
            while(side<4)
            {
                const CavePlanSide &planSide=plan.sides[side];
                //top-style exit (the TOP side): the walk-up GAP sits one below the door
                if(planSide.used!=0 && planSide.floor==level && side==2)
                    floorGrid[planSide.exitX+(planSide.exitY+1)*singleMapWidth]=1;
                side++;
            }
        }
        //wipe every tile layer of the region, then paint this floor: ground
        //everywhere, mountain-style ring on the blocked cells that FACE the
        //corridor (same look as the overworld mountains), rock block deeper in
        {
            unsigned int ty=y0;
            while(ty<y0+singleMapHeight)
            {
                unsigned int tx=x0;
                while(tx<x0+singleMapWidth)
                {
                    unsigned int layerIndex=0;
                    while(layerIndex<(unsigned int)worldMap.layerCount())
                    {
                        Tiled::Layer * const layer=worldMap.layerAt(layerIndex);
                        if(layer->isTileLayer())
                            static_cast<Tiled::TileLayer *>(layer)->setCell(tx,ty,Tiled::Cell());
                        layerIndex++;
                    }
                    const int lx=tx-x0;
                    const int ly=ty-y0;
                    //floor everywhere: ring tiles have transparent parts and
                    //must sit on cave ground, not on the void
                    walkLayer->setCell(tx,ty,Tiled::Cell(floorTile));
                    if(floorGrid[lx+ly*singleMapWidth]==0)
                    {
                        //only the BORDER of the blocked zone carries collision
                        //(like the overworld mountains); the enclosed mass stays
                        //plain ground, unreachable behind the ring
                        uint8_t walkableMask=0;
                        const int neighborDx[8]={-1,0,1,-1,1,-1,0,1};
                        const int neighborDy[8]={-1,-1,-1,0,0,1,1,1};
                        const uint8_t neighborBit[8]={1,2,4,8,16,32,64,128};
                        int neighbor=0;
                        while(neighbor<8)
                        {
                            const int nx=lx+neighborDx[neighbor];
                            const int ny=ly+neighborDy[neighbor];
                            if(nx>=0 && ny>=0 && nx<(int)singleMapWidth && ny<(int)singleMapHeight)
                                if(floorGrid[nx+ny*singleMapWidth]!=0)
                                    walkableMask|=neighborBit[neighbor];
                            neighbor++;
                        }
                        if(walkableMask!=0)
                        {
                            Tiled::Tile *blockTile=wallTile;
                            if(mountainRingTiles.size()>=12)
                                blockTile=mountainRingTiles.at(mountainBorderTileIndex(walkableMask));
                            else if(wallTiles.size()>=9)
                                blockTile=wallTiles.at((ly%3)*3+(lx%3));
                            colliLayer->setCell(tx,ty,Tiled::Cell(blockTile));
                        }
                    }
                    tx++;
                }
                ty++;
            }
        }

        //objects of this floor, removed again after the floor is written
        std::vector<Tiled::MapObject*> levelMovingObjects;
        std::vector<Tiled::MapObject*> levelBotObjects;

        //exits of this floor: the DOOR sits ON the cliff collision at the
        //extremity it serves (push to use). Top exit: exitTopTile one above
        //the walk-up gap (the topmost collision accessible); bottom exit:
        //exitBottomTile with the floor directly above it (the bottommost
        //collision accessible, often the sealed border line itself). Both
        //land in front of the overworld mouth.
        {
            int side=0;
            while(side<4)
            {
                const CavePlanSide &planSide=plan.sides[side];
                if(planSide.used!=0 && planSide.floor==level)
                {
                    const int exitX=planSide.exitX;
                    const int exitY=planSide.exitY;
                    if(side==2)
                        colliLayer->setCell(x0+exitX,y0+exitY,Tiled::Cell(exitTopTile));
                    else
                        colliLayer->setCell(x0+exitX,y0+exitY,Tiled::Cell(exitBottomTile));
                    Tiled::MapObject *exitObject=new Tiled::MapObject("","teleport on push",
                        QPointF((x0+exitX)*worldMap.tileWidth(),(y0+exitY+1)*worldMap.tileHeight()),
                        QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                    exitObject->setProperty("map",QString::fromStdString(overworldBase));
                    exitObject->setProperty("x",QString::number(planSide.landX));
                    exitObject->setProperty("y",QString::number(planSide.landY));
                    Tiled::Cell exitCell;
                    exitCell.setTile(invisibleTileset->tileAt(2));
                    exitObject->setCell(exitCell);
                    movingGroup->addObject(exitObject);
                    levelMovingObjects.push_back(exitObject);
                }
                side++;
            }
        }

        //stairs: down to the next floor / up back to the previous one
        //(walking onto the stair tile teleports; landing is one cell under
        //the opposite stair so the player never lands ON a teleport)
        if(level+1<depth)
        {
            //this floor's frame coordinates of the link to floor level+1;
            //adjacent floors are mirrored, so the same base cell is at W-1-x
            const int sx=stairCells.at(level).first;
            const int sy=stairCells.at(level).second;
            const int nextX=(int)singleMapWidth-1-sx;
            walkLayer->setCell(x0+sx,y0+sy,Tiled::Cell(stairDownTile));
            Tiled::MapObject *down=new Tiled::MapObject("","teleport on it",
                QPointF((x0+sx)*worldMap.tileWidth(),(y0+sy+1)*worldMap.tileHeight()),
                QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
            down->setProperty("map",QString::fromStdString(levelBaseName.at(level+1)));
            down->setProperty("x",QString::number(nextX));
            down->setProperty("y",QString::number(sy+1));
            Tiled::Cell downCell;
            downCell.setTile(invisibleTileset->tileAt(2));
            down->setCell(downCell);
            movingGroup->addObject(down);
            levelMovingObjects.push_back(down);
        }
        if(level>0)
        {
            const int prevSx=stairCells.at(level-1).first;
            const int prevSy=stairCells.at(level-1).second;
            const int sx=(int)singleMapWidth-1-prevSx;//the link cell in THIS floor's frame
            const int sy=prevSy;
            walkLayer->setCell(x0+sx,y0+sy,Tiled::Cell(stairUpTile));
            Tiled::MapObject *up=new Tiled::MapObject("","teleport on it",
                QPointF((x0+sx)*worldMap.tileWidth(),(y0+sy+1)*worldMap.tileHeight()),
                QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
            up->setProperty("map",QString::fromStdString(levelBaseName.at(level-1)));
            up->setProperty("x",QString::number(prevSx));
            up->setProperty("y",QString::number(prevSy+1));
            Tiled::Cell upCell;
            upCell.setTile(invisibleTileset->tileAt(2));
            up->setCell(upCell);
            movingGroup->addObject(up);
            levelMovingObjects.push_back(up);
        }

        //a few trainers inside the corridor (deeper = a bit harder)
        QString botXml;
        {
            unsigned int localBotId=1;
            const int wanted=2+rand()%2;
            int placed=0,tries=0;
            static const char* const lookDirs[4]={"bottom","top","left","right"};
            while(placed<wanted && tries<200)
            {
                tries++;
                const int lx=4+rand()%((int)singleMapWidth-8);
                const int ly=4+rand()%((int)singleMapHeight-8);
                if(floorGrid[lx+ly*singleMapWidth]!=0
                        && colliLayer->cellAt(x0+lx,y0+ly).tile()==NULL)
                {
                    Tiled::MapObject *bot=new Tiled::MapObject("","bot",
                        QPointF((x0+lx)*worldMap.tileWidth(),(y0+ly+1)*worldMap.tileHeight()),
                        QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                    bot->setProperty("id",QString::number(localBotId));
                    bot->setProperty("lookAt",lookDirs[rand()%4]);
                    bot->setProperty("skin",QString::fromStdString(setting.botSkins.at(rand()%setting.botSkins.size())));
                    Tiled::Cell botCell;
                    botCell.setTile(invisibleTileset->tileAt(0));
                    bot->setCell(botCell);
                    objectGroup->addObject(bot);
                    levelBotObjects.push_back(bot);
                    uint8_t botLevel=roadIndex.level;
                    if((unsigned int)botLevel+2*level<=100)
                        botLevel=botLevel+2*level;
                    botXml+=botStepXml(localBotId,BotKind_fight,std::to_string(localBotId),
                                       bot->property("lookAt").toString(),setting,
                                       roadIndex.roadMonsters,botLevel,
                                       std::string(),std::vector<std::string>());
                    localBotId++;
                    placed++;
                }
            }
        }

        //ground items (the configured percent per floor): the engine picks up an
        //"object" typed object carrying an item property; the tile is the visual
        if(itemTile!=NULL && !setting.caveItems.empty()
                && (unsigned int)(rand()%100)<setting.caveItemPercent)
        {
            int tries=0;
            bool itemPlaced=false;
            while(tries<200 && !itemPlaced)
            {
                tries++;
                const int lx=4+rand()%((int)singleMapWidth-8);
                const int ly=4+rand()%((int)singleMapHeight-8);
                if(floorGrid[lx+ly*singleMapWidth]!=0
                        && colliLayer->cellAt(x0+lx,y0+ly).tile()==NULL)
                {
                    Tiled::MapObject *item=new Tiled::MapObject("","object",
                        QPointF((x0+lx)*worldMap.tileWidth(),(y0+ly+1)*worldMap.tileHeight()),
                        QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                    item->setProperty("item",QString::number(setting.caveItems.at(rand()%setting.caveItems.size())));
                    Tiled::Cell itemCell;
                    itemCell.setTile(itemTile);
                    item->setCell(itemCell);
                    objectGroup->addObject(item);
                    levelBotObjects.push_back(item);
                    itemPlaced=true;
                }
            }
        }

        //this floor's xml: cave encounters (deeper: +2 levels, luck rotated for
        //some species variation) + the trainers
        std::string additionalXmlInfo;
        if(!roadIndex.roadMonsters.empty())
        {
            additionalXmlInfo+="  <cave>\n";
            const unsigned int monsterCount=roadIndex.roadMonsters.size();
            unsigned int roadMonsterIndex=0;
            while(roadMonsterIndex<monsterCount)
            {
                const RoadMonster &roadMonster=roadIndex.roadMonsters.at(roadMonsterIndex);
                //deeper floors shift the luck between the species
                const RoadMonster &luckSource=roadIndex.roadMonsters.at((roadMonsterIndex+level)%monsterCount);
                unsigned int minLevel=roadMonster.minLevel+2*level;
                unsigned int maxLevel=roadMonster.maxLevel+2*level;
                if(minLevel>100)
                    minLevel=100;
                if(maxLevel>100)
                    maxLevel=100;
                additionalXmlInfo+="    <monster id=\""+monsterRef(roadMonster.monsterId,setting).toStdString()+"\"";
                if(minLevel==maxLevel)
                    additionalXmlInfo+=" level=\""+std::to_string(minLevel)+"\"";
                else
                    additionalXmlInfo+=" minLevel=\""+std::to_string(minLevel)+
                            "\" maxLevel=\""+std::to_string(maxLevel)+"\"";
                additionalXmlInfo+=" luck=\""+std::to_string(luckSource.luck)+"\"/>\n";
                roadMonsterIndex++;
            }
            additionalXmlInfo+="  </cave>\n";
        }
        additionalXmlInfo+=botXml.toStdString();

        std::string interiorFile=interiorPathBase;
        if(level>0)
            interiorFile+="-"+std::to_string(level+1);
        interiorFile+=".tmx";
        std::string levelName="Cave";
        if(level>0)
            levelName="Cave B"+std::to_string(level+1);
        std::vector<PartialMap::RecuesPoint> unusedRecuesPoints;
        if(!PartialMap::save(worldMap,
                             x0,y0,x0+singleMapWidth,y0+singleMapHeight,
                             interiorFile,
                             unusedRecuesPoints,
                             "cave",zoneName,levelName,
                             additionalXmlInfo,true,true,
                             " color=\"#000000\" alpha=\"60\""))
            return false;

        //clean this floor's objects so they don't leak into the next one
        {
            unsigned int objectIndex=0;
            while(objectIndex<levelMovingObjects.size())
            {
                movingGroup->removeObject(levelMovingObjects.at(objectIndex));
                delete levelMovingObjects.at(objectIndex);
                objectIndex++;
            }
            objectIndex=0;
            while(objectIndex<levelBotObjects.size())
            {
                objectGroup->removeObject(levelBotObjects.at(objectIndex));
                delete levelBotObjects.at(objectIndex);
                objectIndex++;
            }
        }
        level++;
    }
    return true;
}

//translate a template-map cell to the matching WORLD tileset cell
static Tiled::Cell worldCellFromTemplate(const MapBrush::MapTemplate &mapTemplate, const Tiled::Cell &cell)
{
    Tiled::Cell out;
    if(cell.isEmpty())
        return out;
    Tiled::Tileset * const worldTileset=mapTemplate.templateTilesetToMapTileset.at(cell.tile()->tileset());
    out.setTile(worldTileset->tileAt(cell.tile()->id()));
    return out;
}

//derive the avenue ground from a city template tmx: the most used Walkable tile
//is the path fill, the OnGrass bounding ring gives the 3x3 border set
static void deriveCityGround(const MapBrush::MapTemplate &mapTemplate, LoadMapAll::CityGround &ground)
{
    ground.valid=false;
    Tiled::TileLayer * const walk=LoadMap::searchTileLayerByName(*mapTemplate.tiledMap,"Walkable");
    if(walk==NULL)
        return;
    std::map<Tiled::Tile*,unsigned int> fillCount;
    int ty=0;
    while(ty<mapTemplate.tiledMap->height())
    {
        int tx=0;
        while(tx<mapTemplate.tiledMap->width())
        {
            Tiled::Tile * const tile=walk->cellAt(tx,ty).tile();
            if(tile!=NULL)
            {
                if(fillCount.find(tile)==fillCount.cend())
                    fillCount[tile]=1;
                else
                    fillCount[tile]++;
            }
            tx++;
        }
        ty++;
    }
    Tiled::Tile *fillTile=NULL;
    unsigned int bestCount=0;
    for(const std::pair<Tiled::Tile* const,unsigned int> &entry : fillCount)
    {
        if(entry.second>bestCount)
        {
            bestCount=entry.second;
            fillTile=entry.first;
        }
    }
    if(fillTile==NULL)
        return;
    {
        Tiled::Cell fillCell;
        fillCell.setTile(fillTile);
        ground.fill=worldCellFromTemplate(mapTemplate,fillCell);
    }
    //border ring from the OnGrass bounding box corners/edges
    Tiled::TileLayer * const onGrass=LoadMap::searchTileLayerByName(*mapTemplate.tiledMap,"OnGrass");
    if(onGrass!=NULL)
    {
        int x0=-1,y0=-1,x1=-1,y1=-1;
        ty=0;
        while(ty<mapTemplate.tiledMap->height())
        {
            int tx=0;
            while(tx<mapTemplate.tiledMap->width())
            {
                if(onGrass->cellAt(tx,ty).tile()!=NULL)
                {
                    if(x0<0 || tx<x0) x0=tx;
                    if(x1<0 || tx>x1) x1=tx;
                    if(y0<0 || ty<y0) y0=ty;
                    if(y1<0 || ty>y1) y1=ty;
                }
                tx++;
            }
            ty++;
        }
        if(x0>=0 && x1>x0 && y1>y0)
        {
            ground.border[0][0]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x0,y0));
            ground.border[0][1]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x0+1,y0));
            ground.border[0][2]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x1,y0));
            ground.border[1][0]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x0,y0+1));
            ground.border[1][2]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x1,y0+1));
            ground.border[2][0]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x0,y1));
            ground.border[2][1]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x0+1,y1));
            ground.border[2][2]=worldCellFromTemplate(mapTemplate,onGrass->cellAt(x1,y1));
        }
    }
    ground.valid=true;
}

//perform ONE city building placement at an already validated position: facade
//brush, plain house with interior, or key building (heal/shop/gym) chain
static void placeCityBuilding(Tiled::Map &worldMap, MapBrush::MapTemplate &temp,
        const LoadMapAll::BotKind slotKind, const std::string &slotBaseName, const bool facadeOnly,
        const unsigned int x, const unsigned int y,
        const unsigned int mapWidth, const unsigned int mapHeight,
        const std::pair<uint8_t,uint8_t> &pos,
        LoadMapAll::City &city, const std::string &cityLowerCaseName,
        const SettingsAll::SettingsExtra &setting, LoadMapAll::RoomSettings &rs, int &buildingId,
        const std::string &gymTypeName, const std::vector<std::string> &gymTypeMonsters)
{
    //remember the footprint: the avenue/path paint must never touch its tiles
    {
        LoadMapAll::BuildingRect buildingRect;
        buildingRect.x=x*mapWidth+pos.first;
        buildingRect.y=y*mapHeight+pos.second;
        buildingRect.w=temp.width;
        buildingRect.h=temp.height;
        LoadMapAll::cityBuildingRects.push_back(buildingRect);
    }
    if(facadeOnly)
        LoadMapAll::brushFacade(temp,worldMap,x*mapWidth+pos.first,y*mapHeight+pos.second);
    else if(slotBaseName.empty())
    {
        LoadMapAll::generateRoom(worldMap,temp,buildingId,x,y,pos,city,cityLowerCaseName,setting,rs);
        buildingId++;
    }
    else
    {
        //a key building (heal/shop/gym). For a gym build a VALID
        //monster pool from an adjacent road tile (the city tile
        //itself has none) — never .at() the city coord; if no
        //adjacent pool exists botStepXml falls back to a text bot.
        std::vector<LoadMapAll::RoadMonster> buildingPool;
        const uint8_t buildingLevel=city.level;
        std::string desc="Building";
        if(slotKind==LoadMapAll::BotKind_heal)
            desc="Heal";
        else if(slotKind==LoadMapAll::BotKind_shop)
            desc="Shop";
        else if(slotKind==LoadMapAll::BotKind_fight)
        {
            desc="Gym";
            if(!gymTypeName.empty())
                desc="Gym ("+gymTypeName+")";
            const int nbx[4]={(int)x-1,(int)x+1,(int)x,(int)x};
            const int nby[4]={(int)y,(int)y,(int)y-1,(int)y+1};
            int n=0;
            while(n<4 && buildingPool.empty())
            {
                const int nx=nbx[n];
                const int ny=nby[n];
                if(nx>=0 && ny>=0
                        && LoadMapAll::roadCoordToIndex.find((uint16_t)nx)!=LoadMapAll::roadCoordToIndex.cend()
                        && LoadMapAll::roadCoordToIndex.at((uint16_t)nx).find((uint16_t)ny)!=LoadMapAll::roadCoordToIndex.at((uint16_t)nx).cend())
                {
                    const LoadMapAll::RoadIndex &adjacent=LoadMapAll::roadCoordToIndex.at((uint16_t)nx).at((uint16_t)ny);
                    if(!adjacent.roadMonsters.empty())
                        buildingPool=adjacent.roadMonsters;
                }
                n++;
            }
        }
        LoadMapAll::addBuildingChain(slotBaseName,desc,temp,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName,slotKind,setting,buildingPool,buildingLevel,gymTypeName,gymTypeMonsters);
    }
}

//street-front lots along the avenue (classic lot-subdivision town layout):
//contiguous lots are filled CENTER-OUT on both frontages; the near (top)
//frontage is for doored buildings (door opens on the avenue), the far (bottom)
//frontage only takes doorless facades.
struct AvenueLotState
{
    Tiled::Map *worldMap;
    unsigned int *map;
    const unsigned char *lotObstacle;
    Tiled::TileLayer *colliLayer;
    Tiled::TileLayer *waterLayer;
    unsigned int scaleWidth,scaleHeight,scale,mapWidth,mapHeight,chunkX,chunkY;
    unsigned int hy0,hy1;
    int aboveRight,aboveLeft,belowRight,belowLeft;
};

//door-front tiles (the engine "push" cell of each door) must stay free or the
//player can never enter; band cells are fine (the avenue paint clears them)
static bool cityDoorFrontsFree(AvenueLotState &lots, MapBrush::MapTemplate &temp,
                               const int posX, const int posY, const bool markReserved)
{
    const std::vector<Tiled::MapObject*> doorList=LoadMapAll::getDoorsListAndTp(temp.tiledMap);
    unsigned int doorIndex=0;
    while(doorIndex<doorList.size())
    {
        const int frontX=posX+(int)(doorList.at(doorIndex)->x()/lots.worldMap->tileWidth());
        const int frontY=posY+(int)(doorList.at(doorIndex)->y()/lots.worldMap->tileHeight());
        if(frontX<1 || frontY<1 || frontX>=(int)lots.mapWidth-1 || frontY>=(int)lots.mapHeight-1)
            return false;
        const unsigned int cell=(frontX/(int)lots.scale)+(frontY/(int)lots.scale)*lots.scaleWidth;
        if(markReserved)
        {
            //keep later buildings off the doorstep, but stay walkable for pathing
            if(lots.map[cell]==0 || lots.map[cell]==1)
                lots.map[cell]=0x0F;
            //and keep the VEGETATION off it too: a tree planted on the doorstep
            //blocked the entrance (mapMask is only OR-ed, never cleared)
            {
                const unsigned int tx=lots.chunkX*lots.mapWidth+frontX;
                const unsigned int ty=lots.chunkY*lots.mapHeight+frontY;
                const unsigned int maxMapSize=(lots.worldMap->width()*lots.worldMap->height()/8+1);
                const unsigned int bitMask=tx+ty*lots.worldMap->width();
                if(bitMask/8<maxMapSize)
                    MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
            }
        }
        else if(lots.map[cell]!=CITY_AVENUE_CODE)
        {
            const unsigned int tx=lots.chunkX*lots.mapWidth+frontX;
            const unsigned int ty=lots.chunkY*lots.mapHeight+frontY;
            if(lots.map[cell]==5 || lots.map[cell]==0x9)
                return false;
            if(lots.colliLayer->cellAt(tx,ty).tile()!=NULL
                    || (lots.waterLayer!=NULL && lots.waterLayer->cellAt(tx,ty).tile()!=NULL))
                return false;
        }
        doorIndex++;
    }
    return true;
}

//every doored building must be linked to the city's MAIN PATH: BFS on the
//scale grid from each door front to the nearest avenue cell, then mark the
//route as avenue so the paint pass draws the path (and nothing builds on it)
static void connectDoorFrontsToAvenue(AvenueLotState &lots, MapBrush::MapTemplate &temp,
                                      const int posX, const int posY)
{
    const std::vector<Tiled::MapObject*> doorList=LoadMapAll::getDoorsListAndTp(temp.tiledMap);
    unsigned int doorIndex=0;
    while(doorIndex<doorList.size())
    {
        const int frontX=posX+(int)(doorList.at(doorIndex)->x()/lots.worldMap->tileWidth());
        const int frontY=posY+(int)(doorList.at(doorIndex)->y()/lots.worldMap->tileHeight());
        if(frontX>=0 && frontY>=0 && frontX<(int)lots.mapWidth && frontY<(int)lots.mapHeight)
        {
            const unsigned int cellCount=lots.scaleWidth*lots.scaleHeight;
            const unsigned int startCell=(frontX/(int)lots.scale)+(frontY/(int)lots.scale)*lots.scaleWidth;
            std::vector<int> parent(cellCount,-2);//-2 unseen, -1 start
            std::vector<unsigned int> queue;
            parent[startCell]=-1;
            queue.push_back(startCell);
            int foundCell=-1;
            unsigned int queueIndex=0;
            while(queueIndex<queue.size() && foundCell<0)
            {
                const unsigned int current=queue.at(queueIndex);
                if(lots.map[current]==CITY_AVENUE_CODE)
                    foundCell=current;
                else
                {
                    const int cx=current%lots.scaleWidth;
                    const int cy=current/lots.scaleWidth;
                    const int nextDx[4]={0,0,-1,1};
                    const int nextDy[4]={-1,1,0,0};
                    int direction=0;
                    while(direction<4)
                    {
                        const int nx=cx+nextDx[direction];
                        const int ny=cy+nextDy[direction];
                        if(nx>=0 && ny>=0 && nx<(int)lots.scaleWidth && ny<(int)lots.scaleHeight)
                        {
                            const unsigned int next=nx+ny*lots.scaleWidth;
                            if(parent[next]==-2)
                            {
                                const unsigned int code=lots.map[next];
                                //cross only open ground or existing paths
                                if(code==0 || code==1 || code==0x9 || code==0x0F || code==CITY_AVENUE_CODE)
                                {
                                    if(lots.lotObstacle[next]==0 || code==CITY_AVENUE_CODE)
                                    {
                                        parent[next]=current;
                                        queue.push_back(next);
                                    }
                                }
                            }
                        }
                        direction++;
                    }
                }
                queueIndex++;
            }
            //mark the route as avenue: painted as path, protected from buildings
            if(foundCell>=0)
            {
                int walkCell=foundCell;
                while(walkCell>=0)
                {
                    if(lots.map[walkCell]!=5 && lots.map[walkCell]!=CITY_AVENUE_CODE)
                        lots.map[walkCell]=CITY_AVENUE_CODE;
                    const int parentCell=parent[walkCell];
                    walkCell=(parentCell==-1) ? -3 : parentCell;
                    if(walkCell==-3)
                        break;
                }
            }
        }
        doorIndex++;
    }
}

static bool placeOnAvenueLot(AvenueLotState &lots, MapBrush::MapTemplate &temp,
        const LoadMapAll::BotKind slotKind, const std::string &slotBaseName, const bool facadeOnly,
        LoadMapAll::City &city, const std::string &cityLowerCaseName,
        const SettingsAll::SettingsExtra &setting, LoadMapAll::RoomSettings &rs, int &buildingId,
        const std::string &gymTypeName, const std::vector<std::string> &gymTypeMonsters)
{
    const int bandTopTile=(int)lots.hy0*(int)lots.scale;
    const int bandBottomTile=((int)lots.hy1+1)*(int)lots.scale;
    const int center=(int)lots.mapWidth/2;
    int side=0;
    while(side<2)
    {
        //doors face bottom: only the top frontage gives a door on the avenue
        if(side==1 && !facadeOnly)
            return false;
        const int posY=(side==0) ? bandTopTile-(int)temp.height : bandBottomTile;
        if(posY>=2 && posY+(int)temp.height<=(int)lots.mapHeight-2)
        {
            int &rightCursor=(side==0) ? lots.aboveRight : lots.belowRight;
            int &leftCursor=(side==0) ? lots.aboveLeft : lots.belowLeft;
            //fill the lot row from the center outward, balancing both directions
            const bool firstGoRight=(rightCursor-center<=center-leftCursor);
            int direction=0;
            while(direction<2)
            {
                const bool goRight=(direction==0) ? firstGoRight : !firstGoRight;
                int posX=goRight ? rightCursor : leftCursor-(int)temp.width;
                while((goRight && posX+(int)temp.width<(int)lots.mapWidth-3)
                        || (!goRight && posX>=3))
                {
                    bool valid=true;
                    const unsigned int bx=posX/(int)lots.scale;
                    const unsigned int by=posY/(int)lots.scale;
                    const unsigned int ex=(posX+(int)temp.width+(int)lots.scale-1)/(int)lots.scale;
                    const unsigned int ey=(posY+(int)temp.height+(int)lots.scale-1)/(int)lots.scale;
                    unsigned int cy=by;
                    while(cy<ey && valid)
                    {
                        unsigned int cx=bx;
                        while(cx<ex && valid)
                        {
                            const unsigned int cell=cx+cy*lots.scaleWidth;
                            if(lots.map[cell]!=0 && lots.map[cell]!=1)
                                valid=false;
                            else if(lots.lotObstacle[cell]!=0)
                                valid=false;
                            cx++;
                        }
                        cy++;
                    }
                    if(valid && !facadeOnly)
                        valid=cityDoorFrontsFree(lots,temp,posX,posY,false);
                    if(valid)
                    {
                        const std::pair<uint8_t,uint8_t> pos(posX,posY);
                        placeCityBuilding(*lots.worldMap,temp,slotKind,slotBaseName,facadeOnly,
                                          lots.chunkX,lots.chunkY,lots.mapWidth,lots.mapHeight,pos,
                                          city,cityLowerCaseName,setting,rs,buildingId,
                                          gymTypeName,gymTypeMonsters);
                        cy=by;
                        while(cy<ey)
                        {
                            unsigned int cx=bx;
                            while(cx<ex)
                            {
                                lots.map[cx+cy*lots.scaleWidth]=5;
                                cx++;
                            }
                            cy++;
                        }
                        if(!facadeOnly)
                        {
                            cityDoorFrontsFree(lots,temp,posX,posY,true);
                            connectDoorFrontsToAvenue(lots,temp,posX,posY);
                        }
                        if(goRight)
                            rightCursor=posX+(int)temp.width+1;
                        else
                            leftCursor=posX-1;
                        return true;
                    }
                    if(goRight)
                        posX+=2;
                    else
                        posX-=2;
                }
                //this direction is exhausted: park the cursor at the edge
                if(goRight)
                    rightCursor=(int)lots.mapWidth;
                else
                    leftCursor=0;
                direction++;
            }
        }
        side++;
    }
    return false;
}

void LoadMapAll::brushFacade(const MapBrush::MapTemplate &mapTemplate, Tiled::Map &worldMap,
                             const int &tileX, const int &tileY)
{
    //hide the door objects: brushTheMap skips objects whose cell has no tile, so
    //the facade is drawn without any working door (no content behind it)
    std::vector<Tiled::MapObject*> doors=getDoorsListAndTp(mapTemplate.tiledMap);
    std::vector<Tiled::Cell> oldCells;
    unsigned int index=0;
    while(index<doors.size())
    {
        oldCells.push_back(doors.at(index)->cell());
        doors.at(index)->setCell(Tiled::Cell());
        index++;
    }
    MapBrush::brushTheMap(worldMap,mapTemplate,tileX,tileY,MapBrush::mapMask,true);
    index=0;
    while(index<doors.size())
    {
        doors.at(index)->setCell(oldCells.at(index));
        index++;
    }
}

static void writeGymTsx(const QString &destDir, const QString &fileName, const QString &tilesetName)
{
    QFile tsx(destDir+fileName+".tsx");
    if(tsx.open(QFile::WriteOnly))
    {
        const QString content("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<tileset name=\""+tilesetName+"\" tilewidth=\"16\" tileheight=\"16\">\n"
                              " <image source=\""+fileName+".png\"/>\n"
                              "</tileset>\n");
        const QByteArray contentData(content.toUtf8());
        tsx.write(contentData.constData(),contentData.size());
        tsx.close();
    }
    else
    {
        std::cerr << "Unable to write " << (destDir+fileName+".tsx").toStdString() << std::endl;
        abort();
    }
}

void LoadMapAll::generateGymTilesets(const SettingsAll::SettingsExtra &setting)
{
    if(setting.gymTypeNames.empty())
        return;
    //blue gym sprite sheet: the building on top, then the recolorable parts
    //(roof + door) drawn again exactly 128px below their building position
    QString srcPath("tileset/gym.png");
    if(!QFile::exists(srcPath))
        srcPath=QCoreApplication::applicationDirPath()+"/../../tileset/gym.png";
    QImage src(srcPath);
    if(src.isNull())
    {
        std::cerr << "tileset/gym.png not found, gym keeps the default building look" << std::endl;
        return;
    }
    src=src.convertToFormat(QImage::Format_ARGB32);
    const int overlayOffset=128;
    if(src.height()<=overlayOffset)
    {
        std::cerr << "tileset/gym.png has no recolor overlay part (height<=128)" << std::endl;
        return;
    }
    const QString destDir=QCoreApplication::applicationDirPath()+"/dest/map/tileset/";
    //stage the base (blue) building so the gym-building template tileset resolves.
    //File name gym-base.* (NOT gym.*): a "tileset/gym.tsx" relative ref would also
    //resolve against the tool's OWN source tileset/ from the run CWD and leak an
    //out-of-datapack path into the written maps. Internal name stays "gym" (the
    //per-type tileset swap matches on it).
    src.copy(0,0,src.width(),overlayOffset).save(destDir+"gym-base.png");
    writeGymTsx(destDir,"gym-base","gym");
    unsigned int indexType=0;
    while(indexType<setting.gymTypeNames.size())
    {
        const QString &color=setting.gymTypeColors.at(indexType);
        if(!color.isEmpty())
        {
            const QColor typeColor(color);
            int typeH=0,typeS=0,typeV=0;
            typeColor.getHsv(&typeH,&typeS,&typeV);
            QImage out=src.copy(0,0,src.width(),overlayOffset);
            int oy=overlayOffset;
            while(oy<src.height())
            {
                int ox=0;
                while(ox<src.width())
                {
                    const QRgb pixel=src.pixel(ox,oy);
                    if(qAlpha(pixel)>0)
                    {
                        QColor c(pixel);
                        int h=0,s=0,v=0;
                        c.getHsv(&h,&s,&v);
                        //tint: hue/saturation of the type color, brightness of the
                        //sprite, so outlines stay dark and white details stay white
                        int outS=s;
                        if(s>80)
                            outS=typeS;
                        int outV=v*typeV/240;
                        if(outV>255)
                            outV=255;
                        const QColor outColor=QColor::fromHsv(typeH,outS,outV,qAlpha(pixel));
                        out.setPixel(ox,oy-overlayOffset,outColor.rgba());
                    }
                    ox++;
                }
                oy++;
            }
            const QString typeName=QString::fromStdString(setting.gymTypeNames.at(indexType));
            if(!out.save(destDir+"gym-"+typeName+".png"))
            {
                std::cerr << "Unable to write " << (destDir+"gym-"+typeName+".png").toStdString() << std::endl;
                abort();
            }
            writeGymTsx(destDir,"gym-"+typeName,"gym-"+typeName);
        }
        indexType++;
    }
}

struct LedgeMarker
{
    unsigned int x;
    unsigned int y;
    unsigned int length;
    bool horizontal;
};

struct ZoneMarker
{
    ZoneMarker(int x, int y, int w, int h, int id, unsigned int area = 0, unsigned int type=1, int from=-1)
        :x(x), y(y), w(w), h(h), id(id), area(area), type(type), from(from), neightbourg(0){}

    int x;
    int y;
    int w;
    int h;
    int id;
    unsigned int area;
    unsigned int type;
    int from;
    int neightbourg;
};

class ZoneSorter {
public:
    ZoneSorter(int w, int h):w(w), h(h){}

    bool operator()(ZoneMarker* &a, ZoneMarker* &b) const
    {
        return abs(a->x-w)+abs(a->y-h) < abs(b->x-w)+abs(b->y-h);
    }
private:
    int w;
    int h;
};

void findAreaMarker(ZoneMarker *marker, const unsigned int &mapw, unsigned int* area){
    unsigned int w = marker->x+marker->w;
    unsigned int h = marker->y+marker->h;

    for(unsigned int x=marker->x; x<w; x++){
        for(unsigned int y=marker->y; y<h; y++){
            if(area[mapw*y+x] != 0){
                marker->area = area[mapw*y+x];
                return;
            }
        }
    }
}

std::vector<ZoneMarker*> placeZones(unsigned int* map, unsigned int w, unsigned int h, unsigned int orientation, const SettingsAll::SettingsExtra &setting){
    std::vector<ZoneMarker*> zones = std::vector<ZoneMarker*>();

    unsigned int* area = new unsigned int[w*h]; // All diffrents area
    unsigned int areaId = 0;
    int margin = 1;
    unsigned int maxsize = 4;
    unsigned int minsize = 2;
    unsigned int maxsizevertical = 6;
    unsigned int minsizevertical = 3;
    int walkwayRange = 2;
    int id = 0;
    int limit = setting.roadRetry;
    int pass= setting.regionTry;
    int retry = setting.walkwayTry;

    for(unsigned int y=0; y<h; y++){
        for(unsigned int x=0; x<w; x++){
            if((map[x+y*w]&0x08) == 0){
                if(x > 0 && area[x-1+y*w] != 0){
                    area[x+y*w] = area[x-1+y*w];
                }else if(y > 0 && area[x+(y-1)*w] != 0){
                    area[x+y*w] = area[x+(y-1)*w];
                }else{
                    areaId++;
                    area[x+y*w] = areaId;
                }
            }else{
                area[x+y*w] = 0;
            }
        }
    }

    if(areaId != 1){
        // Spread zones
        for(unsigned int y=0; y<h; y++){
            for(unsigned int x=0; x<w; x++){
                std::vector<unsigned int> spread = std::vector<unsigned int>();
                const unsigned a = area[x+y*w];

                if(a > 0){
                    spread.push_back(x+y*w);

                    while(!spread.empty()){
                        unsigned int i = spread.at(spread.size()-1 );
                        spread.pop_back();

                        if(i%w != 0 && area[i-1] > a){
                            spread.push_back(i-1);
                            area[i-1] = a;
                        }
                        if(i%w != w-1 && area[i+1] > a){
                            spread.push_back(i+1);
                            area[i+1] = a;
                        }
                        if(i/w > 0 && area[i-w] > a){
                            spread.push_back(i-w);
                            area[i-w] = a;
                        }
                        if(i/w < h-1 && area[i+w] > a){
                            spread.push_back(i+w);
                            area[i+w] = a;
                        }
                    }
                }
            }
        }
    }

    if((orientation & LoadMapAll::Orientation_bottom) != 0){
        zones.push_back(new ZoneMarker(w/2-1, h-2, 2, 2, id++, 0, 0xFF));
        findAreaMarker(zones.at(zones.size()-1), w, area);
    }
    if((orientation & LoadMapAll::Orientation_left) != 0){
        zones.push_back(new ZoneMarker(0, h/2-1, 2, 2, id++, 0, 0xFF));
        findAreaMarker(zones.at(zones.size()-1), w, area);
    }
    if((orientation & LoadMapAll::Orientation_right) != 0){
        zones.push_back(new ZoneMarker(w-2, h/2-1, 2, 2, id++, 0, 0xFF));
        findAreaMarker(zones.at(zones.size()-1), w, area);
    }
    if((orientation & LoadMapAll::Orientation_top) != 0){
        zones.push_back(new ZoneMarker(w/2-1, 0, 2, 2, id++, 0, 0xFF));
        findAreaMarker(zones.at(zones.size()-1), w, area);
    }

    if(zones.size() == 0){
        delete [] area;
        return std::vector<ZoneMarker*>();
    } else if(zones.size() == 1){
        zones.push_back(new ZoneMarker(w/2-1, h/2-1, 2, 2, id++, 0, 0xFF));
        findAreaMarker(zones.at(zones.size()-1), w, area);
    }

    std::vector<ZoneMarker*> startingPoint = std::vector<ZoneMarker*>(zones);
    std::vector<ZoneMarker*> candidate = std::vector<ZoneMarker*>();

    bool isDone = false;
    while(!isDone && limit > 0){
        for(int i=0; i<pass; i++){
            const unsigned int index = rand()%zones.size();
            const ZoneMarker* zone = zones[index];

            ZoneMarker *tmp = NULL;
            unsigned int s1, s2; // Different size

            if(rand()%2 == 1){
                s1 = minsize+(rand()%(maxsize-minsize));
                s2 = minsizevertical+(rand()%(maxsizevertical-minsizevertical));
            }else{
                s2 = minsize+(rand()%(maxsize-minsize));
                s1 = minsizevertical+(rand()%(maxsizevertical-minsizevertical));
            }

            switch (rand()%4) {
            case 0:
                tmp = new ZoneMarker(zone->x + zone->w, zone->y + (rand()%(zone->h*2-3)-zone->h+1), s1, s2, id, 0, 0xFE, index);
                break;
            case 1:
                tmp = new ZoneMarker(zone->x + (rand()%(zone->w*2-3)-zone->w+1), zone->y+zone->h, s1, s2, id, 0, 0xFE, index);
                break;
            case 2:
                tmp = new ZoneMarker(zone->x-s1, zone->y + (rand()%(zone->h*2-3)-zone->h+1), s1, s2, id, 0, 0xFE, index);
                break;
            case 3:
                tmp = new ZoneMarker(zone->x + (rand()%(zone->w*2-3)-zone->w+1), zone->y-s2, s1, s2, id, 0, 0xFE, index);
                break;
            default:
                std::cerr << "Invalid rand() ?! ";
                abort();
                break;
            }

            if(tmp->x < margin || tmp->y < margin || tmp->x + tmp->w > (int)w-margin || tmp->y + tmp->h > (int)h-margin){
                delete tmp;
                tmp = NULL;
                continue;
            }else{
                for(const ZoneMarker* box2: zones){
                    if((box2->x < tmp->x + tmp->w)
                        && (box2->x + box2->w > tmp->x)
                        && (box2->y < tmp->y + tmp->h)
                        && (box2->y + box2->h > tmp->y))
                    {
                        delete tmp;
                        tmp = NULL;
                        break;
                    }
                }

                if(tmp != NULL){
                    unsigned int right = tmp->x+tmp->w;
                    unsigned int bottom = tmp->y+tmp->h;

                    for(unsigned int x=tmp->x; x<right; x++){
                        for(unsigned int y=tmp->y; y<bottom; y++){
                            if(area[w*y+x] == 0){
                                delete tmp;
                                tmp = NULL;
                                break;
                            }
                        }

                        if(tmp == NULL){
                            break;
                        }
                    }

                    if(tmp != NULL){
                        tmp->id = id;
                        zones.push_back(tmp);
                        id++;
                    }
                }
            }
        }

        // If all path are ok, break
        bool* validated = new bool[id];
        isDone = true;
        int i;

        for(i=0; i<id; i++){
            validated[i] = false;
        }

        std::vector<ZoneMarker*> remainingZones = std::vector<ZoneMarker*>();
        remainingZones.push_back(startingPoint.front());

        while(!remainingZones.empty()){
            ZoneMarker* m = remainingZones.back();
            remainingZones.pop_back();

            validated[m->id] = true;
            for(ZoneMarker *box2: zones){
                if(!validated[box2->id] && (
                            ((box2->x < m->x + m->w+1)
                                &&(box2->x + box2->w >= m->x)
                                &&(box2->y < m->y + m->h)
                                &&(box2->y + box2->h > m->y))
                            ||((box2->x < m->x + m->w)
                                &&(box2->x + box2->w > m->x)
                                &&(box2->y < m->y + m->h+1)
                                &&(box2->y + box2->h >= m->y)))){
                    remainingZones.push_back(box2);
                }
            }
        }

        for(const ZoneMarker *target: startingPoint){
            if(!validated[target->id]){
                isDone = false;
            }
        }

        if(isDone){
            delete[] validated;
            break;
        }

        int oldId = id;
        // Look for walkway
        for(i=0; i<oldId;i++){
            if(!validated[i]){
                const ZoneMarker *tmp = zones.at(i);
                for(ZoneMarker *target: zones){
                    if(validated[target->id]
                            && (target->x-walkwayRange < tmp->x + tmp->w)
                            && (target->x + target->w+walkwayRange > tmp->x)
                            && (target->y-walkwayRange < tmp->y + tmp->h)
                            && (target->y + target->h+walkwayRange > tmp->y)){
                        candidate.push_back(target);
                    }
                }

                if(!candidate.empty()){
                    const ZoneMarker *random = candidate.at(rand()%candidate.size());
                    ZoneMarker* walkway = new ZoneMarker(0,0,0,0,0);
                    candidate.clear();

                    switch (rand()%4) {
                    case 0:
                        walkway->x = random->x + (rand()/(double)RAND_MAX)*random->w;
                        walkway->y = random->y + random->h;
                        break;
                    case 1:
                        walkway->x = random->x + (rand()/(double)RAND_MAX)*random->w;
                        walkway->y = random->y - 1;
                        break;
                    case 2:
                        walkway->x = random->x + random->w;
                        walkway->y = random->y + (rand()/(double)RAND_MAX)*random->h;
                        break;
                    case 3:
                        walkway->x = random->x - 1;
                        walkway->y = random->y + (rand()/(double)RAND_MAX)*random->h;
                        break;
                    default:
                        walkway->x = random->x;
                        walkway->y = random->y;
                        break;
                    }
                    walkway->h = 1;
                    walkway->w = 1;
                    walkway->from = tmp->id;

                    bool valid = true;

                    if(walkway->x < margin || walkway->y < margin || walkway->x + walkway->w > (int)w-margin || walkway->y + walkway->h > (int)h-margin){
                        valid = false;
                    } else if(area[w*walkway->y+walkway->x] == 0
                         && !(area[w*walkway->y+walkway->x+1] == 0
                             && area[w*walkway->y+walkway->x-1] == 0
                             && area[w*walkway->y+walkway->x-w] != 0
                             && area[w*walkway->y+walkway->x+w] != 0)
                         && !(area[w*walkway->y+walkway->x+1] != 0
                             && area[w*walkway->y+walkway->x-1] != 0
                             && area[w*walkway->y+walkway->x-w] == 0
                             && area[w*walkway->y+walkway->x+w] == 0)){
                     valid = false;
                 } else {
                        for(const ZoneMarker* box2: zones){
                            if((box2->x < walkway->x + walkway->w)
                                && (box2->x + box2->w > walkway->x)
                                && (box2->y < walkway->y + walkway->h)
                                && (box2->y + box2->h > walkway->y))
                            {
                                valid = false;
                                break;
                            }
                        }
                    }

                    if(valid){
                        walkway->id = id;
                        walkway->type = (area[w*walkway->y+walkway->x] == 0)?0x9:0xFE;
                        findAreaMarker(walkway, w, area);
                        zones.push_back(walkway);
                        id++;
                        break;
                    }
                }
            }
        }

        delete [] validated;

        limit--;
        pass = retry;
    }

    // Do the pathing
    if(limit > 0){
        std::vector<std::vector<int>> paths = std::vector<std::vector<int>>();

        for(unsigned int i=0; i<zones.size(); i++){
            ZoneMarker* box1 = zones[i];
            paths.push_back(std::vector<int>());

            for(const ZoneMarker* box2: zones){
                if(((box2->x < box1->x + box1->w+1)
                    &&(box2->x + box2->w >= box1->x)
                    &&(box2->y < box1->y + box1->h)
                    &&(box2->y + box2->h > box1->y))
                ||((box2->x < box1->x + box1->w)
                    &&(box2->x + box2->w > box1->x)
                    &&(box2->y < box1->y + box1->h+1)
                   &&(box2->y + box2->h >= box1->y))){
                    paths[i].push_back(box2->id);
                }
            }
            box1->neightbourg = paths[i].size()-1;
        }

        // Eliminate road to nowhere
        std::vector<int> *toCheck = new std::vector<int>();
        std::vector<int> *waitingList = new std::vector<int>();

        for(unsigned int i=0; i<zones.size(); i++){
            toCheck->push_back(i);
        }

        while(!toCheck->empty()){
            for(unsigned int i=0; i<toCheck->size(); i++){
                ZoneMarker* box1 = zones[toCheck->at(i)];
                if(box1->neightbourg < 2 && box1->type != 0xFF && box1->type != 0){
                    box1->type = 0;

                    std::vector<int> &p = paths[box1->id];
                    for(const int& j: p){
                        ZoneMarker* box2 = zones.at(j);
                        if(box2->type != 0x0){
                            box2->neightbourg--;
                            waitingList->push_back(box2->id);
                        }
                    }
                }
            }

            std::vector<int> *tmp = toCheck;
            toCheck->clear();
            toCheck = waitingList;
            waitingList = tmp;
        }

        // Check shortest path
        bool* validated = new bool[id];
        int i;

        for(i=0; i<id; i++){
            validated[i] = false;
        }

        toCheck->clear();
        waitingList->clear();

        ZoneMarker *exit = startingPoint.at(rand()%startingPoint.size());
        exit->type = 0x1;
        toCheck->push_back(exit->id);
        validated[exit->id] = true;

        while(!toCheck->empty()){

            for(int &id: *toCheck){
                for(const int& j: paths[id]){
                    if(!validated[j]){
                        validated[j] = true;
                        ZoneMarker* box2 = zones.at(j);
                        box2->from = id;

                        waitingList->push_back(j);
                    }
                }
            }

            std::vector<int> *tmp = toCheck;
            toCheck->clear();
            toCheck = waitingList;
            waitingList = tmp;
        }

        for(ZoneMarker *box1: startingPoint){
            while(box1->from != -1){
                if(box1->type != 0x09)
                    box1->type = 0x1;
                else
                    box1->type = 0x4;
                box1 = zones.at(box1->from);
            }
        }

        for(ZoneMarker *box1: zones){
            if(box1->type == 0xFE){

                toCheck->clear();
                waitingList->clear();

                int type = rand()/(double)RAND_MAX < setting.roadWaterChance? 2: 3;
                box1->type = type;
                toCheck->push_back(box1->id);

                while(!toCheck->empty()){
                    for(int &id: *toCheck){
                        for(const int& j: paths[id]){
                            ZoneMarker* box2 = zones.at(j);

                            if(box2->type == 0xFE){
                                box2->type = type;

                                waitingList->push_back(j);
                            }
                        }
                    }

                    std::vector<int> *tmp = toCheck;
                    toCheck->clear();
                    toCheck = waitingList;
                    waitingList = tmp;
                }
            }
        }

        delete [] validated;
        delete waitingList;
        delete toCheck;
    }

    delete [] area;
    return zones;
}

LoadMap::Terrain* searchWater(){
    for(int height=0;height<5;height++){
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            if(terrain.terrainName.compare(QString("water"), Qt::CaseInsensitive) == 0 )
            {
                return &terrain;
            }else{
                std::cout << terrain.terrainName.toStdString() << std::endl;
            }
        }
    }

    return NULL;
}

void LoadMapAll::generateRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting){
    const unsigned int mapWidth=worldMap.width()/setting.mapXCount;
    const unsigned int mapHeight=worldMap.height()/setting.mapYCount;
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    const unsigned int scale = 2;
    const unsigned int scaleWidth  = mapWidth/scale;
    const unsigned int scaleHeight = mapHeight/scale;

    unsigned int y=0;
    LoadMapAll::roadData = new unsigned int*[w*h];
    Tiled::TileLayer *colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::TileLayer *waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::Tileset *invis = LoadMap::searchTilesetByName(worldMap, "invisible");
    LoadMap::Terrain* water = searchWater();

    Tiled::Cell waterTile = Tiled::Cell();
    MapBrush::MapTemplate mapTemplatebuildingshop;
    MapBrush::MapTemplate mapTemplatebuildingheal;
    MapBrush::MapTemplate mapTemplatebuilding1;
    MapBrush::MapTemplate mapTemplatebuilding2;
    MapBrush::MapTemplate mapTemplatebuildingbig1;
    loadMapTemplate("building-shop/",mapTemplatebuildingshop,"building-shop",mapWidth,mapHeight,worldMap);
    loadMapTemplate("building-heal/",mapTemplatebuildingheal,"building-heal",mapWidth,mapHeight,worldMap);
    loadMapTemplate("building-1/",mapTemplatebuilding1,"building-1",mapWidth,mapHeight,worldMap);
    loadMapTemplate("building-2/",mapTemplatebuilding2,"building-2",mapWidth,mapHeight,worldMap);
    loadMapTemplate("building-big-1/",mapTemplatebuildingbig1,"building-big-1",mapWidth,mapHeight,worldMap);

    citySigns.clear();
    cityBuildingRects.clear();
    cavePlans.clear();
    //per-type recolored gym tilesets into dest/map/tileset/ (and the blue base)
    generateGymTilesets(setting);
    //make sure the tilesets named by the cave tiles are part of the world
    {
        preloadTilesetOfTileRef(worldMap,setting.caveFloorTile);
        preloadTilesetOfTileRef(worldMap,setting.caveEntranceTile);
        preloadTilesetOfTileRef(worldMap,setting.caveStairDownTile);
        preloadTilesetOfTileRef(worldMap,setting.caveStairUpTile);
        preloadTilesetOfTileRef(worldMap,setting.caveItemTile);
        const QStringList wallTileList=setting.caveWallTile.split(",");
        int wallTileIndex=0;
        while(wallTileIndex<wallTileList.size())
        {
            preloadTilesetOfTileRef(worldMap,wallTileList.at(wallTileIndex));
            wallTileIndex++;
        }
    }
    //gym building template (typed facade); falls back to building-big-1 when absent
    MapBrush::MapTemplate mapTemplategym;
    bool haveGymTemplate=false;
    if(QFile::exists("template/gym-building/gym-building.tmx")
            && QFile::exists(QCoreApplication::applicationDirPath()+"/dest/map/tileset/gym-base.tsx"))
    {
        //pre-add the staged blue gym tileset so the template matches the dest copy
        LoadMap::readTileset("tileset/gym-base.tsx",&worldMap);
        loadMapTemplate("gym-building/",mapTemplategym,"gym-building",mapWidth,mapHeight,worldMap);
        haveGymTemplate=true;
    }
    //world tileset per gym type, added on first use
    std::map<std::string,Tiled::Tileset*> gymTypeWorldTilesets;

    //city avenue/plaza path terrain comes from an editable template tmx ([city])
    cityGroundBig.valid=false;
    cityGroundMedium.valid=false;
    MapBrush::MapTemplate mapTemplateCityBig;
    MapBrush::MapTemplate mapTemplateCityMedium;
    bool haveCityBigTemplate=false;
    bool haveCityMediumTemplate=false;
    if(!setting.cityBigTemplate.isEmpty() && QFile::exists("template/"+setting.cityBigTemplate+".tmx"))
    {
        loadMapTemplate("",mapTemplateCityBig,setting.cityBigTemplate,mapWidth,mapHeight,worldMap);
        deriveCityGround(mapTemplateCityBig,cityGroundBig);
        haveCityBigTemplate=true;
    }
    if(!setting.cityMediumTemplate.isEmpty() && QFile::exists("template/"+setting.cityMediumTemplate+".tmx"))
    {
        loadMapTemplate("",mapTemplateCityMedium,setting.cityMediumTemplate,mapWidth,mapHeight,worldMap);
        deriveCityGround(mapTemplateCityMedium,cityGroundMedium);
        haveCityMediumTemplate=true;
    }

    //assign an ELEMENT TYPE to every city from its surrounding terrain (water
    //city by the sea, stone in the mountains, plant in the grass...) while
    //keeping the per-type counts balanced; the gym type follows the city type
    if(!setting.gymTypeNames.empty() && !cities.empty())
    {
        std::vector<unsigned int> typeCount(setting.gymTypeNames.size(),0);
        //similar ratio of each type: a terrain match only wins while its type
        //is under the per-type quota, the rest goes to the least used types
        const unsigned int typeQuota=(cities.size()+setting.gymTypeNames.size()-1)/setting.gymTypeNames.size();
        unsigned int cityIndex=0;
        while(cityIndex<cities.size())
        {
            City &city=cities[cityIndex];
            //terrain profile of the chunk plus a 4-tile margin (the coast counts)
            std::map<std::string,unsigned int> terrainCount;
            {
                const int x0=(int)(city.x*mapWidth)-4;
                const int y0=(int)(city.y*mapHeight)-4;
                const int x1=(int)((city.x+1)*mapWidth)+4;
                const int y1=(int)((city.y+1)*mapHeight)+4;
                int sampleY=y0;
                while(sampleY<y1)
                {
                    int sampleX=x0;
                    while(sampleX<x1)
                    {
                        if(sampleX>=0 && sampleY>=0 && sampleX<worldMap.width() && sampleY<worldMap.height())
                        {
                            //terrain by VORONOI zone (the layer tiles may sit on
                            //transition groups at this stage): same lookup as the
                            //terrain painter, terrainList[height][moisure-1]
                            const unsigned int zoneIndex=VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[sampleX+sampleY*worldMap.width()].index;
                            const VoronioForTiledMapTmx::PolygonZone &zone=VoronioForTiledMapTmx::voronoiMap.zones.at(zoneIndex);
                            if(zone.height<5 && zone.moisure>=1 && zone.moisure<=6)
                                terrainCount[LoadMap::terrainList[zone.height][zone.moisure-1].terrainName.toLower().toStdString()]++;
                        }
                        sampleX+=2;
                    }
                    sampleY+=2;
                }
            }
            //score the configured types by their terrain keywords; among the
            //matching ones take the least used (similar ratio of each type)
            int bestType=-1;
            unsigned int bestScore=0,bestUsed=0;
            unsigned int mappingIndex=0;
            while(mappingIndex<setting.cityTypeTerrains.size())
            {
                const std::string &mappedType=setting.cityTypeTerrains.at(mappingIndex).first;
                int typeIndex=-1;
                {
                    unsigned int gymIndex=0;
                    while(gymIndex<setting.gymTypeNames.size())
                    {
                        if(setting.gymTypeNames.at(gymIndex)==mappedType)
                            typeIndex=gymIndex;
                        gymIndex++;
                    }
                }
                if(typeIndex>=0)
                {
                    unsigned int score=0;
                    const std::vector<std::string> &keywords=setting.cityTypeTerrains.at(mappingIndex).second;
                    for(const std::pair<const std::string,unsigned int> &entry : terrainCount)
                    {
                        unsigned int keywordIndex=0;
                        while(keywordIndex<keywords.size())
                        {
                            if(entry.first.find(keywords.at(keywordIndex))!=std::string::npos)
                            {
                                score+=entry.second;
                                break;
                            }
                            keywordIndex++;
                        }
                    }
                    if(score>0 && typeCount.at(typeIndex)<typeQuota)
                    {
                        if(bestType<0
                                || typeCount.at(typeIndex)<bestUsed
                                || (typeCount.at(typeIndex)==bestUsed && score>bestScore))
                        {
                            bestType=typeIndex;
                            bestScore=score;
                            bestUsed=typeCount.at(typeIndex);
                        }
                    }
                }
                mappingIndex++;
            }
            if(bestType<0)
            {
                //no terrain match: the least used type keeps the global ratio
                unsigned int typeIndex=0;
                while(typeIndex<typeCount.size())
                {
                    if(bestType<0 || typeCount.at(typeIndex)<typeCount.at(bestType))
                        bestType=typeIndex;
                    typeIndex++;
                }
            }
            city.elementType=setting.gymTypeNames.at(bestType);
            typeCount[bestType]++;
            cityIndex++;
        }
    }

    if(water != NULL && water->tile != NULL){
        waterTile.setTile(water->tile);
    }else{
        std::cerr << "Watertile not found" << std::endl;
    }

    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            // For each chunk
            // Do the random road gen
            const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
            const bool isCity=haveCityEntry(citiesCoordToIndex,x,y);

            if((zoneOrientation&(Orientation_bottom|Orientation_left|Orientation_right|Orientation_top)) != 0){

                //a configurable percent of road chunks becomes a CAVE: the
                //overworld stays natural terrain and the cave CANNOT be
                //bypassed — the chunk qualifies only when its road connections
                //are SEPARATED by the terrain (cliffs/water); each connection
                //gets a mouth placed directly ON the cliff collision line
                if(!isCity && setting.cavePercent>0
                        && !setting.caveWallTile.isEmpty() && !setting.caveFloorTile.isEmpty()
                        && !setting.caveEntranceTile.isEmpty() && !setting.caveEntranceTopTile.isEmpty()
                        && !setting.caveExitBottomTile.isEmpty() && !setting.caveExitTopTile.isEmpty()
                        && roadCoordToIndex.find(x)!=roadCoordToIndex.cend()
                        && roadCoordToIndex.at(x).find(y)!=roadCoordToIndex.at(x).cend())
                {
                    if((unsigned int)(rand()%100)<setting.cavePercent)
                    {
                        //OPEN SPACE test: flood the walkable ground from the chunk
                        //center; when more than 90% of the border is reachable
                        //without collision on the path, nothing can force the
                        //player through the cave — disable it for this chunk
                        bool openSpace=false;
                        {
                            std::vector<unsigned char> reached(mapWidth*mapHeight,0);
                            std::vector<unsigned int> queue;
                            //start at the center (or the first walkable cell near it)
                            int startX=-1,startY=-1;
                            {
                                int radius=0;
                                while(radius<6 && startX<0)
                                {
                                    int offsetY=-radius;
                                    while(offsetY<=radius && startX<0)
                                    {
                                        int offsetX=-radius;
                                        while(offsetX<=radius && startX<0)
                                        {
                                            const int lx=(int)mapWidth/2+offsetX;
                                            const int ly=(int)mapHeight/2+offsetY;
                                            if(colliLayer->cellAt(x*mapWidth+lx,y*mapHeight+ly).tile()==NULL
                                                    && waterLayer->cellAt(x*mapWidth+lx,y*mapHeight+ly).tile()==NULL)
                                            {
                                                startX=lx;
                                                startY=ly;
                                            }
                                            offsetX++;
                                        }
                                        offsetY++;
                                    }
                                    radius++;
                                }
                            }
                            if(startX>=0)
                            {
                                reached[startX+startY*mapWidth]=1;
                                queue.push_back(startX+startY*mapWidth);
                                unsigned int queueIndex=0;
                                while(queueIndex<queue.size())
                                {
                                    const unsigned int current=queue.at(queueIndex);
                                    const int cx=current%mapWidth;
                                    const int cy=current/mapWidth;
                                    const int nextDx[4]={0,0,-1,1};
                                    const int nextDy[4]={-1,1,0,0};
                                    int direction=0;
                                    while(direction<4)
                                    {
                                        const int nx=cx+nextDx[direction];
                                        const int ny=cy+nextDy[direction];
                                        if(nx>=0 && ny>=0 && nx<(int)mapWidth && ny<(int)mapHeight)
                                        {
                                            const unsigned int next=nx+ny*mapWidth;
                                            if(reached[next]==0
                                                    && colliLayer->cellAt(x*mapWidth+nx,y*mapHeight+ny).tile()==NULL
                                                    && waterLayer->cellAt(x*mapWidth+nx,y*mapHeight+ny).tile()==NULL)
                                            {
                                                reached[next]=1;
                                                queue.push_back(next);
                                            }
                                        }
                                        direction++;
                                    }
                                    queueIndex++;
                                }
                                unsigned int borderTotal=0,borderReached=0;
                                int ly=0;
                                while(ly<(int)mapHeight)
                                {
                                    int lx=0;
                                    while(lx<(int)mapWidth)
                                    {
                                        if(lx==0 || ly==0 || lx==(int)mapWidth-1 || ly==(int)mapHeight-1)
                                        {
                                            borderTotal++;
                                            if(reached[lx+ly*mapWidth]!=0)
                                                borderReached++;
                                        }
                                        lx++;
                                    }
                                    ly++;
                                }
                                if(borderReached*100>borderTotal*90)
                                    openSpace=true;
                            }
                        }
                        if(openSpace)
                        {
                            //chunk stays a normal open road, no cave
                        }
                        else
                        {
                        //the cave is NEVER dropped, and there is NO artificial
                        //cliff ring (owner: ugly): walking straight inward from
                        //the border, the FIRST natural cliff hosts the mouth;
                        //on flat ground a small rock outcrop stands in a
                        //vegetation-free clearing instead. The surrounding
                        //terrain/vegetation does the sealing naturally.
                        CavePlan plan;
                        plan.depth=1;
                        const int mid[4]={(int)mapHeight/2,(int)mapHeight/2,(int)mapWidth/2,(int)mapWidth/2};
                        const int dirX[4]={1,-1,0,0};
                        const int dirY[4]={0,0,1,-1};
                        const int borderX[4]={0,(int)mapWidth-1,(int)mapWidth/2,(int)mapWidth/2};
                        const int borderY[4]={(int)mapHeight/2,(int)mapHeight/2,0,(int)mapHeight-1};
                        int side=0;
                        while(side<4)
                        {
                            CavePlanSide &planSide=plan.sides[side];
                            planSide.used=0;
                            const Orientation sideOrientation[4]={Orientation_left,Orientation_right,Orientation_top,Orientation_bottom};
                            if((zoneOrientation&sideOrientation[side])!=0)
                            {
                                planSide.used=1;
                                planSide.floor=0;
                                //first natural cliff on the approach? The mouth may
                                //NEVER sit on a cliff CORNER: both flank neighbors
                                //(perpendicular to the approach) must be cliff too;
                                //a corner blocks that line, try the parallel ones
                                int mouthStep=-1;
                                int mouthPerp=0;
                                {
                                    const int perpOrder[3]={0,-1,1};
                                    int perpIndex=0;
                                    while(perpIndex<3 && mouthStep<0)
                                    {
                                        const int perp=perpOrder[perpIndex];
                                        bool lineBlocked=false;
                                        int step=1;
                                        while(step<=12 && mouthStep<0 && !lineBlocked)
                                        {
                                            const int lx=borderX[side]+dirX[side]*step+dirY[side]*perp;
                                            const int ly=borderY[side]+dirY[side]*step+dirX[side]*perp;
                                            if(lx<1 || ly<1 || lx>=(int)mapWidth-1 || ly>=(int)mapHeight-1)
                                                break;
                                            if(waterLayer->cellAt(x*mapWidth+lx,y*mapHeight+ly).tile()!=NULL)
                                                break;//water ahead: stand the outcrop before it
                                            if(colliLayer->cellAt(x*mapWidth+lx,y*mapHeight+ly).tile()!=NULL)
                                            {
                                                const int flankAx=lx+dirY[side];
                                                const int flankAy=ly+dirX[side];
                                                const int flankBx=lx-dirY[side];
                                                const int flankBy=ly-dirX[side];
                                                if(flankAx>=0 && flankAy>=0 && flankAx<(int)mapWidth && flankAy<(int)mapHeight
                                                        && flankBx>=0 && flankBy>=0 && flankBx<(int)mapWidth && flankBy<(int)mapHeight
                                                        && colliLayer->cellAt(x*mapWidth+flankAx,y*mapHeight+flankAy).tile()!=NULL
                                                        && colliLayer->cellAt(x*mapWidth+flankBx,y*mapHeight+flankBy).tile()!=NULL)
                                                {
                                                    mouthStep=step;
                                                    mouthPerp=perp;
                                                }
                                                else
                                                    lineBlocked=true;//corner piece: cannot pass it
                                            }
                                            step++;
                                        }
                                        perpIndex++;
                                    }
                                }
                                if(mouthStep>0)
                                {
                                    //mouth embedded in the natural cliff
                                    planSide.outcrop=0;
                                    planSide.mouthKind=(side==2) ? 2 : 1;//from the top the player sees the cliff top
                                    planSide.mouthX=borderX[side]+dirX[side]*mouthStep+dirY[side]*mouthPerp;
                                    planSide.mouthY=borderY[side]+dirY[side]*mouthStep+dirX[side]*mouthPerp;
                                    planSide.landX=borderX[side]+dirX[side]*(mouthStep-1)+dirY[side]*mouthPerp;
                                    planSide.landY=borderY[side]+dirY[side]*(mouthStep-1)+dirX[side]*mouthPerp;
                                }
                                else
                                {
                                    //small rock outcrop on the flat ground, the mouth
                                    //(always south-facing) on its bottom line middle
                                    planSide.outcrop=1;
                                    planSide.mouthKind=1;
                                    if(side==0)
                                    {
                                        planSide.mouthX=3;
                                        planSide.mouthY=mid[0];
                                    }
                                    else if(side==1)
                                    {
                                        planSide.mouthX=(int)mapWidth-4;
                                        planSide.mouthY=mid[1];
                                    }
                                    else if(side==2)
                                    {
                                        planSide.mouthX=mid[2];
                                        planSide.mouthY=3;
                                    }
                                    else
                                    {
                                        planSide.mouthX=mid[3];
                                        planSide.mouthY=(int)mapHeight-4;
                                    }
                                    planSide.landX=planSide.mouthX;
                                    planSide.landY=planSide.mouthY+1;
                                }
                            }
                            side++;
                        }
                        roadCoordToIndex[x][y].isCave=true;
                        cavePlans[std::pair<uint16_t,uint16_t>(x,y)]=plan;
                        }
                    }
                }

                unsigned int* map = new unsigned int[scaleWidth * scaleHeight];
                roadData[x+y*w] = map;

                // Step 1: analyse
                for(unsigned int sy = 0; sy < scaleWidth; sy++){
                    for(unsigned int sx = 0; sx < scaleWidth; sx++){
                        map[sx + sy*scaleWidth] = 0;

                        for(unsigned int j= 0; j < scale; j++){
                            for(unsigned int k = 0; k < scale; k++){
                                if(colliLayer->cellAt(x*mapWidth+sx*scale+j, y*mapHeight+sy*scale+k).tile() != NULL
                                        || waterLayer->cellAt(x*mapWidth+sx*scale+j, y*mapHeight+sy*scale +k).tile() != NULL){
                                    map[sx + sy*scaleWidth] |= 0x08; // Obstacle
                                }
                            }
                        }
                    }
                }

                // Step 2: place the random zone
                std::vector<ZoneMarker*> zones = placeZones(map, scaleWidth, scaleHeight, zoneOrientation, setting);

                // If can join all the path
                for(unsigned int x = 0; x<scaleWidth; x++){
                    for(unsigned int y = 0; y<scaleHeight; y++){
                        map[x+y*scaleWidth] = 0;
                    }
                }
                Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");

                if(isCity){
                    for(ZoneMarker *marker: zones){
                        if(marker->type != 0 && marker->type != 4){
                            marker->type = 1;
                        }
                    }
                }

                for(const ZoneMarker *marker: zones){
                    for(int x = marker->x; x<marker->x+marker->w; x++){
                        for(int y = marker->y; y<marker->y+marker->h; y++){
                            map[x+y*scaleWidth] = marker->type;
                        }
                    }
                }

                if(isCity){
                    City *city = NULL;
                    for(City &c: cities){
                        if(c.x == x && c.y == y)
                            city = &c;
                    }

                    //---- avenue: a rigid paved band crossing the town between its
                    //entrances (medium: just the avenue; big: + aligned buildings,
                    //plaza patches and signs). Path terrain comes from the city
                    //template tmx (see settings [city]).
                    const bool isBigCity=(city!=NULL && city->type==CityType_big);
                    const bool isMediumCity=(city!=NULL && city->type==CityType_medium);
                    const bool avenueGroundOk=(isBigCity && cityGroundBig.valid)
                            || (isMediumCity && cityGroundMedium.valid);
                    bool haveHorizontalBand=false;
                    bool haveVerticalBand=false;
                    const unsigned int hy0=scaleHeight/2-1, hy1=scaleHeight/2;
                    const unsigned int vx0=scaleWidth/2-1, vx1=scaleWidth/2;
                    if(avenueGroundOk)
                    {
                        if((zoneOrientation&(Orientation_left|Orientation_right))!=0)
                        {
                            haveHorizontalBand=true;
                            unsigned int cx0=0, cx1=scaleWidth-1;
                            if((zoneOrientation&Orientation_left)==0)
                                cx0=vx0;
                            if((zoneOrientation&Orientation_right)==0)
                                cx1=vx1;
                            unsigned int cx=cx0;
                            while(cx<=cx1)
                            {
                                map[cx+hy0*scaleWidth]=CITY_AVENUE_CODE;
                                map[cx+hy1*scaleWidth]=CITY_AVENUE_CODE;
                                cx++;
                            }
                        }
                        if((zoneOrientation&(Orientation_top|Orientation_bottom))!=0)
                        {
                            haveVerticalBand=true;
                            unsigned int cy0=0, cy1=scaleHeight-1;
                            if((zoneOrientation&Orientation_top)==0)
                                cy0=hy0;
                            if((zoneOrientation&Orientation_bottom)==0)
                                cy1=hy1;
                            unsigned int cy=cy0;
                            while(cy<=cy1)
                            {
                                map[vx0+cy*scaleWidth]=CITY_AVENUE_CODE;
                                map[vx1+cy*scaleWidth]=CITY_AVENUE_CODE;
                                cy++;
                            }
                        }
                        //use the template tmx directly as base terrain: brushed
                        //centered, so the owner can shape the city ground in Tiled
                        const bool useAsBase=isBigCity ? setting.cityBigUseAsBase : setting.cityMediumUseAsBase;
                        const bool haveTemplate=isBigCity ? haveCityBigTemplate : haveCityMediumTemplate;
                        if(useAsBase && haveTemplate)
                        {
                            const MapBrush::MapTemplate &cityTemplate=isBigCity ? mapTemplateCityBig : mapTemplateCityMedium;
                            const int templateW=cityTemplate.tiledMap->width();
                            const int templateH=cityTemplate.tiledMap->height();
                            const int patchX=(int)(mapWidth-templateW)/2;
                            const int patchY=(int)(mapHeight-templateH)/2;
                            MapBrush::brushTheMap(worldMap,cityTemplate,x*mapWidth+patchX,y*mapHeight+patchY,MapBrush::mapMask,true);
                            unsigned int cy=patchY/scale;
                            while(cy<(unsigned int)(patchY+templateH+(int)scale-1)/scale && cy<scaleHeight)
                            {
                                unsigned int cx=patchX/scale;
                                while(cx<(unsigned int)(patchX+templateW+(int)scale-1)/scale && cx<scaleWidth)
                                {
                                    //keep the avenue band paintable over the patch
                                    if(map[cx+cy*scaleWidth]!=CITY_AVENUE_CODE)
                                        map[cx+cy*scaleWidth]=5;
                                    cx++;
                                }
                                cy++;
                            }
                        }
                    }

                    // City buildings: a heal center first, an optional gym for
                    // non-small cities, an optional shop, then a few houses.
                    // templateKind / templateBaseName run parallel to templates
                    // and drive per-slot bot generation. baseName "" means a
                    // plain house (handled by generateRoom, or a doorless facade
                    // in a big city).
                    QList<MapBrush::MapTemplate> templates = QList<MapBrush::MapTemplate>();
                    std::vector<BotKind> templateKind;
                    std::vector<std::string> templateBaseName;

                    templates.push_back(mapTemplatebuildingheal);
                    templateKind.push_back(BotKind_heal);
                    templateBaseName.push_back("building-heal");

                    //each gym picks a type: part of the trainer monsters come from
                    //the type pool and the gym facade is recolored with the type
                    //color (gym-<type> tileset generated into dest/map/tileset/)
                    std::string gymTypeName;
                    std::vector<std::string> gymTypeMonsters;
                    if(setting.doGym && city!=NULL && city->type!=CityType_small){
                        MapBrush::MapTemplate gymTemplate=haveGymTemplate ? mapTemplategym : mapTemplatebuildingbig1;
                        if(!setting.gymTypeNames.empty())
                        {
                            //the gym type MATCHES the city element type
                            unsigned int gymTypeIndex=0;
                            {
                                unsigned int gymIndex=0;
                                while(gymIndex<setting.gymTypeNames.size())
                                {
                                    if(setting.gymTypeNames.at(gymIndex)==city->elementType)
                                        gymTypeIndex=gymIndex;
                                    gymIndex++;
                                }
                            }
                            gymTypeName=setting.gymTypeNames.at(gymTypeIndex);
                            gymTypeMonsters=setting.gymTypeMonsters.at(gymTypeIndex);
                            if(haveGymTemplate && !setting.gymTypeColors.at(gymTypeIndex).isEmpty())
                            {
                                //swap the blue gym tileset for the type-colored one
                                Tiled::Tileset *typeTileset=NULL;
                                if(gymTypeWorldTilesets.find(gymTypeName)!=gymTypeWorldTilesets.cend())
                                    typeTileset=gymTypeWorldTilesets.at(gymTypeName);
                                else if(QFile::exists(QCoreApplication::applicationDirPath()+"/dest/map/tileset/gym-"+QString::fromStdString(gymTypeName)+".tsx"))
                                {
                                    typeTileset=LoadMap::readTileset("tileset/gym-"+QString::fromStdString(gymTypeName)+".tsx",&worldMap);
                                    gymTypeWorldTilesets[gymTypeName]=typeTileset;
                                }
                                if(typeTileset!=NULL)
                                {
                                    unsigned int indexTileset=0;
                                    while(indexTileset<(unsigned int)gymTemplate.tiledMap->tilesetCount())
                                    {
                                        Tiled::SharedTileset templateTileset=gymTemplate.tiledMap->tilesetAt(indexTileset);
                                        if(templateTileset->name()=="gym")
                                            gymTemplate.templateTilesetToMapTileset[templateTileset.get()]=typeTileset;
                                        indexTileset++;
                                    }
                                }
                            }
                        }
                        templates.push_back(gymTemplate);
                        templateKind.push_back(BotKind_fight);
                        templateBaseName.push_back("gym");
                    }

                    if(rand()%2){
                        templates.push_back(mapTemplatebuildingshop);
                        templateKind.push_back(BotKind_shop);
                        templateBaseName.push_back("building-shop");
                    }

                    //a big city has more buildings, aligned around the avenue;
                    //most of them are doorless facades (no content)
                    int building;
                    if(isBigCity)
                        building = rand()%4 + 4;
                    else
                        building = rand()%4 + 2;

                    for(int i=0; i<building; i++){
                        switch (rand()%3) {
                        case 0:
                            templates.push_back(mapTemplatebuilding1);
                            break;
                        case 1:
                            templates.push_back(mapTemplatebuilding2);
                            break;
                        case 2:
                            templates.push_back(mapTemplatebuildingbig1);
                            break;
                        }
                        templateKind.push_back(BotKind_text);
                        templateBaseName.push_back("");
                    }

                    std::sort(zones.begin(), zones.end(), ZoneSorter(scaleWidth, scaleHeight));

                    if(city != NULL){
                        LoadMapAll::RoomSettings rs;
                        {
                            std::vector<SettingsAll::Furnitures> tables{};
                            std::vector<SettingsAll::Furnitures> exits{};
                            std::vector<SettingsAll::Furnitures> down{};
                            std::vector<SettingsAll::Furnitures> up{};

                            for(SettingsAll::Furnitures f: setting.room.furnitures){
                                if(f.tags.contains("table", Qt::CaseInsensitive)){
                                    tables.push_back(f);
                                }
                                if(f.tags.contains("exit", Qt::CaseInsensitive)){
                                    exits.push_back(f);
                                }
                                if(f.tags.contains("stair-down", Qt::CaseInsensitive)){
                                    down.push_back(f);
                                }
                                if(f.tags.contains("stair-up", Qt::CaseInsensitive)){
                                    up.push_back(f);
                                }
                            }

                            if(!tables.empty()){
                                rs.table = tables.at(rand()%tables.size());
                            }else{
                                std::cerr << "No table in furniture" << std::endl;
                                abort();
                            }

                            if(!exits.empty()){
                                rs.exit = exits.at(rand()%exits.size());
                            }else{
                                std::cerr << "No exit in furniture" << std::endl;
                                abort();
                            }

                            if(!down.empty()){
                                rs.stairDown = down.at(rand()%down.size());
                            }else{
                                std::cerr << "No stair-down in furniture" << std::endl;
                                abort();
                            }

                            if(!up.empty()){
                                rs.stairUp = up.at(rand()%up.size());
                            }else{
                                std::cerr << "No stair-up in furniture" << std::endl;
                                abort();
                            }
                        }
                        rs.wall = setting.room.walls.at(rand()%setting.room.walls.size());
                        rs.floor = setting.room.floors.at(rand()%setting.room.floors.size());

                        const std::string &cityLowerCaseName=LoadMapAll::lowerCase(city->name);

                        std::vector<std::pair<uint8_t, uint8_t>> startingPoint {};

                        if((zoneOrientation & LoadMapAll::Orientation_bottom) != 0){
                            startingPoint.push_back(std::pair<uint8_t, uint8_t>(scaleWidth/2, scaleHeight-1));
                        }
                        if((zoneOrientation & LoadMapAll::Orientation_left) != 0){
                            startingPoint.push_back(std::pair<uint8_t, uint8_t>(0, scaleHeight/2));
                        }
                        if((zoneOrientation & LoadMapAll::Orientation_right) != 0){
                            startingPoint.push_back(std::pair<uint8_t, uint8_t>(scaleWidth-1, scaleHeight/2));
                        }
                        if((zoneOrientation & LoadMapAll::Orientation_top) != 0){
                            startingPoint.push_back(std::pair<uint8_t, uint8_t>(scaleWidth/2, 0));
                        }

                        int buildingId=1;
                        bool interiorHouseDone=false;

                        //street-front lot state: the random zones only cover part
                        //of the town, so lots recheck the GROUND directly (the
                        //step-1 obstacle bits were consumed when map[] was reused)
                        std::vector<unsigned char> lotObstacle(scaleWidth*scaleHeight,0);
                        {
                            unsigned int sy=0;
                            while(sy<scaleHeight)
                            {
                                unsigned int sx=0;
                                while(sx<scaleWidth)
                                {
                                    unsigned int j=0;
                                    while(j<scale*scale)
                                    {
                                        const unsigned int tx=x*mapWidth+sx*scale+(j%scale);
                                        const unsigned int ty=y*mapHeight+sy*scale+(j/scale);
                                        if(colliLayer->cellAt(tx,ty).tile()!=NULL
                                                || waterLayer->cellAt(tx,ty).tile()!=NULL)
                                        {
                                            lotObstacle[sx+sy*scaleWidth]=1;
                                            break;
                                        }
                                        j++;
                                    }
                                    sx++;
                                }
                                sy++;
                            }
                        }
                        AvenueLotState lots;
                        lots.worldMap=&worldMap;
                        lots.map=map;
                        lots.lotObstacle=lotObstacle.data();
                        lots.colliLayer=colliLayer;
                        lots.waterLayer=waterLayer;
                        lots.scaleWidth=scaleWidth;
                        lots.scaleHeight=scaleHeight;
                        lots.scale=scale;
                        lots.mapWidth=mapWidth;
                        lots.mapHeight=mapHeight;
                        lots.chunkX=x;
                        lots.chunkY=y;
                        lots.hy0=hy0;
                        lots.hy1=hy1;
                        lots.aboveRight=(int)mapWidth/2;
                        lots.aboveLeft=(int)mapWidth/2;
                        lots.belowRight=(int)mapWidth/2;
                        lots.belowLeft=(int)mapWidth/2;

                        unsigned int templateIndex=0;
                        while(templateIndex<(unsigned int)templates.size()){
                            MapBrush::MapTemplate &temp=templates[templateIndex];
                            const BotKind slotKind=templateKind.at(templateIndex);
                            const std::string &slotBaseName=templateBaseName.at(templateIndex);
                            //big city: most plain houses are doorless facades (no content)
                            bool facadeOnly=false;
                            if(isBigCity && slotBaseName.empty())
                            {
                                if(interiorHouseDone)
                                    facadeOnly=true;
                                else
                                    interiorHouseDone=true;
                            }
                            bool placed=false;
                            //big city: street-front lots filled center-out so the
                            //buildings line up along the avenue
                            if(isBigCity && haveHorizontalBand)
                                placed=placeOnAvenueLot(lots,temp,slotKind,slotBaseName,facadeOnly,
                                                        *city,cityLowerCaseName,setting,rs,buildingId,
                                                        gymTypeName,gymTypeMonsters);
                            int i=0;
                            int limit = 500;
                            if(!placed)
                            for(i=0; i<limit; i++){
                                double index = rand() / (double) RAND_MAX;
                                ZoneMarker* zone = zones[(1-index*index)*zones.size()];

                                if(zone->type == 1){
                                    bool valid= true;
                                    std::pair<uint8_t,uint8_t> pos(zone->x*scale + rand()%(zone->w*scale ), zone->y*scale+ rand()%(zone->h*scale ));
                                    //std::pair<uint8_t,uint8_t> pos(rand() % setting.mapWidth, rand() % setting.mapHeight);
                                    unsigned int bx = pos.first/scale;
                                    unsigned int by = pos.second/scale;
                                    //cover EVERY cell the footprint touches: at an odd
                                    //position the last tile row/column spills one cell
                                    //further than ceil(size/scale) from the start cell —
                                    //that uncovered cell let the door paths route THROUGH
                                    //the building and the avenue paint erased its tiles
                                    const unsigned int ex=(pos.first+temp.width+scale-1)/scale;
                                    const unsigned int ey=(pos.second+temp.height+scale-1)/scale;

                                    if(ex < scaleWidth && ey < scaleHeight){
                                        for(unsigned int tx = bx; tx<ex; tx++){
                                            for(unsigned int ty = by; ty<ey; ty++){
                                                if(map[tx+ty*scaleWidth]!= 1){
                                                    valid =false;
                                                }
                                            }
                                        }
                                    }else{
                                        valid = false;
                                    }

                                    if(valid){
                                        for(unsigned int tx = bx; tx<ex; tx++){
                                            for(unsigned int ty = by; ty<ey; ty++){
                                                map[tx+ty*scaleWidth] = 0;
                                            }
                                        }


                                        for(std::pair<uint8_t,uint8_t> zone1: startingPoint){
                                            for(std::pair<uint8_t,uint8_t> zone2: startingPoint){
                                                if(zone1 != zone2 && !checkPathing(map, scaleWidth, scaleHeight, zone1.first, zone1.second, zone2.first, zone2.second)){
                                                    valid = false;
                                                }
                                            }
                                        }

                                        //a doored building stays enterable: the
                                        //doorstep tiles must be free ground
                                        if(valid && !facadeOnly)
                                            valid=cityDoorFrontsFree(lots,temp,pos.first,pos.second,false);

                                        if(valid){
                                            placeCityBuilding(worldMap,temp,slotKind,slotBaseName,facadeOnly,
                                                              x,y,mapWidth,mapHeight,pos,*city,cityLowerCaseName,
                                                              setting,rs,buildingId,gymTypeName,gymTypeMonsters);
                                            zone->type = 5;
                                            //mark the footprint BEFORE the door path: the
                                            //route must never cross the building cells
                                            for(unsigned int tx = bx; tx<ex; tx++){
                                                for(unsigned int ty = by; ty<ey; ty++){
                                                    map[tx+ty*scaleWidth] = 5;
                                                }
                                            }
                                            if(!facadeOnly)
                                            {
                                                cityDoorFrontsFree(lots,temp,pos.first,pos.second,true);
                                                connectDoorFrontsToAvenue(lots,temp,pos.first,pos.second);
                                            }
                                            break;
                                        }

                                        zone->type = 1;

                                        for(unsigned int tx = bx; tx<ex; tx++){
                                            for(unsigned int ty = by; ty<ey; ty++){
                                                map[tx+ty*scaleWidth] = 1;
                                            }
                                        }
                                    }
                                }
                            }

                            if(!placed && i >= limit && slotKind == BotKind_heal){
                                Tiled::ObjectGroup* moving = LoadMap::searchObjectGroupByName(worldMap, "Moving");

                                qreal point_x = (x+.5)*mapWidth;
                                qreal point_y = (y+.5)*mapHeight;

                                int tileWidth = worldMap.tileWidth();
                                int tileHeight = worldMap.tileHeight();

                                // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                                qreal pixels_x = point_x * tileWidth;
                                qreal pixels_y = point_y * tileHeight;

                                QPointF point = QPointF(pixels_x, pixels_y);

                                std::cout << "point_x: " << point_x << std::endl;
                                std::cout << "point_y: " << point_y << std::endl;

                                Tiled::MapObject* rescue = new Tiled::MapObject("", "rescue", point, QSizeF(tileWidth,tileHeight));
                                rescue->setCell(Tiled::Cell(invis->tileAt(1)));
                                moving->addObject(rescue);
                            }
                            templateIndex++;
                        }

                        //big city: fill the remaining street-front lots with extra
                        //doorless facades so the avenue reads as a dense main street
                        if(isBigCity && haveHorizontalBand)
                        {
                            int extraFacades=4+rand()%4;
                            bool lotsLeft=true;
                            while(extraFacades>0 && lotsLeft)
                            {
                                MapBrush::MapTemplate *facadeTemplate=&mapTemplatebuilding1;
                                switch(rand()%3)
                                {
                                case 0:
                                    facadeTemplate=&mapTemplatebuilding1;
                                    break;
                                case 1:
                                    facadeTemplate=&mapTemplatebuilding2;
                                    break;
                                default:
                                    facadeTemplate=&mapTemplatebuildingbig1;
                                    break;
                                }
                                lotsLeft=placeOnAvenueLot(lots,*facadeTemplate,BotKind_text,std::string(),true,
                                                          *city,cityLowerCaseName,setting,rs,buildingId,
                                                          gymTypeName,gymTypeMonsters);
                                extraFacades--;
                            }
                        }

                        //big city: plaza patches — the city template tmx brushed as
                        //"alternative floor" islands on free ground
                        if(isBigCity && haveCityBigTemplate && !setting.cityBigUseAsBase)
                        {
                            const int templateW=mapTemplateCityBig.tiledMap->width();
                            const int templateH=mapTemplateCityBig.tiledMap->height();
                            int patches=1+rand()%2;
                            while(patches>0)
                            {
                                int tries=0;
                                bool patchPlaced=false;
                                while(tries<40 && !patchPlaced)
                                {
                                    const int patchX=2+rand()%((int)mapWidth-templateW-4);
                                    const int patchY=2+rand()%((int)mapHeight-templateH-4);
                                    bool valid=true;
                                    const unsigned int bx=patchX/(int)scale;
                                    const unsigned int by=patchY/(int)scale;
                                    const unsigned int ex=(patchX+templateW+(int)scale-1)/(int)scale;
                                    const unsigned int ey=(patchY+templateH+(int)scale-1)/(int)scale;
                                    unsigned int cy=by;
                                    while(cy<ey && valid)
                                    {
                                        unsigned int cx=bx;
                                        while(cx<ex && valid)
                                        {
                                            if(map[cx+cy*scaleWidth]!=1)
                                                valid=false;
                                            cx++;
                                        }
                                        cy++;
                                    }
                                    if(valid)
                                    {
                                        MapBrush::brushTheMap(worldMap,mapTemplateCityBig,x*mapWidth+patchX,y*mapHeight+patchY,MapBrush::mapMask,true);
                                        cy=by;
                                        while(cy<ey)
                                        {
                                            unsigned int cx=bx;
                                            while(cx<ex)
                                            {
                                                map[cx+cy*scaleWidth]=5;
                                                cx++;
                                            }
                                            cy++;
                                        }
                                        patchPlaced=true;
                                    }
                                    tries++;
                                }
                                patches--;
                            }
                        }

                        //signs near the border at the avenue entrances; ONE sign
                        //style per city, picked from the per-city-type tile list
                        {
                            const std::vector<std::string> *signTiles=NULL;
                            if(isBigCity && !setting.cityBigSignTiles.empty())
                                signTiles=&setting.cityBigSignTiles;
                            else if(isMediumCity && !setting.cityMediumSignTiles.empty())
                                signTiles=&setting.cityMediumSignTiles;
                            if(signTiles!=NULL && (haveHorizontalBand || haveVerticalBand))
                            {
                                Tiled::Tile * const signTile=fetchTile(worldMap,QString::fromStdString(signTiles->at(rand()%signTiles->size())));
                                Tiled::TileLayer * const signColliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
                                Tiled::TileLayer * const signWaterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
                                if(signTile!=NULL && signColliLayer!=NULL)
                                {
                                    //one sign per entrance, beside the band, near the border
                                    int signX[4];
                                    int signY[4];
                                    int signCount=0;
                                    if(haveHorizontalBand && (zoneOrientation&Orientation_left)!=0)
                                    {
                                        signX[signCount]=2;
                                        signY[signCount]=(int)hy0*(int)scale-1;
                                        signCount++;
                                    }
                                    if(haveHorizontalBand && (zoneOrientation&Orientation_right)!=0)
                                    {
                                        signX[signCount]=(int)mapWidth-3;
                                        signY[signCount]=(int)hy0*(int)scale-1;
                                        signCount++;
                                    }
                                    if(haveVerticalBand && (zoneOrientation&Orientation_top)!=0)
                                    {
                                        signX[signCount]=(int)vx0*(int)scale-1;
                                        signY[signCount]=2;
                                        signCount++;
                                    }
                                    if(haveVerticalBand && (zoneOrientation&Orientation_bottom)!=0)
                                    {
                                        signX[signCount]=(int)vx0*(int)scale-1;
                                        signY[signCount]=(int)mapHeight-3;
                                        signCount++;
                                    }
                                    //a MEDIUM city gets exactly ONE sign, at the
                                    //entrance toward the LOWEST-level neighbor
                                    //(where the players come from, see the level map)
                                    if(isMediumCity && signCount>1)
                                    {
                                        const int neighborDx[4]={-1,1,0,0};
                                        const int neighborDy[4]={0,0,-1,1};
                                        const Orientation neighborOrientation[4]={Orientation_left,Orientation_right,Orientation_top,Orientation_bottom};
                                        int bestSide=-1;
                                        unsigned int bestLevel=0;
                                        int neighborSide=0;
                                        int slotCursor=0;
                                        int slotOfSide[4]={-1,-1,-1,-1};
                                        //signX/signY were filled in the same left,right,top,bottom order
                                        while(neighborSide<4)
                                        {
                                            const bool bandOk=(neighborSide<2) ? haveHorizontalBand : haveVerticalBand;
                                            if(bandOk && (zoneOrientation&neighborOrientation[neighborSide])!=0)
                                            {
                                                slotOfSide[neighborSide]=slotCursor;
                                                slotCursor++;
                                                const int nx=(int)x+neighborDx[neighborSide];
                                                const int ny=(int)y+neighborDy[neighborSide];
                                                unsigned int neighborLevel=255;
                                                if(nx>=0 && ny>=0)
                                                {
                                                    if(haveCityEntry(citiesCoordToIndex,nx,ny))
                                                        neighborLevel=cities.at(citiesCoordToIndex.at(nx).at(ny)).level;
                                                    else if(roadCoordToIndex.find((uint16_t)nx)!=roadCoordToIndex.cend()
                                                            && roadCoordToIndex.at((uint16_t)nx).find((uint16_t)ny)!=roadCoordToIndex.at((uint16_t)nx).cend())
                                                        neighborLevel=roadCoordToIndex.at((uint16_t)nx).at((uint16_t)ny).level;
                                                }
                                                if(bestSide<0 || neighborLevel<bestLevel)
                                                {
                                                    bestSide=neighborSide;
                                                    bestLevel=neighborLevel;
                                                }
                                            }
                                            neighborSide++;
                                        }
                                        if(bestSide>=0 && slotOfSide[bestSide]>=0)
                                        {
                                            signX[0]=signX[slotOfSide[bestSide]];
                                            signY[0]=signY[slotOfSide[bestSide]];
                                            signCount=1;
                                        }
                                    }
                                    int signIndex=0;
                                    while(signIndex<signCount)
                                    {
                                        const int lx=signX[signIndex];
                                        const int ly=signY[signIndex];
                                        const unsigned int tx=x*mapWidth+lx;
                                        const unsigned int ty=y*mapHeight+ly;
                                        const unsigned int cellCode=map[(lx/(int)scale)+(ly/(int)scale)*scaleWidth];
                                        //free ground only, never on water or a building
                                        if((cellCode==0 || cellCode==1)
                                                && (signWaterLayer==NULL || signWaterLayer->cellAt(tx,ty).tile()==NULL))
                                        {
                                            signColliLayer->setCell(tx,ty,Tiled::Cell(signTile));
                                            map[(lx/(int)scale)+(ly/(int)scale)*scaleWidth]=5;
                                            //remember it (re-asserted after vegetation) and keep
                                            //the trees at distance so the sign stays visible
                                            {
                                                CitySign sign;
                                                sign.x=tx;
                                                sign.y=ty;
                                                sign.cell=Tiled::Cell(signTile);
                                                citySigns.push_back(sign);
                                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                                int ringY=-2;
                                                while(ringY<=2)
                                                {
                                                    int ringX=-2;
                                                    while(ringX<=2)
                                                    {
                                                        const int mx=(int)tx+ringX;
                                                        const int my=(int)ty+ringY;
                                                        if(mx>=0 && my>=0 && mx<worldMap.width() && my<worldMap.height())
                                                        {
                                                            const unsigned int bitMask=mx+my*worldMap.width();
                                                            if(bitMask/8<maxMapSize)
                                                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                                        }
                                                        ringX++;
                                                    }
                                                    ringY++;
                                                }
                                            }
                                            Tiled::MapObject *signBot=new Tiled::MapObject("","bot",
                                                QPointF(tx*worldMap.tileWidth(),(ty+1)*worldMap.tileHeight()),
                                                QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                                            signBot->setProperty("lookAt","bottom");
                                            //no skin: the sign tile is the visual, the bot only carries the text
                                            signBot->setProperty("signtext","Welcome to "+QString::fromStdString(city->name)+"!");
                                            Tiled::Cell signCell;
                                            signCell.setTile(invis->tileAt(0));
                                            signBot->setCell(signCell);
                                            objectLayer->addObject(signBot);
                                        }
                                        signIndex++;
                                    }
                                }
                            }
                        }
                    }
                }

                // Paint the road (cave chunk overworld stays natural: no water zone)
                const bool genChunkIsCave=(!isCity && isCaveChunk(x,y));
                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        const unsigned int type = map[(dx/scale) + (dy/scale)*scaleWidth];

                        if(!isCity && !genChunkIsCave && type == 0x2){
                            waterLayer->setCell(tx, ty, waterTile);
                        }
                    }
                }

                if(setting.displayregion){
                    for(const ZoneMarker *marker: zones){
                        // TODO: Position of MapObject() may need conversion of units from tiles to pixels (API change introduced in v0.10.0)
                        Tiled::MapObject *region = new Tiled::MapObject("", "region", QPointF(marker->x*2+mapWidth*x, marker->y*2+mapHeight*y), QSizeF(marker->w*2, marker->h*2));
                        region->setProperty("id", QString::number(marker->id));
                        region->setProperty("from", QString::number(marker->from));
                        region->setProperty("type", QString::number(marker->type));
                        region->setProperty("neightbourg", QString::number(marker->neightbourg));
                        objectLayer->addObject(region);
                    }
                }

                //CAVE chunk: finish the plan now that the corridor exists —
                //depth, stair links, and per-side floor + interior exit cell
                //("sometimes you have to go deeper to exit")
                if(!isCity && isCaveChunk(x,y)
                        && cavePlans.find(std::pair<uint16_t,uint16_t>(x,y))!=cavePlans.cend())
                {
                    CavePlan &plan=cavePlans[std::pair<uint16_t,uint16_t>(x,y)];
                    plan.depth=1;
                    if(setting.caveMaxDepth>1
                            && !setting.caveStairDownTile.isEmpty() && !setting.caveStairUpTile.isEmpty())
                        plan.depth=1+rand()%setting.caveMaxDepth;
                    //stair link cells (base-frame even floors; odd floors are the
                    //horizontal mirror): cell and the one under it must be corridor,
                    //away from the chunk border (the corridor edge line is sealed)
                    {
                        unsigned int level=0;
                        while(level+1<(unsigned int)plan.depth)
                        {
                            int foundX=-1,foundY=-1,tries=0;
                            while(tries<300 && foundX<0)
                            {
                                tries++;
                                int lx=6+rand()%((int)mapWidth-12);
                                const int ly=6+rand()%((int)mapHeight-12);
                                if(level%2!=0)
                                    lx=(int)mapWidth-1-lx;
                                const int baseX=(level%2!=0) ? (int)mapWidth-1-lx : lx;
                                const unsigned int cell=(baseX/(int)scale)+(ly/(int)scale)*scaleWidth;
                                const unsigned int cellUnder=(baseX/(int)scale)+((ly+1)/(int)scale)*scaleWidth;
                                if(map[cell]!=0 && map[cell]!=0x9 && map[cellUnder]!=0 && map[cellUnder]!=0x9)
                                {
                                    foundX=lx;
                                    foundY=ly;
                                }
                            }
                            if(foundX<0)
                                break;
                            plan.stairCells.push_back(std::pair<uint8_t,uint8_t>(foundX,foundY));
                            level++;
                        }
                        if(plan.stairCells.size()+1<(unsigned int)plan.depth)
                            plan.depth=plan.stairCells.size()+1;
                    }
                    //per side: a floor and an exit DOOR on the cliff collision
                    //of that floor, at the extremity the side serves:
                    // - top side: the TOPMOST collision accessible — the door
                    //   (exitTopTile) sits one above a walk-up gap in the ring
                    // - bottom/left/right: the BOTTOMMOST collision accessible —
                    //   the door (exitBottomTile) is a ring cell with the floor
                    //   directly above it (the sealed border line counts)
                    {
                        int side=0;
                        while(side<4)
                        {
                            if(plan.sides[side].used!=0)
                            {
                                plan.sides[side].floor=rand()%plan.depth;
                                const bool mirrored=(plan.sides[side].floor%2!=0);
                                const bool topStyle=(side==2);//the TOP side exits upward
                                int bestX=-1,bestY=-1,bestPrimary=0,bestSecondary=0;
                                int ly=2;
                                while(ly<(int)mapHeight)
                                {
                                    int lx=1;
                                    while(lx<(int)mapWidth-1)
                                    {
                                        bool candidate=false;
                                        if(topStyle)
                                        {
                                            //the GAP: blocked, floor below, blocked above (door);
                                            //gap AND door flanked by wall (never a corner)
                                            candidate=!caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx,ly)
                                                    && caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx,ly+1)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx,ly-1)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx-1,ly)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx+1,ly)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx-1,ly-1)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx+1,ly-1)
                                                    && ly-1>=1;
                                        }
                                        else
                                        {
                                            //the DOOR: blocked, floor directly above,
                                            //flanked by wall on both sides (never a corner)
                                            candidate=!caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx,ly)
                                                    && caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx,ly-1)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx-1,ly)
                                                    && !caveFloorAtTile(map,scale,scaleWidth,mapWidth,mapHeight,mirrored,lx+1,ly);
                                        }
                                        if(candidate)
                                        {
                                            //top: minimal row; others: maximal row; then
                                            //the column nearest the side it serves
                                            const int primary=topStyle ? ly : -ly;
                                            int secondary;
                                            if(side==0)
                                                secondary=lx;
                                            else if(side==1)
                                                secondary=(int)mapWidth-1-lx;
                                            else
                                                secondary=(lx>(int)mapWidth/2) ? lx-(int)mapWidth/2 : (int)mapWidth/2-lx;
                                            if(bestX<0 || primary<bestPrimary
                                                    || (primary==bestPrimary && secondary<bestSecondary))
                                            {
                                                bestX=lx;
                                                bestY=ly;
                                                bestPrimary=primary;
                                                bestSecondary=secondary;
                                            }
                                        }
                                        lx++;
                                    }
                                    ly++;
                                }
                                if(bestX>=0)
                                {
                                    if(topStyle)
                                    {
                                        //door above the gap; land below the gap
                                        plan.sides[side].exitX=bestX;
                                        plan.sides[side].exitY=bestY-1;
                                        plan.sides[side].exitLandX=bestX;
                                        plan.sides[side].exitLandY=bestY+1;
                                    }
                                    else
                                    {
                                        //the door itself; land on the floor above it
                                        plan.sides[side].exitX=bestX;
                                        plan.sides[side].exitY=bestY;
                                        plan.sides[side].exitLandX=bestX;
                                        plan.sides[side].exitLandY=bestY-1;
                                    }
                                }
                                else
                                    plan.sides[side].used=0;//no exit spot: side dropped
                            }
                            side++;
                        }
                    }
                }

                for(ZoneMarker* zone: zones){
                    delete zone;
                }

                zones.clear();

                /*
                if(isCity){
                    Tiled::MapObject* rescue = new Tiled::MapObject("", "rescue", QPointF((0.5+x)*mapWidth, (0.5+y)*mapHeight), QSizeF(1,1));
                    rescue->setCell(Tiled::Cell(invisibleTileset->tileAt(1)));

                    movingLayer->addObject(rescue);
                }
                */
            }
            x++;
        }
        y++;
    }
    LoadMapAll::deleteMapList(mapTemplatebuildingshop);
    LoadMapAll::deleteMapList(mapTemplatebuildingheal);
    LoadMapAll::deleteMapList(mapTemplatebuilding1);
    LoadMapAll::deleteMapList(mapTemplatebuilding2);
    LoadMapAll::deleteMapList(mapTemplatebuildingbig1);
    if(haveGymTemplate)
        LoadMapAll::deleteMapList(mapTemplategym);
    if(haveCityBigTemplate)
        LoadMapAll::deleteMapList(mapTemplateCityBig);
    if(haveCityMediumTemplate)
        LoadMapAll::deleteMapList(mapTemplateCityMedium);
}

bool checkEmptyRoad( const SettingsAll::SettingsExtra &setting, int tx, int ty){
    int x = tx/setting.mapWidth;
    int y = ty/setting.mapHeight;
    int scale = 2;

    if( LoadMapAll::mapPathDirection[x+y*setting.mapXCount] != 0){
        //a cave chunk overworld carries NO road (the corridor is interior-only)
        if(LoadMapAll::isCaveChunk(x,y))
            return false;
        unsigned int* map = LoadMapAll::roadData[x+y*setting.mapXCount];
        unsigned int type = map[(tx%setting.mapWidth)/scale+(ty%setting.mapHeight)/scale*setting.mapWidth/scale];

        return type != 0
                && type != 9;
    }

    return false;
}

// terrainLayers[i]/terrainIsMountain[i] are pre-resolved ONCE per addRoadContent (parallel to
// terrains): searchTileLayerByName is a linear name scan and mountainTerrain.contains a QString
// search, both independent of (tx,ty); doing them here ran ~9x per cell x |terrains| and dominated
// findChunk/QString cost.  Pre-resolution is byte-for-byte identical (same layer pointers).
bool checkTerrain(const std::vector<LoadMap::Terrain*> &terrains, const std::vector<Tiled::TileLayer*> &terrainLayers, const std::vector<char> &terrainIsMountain, unsigned int tx, unsigned int ty){

    unsigned int i=0;
    while(i<terrains.size()){
        // Only a MOUNTAIN terrain can change the result (return false). A non-mountain match was
        // discarded by the original inner if, so its cellAt was pure dead work -> skip it. The
        // cellAt (findChunk QHash lookup) was the dominant cost, and checkTerrain runs ~9x/cell.
        if(terrainIsMountain[i]){
            if(terrainLayers[i]->cellAt(tx, ty).tile() == terrains[i]->tile){
                return false;
            }
        }
        i++;
    }

    return true;
}

void LoadMapAll::addRoadContent(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting)
{
    const unsigned int mapWidth=worldMap.width()/setting.mapXCount;
    const unsigned int mapHeight=worldMap.height()/setting.mapYCount;
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    const unsigned int scale = 2;
    const unsigned int scaleWidth  = mapWidth/scale;

    unsigned int botCount = 0;
    std::vector<LoadMap::Terrain*> terrains = std::vector<LoadMap::Terrain*>();

    Tiled::TileLayer *colliLayer=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::TileLayer *walkLayer =LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer *ldownLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesDown");
    Tiled::TileLayer *lleftLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesLeft");
    Tiled::TileLayer *lrighLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesRight");
    Tiled::TileLayer *grassLayer=LoadMap::searchTileLayerByName(worldMap,"Grass");
    Tiled::TileLayer *waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::TileLayer *onGrassLayer=LoadMap::searchTileLayerByName(worldMap,"OnGrass");
    //cave overworld tiles: only the mouth pocket floor is painted here (the
    //walls live in the separate interior map written at split time)
    Tiled::Cell caveFloorCell;
    bool caveTilesOk=false;
    if(!setting.caveFloorTile.isEmpty())
    {
        Tiled::Tile * const caveFloorTile=fetchTile(worldMap,setting.caveFloorTile);
        if(caveFloorTile!=NULL)
        {
            caveFloorCell.setTile(caveFloorTile);
            caveTilesOk=true;
        }
    }
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
    const Tiled::Tileset * const mainTileset=LoadMap::searchTilesetByName(worldMap,"t1.tsx");
    Tiled::Cell newCell;

    newCell.setFlippedAntiDiagonally(false);
    newCell.setFlippedHorizontally(false);
    newCell.setFlippedVertically(false);
    newCell.setTile(invisibleTileset->tileAt(0));

    unsigned int y=0;

    unsigned int* map;

    Tiled::Cell bottomLedge;
    bottomLedge.setTile(mainTileset->tileAt(setting.ledgebottom));
    bottomLedge.setFlippedHorizontally(false);
    bottomLedge.setFlippedVertically(false);
    bottomLedge.setFlippedAntiDiagonally(false);

    Tiled::Cell leftLedge;
    leftLedge.setTile(mainTileset->tileAt(setting.ledgeleft));
    leftLedge.setFlippedHorizontally(false);
    leftLedge.setFlippedVertically(false);
    leftLedge.setFlippedAntiDiagonally(false);

    Tiled::Cell rightLedge;
    rightLedge.setTile(mainTileset->tileAt(setting.ledgeright));
    rightLedge.setFlippedHorizontally(false);
    rightLedge.setFlippedVertically(false);
    rightLedge.setFlippedAntiDiagonally(false);

    Tiled::Cell topLedge; // Unused
    topLedge.setTile(mainTileset->tileAt(setting.ledgebottom));
    topLedge.setFlippedHorizontally(false);
    topLedge.setFlippedVertically(true);
    topLedge.setFlippedAntiDiagonally(false);

    Tiled::Cell empty;
    empty.setTile(nullptr);
    empty.setFlippedHorizontally(false);
    empty.setFlippedVertically(false);
    empty.setFlippedAntiDiagonally(false);
    Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");

    std::vector<Tiled::TileLayer*> transitionsLayers = std::vector<Tiled::TileLayer*>();
    std::unordered_map<std::string, Tiled::Cell> grassTiles = std::unordered_map<std::string, Tiled::Cell>();

    for(int height=0;height<5;height++){
        for(int moisure=0;moisure<6;moisure++){
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];

            bool found = false;
            for(LoadMap::Terrain* terr: terrains ){
                if(terr->terrainName == terrain.terrainName){
                    found = true;
                }
            }

            if(!found){
                terrains.push_back(&terrain);
            }
        }
    }

    QStringList extratileset = setting.extratileset.split(";");

    for(QString tileset : extratileset){
        QStringList tilesetdata = tileset.split("->");
        if(tilesetdata.size() == 2){
            LoadMap::readTileset(tilesetdata[1], &worldMap)->setName(tilesetdata[0]);
        }
    }

    QStringList grassDataList = setting.grass.split(";");

    for(QString str: grassDataList){
        QStringList grassData = str.split("->");

        if(grassData.size() == 2){
            Tiled::Tile* tile = fetchTile(worldMap, grassData[1]);
            grassTiles[grassData.at(0).toStdString()] = Tiled::Cell(tile);
        }else{
            std::cerr << "Invalid settings [grassData] " << str.toStdString() << std::endl;
        }
    }

    for(Tiled::Layer* layer: worldMap.tileLayers()){

        Tiled::TileLayer *tileLayer = dynamic_cast<Tiled::TileLayer*>(layer);

        if (tileLayer != nullptr)
        {
            if(tileLayer->name().contains("OnGrass")){
                transitionsLayers.push_back(tileLayer);
            }
        }
        else
        {
            std::cout << "This is not a Tile Layer! File: " << __FILE__ << ", Line: " << __LINE__ << std::endl;
            exit(-1);
        }
    }

    LoadMapAll::RoadMountain mountain = LoadMapAll::mountain;
    Tiled::TileLayer *mountainLayer=LoadMap::searchTileLayerByName(worldMap,mountain.layer);
    QStringList mountainTerrain = mountain.terrain.split(",");
    QStringList mountainTile = mountain.tile.split(",");
    const Tiled::Tileset * const mountainTsx = LoadMap::searchTilesetByName(worldMap, mountain.tsx);
    QList<QPair<Tiled::Tile*, Tiled::Tile*>> walkway = QList<QPair<Tiled::Tile*, Tiled::Tile*>>();

    {
        QStringList walkwayList = setting.walkway.split(";");

        for(QString str: walkwayList){
            if(!str.isEmpty()){
                QStringList tileData = str.split("->");

                if(tileData.size() == 2){
                    walkway.push_back(QPair<Tiled::Tile*, Tiled::Tile*>(fetchTile(worldMap, tileData[0]), fetchTile(worldMap, tileData[1])));
                }
            }
        }
    }

    // Pre-resolve each terrain's layer pointer + mountain flag ONCE (used by checkTerrain ~9x/cell
    // and the type==0x3 grass test below). Both are (tx,ty)-independent; resolving per-cell was the
    // bulk of addRoadContent's findChunk/QString cost. Same pointers -> identical output.
    std::vector<Tiled::TileLayer*> terrainLayers = std::vector<Tiled::TileLayer*>();
    std::vector<char> terrainIsMountain = std::vector<char>();
    {
        unsigned int i=0;
        while(i<terrains.size()){
            terrainLayers.push_back(LoadMap::searchTileLayerByName(worldMap, terrains[i]->tmp_layerString));
            terrainIsMountain.push_back(mountainTerrain.contains(terrains[i]->terrainName) ? 1 : 0);
            i++;
        }
    }

    // Pre-fetch the mountain road-border tiles once (indexTile is 0..mountainTile.size()-1). The
    // inner loop did mountainTile.at(indexTile).toInt() (QString parse) + tileAt (QMap lookup) per
    // road-border cell; the tile set is tiny and fixed -> resolve once. Same tiles -> identical.
    std::vector<Tiled::Tile*> mountainTiles = std::vector<Tiled::Tile*>();
    {
        int i=0;
        while(i<mountainTile.size()){
            mountainTiles.push_back(mountainTsx->tileAt(mountainTile.at(i).toInt()));
            i++;
        }
    }

    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            // For each chunk
            // Do the random road gen
            const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
            const bool isCity=haveCityEntry(citiesCoordToIndex,x,y);

            if((zoneOrientation&(Orientation_bottom|Orientation_left|Orientation_right|Orientation_top)) != 0){
                map = LoadMapAll::roadData[x+y*w];

                //avenue ground of this city size (derived from the city template)
                const LoadMapAll::CityGround *paintGround=NULL;
                if(isCity)
                {
                    const City &paintCity=cities.at(citiesCoordToIndex.at(x).at(y));
                    if(paintCity.type==CityType_big)
                        paintGround=&cityGroundBig;
                    else if(paintCity.type==CityType_medium)
                        paintGround=&cityGroundMedium;
                }
                bool chunkIsCave=false;
                if(!isCity && caveTilesOk)
                    chunkIsCave=isCaveChunk(x,y);

                // Paint the road
                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);

                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        //a CAVE chunk overworld stays natural terrain: behave as if
                        //no road crossed it (no mask, mountain border decoration)
                        unsigned int type = map[(dx/scale) + (dy/scale)*scaleWidth];
                        if(chunkIsCave)
                            type=0;

                        if(type != 0 && type != 0x9) {
                            const unsigned int &bitMask=tx+ty*worldMap.width();
                            if(bitMask/8>=maxMapSize)
                                abort();
                            MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                        }else if(!checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty)){
                            uint8_t to_type_match=0;
                            if(tx>0 && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty-1) || checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty-1))
                                    to_type_match|=1;
                            }
                            if(ty>0)
                            {
                                if(checkEmptyRoad(setting, tx, ty-1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty-1))
                                    to_type_match|=2;
                            }
                            if((int)tx<(worldMap.width()-1) && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx+1, ty-1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty-1))
                                    to_type_match|=4;
                            }
                            if(tx>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty))
                                    to_type_match|=8;
                            }
                            if((int)tx<(worldMap.width()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty))
                                    to_type_match|=16;
                            }
                            if(tx>0 && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx-1, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty+1))
                                    to_type_match|=32;
                            }
                            if((int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty+1))
                                    to_type_match|=64;
                            }
                            if((int)tx<(worldMap.width()-1) && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty+1))
                                    to_type_match|=128;
                            }

                            if(to_type_match!=0)
                            {
                                unsigned int indexTile=0;

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
                                    indexTile=11;
                                else if(to_type_match&4)
                                    indexTile=10;

                                mountainLayer->setCell(tx,ty,Tiled::Cell(mountainTiles.at(indexTile)));
                            }
                        }

                        if(type == 0x1 || type==0x4) {
                            Tiled::Tile* colliTile = colliLayer->cellAt(tx, ty).tile();

                            if(colliTile != NULL){
                                for(QPair<Tiled::Tile*, Tiled::Tile*> replaceSet: walkway){
                                    if(replaceSet.first == colliTile){
                                        colliLayer->setCell(tx, ty, Tiled::Cell());
                                        walkLayer->setCell(tx, ty, Tiled::Cell(replaceSet.second));
                                        break;
                                    }
                                }
                            }
                        }else if(!isCity && type == 0x2){
                            walkLayer->setCell(tx, ty, empty);

                            for(Tiled::TileLayer* layer: transitionsLayers){
                                layer->setCell(tx, ty, empty);
                            }
                        } else if(!isCity && type == 0x3){
                            unsigned int ti=0;
                            while(ti<terrains.size()){
                                LoadMap::Terrain* terrain=terrains[ti];
                                if(terrainLayers[ti]->cellAt(tx, ty).tile() == terrain->tile){
                                    if(grassTiles.find(terrain->terrainName.toStdString()) != grassTiles.end()){
                                        //Fill the whole grass ZONE: a solid rectangular block of tall
                                        //grass is the intended Pokemon style (grid-aligned, like the
                                        //doc/mapping Goodmap) AND compresses far better than scattered
                                        //cells (long runs of one gid).  The terrain generator already
                                        //shapes the grass zones; do NOT thin them with noise.
                                        grassLayer->setCell(tx, ty, grassTiles[terrain->terrainName.toStdString()]);
                                    }
                                    break;
                                }
                                ti++;
                            }
                        }

                        //city avenue: rigid paved band, fill on Walkable + the
                        //template's border ring on OnGrass at the band perimeter.
                        //NEVER paint a tile that belongs to a placed building.
                        bool insideBuilding=false;
                        if(isCity && type==CITY_AVENUE_CODE)
                        {
                            unsigned int rectIndex=0;
                            while(rectIndex<cityBuildingRects.size() && !insideBuilding)
                            {
                                const BuildingRect &buildingRect=cityBuildingRects.at(rectIndex);
                                if(tx>=buildingRect.x && tx<buildingRect.x+buildingRect.w
                                        && ty>=buildingRect.y && ty<buildingRect.y+buildingRect.h)
                                    insideBuilding=true;
                                rectIndex++;
                            }
                        }
                        if(isCity && type==CITY_AVENUE_CODE && !insideBuilding && paintGround!=NULL && paintGround->valid){
                            if(waterLayer==NULL || waterLayer->cellAt(tx,ty).tile()==NULL){
                                colliLayer->setCell(tx,ty,empty);
                                grassLayer->setCell(tx,ty,empty);
                                walkLayer->setCell(tx,ty,paintGround->fill);
                                if(onGrassLayer!=NULL){
                                    bool upIn=true,downIn=true,leftIn=true,rightIn=true;
                                    if(dy>0)
                                        upIn=(map[(dx/scale)+((dy-1)/scale)*scaleWidth]==CITY_AVENUE_CODE);
                                    if(dy<mapHeight-1)
                                        downIn=(map[(dx/scale)+((dy+1)/scale)*scaleWidth]==CITY_AVENUE_CODE);
                                    if(dx>0)
                                        leftIn=(map[((dx-1)/scale)+(dy/scale)*scaleWidth]==CITY_AVENUE_CODE);
                                    if(dx<mapWidth-1)
                                        rightIn=(map[((dx+1)/scale)+(dy/scale)*scaleWidth]==CITY_AVENUE_CODE);
                                    unsigned int borderRow=1,borderCol=1;
                                    if(!upIn)
                                        borderRow=0;
                                    else if(!downIn)
                                        borderRow=2;
                                    if(!leftIn)
                                        borderCol=0;
                                    else if(!rightIn)
                                        borderCol=2;
                                    if(borderRow!=1 || borderCol!=1){
                                        const Tiled::Cell &borderCell=paintGround->border[borderRow][borderCol];
                                        if(!borderCell.isEmpty())
                                            onGrassLayer->setCell(tx,ty,borderCell);
                                        else
                                            onGrassLayer->setCell(tx,ty,empty);
                                    }else
                                        onGrassLayer->setCell(tx,ty,empty);
                                }
                            }
                        }

                    }
                }

                //CAVE chunk overworld: NO artificial cliff (owner: ugly).
                //Each road connection gets a vegetation-free clearing on the
                //approach; the mouth is either embedded in the FIRST natural
                //cliff on the approach line, or a small rock outcrop standing
                //on the flat ground; the terrain/vegetation seals naturally.
                if(chunkIsCave && caveTilesOk
                        && cavePlans.find(std::pair<uint16_t,uint16_t>(x,y))!=cavePlans.cend()){
                    Tiled::ObjectGroup * const movingGroup=LoadMap::searchObjectGroupByName(worldMap,"Moving");
                    Tiled::Tile * const entranceTile=fetchTile(worldMap,setting.caveEntranceTile);
                    Tiled::Tile * const entranceTopTile=fetchTile(worldMap,setting.caveEntranceTopTile);
                    const std::string caveBase=caveInteriorBaseName(x,y);
                    const CavePlan &plan=cavePlans.at(std::pair<uint16_t,uint16_t>(x,y));
                    if(movingGroup!=NULL && entranceTile!=NULL && entranceTopTile!=NULL && !caveBase.empty())
                    {
                        const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                        const int dirX[4]={1,-1,0,0};
                        const int dirY[4]={0,0,1,-1};
                        const int borderX[4]={0,(int)mapWidth-1,(int)mapWidth/2,(int)mapWidth/2};
                        const int borderY[4]={(int)mapHeight/2,(int)mapHeight/2,0,(int)mapHeight-1};
                        //the 9-tile rock block builds the flat-terrain outcrops
                        std::vector<Tiled::Tile *> rockTiles;
                        {
                            const QStringList wallTileList=setting.caveWallTile.split(",");
                            int wallTileIndex=0;
                            while(wallTileIndex<wallTileList.size())
                            {
                                Tiled::Tile * const tile=fetchTile(worldMap,wallTileList.at(wallTileIndex).trimmed());
                                if(tile!=NULL)
                                    rockTiles.push_back(tile);
                                wallTileIndex++;
                            }
                        }
                        int side=0;
                        while(side<4)
                        {
                            const CavePlanSide &planSide=plan.sides[side];
                            if(planSide.used!=0)
                            {
                                //clear the APPROACH from the border to the mouth
                                //front: a 3-wide band, obstacles removed and
                                //vegetation masked so the mouth stays reachable
                                int approachDepth;
                                if(planSide.outcrop!=0)
                                    approachDepth=5;
                                else
                                    approachDepth=
                                            (dirX[side]!=0)
                                            ? ((int)planSide.landX-borderX[side])*dirX[side]
                                            : ((int)planSide.landY-borderY[side])*dirY[side];
                                int step=0;
                                while(step<=approachDepth)
                                {
                                    int perp=-1;
                                    while(perp<=1)
                                    {
                                        const int lx=borderX[side]+dirX[side]*step+dirY[side]*perp;
                                        const int ly=borderY[side]+dirY[side]*step+dirX[side]*perp;
                                        if(lx>=0 && ly>=0 && lx<(int)mapWidth && ly<(int)mapHeight)
                                        {
                                            const unsigned int tx=x*mapWidth+lx;
                                            const unsigned int ty=y*mapHeight+ly;
                                            colliLayer->setCell(tx,ty,empty);
                                            grassLayer->setCell(tx,ty,empty);
                                            if(waterLayer!=NULL)
                                                waterLayer->setCell(tx,ty,empty);
                                            const unsigned int bitMask=tx+ty*worldMap.width();
                                            if(bitMask/8<maxMapSize)
                                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                        }
                                        perp++;
                                    }
                                    step++;
                                }
                                const unsigned int htx=x*mapWidth+planSide.mouthX;
                                const unsigned int hty=y*mapHeight+planSide.mouthY;
                                if(planSide.outcrop!=0 && !rockTiles.empty())
                                {
                                    //small rock outcrop standing free: the 3x3
                                    //block, mouth on its bottom line at the middle
                                    int blockRow=0;
                                    while(blockRow<3)
                                    {
                                        int blockColumn=0;
                                        while(blockColumn<3)
                                        {
                                            const unsigned int btx=htx-1+blockColumn;
                                            const unsigned int bty=hty-2+blockRow;
                                            Tiled::Tile *blockTile=rockTiles.front();
                                            if(rockTiles.size()>=9)
                                                blockTile=rockTiles.at(blockRow*3+blockColumn);
                                            colliLayer->setCell(btx,bty,Tiled::Cell(blockTile));
                                            grassLayer->setCell(btx,bty,empty);
                                            if(onGrassLayer!=NULL)
                                                onGrassLayer->setCell(btx,bty,empty);
                                            if(waterLayer!=NULL)
                                                waterLayer->setCell(btx,bty,empty);
                                            const unsigned int bitMask=btx+bty*worldMap.width();
                                            if(bitMask/8<maxMapSize)
                                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                            blockColumn++;
                                        }
                                        blockRow++;
                                    }
                                }
                                if(planSide.mouthKind==1)
                                    colliLayer->setCell(htx,hty,Tiled::Cell(entranceTile));
                                else
                                    colliLayer->setCell(htx,hty,Tiled::Cell(entranceTopTile));
                                std::string targetMap=caveBase;
                                if(planSide.floor>0)
                                    targetMap+="-"+std::to_string(planSide.floor+1);
                                Tiled::MapObject *hole=new Tiled::MapObject("","teleport on push",
                                    QPointF(htx*worldMap.tileWidth(),(hty+1)*worldMap.tileHeight()),
                                    QSizeF(worldMap.tileWidth(),worldMap.tileHeight()));
                                hole->setProperty("map",QString::fromStdString(targetMap));
                                hole->setProperty("x",QString::number(planSide.exitLandX));
                                hole->setProperty("y",QString::number(planSide.exitLandY));
                                Tiled::Cell holeCell;
                                holeCell.setTile(invisibleTileset->tileAt(2));
                                hole->setCell(holeCell);
                                movingGroup->addObject(hole);
                            }
                            side++;
                        }
                    }
                }

                // Ledges (a cave corridor gets none)
                if(!isCity && !chunkIsCave && setting.doledge){
                    unsigned int size = mapHeight * mapWidth / 4;
                    std::vector<LedgeMarker> possibleLedge;

                    // LedgeMarker
                    int maxX = mapWidth/2-1;
                    int maxY = mapHeight/2-1;

                    for(int ty=1; ty < maxY; ty++){
                        for(int tx=1; tx < maxX; tx++){
                            unsigned int coord = tx + ty*(mapWidth/2);

                            if((map[coord] & 0x1) == 0x1){
                                int orientation = 0;
                                int directions = 0;

                                if((map[coord-1] & 0x1) == 0x1){
                                    orientation |= Orientation_left;
                                    directions ++;
                                }
                                if((map[coord+1] & 0x1) == 0x1){
                                    orientation |= Orientation_right;
                                    directions ++;
                                }
                                if((map[coord-mapWidth/2] & 0x1) == 0x1){
                                    orientation |= Orientation_top;
                                    directions ++;
                                }
                                if((map[coord+mapWidth/2] & 0x1) == 0x1){
                                    orientation |= Orientation_bottom;
                                    directions ++;
                                }

                                if(directions == 2){
                                    if(orientation == (Orientation_left | Orientation_right)){
                                        LedgeMarker m;
                                        m.horizontal = true;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;
                                        possibleLedge.push_back(m);
                                    }else if(orientation == (Orientation_top | Orientation_bottom)){
                                        LedgeMarker m;
                                        m.horizontal = false;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;
                                        possibleLedge.push_back(m);
                                    }
                                }
                            }
                        }
                    }

                    while(!possibleLedge.empty()){
                        int s = rand() % possibleLedge.size();

                        LedgeMarker m = possibleLedge[s];

                        possibleLedge[s] = possibleLedge[possibleLedge.size()-1];
                        possibleLedge.pop_back();

                        if(rand() / (float)RAND_MAX < setting.ledgechance){
                            bool cancel = false;

                            for(unsigned int i=0; i<m.length; i++){
                                if(m.horizontal){
                                    bool inverted = rand()%2;
                                    if(inverted){
                                        map[m.x+(i+m.y)*mapWidth/2] |= 0x10;
                                        if((map[m.x+(i+m.y)*mapWidth/2-1] & 0x20) == 0x20){
                                            cancel = true;
                                            break;
                                        }
                                        if((map[m.x+(i+m.y)*mapWidth/2+1] & 0x20) == 0x20){
                                            cancel = true;
                                            break;
                                        }
                                    }else{
                                        map[i+m.x+(i+m.y)*mapWidth/2] |= 0x20;
                                        if((map[m.x+(i+m.y)*mapWidth/2-1] & 0x10) == 0x10){
                                            cancel = true;
                                            break;
                                        }
                                        if((map[m.x+(i+m.y)*mapWidth/2+1] & 0x10) == 0x10){
                                            cancel = true;
                                            break;
                                        }
                                    }
                                }else{
                                    map[i+m.x+i+m.y*mapWidth/2] |= 0x80;
                                }
                            }

                            if(cancel){
                                if(m.horizontal){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[m.x+(i+m.y)*mapWidth/2] &= ~0xF0;
                                    }
                                }else{
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[i+m.x+m.y*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }else if(m.horizontal){
                                if(!checkPathing(map, mapWidth/2, mapHeight/2, m.x-1, m.y, m.x+1, m.y)
                                        || !checkPathing(map, mapWidth/2, mapHeight/2, m.x+1, m.y, m.x-1, m.y)){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[m.x+(i+m.y)*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }else{
                                if(!checkPathing(map, mapWidth/2, mapHeight/2, m.x, m.y-1, m.x, m.y+1)
                                        || !checkPathing(map, mapWidth/2, mapHeight/2, m.x, m.y+1, m.x, m.y-1)){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[i+m.x+m.y*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }

                        }
                    }

                    // Apply ledge
                    for(unsigned int i = 0; i<size; i++){
                        unsigned int c = map[i] & 0xF0;
                        switch (c) {
                        case 0x10:
                            lleftLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, leftLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            lleftLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, leftLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x20:
                            lrighLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, rightLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            lrighLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, rightLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x40: // Unused
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, topLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, empty);
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, topLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x80:
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, bottomLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, bottomLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            break;
                        default:
                            break;
                        }
                    }
                }

                // Add bots (cave chunk: trainers go INSIDE the cave interior)
                if(!isCity && !chunkIsCave){
                    // Resize
                    unsigned int* real_map = new unsigned int[mapWidth * mapHeight];

                    for(unsigned int i=0; i<mapWidth * mapHeight; i++){
                        real_map[i] = map[(i%mapWidth)/scale+(i/mapWidth/scale)*scaleWidth];
                    }

                    // Do the random bots
                    LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);
                    const char* directions[] = {"left", "right", "top", "bottom"};

                    for(int i = 0; i<15; i++){
                        unsigned int ox = rand()%mapWidth;
                        unsigned int oy = rand()%mapHeight;
                        unsigned int j = ox + oy*mapWidth;

                        if((real_map[j] & 0xF9) == 0x1){
                            bool valid = true;

                            for(int start = 0; j<4; j++){
                                unsigned int sx = start%2 == 1? ox: ox+2-start;
                                unsigned int sy = start%2 == 0? oy: oy+3-start;

                                if(real_map[sx + sy*mapWidth] & 0x1){
                                    for(int dest = 0; dest<4; dest++){
                                        unsigned int dx = dest%2 == 1? ox: ox+2-dest;
                                        unsigned int dy = dest%2 == 0? oy: oy+3-dest;

                                        if(start != dest && (real_map[dx + dy*mapWidth] & 0x1) && !checkPathing(real_map, mapWidth, mapHeight, sx, sy, dx, dy)){
                                            valid = false;
                                        }
                                    }
                                }
                            }

                            if(valid){
                                RoadBot roadBot;
                                roadBot.x = ox + x * mapWidth;
                                roadBot.y = oy + y * mapHeight;
                                roadBot.id = botId++;
                                roadBot.look_at = rand()%4;
                                roadBot.skin = rand()%80; // read config for this value
                                roadIndex.roadBot.push_back(roadBot);

                                int tiles_x = roadBot.x;
                                int tiles_y = roadBot.y+1;

                                int tileWidth = worldMap.tileWidth();
                                int tileHeight = worldMap.tileHeight();

                                // Convert to pixel units when creating a new Tiled::MapObject
                                // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                                int pixels_x = tiles_x * tileWidth;
                                int pixels_y = tiles_y * tileHeight;

                                Tiled::MapObject *bot = new Tiled::MapObject("", "bot", QPointF(pixels_x, pixels_y), QSizeF(tileWidth, tileHeight));
                                //the per-chunk local id (and inline <bot> def) is assigned at
                                //split time by LoadMapAll::emitRoadBotsForChunk(); this id is a
                                //placeholder that gets overwritten there.
                                bot->setProperty("id", QString::number(roadBot.id));
                                bot->setProperty("lookAt", directions[roadBot.look_at]);
                                //skin is a datapack skin NAME, picked from the configured pool
                                bot->setProperty("skin", QString::fromStdString(setting.botSkins.at(rand()%setting.botSkins.size())));
                                bot->setCell(newCell);
                                objectLayer->addObject(bot);

                                botCount++;
                            }
                        }
                    }
                    delete []real_map;
                }

            }else{
                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        if(!checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty)){
                            uint8_t to_type_match=0;
                            if(tx>0 && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty-1) || checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty-1))
                                    to_type_match|=1;
                            }
                            if(ty>0)
                            {
                                if(checkEmptyRoad(setting, tx, ty-1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty-1))
                                    to_type_match|=2;
                            }
                            if((int)tx<(worldMap.width()-1) && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx+1, ty-1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty-1))
                                    to_type_match|=4;
                            }
                            if(tx>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty))
                                    to_type_match|=8;
                            }
                            if((int)tx<(worldMap.width()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty))
                                    to_type_match|=16;
                            }
                            if(tx>0 && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx-1, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx-1, ty+1))
                                    to_type_match|=32;
                            }
                            if((int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx, ty+1))
                                    to_type_match|=64;
                            }
                            if((int)tx<(worldMap.width()-1) && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty+1)|| checkTerrain(terrains, terrainLayers, terrainIsMountain, tx+1, ty+1))
                                    to_type_match|=128;
                            }

                            if(to_type_match!=0)
                            {
                                unsigned int indexTile=0;

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
                                    indexTile=11;
                                else if(to_type_match&4)
                                    indexTile=10;

                                mountainLayer->setCell(tx,ty,Tiled::Cell(mountainTiles.at(indexTile)));
                            }
                        }
                    }
                }
            }
            x++;
        }
        y++;
    }
}

bool LoadMapAll::checkPathing(unsigned int *map, unsigned int width, unsigned int height, unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy)
{
    bool *valid = new bool[width*height];
    std::vector<unsigned int> pathLeft;
    unsigned int target = dx+dy*width;

    for(unsigned int i =0; i<width*height;i++){
        valid[i] = ((map[i] & 0x1) == 0x1);
    }

    if(!valid[target]){
        delete[] valid;
        return false;
    }

    int s = 0;
    pathLeft.push_back(sx+sy*width);
    valid[pathLeft.at(0)] = false;

    while(!pathLeft.empty()){
        unsigned int point = pathLeft.at(pathLeft.size()-1);
        pathLeft.pop_back();

        if(!valid[target]){
            delete[] valid;
            return true;
        }

        if((point%width) != 0 && valid[point-1] && (map[point-1]&0xE0) == 0){
            valid[point-1] = false;
            pathLeft.push_back(point-1);
        }
        if((point%width) != width-1 && valid[point+1] && (map[point+1]&0xD0) == 0){
            valid[point+1] = false;
            pathLeft.push_back(point+1);
        }
        if(point>=width && valid[point-width] && (map[point-width]&0xB0) == 0){
            valid[point-width] = false;
            pathLeft.push_back(point-width);
        }
        if(point+width<width*height && valid[point+width] && (map[point+width]&0x70) == 0){
            valid[point+width] = false;
            pathLeft.push_back(point+width);
        }
        s++;
    }

    delete[] valid;

    return false;
}

//Emit the inline <bot> defs for the road trainers that fall inside ONE split
//chunk, and renumber those world-map bot objects to per-file local ids (1..K).
//The returned XML is spliced into the chunk's own .xml — the only place the
//engine reads bots (Map_loader.cpp). Fights are inline, so there is no longer
//any -bots.xml sidecar or /fight/ directory (the engine never read them).
QString LoadMapAll::emitRoadBotsForChunk(Tiled::Map &worldMap,
                                         const unsigned int &chunkTileX, const unsigned int &chunkTileY,
                                         const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                         const RoadIndex &roadIndex, const SettingsAll::SettingsExtra &setting)
{
    QString out;
    Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");
    if(objectLayer==NULL)
        return out;
    const unsigned int x0=chunkTileX*singleMapWidth;
    const unsigned int y0=chunkTileY*singleMapHeight;
    const unsigned int x1=x0+singleMapWidth;
    const unsigned int y1=y0+singleMapHeight;
    const QList<Tiled::MapObject*> &objects=objectLayer->objects();
    unsigned int localBotId=1;
    unsigned int index=0;
    while(index<(unsigned int)objects.size())
    {
        Tiled::MapObject *object=objects.at(index);
        if(object->type()=="bot")
        {
            const unsigned int tileX=(unsigned int)(object->x())/worldMap.tileWidth();
            const unsigned int tileY=(unsigned int)(object->y())/worldMap.tileHeight();
            if(tileX>=x0 && tileX<x1 && tileY>=y0 && tileY<y1)
            {
                Tiled::Properties properties=object->properties();
                properties.remove("file");//dead engine property
                properties["id"]=QString::number(localBotId);
                object->setProperties(properties);
                out+=botStepXml(localBotId,BotKind_fight,std::to_string(localBotId),
                                properties.value("lookAt").toString(),setting,
                                roadIndex.roadMonsters,roadIndex.level,
                                std::string(),std::vector<std::string>());
                localBotId++;
            }
        }
        index++;
    }
    return out;
}

QString LoadMapAll::emitCityBotsForChunk(Tiled::Map &worldMap,
                                         const unsigned int &chunkTileX, const unsigned int &chunkTileY,
                                         const unsigned int &singleMapWidth, const unsigned int &singleMapHeight,
                                         const SettingsAll::SettingsExtra &setting)
{
    QString out;
    Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");
    if(objectLayer==NULL)
        return out;
    const unsigned int x0=chunkTileX*singleMapWidth;
    const unsigned int y0=chunkTileY*singleMapHeight;
    const unsigned int x1=x0+singleMapWidth;
    const unsigned int y1=y0+singleMapHeight;
    const QList<Tiled::MapObject*> &objects=objectLayer->objects();
    unsigned int localBotId=1;
    unsigned int index=0;
    while(index<(unsigned int)objects.size())
    {
        Tiled::MapObject *object=objects.at(index);
        if(object->type()=="bot")
        {
            const unsigned int tileX=(unsigned int)(object->x())/worldMap.tileWidth();
            const unsigned int tileY=(unsigned int)(object->y())/worldMap.tileHeight();
            if(tileX>=x0 && tileX<x1 && tileY>=y0 && tileY<y1)
            {
                Tiled::Properties properties=object->properties();
                properties["id"]=QString::number(localBotId);
                if(properties.contains("signtext"))
                {
                    //a sign: one text step with the city welcome message; the
                    //property is generator-internal, drop it from the tmx object
                    QString signText=properties.value("signtext").toString();
                    properties.remove("signtext");
                    object->setProperties(properties);
                    signText.replace("]]>","]]&gt;");
                    out+="  <bot id=\""+QString::number(localBotId)+"\">\n"
                         "    <name>Sign</name>\n"
                         "    <step id=\"1\" type=\"text\"><text><![CDATA["+signText+"]]></text></step>\n"
                         "  </bot>\n";
                }
                else
                {
                    object->setProperties(properties);
                    //townsfolk: a plain text bot with a themed one-liner (no fight)
                    out+=botStepXml(localBotId,BotKind_text,std::to_string(localBotId),
                                    properties.value("lookAt").toString(),setting,
                                    std::vector<RoadMonster>(),0,
                                    std::string(),std::vector<std::string>());
                }
                localBotId++;
            }
        }
        index++;
    }
    return out;
}

void LoadMapAll::addCityTownsfolk(Tiled::Map &worldMap, const SettingsAll::SettingsExtra &setting,
                                  const unsigned int mapWidth, const unsigned int mapHeight)
{
    Tiled::ObjectGroup* objGroup=LoadMap::searchObjectGroupByName(worldMap,"Object");
    Tiled::TileLayer* walk=LoadMap::searchTileLayerByName(worldMap,"Walkable");
    Tiled::TileLayer* coll=LoadMap::searchTileLayerByName(worldMap,"Collisions");
    Tiled::Tileset* invis=LoadMap::searchTilesetByName(worldMap,"invisible");
    if(objGroup==NULL || walk==NULL || coll==NULL || invis==NULL || setting.botSkins.empty())
        return;
    //decoration: scatter animated flower tufts on the open ground (OnGrass layer,
    //non-collision) so a town is not a bare field.  tiles 320 (red) and 352
    //(blue) of the animations sheet are 4-frame animated flowers (legacy
    //"animation" property for the engine + standard Tiled <animation>).
    Tiled::TileLayer* onGrass=LoadMap::searchTileLayerByName(worldMap,"OnGrass");
    Tiled::Tileset* anim=LoadMap::searchTilesetByName(worldMap,"animations.tsx");
    Tiled::Tile* flowerRed=(anim!=NULL) ? anim->tileAt(320) : NULL;
    Tiled::Tile* flowerBlue=(anim!=NULL) ? anim->tileAt(352) : NULL;
    //flowers only grow on walkable GRASS ground (never on pavement/sand/snow)
    std::unordered_set<Tiled::Tile*> grassGroundTiles;
    {
        int height=0;
        while(height<5)
        {
            int moisure=0;
            while(moisure<6)
            {
                const LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
                if(terrain.tile!=NULL && terrain.terrainName.compare(QString("grass"),Qt::CaseInsensitive)==0)
                    grassGroundTiles.insert(terrain.tile);
                moisure++;
            }
            height++;
        }
    }
    const int tw=worldMap.tileWidth(), th=worldMap.tileHeight();
    unsigned int ci=0;
    while(ci<cities.size())
    {
        const City &city=cities.at(ci);
        const int x0=(int)(city.x*mapWidth), y0=(int)(city.y*mapHeight);
        //scale map of this chunk: used to keep flower tufts off the paved avenue
        unsigned int *chunkMap=NULL;
        if(mapPathDirection[city.x+city.y*setting.mapXCount]!=0)
            chunkMap=roadData[city.x+city.y*setting.mapXCount];
        const int wanted=3+rand()%3; //3..5 townsfolk per town
        int placed=0, tries=0;
        while(placed<wanted && tries<300)
        {
            tries++;
            const int tx=x0+2+rand()%((int)mapWidth-4);
            const int ty=y0+2+rand()%((int)mapHeight-4);
            if(tx<0 || ty<0 || tx>=worldMap.width() || ty>=worldMap.height())
                continue;
            //open ground only: walkable and not a wall/building/tree
            if(walk->cellAt(tx,ty).tile()==NULL || coll->cellAt(tx,ty).tile()!=NULL)
                continue;
            //object Y carries the engine's -1 tile correction, so add 1 row
            Tiled::MapObject* npc=new Tiled::MapObject("","bot",QPointF(tx*tw,(ty+1)*th),QSizeF(tw,th));
            npc->setProperty("skin",QString::fromStdString(setting.botSkins.at(rand()%setting.botSkins.size())));
            static const char* const dirs[4]={"bottom","top","left","right"};
            npc->setProperty("lookAt",dirs[rand()%4]);
            Tiled::Cell cell; cell.setTile(invis->tileAt(0));
            npc->setCell(cell);
            objGroup->addObject(npc);
            placed++;
        }
        //flower tufts: each town leans red or blue with a few of the other color
        if(onGrass!=NULL && flowerRed!=NULL)
        {
            Tiled::Tile* mainFlower=flowerRed;
            Tiled::Tile* otherFlower=flowerBlue;
            if(flowerBlue!=NULL && rand()%2==0)
            {
                mainFlower=flowerBlue;
                otherFlower=flowerRed;
            }
            const int fwant=10+rand()%14; //10..23 tufts
            int fplaced=0, ftries=0;
            while(fplaced<fwant && ftries<400)
            {
                ftries++;
                const int tx=x0+2+rand()%((int)mapWidth-4);
                const int ty=y0+2+rand()%((int)mapHeight-4);
                if(tx<0 || ty<0 || tx>=worldMap.width() || ty>=worldMap.height())
                    continue;
                if(walk->cellAt(tx,ty).tile()==NULL || coll->cellAt(tx,ty).tile()!=NULL
                        || onGrass->cellAt(tx,ty).tile()!=NULL)
                    continue;
                if(grassGroundTiles.find(walk->cellAt(tx,ty).tile())==grassGroundTiles.cend())
                    continue;
                if(chunkMap!=NULL && chunkMap[((tx-x0)/2)+((ty-y0)/2)*((int)mapWidth/2)]==CITY_AVENUE_CODE)
                    continue;
                Tiled::Tile* flower=mainFlower;
                if(otherFlower!=NULL && rand()%4==0)
                    flower=otherFlower;
                onGrass->setCell(tx,ty,Tiled::Cell(flower));
                fplaced++;
            }
        }
        ci++;
    }
}

//overworld mountain border convention: which of the 12 ring tiles to use for a
//blocked cell given the 8-neighbor walkable mask (same mapping as the
//to_type_match logic of addRoadContent)
static unsigned int mountainBorderTileIndex(const uint8_t to_type_match)
{
    unsigned int indexTile=0;
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
        indexTile=11;
    else if(to_type_match&4)
        indexTile=10;
    return indexTile;
}

//the cave stair/item tiles may reference a tileset (e.g. rename.tsx) that the
//world never loaded: pull it from the dest staging by NAME before fetchTile
static bool worldHasTilesetNamed(Tiled::Map &worldMap, const QString &name)
{
    unsigned int index=0;
    while(index<(unsigned int)worldMap.tilesetCount())
    {
        if(worldMap.tilesetAt(index)->name()==name)
            return true;
        index++;
    }
    return false;
}

static void preloadTilesetOfTileRef(Tiled::Map &worldMap, const QString &tileRef)
{
    const QStringList parts=tileRef.trimmed().split("/");
    if(parts.size()==2)
    {
        const QString name=parts.at(0).trimmed();
        if(!name.isEmpty() && !worldHasTilesetNamed(worldMap,name))
        {
            QString fileName=name;
            if(!fileName.endsWith(".tsx"))
                fileName+=".tsx";
            if(QFile::exists(QCoreApplication::applicationDirPath()+"/dest/map/tileset/"+fileName))
                LoadMap::readTileset("tileset/"+fileName,&worldMap);
        }
    }
}

Tiled::Tile *LoadMapAll::fetchTile(Tiled::Map &worldMap, QString data)
{
    QStringList tileData = data.split("/");

    if(tileData.size() == 2){
        Tiled::Tileset* ts = LoadMap::searchTilesetByName(worldMap, tileData[0]);

        if(tileData[1].toInt() >= ts->tileCount()){
            std::cout << "tile invalid " << ts->name().toStdString() << "\\" << tileData[1].toInt() << std::endl;
        }

        return ts->tileAt(tileData[1].toInt());
    }else{
        std::cerr << "Invalid settings [fetchTile] " << data.toStdString() << std::endl;
        return NULL;
    }
}

QString LoadMapAll::botStepXml(const unsigned int &id, const BotKind &kind, const std::string &name,
                               const QString &lookAt, const SettingsAll::SettingsExtra &setting,
                               const std::vector<RoadMonster> &monsterPool, const uint8_t &level,
                               const std::string &gymTypeName, const std::vector<std::string> &gymTypeMonsters)
{
    BotKind realKind=kind;
    //a fight bot with no usable monster pool degrades to a plain text bot so we
    //never emit an invalid <monster id> (engine would log "monster not found").
    if((realKind==BotKind_fight || realKind==BotKind_leader) && monsterPool.empty() && gymTypeMonsters.empty())
        realKind=BotKind_text;

    QString out="  <bot id=\""+QString::number(id)+"\"";
    if(!lookAt.isEmpty())
        out+=" lookAt=\""+lookAt+"\"";
    out+=">\n";
    //Display name: an INVENTED person name (deterministic by id) so trainers and
    //NPCs read as people instead of a raw numeric id ("<name>1</name>"). The names
    //are original/neutral — no real game character is reused. The numeric `name`
    //argument is still the per-map bot id and is used as the seed.
    static const char *const botDisplayNames[]={
        "Aldric","Bryn","Cora","Dane","Edda","Finn","Gwen","Hale","Ivo","Juna",
        "Kael","Lyra","Mira","Nuri","Oren","Pell","Quin","Rhea","Soren","Tova",
        "Ulf","Vera","Wren","Xan","Yara","Zane","Bram","Cleo","Dora","Esra",
        "Faye","Gus","Hana","Iris","Jad","Kira","Loris","Mads","Noa","Otto"};
    const size_t botDisplayNameCount=sizeof(botDisplayNames)/sizeof(botDisplayNames[0]);
    size_t nameSeed=0;
    {
        //seed from the passed id string (the per-map bot id) for stable variety
        size_t ci=0;
        while(ci<name.size()) { nameSeed=nameSeed*10+static_cast<size_t>(name[ci]); ci++; }
    }
    QString displayName=botDisplayNames[(nameSeed+id)%botDisplayNameCount];
    out+="    <name>"+displayName+"</name>\n";
    switch(realKind)
    {
        case BotKind_heal:
            //healing-center attendant: greet -> heal -> sign off (target.md §5.4)
            out+="    <step id=\"1\" type=\"text\"><text><![CDATA[Let me restore your team.<br /><a href=\"3;2\">[Heal]</a>]]></text></step>\n";
            out+="    <step id=\"2\" type=\"text\"><text><![CDATA[Good luck out there!]]></text></step>\n";
            out+="    <step id=\"3\" type=\"heal\"/>\n";
        break;
        case BotKind_shop:
        {
            out+="    <step id=\"1\" type=\"text\"><text><![CDATA[Welcome!<br /><a href=\"2\">Buy</a><br /><a href=\"3\">Sell</a>]]></text></step>\n";
            out+="    <step id=\"2\" type=\"shop\">\n";
            unsigned int indexItem=0;
            while(indexItem<setting.shopItems.size())
            {
                out+="      <product item=\""+QString::number(setting.shopItems.at(indexItem))+"\"/>\n";
                indexItem++;
            }
            out+="    </step>\n";
            out+="    <step id=\"3\" type=\"sell\"/>\n";
        }
        break;
        case BotKind_fight:
        case BotKind_leader:
        {
            const bool leader=(realKind==BotKind_leader);
            //team size 1..4 (road math), the gym leader fields one extra
            int monsterCount=rand()%2 + rand()%3 + 1;
            if(leader)
                monsterCount++;
            int reward=level*30+100;
            QString monsterXml;
            int indexMonster=0;
            while(indexMonster<monsterCount)
            {
                //in a typed gym part of the team comes from the type pool, and
                //the leader's last monster (the ace) always does
                bool useGymPool=false;
                if(!gymTypeMonsters.empty())
                {
                    if(monsterPool.empty())
                        useGymPool=true;
                    else if(leader && indexMonster==monsterCount-1)
                        useGymPool=true;
                    else
                        useGymPool=(rand()%2==0);
                }
                int monsterLevel=level*(95+rand()%10)/100;
                if(monsterLevel<1)
                    monsterLevel=1;
                QString monsterId;
                if(useGymPool)
                    monsterId=QString::fromStdString(gymTypeMonsters.at(rand()%gymTypeMonsters.size()));
                else
                    monsterId=monsterRef(monsterPool.at(rand()%monsterPool.size()).monsterId,setting);
                monsterXml+="      <monster id=\""+monsterId+"\" level=\""+QString::number(monsterLevel)+"\"/>\n";
                reward+=monsterLevel*monsterLevel;
                indexMonster++;
            }
            if(leader)
                reward*=2;
            out+="    <step id=\"1\" type=\"fight\"";
            if(leader)
                out+=" leader=\"true\"";//engine ignores it; kept for datapack parity
            out+=">\n";
            if(leader)
            {
                if(!gymTypeName.empty())
                    out+="      <start><![CDATA[I am the leader here. My "+QString::fromStdString(gymTypeName)+" team will test you!]]></start>\n";
                else
                    out+="      <start><![CDATA[I am the leader here. Prove your worth!]]></start>\n";
            }
            else
                out+="      <start><![CDATA[You look strong - let's battle!]]></start>\n";
            out+="      <win><![CDATA[Well fought.]]></win>\n";
            out+=monsterXml;
            out+="      <gain cash=\""+QString::number(reward)+"\"/>\n";
            out+="    </step>\n";
        }
        break;
        case BotKind_text:
        default:
        {
            QString message;
            if(!setting.npcMessage.empty())
                message=QString::fromStdString(setting.npcMessage.at(rand()%setting.npcMessage.size()));
            else
                message="...";
            //a literal ]]> inside a message would close the CDATA section early
            message.replace("]]>","]]&gt;");
            out+="    <step id=\"1\" type=\"text\"><text><![CDATA["+message+"]]></text></step>\n";
        }
        break;
    }
    out+="  </bot>\n";
    return out;
}

void LoadMapAll::generateRoom(Tiled::Map& worldMap, const MapBrush::MapTemplate &mapTemplate, const unsigned int id, const uint32_t &x, const uint32_t &y,
                              const std::pair<uint8_t, uint8_t> pos, const City &city, const std::string &zone,  const SettingsAll::SettingsExtra &setting, RoomSettings &roomSettings)
{
    //search the brush door and retarget
    std::unordered_map<Tiled::MapObject*,Tiled::Properties> oldValue;
    std::vector<Tiled::MapObject*> mainDoors=getDoorsListAndTp(mapTemplate.tiledMap);
    std::vector<Tiled::MapObject*> doors{};
    std::vector<Tiled::Map *> otherMap;
    int w = mapTemplate.tiledMap->width()+4;
    int h = mapTemplate.tiledMap->height()+4;
    unsigned int index=0;

    QString name = "building-"+QString::number(id);
    //Generic houses are SINGLE-floor — multi-floor (floor-N folders) is reserved
    //for big/special buildings, not plain houses (owner feedback: a building-N
    //must not be a multi-floor building).  Keep the rand() draw so the
    //deterministic building-placement stream downstream is unchanged.
    ((void)((double)rand()/RAND_MAX));
    unsigned int floorCount = 1;
    if(mainDoors.size() > 0){
        for(unsigned int i=0; i<floorCount; i++){
            Tiled::Map* room = new Tiled::Map(mapTemplate.tiledMap->orientation(),
                                              w, h,
                                              mapTemplate.tiledMap->tileWidth(), mapTemplate.tiledMap->tileHeight());

            roomSettings.id = i;
            roomSettings.hasFloorDown = (i!=0);
            roomSettings.hasFloorUp = (i!=floorCount-1);
            generateRoomContent(*room, setting, roomSettings);
            otherMap.push_back(room);

            room->setProperty("floor-id", QString::number(i));
        }
    }

    while(index<(unsigned int)mainDoors.size())
    {
        Tiled::MapObject* object=mainDoors.at(index);
        Tiled::Properties properties=object->properties();
        oldValue[object]=object->properties();
        if(otherMap.size()>1)
            properties["map"]=name+"/"+"floor-0";
        else
            properties["map"]=name;
        properties["x"]=QString::number(w/2);
        properties["y"]=QString::number(h-1);
        object->setProperties(properties);
        index++;
    }
    MapBrush::brushTheMap(worldMap,mapTemplate,x*setting.mapWidth+pos.first,y*setting.mapHeight+pos.second,MapBrush::mapMask,true);
    index=0;
    while(index<(unsigned int)mainDoors.size())//reset to the old value
    {
        Tiled::MapObject* object=mainDoors.at(index);
        object->setProperties(oldValue.at(object));
        index++;
    }
    doors.clear();
    //search next hop door and retarget
    {
        unsigned int indexMap=0;
        while(indexMap<otherMap.size())
        {
            std::vector<Tiled::MapObject*> doorsLocale=getDoorsListAndTp(otherMap.at(indexMap));
            doors.insert(doors.end(),doorsLocale.begin(),doorsLocale.end());
            unsigned int index=0;
            while(index<(unsigned int)doorsLocale.size())
            {
                Tiled::MapObject* object=doorsLocale.at(index);
                Tiled::Properties properties=object->properties();
                oldValue[object]=properties;
                if(object->name() == "exit")
                {
                    if(otherMap.size()>1)
                        properties["map"]="../"+QString::fromStdString(LoadMapAll::lowerCase(city.name));
                    else
                        properties["map"]=QString::fromStdString(LoadMapAll::lowerCase(city.name));

                    Tiled::MapObject* door = mainDoors.at(rand()%mainDoors.size());

                    // Convert to pixel units when creating a new Tiled::MapObject
                    // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                    properties["x"]=QString::number((door->x() / worldMap.tileWidth()) + pos.first);
                    properties["y"]=QString::number((door->y() / worldMap.tileHeight()) + pos.second);

                    object->setProperties(properties);
                    object->setName("");
                }
                index++;
            }
            indexMap++;
        }
    }
    //write all next hop
    index=0;
    while(index<(unsigned int)otherMap.size())
    {
        Tiled::Map *nextHopMap=otherMap.at(index);
        Tiled::Properties properties=nextHopMap->properties();
        //renumber this floor's bot objects to local ids 1..K and build the
        //matching inline <bot> defs: the engine reads bots ONLY from a map's own
        //sibling .xml, matched by uint8 id, so per-file local ids are required
        //(the global botId counter overflows uint8 on large worlds).
        QString botXml;
        {
            Tiled::ObjectGroup *npcs=LoadMap::searchObjectGroupByName(*nextHopMap,"Object");
            if(npcs!=NULL)
            {
                const QList<Tiled::MapObject*> &npcObjects=npcs->objects();
                unsigned int localBotId=1;
                unsigned int indexNpc=0;
                while(indexNpc<(unsigned int)npcObjects.size())
                {
                    Tiled::MapObject *npc=npcObjects.at(indexNpc);
                    if(npc->type()=="bot")
                    {
                        Tiled::Properties npcProperties=npc->properties();
                        npcProperties["id"]=QString::number(localBotId);
                        npc->setProperties(npcProperties);
                        botXml+=botStepXml(localBotId,BotKind_text,std::to_string(localBotId),
                                           npcProperties.value("lookAt").toString(),setting,
                                           std::vector<RoadMonster>(),0,
                                           std::string(),std::vector<std::string>());
                        localBotId++;
                    }
                    indexNpc++;
                }
            }
        }
        std::string filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+name.toStdString()+".tmx";
        if(otherMap.size()>1)
            filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+name.toStdString()+"/floor-"+std::to_string(index)+".tmx";

        QFileInfo fileInfo(QCoreApplication::applicationDirPath()+QString::fromStdString(filePath));
        QDir mapDir(fileInfo.absolutePath());
        if(!mapDir.mkpath(fileInfo.absolutePath()))
        {
            std::cerr << "Unable to create path: " << fileInfo.absolutePath().toStdString() << std::endl;
            abort();
        }
        Tiled::MapWriter maprwriter;

#ifdef TILED_CSV
        nextHopMap->setLayerDataFormat(Tiled::Map::CSV);  // DEBUG
#else
        //canonical datapack encoding: base64 + zstd (like the hand-made
        //gen2/johto reference), smaller than the libtiled default zlib.
        nextHopMap->setLayerDataFormat(Tiled::Map::Base64Zstandard);
#endif

        nextHopMap->setProperties(Tiled::Properties());
        if(!maprwriter.writeMap(nextHopMap,fileInfo.absoluteFilePath()))
        {
            std::cerr << "Unable to write " << fileInfo.absoluteFilePath().toStdString() << std::endl;
            abort();
        }
        nextHopMap->setProperties(properties);

        {
            QString xmlPath(fileInfo.absoluteFilePath());
            xmlPath.remove(xmlPath.size()-4,4);
            xmlPath+=".xml";
            QFile xmlinfo(xmlPath);
            if(xmlinfo.open(QFile::WriteOnly))
            {
                QString content("<map");
                if(properties.contains("type"))
                    content+=" type=\""+properties.value("type").toString()+"\"";
                if(!zone.empty())
                    content+=" zone=\""+QString::fromStdString(zone)+"\"";
                content+=">\n"
                         "  <name>floor-"+QString::number(index)+"</name>\n";
                content+=botXml;
                content+="</map>";
                QByteArray contentData(content.toUtf8());
                xmlinfo.write(contentData.constData(),contentData.size());
                xmlinfo.close();
            }
            else
            {
                std::cerr << "Unable to write " << xmlPath.toStdString() << std::endl;
                abort();
            }
        }
        index++;
    }
    //reset next hop
    {
        unsigned int index=0;
        while(index<(unsigned int)doors.size())//reset to the old value
        {
            Tiled::MapObject* object=doors.at(index);
            object->setProperties(oldValue.at(object));
            index++;
        }
    }

    //bot definitions are now emitted inline in each floor's own .xml above (the
    //only place the engine reads bots from); the old <bots>-wrapped *-bot.xml
    //sidecar was never read by the engine. Just free the per-floor room maps.
    {
        unsigned int indexRoom=0;
        while(indexRoom<(unsigned int)otherMap.size())
        {
            delete otherMap.at(indexRoom);
            indexRoom++;
        }
    }
    doors.clear();
}

void LoadMapAll::generateRoomContent(Tiled::Map &roomMap, const SettingsAll::SettingsExtra &setting, const RoomSettings& roomSettings)
{
    SettingsAll::RoomSetting room = setting.room;

    for(QString tileset: room.tilesets){
        QStringList tilesetdata = tileset.split("->");
        Tiled::Tileset* t;

        if(tilesetdata.size() == 2){
            t = LoadMap::readTileset(QString(tilesetdata.at(1)), &roomMap);
            t->setName(tilesetdata.at(0));
        }else{
            t=LoadMap::readTileset(QString(tileset), &roomMap);
        }
    }

    Tiled::TileLayer* walkable = new Tiled::TileLayer("Walkable", 0, 0, roomMap.width(), roomMap.height());
    Tiled::TileLayer* RSLayer;
    SettingsAll::RoomStructure wall = roomSettings.wall;

    roomMap.addLayer(walkable);
    roomMap.addLayer(new Tiled::TileLayer("OnGrass", 0, 0, roomMap.width(), roomMap.height()));
    roomMap.addLayer(new Tiled::TileLayer("Collisions", 0, 0, roomMap.width(), roomMap.height()));
    roomMap.addLayer(new Tiled::TileLayer("WalkBehind", 0, 0, roomMap.width(), roomMap.height()));
    roomMap.addLayer(new Tiled::ObjectGroup("Moving", 0, 0));
    roomMap.addLayer(new Tiled::ObjectGroup("Object", 0, 0));
    RSLayer = LoadMap::searchTileLayerByName(roomMap, wall.layer);

    // Generate wall & floor
    {
        Tiled::Tile* floor = fetchTile(roomMap, roomSettings.floor);

        for(int x=0; x<roomMap.width(); x++){
            for(int y=0; y<roomMap.height(); y++){
                if(y < wall.height){
                    RSLayer->setCell(x, y, Tiled::Cell(fetchTile(roomMap, wall.tiles[x%wall.width+y%wall.height*wall.width])));
                }else{
                    walkable->setCell(x, y, Tiled::Cell(floor));
                }
            }
        }
    }

    // Generate table in the center
    {
        SettingsAll::Furnitures table = roomSettings.table;
        int tx=(roomMap.width()-table.width)/2;
        int ty=(roomMap.height()+wall.height-table.height)/2;

        placeRoomFurniture(roomMap, table, tx, ty);
    }

    // Generate exit door
    if(roomSettings.id == 0){
        SettingsAll::Furnitures exitDoor = roomSettings.exit;
        int tx=(roomMap.width()-exitDoor.width)/2;
        int ty=roomMap.height()-exitDoor.height;
        placeRoomFurniture(roomMap, exitDoor, tx, ty);
    }

    // Random Furnitures
    {
        // Deterministic per config.seed: rand() is seeded by srand(config.seed) (main.cpp).
        // random_device here made interior furniture/bot placement non-reproducible per seed.
        std::shuffle(room.limitations.begin(), room.limitations.end(), std::mt19937{(std::mt19937::result_type)rand()});
        bool* spots = new bool[roomMap.width()];
        memset(spots, false, roomMap.width());

        if(roomSettings.hasFloorDown || roomSettings.hasFloorUp){
            for(int i=roomMap.width()-4; i< roomMap.width(); i++){
                spots[i] = true;
            }
        }

        if(roomSettings.hasFloorDown && roomSettings.hasFloorUp){
            for(int i=0; i< 4; i++){
                spots[i] = true;
            }

            placeRoomFurniture(roomMap, roomSettings.stairDown, 2, wall.height);
            placeRoomFurniture(roomMap, roomSettings.stairUp, roomMap.width()-roomSettings.stairUp.width, wall.height);
        }else if(roomSettings.hasFloorDown){
            placeRoomFurniture(roomMap, roomSettings.stairDown, roomMap.width()-4, wall.height);
        }else if(roomSettings.hasFloorUp){
            placeRoomFurniture(roomMap, roomSettings.stairUp, roomMap.width()-roomSettings.stairUp.width, wall.height);
        }

        for(SettingsAll::FurnituresLimitations limitation : room.limitations){
            int count = limitation.min;
            std::vector<SettingsAll::Furnitures> availables{};

            for(int i=limitation.min; i<limitation.max; i++){
                if((double)rand()/RAND_MAX < limitation.chance) count++;
            }

            for(SettingsAll::Furnitures f: room.furnitures){
                if(f.tags.contains(limitation.tag, Qt::CaseInsensitive)){
                    availables.push_back(f);
                }
            }

            if(count > 0 && availables.size() > 0){
                for(int i=0; i<count; i++){
                    SettingsAll::Furnitures f = availables.at(rand()%availables.size());

                    for(int j=0; j<4; j++) // Try 4 times to place it
                    {
                        int fx = rand()%roomMap.width()+f.offsetX;
                        bool valid = true;
                        if(fx+f.width < roomMap.width()){
                            for(int x=0; x<f.width; x++){
                                if(x+fx<0 || x+fx>=roomMap.width() || spots[x+fx]){
                                    valid = false;
                                    break;
                                }
                            }
                        }else{
                            valid = false;
                        }

                        if(valid){
                            placeRoomFurniture(roomMap, f, fx-f.offsetX, wall.height);

                            for(int x=0; x<f.width; x++){
                                spots[x+fx]=true;
                            }
                            break;
                        }
                    }
                }
            }
        }

        delete [] spots;
    }

    //---- Random townsfolk NPC: an interior must NOT be empty (owner feedback).
    //generateRoom's writer turns any "bot" object in the Object group into an
    //inline <bot> with a themed one-line message (botStepXml/BotKind_text), so a
    //house feels lived-in.  Placed on a free floor tile, random skin + facing.
    {
        Tiled::ObjectGroup* objGroup=LoadMap::searchObjectGroupByName(roomMap,"Object");
        Tiled::TileLayer* coll=LoadMap::searchTileLayerByName(roomMap,"Collisions");
        Tiled::Tileset* invis=LoadMap::searchTilesetByName(roomMap,"invisible");
        const int floorRows=roomMap.height()-wall.height-3; //walkable band (excl. back furniture row + bottom exit)
        if(objGroup!=NULL && coll!=NULL && invis!=NULL && !setting.botSkins.empty() && floorRows>0)
        {
            int bx=-1,by=-1,tries=0;
            while(tries<40 && bx<0)
            {
                const int tx=1+rand()%(roomMap.width()-2);
                const int ty=wall.height+1+rand()%floorRows;
                if(coll->cellAt(tx,ty).tile()==NULL && walkable->cellAt(tx,ty).tile()!=NULL)
                { bx=tx; by=ty; }
                tries++;
            }
            if(bx>=0)
            {
                const int tw=roomMap.tileWidth(), th=roomMap.tileHeight();
                //object Y carries the engine's -1 tile correction, so add 1 row
                Tiled::MapObject* npc=new Tiled::MapObject("","bot",QPointF(bx*tw,(by+1)*th),QSizeF(tw,th));
                npc->setProperty("skin",QString::fromStdString(setting.botSkins.at(rand()%setting.botSkins.size())));
                static const char* const lookDirs[4]={"bottom","top","left","right"};
                npc->setProperty("lookAt",lookDirs[rand()%4]);
                Tiled::Cell botCell; botCell.setTile(invis->tileAt(0));
                npc->setCell(botCell);
                objGroup->addObject(npc);
            }
        }
    }

    for(Tiled::SharedTileset t: roomMap.tilesets()){
        if(roomSettings.hasFloorDown || roomSettings.hasFloorUp){
            t->setFileName("../../"+t->fileName());
        }else{
            t->setFileName("../"+t->fileName());
        }
    }

    {
        Tiled::ObjectGroup* moving = LoadMap::searchObjectGroupByName(roomMap, "Moving");

        for(Tiled::MapObject* door: moving->objects()){
            Tiled::Properties properties = door->properties();

            if(door->name() != "exit"){
                if(door->name()=="stair-up"){
                    properties["map"] = "floor-"+QString::number(roomSettings.id+1);
                    properties["x"] = QString::number(roomMap.width()-4+roomSettings.stairDown.width);
                    properties["y"] = QString::number(wall.height);
                }else{
                    properties["map"] = "floor-"+QString::number(roomSettings.id-1);
                    properties["x"] = QString::number(roomMap.width()-roomSettings.stairUp.width);
                    properties["y"] = QString::number(wall.height);
                }
                door->setName("");
            }else{
                properties["map"] = "exit";
            }
            door->setProperties(properties);
        }
    }

    {
        Tiled::ObjectGroup* npcs = LoadMap::searchObjectGroupByName(roomMap, "Object");
        std::vector<QString> directions {"top", "bottom", "left", "right"};

        for(Tiled::MapObject* npc: npcs->objects()){

            if(npc->type() == "bot"){
                Tiled::Properties properties = npc->properties();
                if(((double) rand() / RAND_MAX) <= properties.value("chance", "1").toFloat()){
                    properties.remove("chance");

                    if(!properties.contains("lookAt")){
                        properties["lookAt"] = directions[rand()%4];
                    }
                    if(!properties.contains("skin")){
                        properties["skin"] = QString::fromStdString(setting.botSkins.at(rand()%setting.botSkins.size()));
                    }
                    properties["id"] = QString::number(botId++);

                    npc->setProperties(properties);
                }else{
                    npcs->removeObject(npc);
                }
            }
        }
    }
}

void LoadMapAll::placeRoomFurniture(Tiled::Map &roomMap, const SettingsAll::Furnitures &furnitures, int x, int y)
{
    x+= furnitures.offsetX;
    y+= furnitures.offsetY;

    if(!furnitures.templatePath.isEmpty()){
        MapBrush::MapTemplate furnitureTemplate;
        loadMapTemplate("",furnitureTemplate, furnitures.templatePath, roomMap.width(), roomMap.height(), roomMap);
        MapBrush::brushTheMap(roomMap, furnitureTemplate, x, y, NULL, true);
        LoadMapAll::deleteMapList(furnitureTemplate);
    }

    if(!furnitures.tiles.empty()){
        Tiled::TileLayer* RSLayer = LoadMap::searchTileLayerByName(roomMap, furnitures.layer);

        for(int tx=0; tx<furnitures.width; tx++){
            for(int ty=0; ty<furnitures.height; ty++){
                RSLayer->setCell(x+tx, y+ty, Tiled::Cell(fetchTile(roomMap, furnitures.tiles[tx+ty*furnitures.width])));
            }
        }
    }
}
