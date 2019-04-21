#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"

#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_tile.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapwriter.h"

#include <iostream>
#include <algorithm>
#include <queue>
#include <unordered_map>

#include <QDir>
#include <QCoreApplication>
#include <QSettings>

unsigned int ** LoadMapAll::roadData = NULL;
int LoadMapAll::botId = 0;
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
                    // City building
                    QList<MapBrush::MapTemplate> templates = QList<MapBrush::MapTemplate>();

                    templates.push_back(mapTemplatebuildingheal);

                    if(rand()%2)
                        templates.push_back(mapTemplatebuildingshop);

                    int building = rand()%4 + 2;

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
                    }

                    std::sort(zones.begin(), zones.end(), ZoneSorter(scaleWidth, scaleHeight));
                    City *city = NULL;

                    for(City &c: cities){
                        if(c.x == x && c.y == y)
                            city = &c;
                    }

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

                        for(MapBrush::MapTemplate &temp: templates){
                            int i;
                            int limit = 500;
                            for(i=0; i<limit; i++){
                                double index = rand() / (double) RAND_MAX;
                                ZoneMarker* zone = zones[(1-index*index)*zones.size()];

                                if(zone->type == 1){
                                    bool valid= true;
                                    std::pair<uint8_t,uint8_t> pos(zone->x*scale + rand()%(zone->w*scale ), zone->y*scale+ rand()%(zone->h*scale ));
                                    //std::pair<uint8_t,uint8_t> pos(rand() % setting.mapWidth, rand() % setting.mapHeight);
                                    unsigned int bx = pos.first/scale;
                                    unsigned int by = pos.second/scale;

                                    if(bx+ceil((double)temp.width/scale) < scaleWidth
                                            && by+ceil((double)temp.height/scale) < scaleHeight){
                                        for(unsigned int tx = bx; tx<bx+ceil((double)temp.width/scale); tx++){
                                            for(unsigned int ty = by; ty<by+ceil((double)temp.height/scale); ty++){
                                                if(map[tx+ty*scaleWidth]!= 1){
                                                    valid =false;
                                                }
                                            }
                                        }
                                    }else{
                                        valid = false;
                                    }

                                    if(valid){
                                        for(unsigned int tx = bx; tx<bx+ceil((double)temp.width/scale); tx++){
                                            for(unsigned int ty = by; ty<by+ceil((double)temp.height/scale); ty++){
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

                                        if(valid){
                                            if(temp.name != mapTemplatebuildingheal.name && temp.name != mapTemplatebuildingshop.name){
                                                generateRoom(worldMap, temp, buildingId++, x, y, pos, *city, cityLowerCaseName, setting, rs);
                                            }else{
                                                addBuildingChain(temp.name,temp.name, temp, worldMap, x, y, mapWidth, mapHeight, pos, *city, cityLowerCaseName);
                                            }
                                            zone->type = 5;
                                            for(unsigned int tx = bx; tx<bx+ceil((double)temp.width/scale); tx++){
                                                for(unsigned int ty = by; ty<by+ceil((double)temp.height/scale); ty++){
                                                    map[tx+ty*scaleWidth] = 5;
                                                }
                                            }
                                            break;
                                        }

                                        zone->type = 1;

                                        for(unsigned int tx = bx; tx<bx+ceil((double)temp.width/scale); tx++){
                                            for(unsigned int ty = by; ty<by+ceil((double)temp.height/scale); ty++){
                                                map[tx+ty*scaleWidth] = 1;
                                            }
                                        }
                                    }
                                }
                            }

                            if(i >= limit && temp.name == mapTemplatebuildingheal.name){
                                Tiled::ObjectGroup* moving = LoadMap::searchObjectGroupByName(worldMap, "Moving");
                                Tiled::MapObject* rescue = new Tiled::MapObject("", "rescue", QPointF((x+.5)*mapWidth, (y+.5)*mapHeight), QSizeF(1,1));
                                rescue->setCell(Tiled::Cell(invis->tileAt(1)));
                                moving->addObject(rescue);
                            }
                        }
                    }
                }

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

                if(setting.displayregion){
                    for(const ZoneMarker *marker: zones){
                        Tiled::MapObject *region = new Tiled::MapObject("", "region", QPointF(marker->x*2+mapWidth*x, marker->y*2+mapHeight*y), QSizeF(marker->w*2, marker->h*2));
                        region->setProperty("id", QString::number(marker->id));
                        region->setProperty("from", QString::number(marker->from));
                        region->setProperty("type", QString::number(marker->type));
                        region->setProperty("neightbourg", QString::number(marker->neightbourg));
                        objectLayer->addObject(region);
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
            Tiled::Tile* tile = fetchTile(worldMap, grassData[1]);
            grassTiles[grassData.at(0).toStdString()] = Tiled::Cell(tile);
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
                    walkway.push_back(QPair<Tiled::Tile*, Tiled::Tile*>(fetchTile(worldMap, tileData[0]), fetchTile(worldMap, tileData[1])));
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
                    const char* directions[] = {"left", "right", "top", "bottom"};
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
                                roadBot.id = botId++;
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
    int fightId = 0;

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
    unsigned int floorCount = 1 + ((double)rand()/RAND_MAX)*1.337;
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

                    properties["x"]=QString::number(door->x()+pos.first);
                    properties["y"]=QString::number(door->y()+pos.second);

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
                    content+=" type=\""+properties.value("type")+"\"";
                if(!zone.empty())
                    content+=" zone=\""+QString::fromStdString(zone)+"\"";
                content+=">\n"
                         "  <name>floor-"+QString::number(index)+"</name>\n"
                                                                        "</map>";
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

    // Write bots files
    for(Tiled::Map* room: otherMap){
        Tiled::ObjectGroup* npcs = LoadMap::searchObjectGroupByName(*room, "Object");

        if(npcs->objectCount() > 0){
            const int i = room->property("floor-id").toInt();

            QString botpath;

            if(floorCount > 1){
                botpath = name+"-bot";
            }else{
                botpath = "floor-"+QString::number(i)+"-bot";
            }


            std::string filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+name.toStdString()+"-bot.xml";

            if(floorCount > 1){
                filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+name.toStdString()+"/floor-"+std::to_string(i)+"-bot.xml";
            }

            QFile xmlinfo(QCoreApplication::applicationDirPath()+QString::fromStdString(filePath));
            QString content("<bots>");

            for(Tiled::MapObject* npc: npcs->objects()){

                if(npc->type() == "bot"){
                    Tiled::Properties properties = npc->properties();

                    properties["file"] = botpath;

                    content += "\n\t<bot id=\""+properties["id"]+"\">";
                    content += "\n\t\t<name>"+properties["id"]+"</name>";
                    content += "\n\t\t<step id=\"1\" type=\"text\"><![CDATA[";
                    content += QString::fromStdString(setting.npcMessage.at(rand()%setting.npcMessage.size()));
                    content += "]]></step>";
                    content += "\n\t</bot>";
                }
            }

            if(xmlinfo.open(QFile::WriteOnly))
            {
                content+= "\n</bots>";
                QByteArray contentData(content.toUtf8());
                xmlinfo.write(contentData.constData(),contentData.size());
                xmlinfo.close();
            }
            else
            {
                std::cerr << "Unable to write " << filePath << std::endl;
                abort();
            }
        }

        delete room;
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
    roomMap.addLayer(new Tiled::ObjectGroup("Moving", 0, 0, roomMap.width(), roomMap.height()));
    roomMap.addLayer(new Tiled::ObjectGroup("Object", 0, 0, roomMap.width(), roomMap.height()));
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
        std::random_shuffle ( room.limitations.begin(), room.limitations.end() );
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

    for(Tiled::Tileset* t: roomMap.tilesets()){
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
                        properties["skin"] = QString::number(rand()%100);
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
