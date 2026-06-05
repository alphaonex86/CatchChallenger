#include "Decoder.hpp"

#include <algorithm>
#include <iostream>

DecodedMap::DecodedMap() :
    group(0),
    map(0),
    width(0),
    height(0),
    blocksPtr(0),
    primaryTileset(0),
    secondaryTileset(0),
    regionSection(0),
    mapType(0),
    cave(0),
    music(0)
{
}

std::string DecodedMap::relativePath() const
{
    std::string g=std::to_string(group);
    std::string m=std::to_string(map);
    if(g.size()<2)
        g="0"+g;
    if(m.size()<2)
        m="0"+m;
    return std::string("group-")+g+"/map-"+m;
}

Decoder::Decoder(const GbaRom &rom) :
    rom_(rom)
{
}

const std::vector<DecodedMap> &Decoder::maps() const
{
    return maps_;
}

const DecodedMap *Decoder::find(uint8_t group, uint8_t map) const
{
    uint16_t key=static_cast<uint16_t>((group<<8)|map);
    std::unordered_map<uint16_t,size_t>::const_iterator it=index_.find(key);
    if(it==index_.cend())
        return nullptr;
    return &maps_[it->second];
}

std::vector<uint32_t> Decoder::walkGroups(uint32_t offset) const
{
    // Each gMapGroups entry points to a group array; group[0] -> MapHeader[0] ->
    // MapLayout.  A real table has many such entries back to back.
    std::vector<uint32_t> groupPtr;
    uint32_t o=offset;
    bool ok=false;
    while(true)
    {
        uint32_t gp=rom_.pointer(o,&ok);
        if(!ok)
            break;
        uint32_t header0=rom_.pointer(gp,&ok);
        if(!ok)
            break;
        uint32_t layout0=rom_.pointer(header0,&ok);
        if(!ok || !looksLikeLayout(layout0))
            break;
        groupPtr.push_back(gp);
        o+=4;
        if(groupPtr.size()>256)
            break;
    }
    return groupPtr;
}

uint32_t Decoder::groupMapCount(uint32_t groupOffset) const
{
    uint32_t count=0;
    bool ok=false;
    while(true)
    {
        uint32_t header=rom_.pointer(groupOffset+count*4,&ok);
        if(!ok)
            break;
        uint32_t layout=rom_.pointer(header,&ok);
        if(!ok || !looksLikeLayout(layout))
            break;
        count++;
        if(count>512)
            break;
    }
    return count;
}

uint32_t Decoder::findMapGroups() const
{
    // Word-aligned sweep: at each offset try to walk a gMapGroups table; keep the
    // candidate (>=8 groups) with the most total maps.  Non-table offsets fail at
    // the first pointer check, so this stays cheap even on a 32MiB hack ROM.
    uint32_t best=0;
    uint64_t bestScore=0;
    uint32_t size=rom_.size();
    uint32_t o=0;
    while(o+4<=size)
    {
        std::vector<uint32_t> gp=walkGroups(o);
        if(gp.size()>=8)
        {
            uint64_t total=0;
            size_t i=0;
            while(i<gp.size())
            {
                total+=groupMapCount(gp[i]);
                i++;
            }
            if(total>bestScore)
            {
                bestScore=total;
                best=o;
            }
            o+=4*static_cast<uint32_t>(gp.size()); // skip past this table
        }
        else
            o+=4;
    }
    return best;
}

bool Decoder::looksLikeLayout(uint32_t layoutOffset) const
{
    if(layoutOffset==0 || layoutOffset+0x18>rom_.size())
        return false;
    int32_t w=rom_.s32(layoutOffset);
    int32_t h=rom_.s32(layoutOffset+4);
    if(w<=0 || w>1000 || h<=0 || h>1000)
        return false;
    bool ok=false;
    rom_.pointer(layoutOffset+0x08,&ok); if(!ok) return false; // border
    rom_.pointer(layoutOffset+0x0C,&ok); if(!ok) return false; // map blocks
    rom_.pointer(layoutOffset+0x10,&ok); if(!ok) return false; // primary tileset
    rom_.pointer(layoutOffset+0x14,&ok); if(!ok) return false; // secondary tileset
    return true;
}

bool Decoder::decodeAll()
{
    const GameInfo &game=rom_.game();
    bool ok=false;
    // gMapGroups: use the per-game offset; if it is 0 (a hack) or fails to
    // validate, scan the whole ROM for a relocated/expanded table.
    uint32_t groupsOffset=game.mapGroupsOffset;
    std::vector<uint32_t> groupPtr;
    if(groupsOffset!=0)
        groupPtr=walkGroups(groupsOffset);
    if(groupPtr.size()<8)
    {
        uint32_t scanned=findMapGroups();
        if(scanned!=0)
        {
            std::vector<uint32_t> alt=walkGroups(scanned);
            if(alt.size()>groupPtr.size())
            {
                groupsOffset=scanned;
                groupPtr=alt;
                std::cout << "Decoder: gMapGroups located by scan at 0x"
                          << std::hex << groupsOffset << std::dec << std::endl;
            }
        }
    }
    if(groupPtr.empty())
    {
        std::cerr << "Decoder: gMapGroups not recognised (per-game offset 0x"
                  << std::hex << game.mapGroupsOffset << std::dec << ")" << std::endl;
        return false;
    }
    // Per-group map count: group arrays are contiguous in ROM, so the count is
    // the gap to the next group start (sorted by address).  The group whose
    // array sits last in memory is read until a header stops validating.
    std::vector<uint32_t> sorted=groupPtr;
    std::sort(sorted.begin(),sorted.end());
    size_t gi=0;
    while(gi<groupPtr.size())
    {
        uint32_t start=groupPtr[gi];
        uint32_t next=0;
        size_t si=0;
        while(si<sorted.size())
        {
            if(sorted[si]>start && (next==0 || sorted[si]<next))
                next=sorted[si];
            si++;
        }
        uint32_t count;
        if(next!=0)
            count=(next-start)/4;
        else
            count=groupMapCount(start);
        uint32_t mi=0;
        while(mi<count)
        {
            uint32_t header=rom_.pointer(start+mi*4,&ok);
            if(ok)
            {
                DecodedMap dm=decodeMap(static_cast<uint8_t>(gi),static_cast<uint8_t>(mi),header);
                if(dm.width>0)
                {
                    index_[static_cast<uint16_t>((gi<<8)|mi)]=maps_.size();
                    maps_.push_back(dm);
                }
            }
            mi++;
        }
        gi++;
    }
    std::cout << "Decoder: " << groupPtr.size() << " groups, " << maps_.size()
              << " maps" << std::endl;
    return !maps_.empty();
}

DecodedMap Decoder::decodeMap(uint8_t group, uint8_t map, uint32_t headerOffset)
{
    DecodedMap dm;
    dm.group=group;
    dm.map=map;
    bool ok=false;
    uint32_t layout=rom_.pointer(headerOffset+0x00,&ok);
    if(!ok || !looksLikeLayout(layout))
        return dm;
    dm.width=rom_.s32(layout+0x00);
    dm.height=rom_.s32(layout+0x04);
    dm.blocksPtr=rom_.pointer(layout+0x0C,&ok);
    dm.primaryTileset=rom_.pointer(layout+0x10,&ok);
    bool ok2=false;
    dm.secondaryTileset=rom_.pointer(layout+0x14,&ok2);
    if(!ok2)
        dm.secondaryTileset=0;
    dm.music=rom_.u16(headerOffset+0x10);
    dm.regionSection=rom_.u8(headerOffset+0x14);
    dm.cave=rom_.u8(headerOffset+0x15);
    dm.mapType=rom_.u8(headerOffset+0x17);

    uint32_t events=rom_.pointer(headerOffset+0x04,&ok);
    if(ok)
        decodeEvents(events,dm);
    uint32_t connections=rom_.pointer(headerOffset+0x0C,&ok);
    if(ok)
        decodeConnections(connections,dm);
    return dm;
}

void Decoder::decodeEvents(uint32_t eventsOffset, DecodedMap &out)
{
    if(eventsOffset+0x14>rom_.size())
        return;
    uint8_t npcCount=rom_.u8(eventsOffset+0x00);
    uint8_t warpCount=rom_.u8(eventsOffset+0x01);
    // coordCount at +0x02 (triggers, not exported yet)
    uint8_t bgCount=rom_.u8(eventsOffset+0x03);
    bool ok=false;
    uint32_t npcPtr=rom_.pointer(eventsOffset+0x04,&ok);
    bool okWarp=false;
    uint32_t warpPtr=rom_.pointer(eventsOffset+0x08,&okWarp);
    bool okBg=false;
    uint32_t bgPtr=rom_.pointer(eventsOffset+0x10,&okBg);

    if(ok)
    {
        uint8_t i=0;
        while(i<npcCount)
        {
            uint32_t e=npcPtr+static_cast<uint32_t>(i)*0x18;
            if(e+0x18>rom_.size())
                break;
            DecodedNpc npc;
            npc.localId=rom_.u8(e+0x00);
            npc.graphicsId=rom_.u8(e+0x01);
            npc.x=rom_.s16(e+0x04);
            npc.y=rom_.s16(e+0x06);
            npc.movementType=rom_.u8(e+0x09);
            npc.trainerType=rom_.u16(e+0x0C);
            bool okScript=false;
            npc.scriptPtr=rom_.pointer(e+0x10,&okScript);
            if(!okScript)
                npc.scriptPtr=0;
            npc.flag=rom_.u16(e+0x14);
            out.npcs.push_back(npc);
            i++;
        }
    }
    if(okWarp)
    {
        uint8_t i=0;
        while(i<warpCount)
        {
            uint32_t e=warpPtr+static_cast<uint32_t>(i)*0x08;
            if(e+0x08>rom_.size())
                break;
            DecodedWarp warp;
            warp.x=static_cast<uint8_t>(rom_.s16(e+0x00));
            warp.y=static_cast<uint8_t>(rom_.s16(e+0x02));
            warp.destWarpId=rom_.u8(e+0x05);
            warp.destMap=rom_.u8(e+0x06);
            warp.destGroup=rom_.u8(e+0x07);
            out.warps.push_back(warp);
            i++;
        }
    }
    if(okBg)
    {
        uint8_t i=0;
        while(i<bgCount)
        {
            uint32_t e=bgPtr+static_cast<uint32_t>(i)*0x0C;
            if(e+0x0C>rom_.size())
                break;
            DecodedSign sign;
            sign.x=rom_.s16(e+0x00);
            sign.y=rom_.s16(e+0x02);
            sign.kind=rom_.u8(e+0x05);
            sign.scriptPtr=0;
            sign.itemId=0;
            // kind 0..4 = script signs (value at +0x08 is a script pointer);
            // kind 5..7 = hidden items (value at +0x08 packs item id).
            if(sign.kind<5)
            {
                bool okS=false;
                sign.scriptPtr=rom_.pointer(e+0x08,&okS);
                if(!okS)
                    sign.scriptPtr=0;
            }
            else
                sign.itemId=rom_.u16(e+0x08);
            out.signs.push_back(sign);
            i++;
        }
    }
}

void Decoder::decodeConnections(uint32_t connectionsOffset, DecodedMap &out)
{
    if(connectionsOffset+8>rom_.size())
        return;
    int32_t count=rom_.s32(connectionsOffset+0x00);
    if(count<=0 || count>64)
        return;
    bool ok=false;
    uint32_t list=rom_.pointer(connectionsOffset+0x04,&ok);
    if(!ok)
        return;
    int32_t i=0;
    while(i<count)
    {
        uint32_t e=list+static_cast<uint32_t>(i)*0x0C;
        if(e+0x0C>rom_.size())
            break;
        DecodedConnection conn;
        conn.direction=rom_.u8(e+0x00);
        conn.offset=rom_.s32(e+0x04);
        conn.destGroup=rom_.u8(e+0x08);
        conn.destMap=rom_.u8(e+0x09);
        out.connections.push_back(conn);
        i++;
    }
}

Terrain Decoder::terrain(uint16_t behavior) const
{
    // Grass is identical across both engines.
    if(behavior==0x02 || behavior==0x03)
        return Terrain::Grass;
    // Surfable water cluster (best-effort; same range used for both engines).
    if(behavior>=0x10 && behavior<=0x15)
        return Terrain::Water;
    // Ledge jumps occupy a 4-behaviour block; map each to a direction.
    if(behavior==0x38)
        return Terrain::LedgeRight; // jump east
    if(behavior==0x39)
        return Terrain::LedgeLeft;  // jump west
    if(behavior==0x3A)
        return Terrain::LedgeUp;    // jump north
    if(behavior==0x3B)
        return Terrain::LedgeDown;  // jump south
    return Terrain::Normal;
}

std::string Decoder::warpType(uint16_t behavior) const
{
    // The Gen3 warp behaviours all sit in the 0x60-0x6F block.  They split into
    // three player-facing kinds, which is exactly CatchChallenger's warp model:
    //
    //  * arrow / edge warps (you walk INTO the tile in the arrow's direction):
    //      0x62 east, 0x63 west, 0x64 north, 0x65 south arrow, 0x6D water-south
    //      arrow, 0x6E deep-south warp.  -> "teleport on push"
    //      (e.g. the south-arrow tile at the bottom of altering-cave).
    //  * a real door is one that ANIMATES when opened: behaviour 0x69
    //      (MB_ANIMATED_DOOR) only.  The non-animated door (0x60) and the cave/
    //      stair openings are NOT drawn as doors, so they must not be typed
    //      "door" (e.g. navel-rock's 0x60 cave warp is not a door).  -> "door"
    //  * every other warp (floor pad, ladder, stairs, hole, escalator, plain
    //      0x60/0x6C openings): activated by standing on it.  -> "teleport on it"
    if(behavior==0x62 || behavior==0x63 || behavior==0x64 || behavior==0x65 ||
       behavior==0x6D || behavior==0x6E)
        return std::string("teleport on push");
    if(behavior==0x69)
        return std::string("door");
    return std::string("teleport on it");
}

uint16_t Decoder::metatileBehavior(const DecodedMap &m, uint16_t metatile) const
{
    const GameInfo &gi=rom_.game();
    uint32_t attrField=(gi.engine==Engine::Frlg) ? 0x14 : 0x10;
    uint32_t metaInPrim=gi.metatilesInPrimary;
    bool ok=false;
    uint32_t base;
    uint32_t idx;
    if(metatile<metaInPrim)
    {
        base=rom_.pointer(m.primaryTileset+attrField,&ok);
        idx=metatile;
    }
    else
    {
        base=rom_.pointer(m.secondaryTileset+attrField,&ok);
        idx=metatile-metaInPrim;
    }
    if(!ok)
        return 0xFFFF;
    uint32_t off=base+idx*gi.attributeSize();
    uint32_t attr=(gi.attributeSize()==4) ? rom_.u32(off) : rom_.u16(off);
    return gi.behavior(attr);
}

std::string Decoder::warpClassAt(const DecodedMap &m, const DecodedWarp &w) const
{
    if(m.blocksPtr==0 || w.x>=m.width || w.y>=m.height)
        return std::string();
    uint16_t meta=static_cast<uint16_t>(rom_.u16(m.blocksPtr+(static_cast<uint32_t>(w.y)*static_cast<uint32_t>(m.width)+w.x)*2) & 0x3FF);
    return warpType(metatileBehavior(m,meta));
}
