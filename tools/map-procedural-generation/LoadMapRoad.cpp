#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"

#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_tile.h"
#include "../../client/tiled/tiled_objectgroup.h"

#include <iostream>
#include <algorithm>
#include <queue>
#include <unordered_map>

#include <QDir>
#include <QCoreApplication>

unsigned int ** LoadMapAll::roadData = NULL;
LoadMapAll::RoadMountain LoadMapAll::mountain;

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

void placeZones(unsigned int* map, unsigned int w, unsigned int h, unsigned int orientation, Tiled::Map &worldMap, unsigned int shiftX, unsigned int shiftY, const SettingsAll::SettingsExtra &setting){
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
        return;
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

    // If can join all the path
    Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");

    for(unsigned int x = 0; x<w; x++){
        for(unsigned int y = 0; y<h; y++){
            map[x+y*w] = 0;
        }
    }

    for(const ZoneMarker *marker: zones){
        Tiled::MapObject *region = new Tiled::MapObject("", "region", QPointF(marker->x*2+shiftX, marker->y*2+shiftY), QSizeF(marker->w*2, marker->h*2));
        region->setProperty("id", QString::number(marker->id));
        region->setProperty("from", QString::number(marker->from));
        region->setProperty("type", QString::number(marker->type));
        region->setProperty("neightbourg", QString::number(marker->neightbourg));
        objectLayer->addObject(region);

        //std::cout << marker.id << " " << (marker.x*2+shiftX)*16 << ":" << (marker.y*2+shiftY)*16 << " - " << marker.w*32 << "x" << marker.h*32 <<  " " << marker.x<< ":" << marker.y<<std::endl;

        for(int x = marker->x; x<marker->x+marker->w; x++){
            for(int y = marker->y; y<marker->y+marker->h; y++){
                map[x+y*w] = marker->type;
            }
        }
    }

    //std::cout << "region data " << id << " <-> " << zones.size() << std::endl;

    delete [] area;
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
    LoadMap::Terrain* water = searchWater();

    Tiled::Cell waterTile = Tiled::Cell();

    if(water != NULL && water->tile != NULL){
        waterTile.tile = water->tile;
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

                unsigned int* map = new unsigned int[scaleWidth * scaleHeight];
                roadData[x+y*w] = map;

                // Step 1: analyse
                for(unsigned int sy = 0; sy < scaleWidth; sy++){
                    for(unsigned int sx = 0; sx < scaleWidth; sx++){
                        map[sx + sy*scaleWidth] = 0;

                        for(unsigned int j= 0; j < scale; j++){
                            for(unsigned int k = 0; k < scale; k++){
                                if(colliLayer->cellAt(x*mapWidth+sx*scale+j, y*mapHeight+sy*scale+k).tile != NULL
                                        || waterLayer->cellAt(x*mapWidth+sx*scale+j, y*mapHeight+sy*scale +k).tile != NULL){
                                    map[sx + sy*scaleWidth] |= 0x08; // Obstacle
                                }
                            }
                        }
                    }
                }

                // Step 2: place the random zone
                placeZones(map, scaleWidth, scaleHeight, zoneOrientation, worldMap, x*mapWidth, y*mapHeight, setting);

                // Paint the road
                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        const unsigned int type = map[(dx/scale) + (dy/scale)*scaleWidth];

                        if(!isCity && type == 0x2){
                            waterLayer->setCell(tx, ty, waterTile);
                        }
                    }
                }
            }
            x++;
        }
        y++;
    }
}

bool checkEmptyRoad( const SettingsAll::SettingsExtra &setting, int tx, int ty){
    int x = tx/setting.mapWidth;
    int y = ty/setting.mapHeight;
    int scale = 2;

    if( LoadMapAll::mapPathDirection[x+y*setting.mapXCount] != 0){
        unsigned int* map = LoadMapAll::roadData[x+y*setting.mapXCount];
        unsigned int type = map[(tx%setting.mapWidth)/scale+(ty%setting.mapHeight)/scale*setting.mapWidth/scale];

        return type != 0
                && type != 9;
    }

    return false;
}

bool checkTerrain(const std::vector<LoadMap::Terrain*> &terrains, const Tiled::Map &worldMap, unsigned int tx, unsigned int ty, const QStringList &mountainTerrain){

    for(LoadMap::Terrain* terrain:terrains){
        if(LoadMap::searchTileLayerByName(worldMap, terrain->tmp_layerString)->cellAt(tx, ty).tile == terrain->tile){
            if(mountainTerrain.contains(terrain->terrainName)){
                return false;
            }
        }
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
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
    const Tiled::Tileset * const mainTileset=LoadMap::searchTilesetByName(worldMap,"t1.tsx");
    Tiled::Cell newCell;

    newCell.flippedAntiDiagonally=false;
    newCell.flippedHorizontally=false;
    newCell.flippedVertically=false;
    newCell.tile=invisibleTileset->tileAt(0);

    unsigned int y=0;

    unsigned int* map;

    Tiled::Cell bottomLedge;
    bottomLedge.tile=mainTileset->tileAt(setting.ledgebottom);
    bottomLedge.flippedHorizontally=false;
    bottomLedge.flippedVertically=false;
    bottomLedge.flippedAntiDiagonally=false;

    Tiled::Cell leftLedge;
    leftLedge.tile=mainTileset->tileAt(setting.ledgeleft);
    leftLedge.flippedHorizontally=false;
    leftLedge.flippedVertically=false;
    leftLedge.flippedAntiDiagonally=false;

    Tiled::Cell rightLedge;
    rightLedge.tile=mainTileset->tileAt(setting.ledgeright);
    rightLedge.flippedHorizontally=false;
    rightLedge.flippedVertically=false;
    rightLedge.flippedAntiDiagonally=false;

    Tiled::Cell topLedge; // Unused
    topLedge.tile=mainTileset->tileAt(setting.ledgebottom);
    topLedge.flippedHorizontally=false;
    topLedge.flippedVertically=true;
    topLedge.flippedAntiDiagonally=false;

    Tiled::Cell empty;
    empty.tile=NULL;
    empty.flippedHorizontally=false;
    empty.flippedVertically=false;
    empty.flippedAntiDiagonally=false;
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
            QStringList tiledata = grassData.at(1).split("/");
            Tiled::Cell grassCell = Tiled::Cell(LoadMap::searchTilesetByName(worldMap, tiledata[0] )->tileAt(tiledata[1].toInt()));
            grassTiles[grassData.at(0).toStdString()] = grassCell;
        }else{
            std::cerr << "Invalid settings [grassData] " << str.toStdString() << std::endl;
        }
    }

    for(Tiled::TileLayer* layer: worldMap.tileLayers()){
        if(layer->name().contains("OnGrass")){
            transitionsLayers.push_back(layer);
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
                    QStringList sourcedata = tileData.at(0).split("/");
                    QStringList targetdata = tileData.at(1).split("/");
                    Tiled::Tile* sourceCell = LoadMap::searchTilesetByName(worldMap, sourcedata[0] )->tileAt(sourcedata[1].toInt());
                    Tiled::Tile* targetCell = LoadMap::searchTilesetByName(worldMap, targetdata[0] )->tileAt(targetdata[1].toInt());

                    walkway.push_back(QPair<Tiled::Tile*, Tiled::Tile*>(sourceCell, targetCell));
                }else{
                }
            }
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

                // Paint the road
                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);

                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        const unsigned int type = map[(dx/scale) + (dy/scale)*scaleWidth];

                        if(type != 0 && type != 0x9) {
                            const unsigned int &bitMask=tx+ty*worldMap.width();
                            if(bitMask/8>=maxMapSize)
                                abort();
                            MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                        }else if(!checkTerrain(terrains, worldMap, tx, ty, mountainTerrain)){
                            uint8_t to_type_match=0;
                            if(tx>0 && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty-1) || checkTerrain(terrains, worldMap, tx-1, ty-1, mountainTerrain))
                                    to_type_match|=1;
                            }
                            if(ty>0)
                            {
                                if(checkEmptyRoad(setting, tx, ty-1)|| checkTerrain(terrains, worldMap, tx, ty-1, mountainTerrain))
                                    to_type_match|=2;
                            }
                            if((int)tx<(worldMap.width()-1) && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx+1, ty-1)|| checkTerrain(terrains, worldMap, tx+1, ty-1, mountainTerrain))
                                    to_type_match|=4;
                            }
                            if(tx>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty)|| checkTerrain(terrains, worldMap, tx-1, ty, mountainTerrain))
                                    to_type_match|=8;
                            }
                            if((int)tx<(worldMap.width()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty)|| checkTerrain(terrains, worldMap, tx+1, ty, mountainTerrain))
                                    to_type_match|=16;
                            }
                            if(tx>0 && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx-1, ty+1)|| checkTerrain(terrains, worldMap, tx-1, ty+1, mountainTerrain))
                                    to_type_match|=32;
                            }
                            if((int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx, ty+1)|| checkTerrain(terrains, worldMap, tx, ty+1, mountainTerrain))
                                    to_type_match|=64;
                            }
                            if((int)tx<(worldMap.width()-1) && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty+1)|| checkTerrain(terrains, worldMap, tx+1, ty+1, mountainTerrain))
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

                                mountainLayer->setCell(tx,ty,Tiled::Cell(mountainTsx->tileAt(mountainTile.at(indexTile).toInt())));
                            }
                        }

                        if(type == 0x1 || type==0x4) {
                            Tiled::Tile* colliTile = colliLayer->cellAt(tx, ty).tile;

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
                            for(LoadMap::Terrain* terrain:terrains){
                                if(LoadMap::searchTileLayerByName(worldMap, terrain->tmp_layerString)->cellAt(tx, ty).tile == terrain->tile){
                                    if(grassTiles.find(terrain->terrainName.toStdString()) != grassTiles.end()){
                                        grassLayer->setCell(tx, ty, grassTiles[terrain->terrainName.toStdString()]);
                                    }
                                    break;
                                }
                            }
                        }

                    }
                }

                // Ledges
                if(!isCity && setting.doledge){
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

                // Add bots
                if(!isCity){
                    // Resize
                    unsigned int* real_map = new unsigned int[mapWidth * mapHeight];

                    for(unsigned int i=0; i<mapWidth * mapHeight; i++){
                        real_map[i] = map[(i%mapWidth)/scale+(i/mapWidth/scale)*scaleWidth];
                    }

                    // Do the random bots
                    LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);
                    const char* directions[] = {"left", "right", "up", "bottom"};
                    std::string file = getMapFile(x, y);
                    std::string filename = file.substr(file.find_last_of('/')+1);

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
                                roadBot.id = botCount;
                                roadBot.look_at = rand()%4;
                                roadBot.skin = rand()%80; // read config for this value
                                roadIndex.roadBot.push_back(roadBot);

                                Tiled::MapObject *bot = new Tiled::MapObject("", "bot", QPointF(roadBot.x, roadBot.y+1), QSizeF(1, 1));
                                bot->setProperty("file", QString::fromStdString(filename+"-bots"));
                                bot->setProperty("id", QString::number(roadBot.id));
                                bot->setProperty("lookAt", directions[roadBot.look_at]);
                                bot->setProperty("skin", QString::number(roadBot.skin));
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
                        if(!checkTerrain(terrains, worldMap, tx, ty, mountainTerrain)){
                            uint8_t to_type_match=0;
                            if(tx>0 && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty-1) || checkTerrain(terrains, worldMap, tx-1, ty-1, mountainTerrain))
                                    to_type_match|=1;
                            }
                            if(ty>0)
                            {
                                if(checkEmptyRoad(setting, tx, ty-1)|| checkTerrain(terrains, worldMap, tx, ty-1, mountainTerrain))
                                    to_type_match|=2;
                            }
                            if((int)tx<(worldMap.width()-1) && ty>0)
                            {
                                if(checkEmptyRoad(setting, tx+1, ty-1)|| checkTerrain(terrains, worldMap, tx+1, ty-1, mountainTerrain))
                                    to_type_match|=4;
                            }
                            if(tx>0)
                            {
                                if(checkEmptyRoad(setting, tx-1, ty)|| checkTerrain(terrains, worldMap, tx-1, ty, mountainTerrain))
                                    to_type_match|=8;
                            }
                            if((int)tx<(worldMap.width()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty)|| checkTerrain(terrains, worldMap, tx+1, ty, mountainTerrain))
                                    to_type_match|=16;
                            }
                            if(tx>0 && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx-1, ty+1)|| checkTerrain(terrains, worldMap, tx-1, ty+1, mountainTerrain))
                                    to_type_match|=32;
                            }
                            if((int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx, ty+1)|| checkTerrain(terrains, worldMap, tx, ty+1, mountainTerrain))
                                    to_type_match|=64;
                            }
                            if((int)tx<(worldMap.width()-1) && (int)ty<(worldMap.height()-1))
                            {
                                if(checkEmptyRoad(setting, tx+1, ty+1)|| checkTerrain(terrains, worldMap, tx+1, ty+1, mountainTerrain))
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

                                mountainLayer->setCell(tx,ty,Tiled::Cell(mountainTsx->tileAt(mountainTile.at(indexTile).toInt())));
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

void LoadMapAll::writeRoadContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    unsigned int y=0;
    unsigned fightId = 0;

    QString fightDir = QCoreApplication::applicationDirPath()+"/dest/map/main/official/fight/";

    if(!QDir(fightDir).exists()){
        QDir().mkdir(fightDir);
    }

    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
            const bool isCity=haveCityEntry(citiesCoordToIndex,x,y);

            if(zoneOrientation != 0 && !isCity){
                std::string file = getMapFile(x, y);
                std::string filename = file.substr(file.find_last_of('/')+1); // parent folder + name
                std::string fightname = file.substr(file.find_last_of('/', file.find_last_of('/')-1)+1); // parent folder + name
                std::replace( fightname.begin(), fightname.end(), '/', '-');

                QString botxml = "<bots>\n";
                QString fightxml = "<fights>\n";
                const LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);

                for(RoadBot bot: roadIndex.roadBot){
                    if(bot.x >= mapWidth*x && bot.x < (x+1)*mapWidth
                            && bot.y >= mapHeight*y && bot.y < (y+1)*mapHeight
                            && !roadIndex.roadMonsters.empty()){
                        int monster = rand()%2 + rand()%3 +1; // Max: 4
                        int reward = roadIndex.level * 30 + 100;
                        fightId++;

                        botxml += "<bot id=\"" + QString::number(bot.id) + "\">\n";
                        botxml += " <name>" + QString::number(bot.id) + "</name>\n";
                        botxml += " <step fightid=\"" + QString::number(fightId) + "\" id=\"1\" type=\"fight\"/>\n";
                        botxml += "</bot>\n";

                        fightxml += "<fight id=\""+QString::number(fightId)+"\">\n";
                        fightxml += " <start><![CDATA[I lost to trainers before you, so I don't mind.]]></start>\n";

                        for(int i = 0; i<monster; i++){
                            int selected = rand() % roadIndex.roadMonsters.size();
                            int level = roadIndex.level * (95. + rand()%10) / 100.;
                            fightxml += " <monster id=\""+QString::number(roadIndex.roadMonsters.at(selected).monsterId)+"\" level=\""+QString::number(level)+"\"/>\n";
                            reward += level * level;
                        }
                        fightxml += " <gain cash=\""+QString::number(reward)+"\"/>\n";

                        fightxml += "</fight>\n";
                    }
                }

                botxml += "</bots>";
                fightxml += "</fights>";
                QFile botinfo(QString::fromStdString(file+"-bots.xml"));
                QFile fightinfo(fightDir+QString::fromStdString(fightname)+".xml");

                if(botinfo.open(QFile::WriteOnly))
                {
                    QByteArray contentData(botxml.toUtf8());
                    botinfo.write(contentData.constData(),contentData.size());
                    botinfo.close();
                }
                else
                {
                    std::cerr << "Unable to write bot file " << filename << std::endl;
                    abort();
                }

                if(fightinfo.open(QFile::WriteOnly))
                {
                    QByteArray contentData(fightxml.toUtf8());
                    fightinfo.write(contentData.constData(),contentData.size());
                    fightinfo.close();
                }
                else
                {
                    std::cerr << "Unable to write fight file " << fightname << std::endl;
                    abort();
                }
            }
            x++;
        }
        y++;
    }
}
