#ifdef CATCHCHALLENGER_TESTING
// Test stubs must precede the production headers.
#include "../../../test/testingmapmanagement/Stubs.hpp"
#endif
#include "MapVisibilityAlgorithm.hpp"
#include <cstring>
#include "ClientWithMap.hpp"
#include "../GlobalServerData.hpp"
#include "../Client.hpp"
#include "../ClientList.hpp"

#include <iostream>

// Wire slots are map-local (cpu/balanced) or recipient-local (network); 255 is reserved.
// Packets: 0x6C self slot, 0x65 clear, 0x6B insert, 0x66 move, 0x69 remove, 0xE3 ping.
using namespace CatchChallenger;

// Scalar DensePlayerState comparisons avoid a separate snapshot prescan.
char MapVisibilityAlgorithm::tempBigBufferForChanges[];
char MapVisibilityAlgorithm::tempBigBufferForRemove[];
uint8_t MapVisibilityAlgorithm::tempInsertSlots[255];
std::vector<MapVisibilityAlgorithm> MapVisibilityAlgorithm::flat_map_list;
DensePlayerState MapVisibilityAlgorithm::tempDenseBuffer[255];
PLAYER_INDEX_FOR_CONNECTED MapVisibilityAlgorithm::tempInsertPlayers[255];
uint8_t MapVisibilityAlgorithm::tempSeenSlot[255];
std::vector<uint8_t> MapVisibilityAlgorithm::tempSlotOfPlayer;
uint32_t MapVisibilityAlgorithm::visibilityTick=0;
// Fallback view extents; resolveViewRange() applies the datapack zoom at load.
uint8_t MapVisibilityAlgorithm::view_x=13;
uint8_t MapVisibilityAlgorithm::view_y=13;

MapVisibilityAlgorithm::MapVisibilityAlgorithm() :
    candidatesCount(0),
    // Must differ from the initial visibilityTick.
    candidatesTick(0xffffffff)
{
    // Seed the shared packet headers once.
    MapVisibilityAlgorithm::tempBigBufferForChanges[0x00]=0x66;
    MapVisibilityAlgorithm::tempBigBufferForRemove[0x00]=0x69;
}

MapVisibilityAlgorithm::~MapVisibilityAlgorithm()
{
}

// Translate a border map into this map's coordinates; sides: top, bottom, left, right.
static bool mapSideOffset(const MapVisibilityAlgorithm &map,const uint8_t &side,
                          CATCHCHALLENGER_TYPE_MAPID &otherIndex,int16_t &offset_x,int16_t &offset_y)
{
    switch(side)
    {
        case 0x00:
            otherIndex=map.border.top.mapIndex;
        break;
        case 0x01:
            otherIndex=map.border.bottom.mapIndex;
        break;
        case 0x02:
            otherIndex=map.border.left.mapIndex;
        break;
        default:
            otherIndex=map.border.right.mapIndex;
        break;
    }
    if(otherIndex==65535)
        return false;
    if(otherIndex>=MapVisibilityAlgorithm::flat_map_list.size())
    {
        std::cerr << "mapSideOffset(): border map index out of the map list: " << otherIndex << std::endl;
        return false;
    }
    const MapVisibilityAlgorithm &other=MapVisibilityAlgorithm::flat_map_list.at(otherIndex);
    switch(side)
    {
        case 0x00:
            offset_x=-static_cast<int16_t>(map.border.top.x_offset);
            offset_y=-static_cast<int16_t>(other.height);
        break;
        case 0x01:
            offset_x=-static_cast<int16_t>(map.border.bottom.x_offset);
            offset_y=static_cast<int16_t>(map.height);
        break;
        case 0x02:
            offset_x=-static_cast<int16_t>(other.width);
            offset_y=-static_cast<int16_t>(map.border.left.y_offset);
        break;
        default:
            offset_x=static_cast<int16_t>(map.width);
            offset_y=-static_cast<int16_t>(map.border.right.y_offset);
        break;
    }
    return true;
}

// Match the client's rectTouch(): only touching maps can be displayed.
static void addNeighbour(MapVisibilityAlgorithm &map,const CATCHCHALLENGER_TYPE_MAPID &selfIndex,
                         const CATCHCHALLENGER_TYPE_MAPID &otherIndex,const int16_t &offset_x,const int16_t &offset_y)
{
    if(otherIndex==selfIndex)
        return;
    unsigned int index=0;
    while(index<map.neighbours.size())
    {
        if(map.neighbours.at(index).mapIndex==otherIndex)
            return;// Keep the first path.
        index++;
    }
    const MapVisibilityAlgorithm &other=MapVisibilityAlgorithm::flat_map_list.at(otherIndex);
    // Include touching edges.
    if((offset_x+static_cast<int16_t>(other.width))<0 || static_cast<int16_t>(map.width)<offset_x)
        return;
    if((offset_y+static_cast<int16_t>(other.height))<0 || static_cast<int16_t>(map.height)<offset_y)
        return;
    MapVisibilityAlgorithm::NeighbourMap neighbour;
    neighbour.mapIndex=otherIndex;
    neighbour.offset_x=offset_x;
    neighbour.offset_y=offset_y;
    map.neighbours.push_back(neighbour);
}

// Resolve visible neighbours once at load.
void MapVisibilityAlgorithm::resolveNeighbours()
{
    unsigned int mapIndex=0;
    while(mapIndex<flat_map_list.size())
    {
        MapVisibilityAlgorithm &map=flat_map_list[mapIndex];
        map.neighbours.clear();
        uint8_t side=0;
        while(side<4)
        {
            CATCHCHALLENGER_TYPE_MAPID directIndex=65535;
            int16_t offset_x=0,offset_y=0;
            if(mapSideOffset(map,side,directIndex,offset_x,offset_y))
            {
                addNeighbour(map,static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex),directIndex,offset_x,offset_y);
                // The second hop reaches diagonals; addNeighbour() rejects non-touching maps.
                uint8_t farSide=0;
                while(farSide<4)
                {
                    CATCHCHALLENGER_TYPE_MAPID farIndex=65535;
                    int16_t farOffsetX=0,farOffsetY=0;
                    if(mapSideOffset(flat_map_list.at(directIndex),farSide,farIndex,farOffsetX,farOffsetY))
                        addNeighbour(map,static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex),farIndex,
                                     static_cast<int16_t>(offset_x+farOffsetX),
                                     static_cast<int16_t>(offset_y+farOffsetY));
                    farSide++;
                }
            }
            side++;
        }
        mapIndex++;
    }
}

// Match client scaling; cover portrait orientation and partially visible tiles.
void MapVisibilityAlgorithm::resolveViewRange(const uint8_t &datapackZoom)
{
    uint32_t zoom=datapackZoom;
    if(zoom<1)
        zoom=CATCHCHALLENGER_SERVER_MAP_VIEW_ZOOM_DEFAULT;
    uint32_t screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH;
    if(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT<screenMin)
        screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT;
    // ceil(screenMin * zoom / 512), clamped to at least 1.
    uint32_t factor=(screenMin*zoom+511)/512;
    if(factor<1)
        factor=1;
    const uint32_t tileScreen=CATCHCHALLENGER_SERVER_MAP_VIEW_TILE_PIXEL*factor;

    uint32_t tiles=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH+tileScreen-1)/tileScreen;
    const uint32_t tilesHeight=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT+tileScreen-1)/tileScreen;
    if(tilesHeight>tiles)
        tiles=tilesHeight;
    uint32_t resolved=tiles/2+1;
    // Use the same extent on both axes to cover portrait orientation.
    if(resolved>127)
        resolved=127;
    view_x=static_cast<uint8_t>(resolved);
    view_y=static_cast<uint8_t>(resolved);
}

// Interaction range includes visible border maps.
bool MapVisibilityAlgorithm::inViewRange(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,
                                         const CATCHCHALLENGER_TYPE_MAPID &otherMapIndex,const COORD_TYPE &otherX,const COORD_TYPE &otherY)
{
    if(mapIndex>=flat_map_list.size() || otherMapIndex>=flat_map_list.size())
        return false;
    int16_t offset_x=0;
    int16_t offset_y=0;
    if(otherMapIndex!=mapIndex)
    {
        const MapVisibilityAlgorithm &map=flat_map_list.at(mapIndex);
        unsigned int index=0;
        while(index<map.neighbours.size() && map.neighbours.at(index).mapIndex!=otherMapIndex)
            index++;
        if(index>=map.neighbours.size())
            return false;
        offset_x=map.neighbours.at(index).offset_x;
        offset_y=map.neighbours.at(index).offset_y;
    }
    int16_t dx=static_cast<int16_t>(static_cast<int16_t>(otherX)+offset_x-static_cast<int16_t>(x));
    if(dx<0)
        dx=-dx;
    int16_t dy=static_cast<int16_t>(static_cast<int16_t>(otherY)+offset_y-static_cast<int16_t>(y));
    if(dy<0)
        dy=-dy;
    return dx<=static_cast<int16_t>(view_x) && dy<=static_cast<int16_t>(view_y);
}

// Advance before broadcasting any map: neighbours share the same tick.
void MapVisibilityAlgorithm::beginTick()
{
    visibilityTick++;
}

// Snapshot once per tick; map membership stays fixed during the broadcast.
void MapVisibilityAlgorithm::refreshCandidates()
{
    if(candidatesTick==visibilityTick)
        return;
    candidatesTick=visibilityTick;
    const size_t slotCount=map_clients_id.size();
    // Preserve sparse slots; consumers skip empty entries.
    if(candidates.size()<slotCount)
        candidates.resize(slotCount);
    size_t slot=0;
    while(slot<slotCount)
    {
        CandidateState &entry=candidates[slot];
        const PLAYER_INDEX_FOR_CONNECTED playerIndex=map_clients_id[slot];
        entry.player=playerIndex;
        if(playerIndex!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(playerIndex))
            #endif
            {
                const Client &c=ClientList::list->at(playerIndex);
                entry.state.set(c.getX(),c.getY(),static_cast<uint8_t>(c.getLastDirection()),c.getPlayerId());
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
            {
                std::cerr << "MapVisibilityAlgorithm::refreshCandidates() ClientList::list.empty(): "
                          << playerIndex << std::endl;
                entry.player=PLAYER_INDEX_FOR_CONNECTED_MAX;
            }
            #endif
        }
        slot++;
    }
    candidatesCount=static_cast<uint16_t>(slotCount);
}
