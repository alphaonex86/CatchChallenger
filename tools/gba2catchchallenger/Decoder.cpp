#include "Decoder.hpp"

#include <algorithm>
#include <iostream>

DecodedMap::DecodedMap() :
    group(0),
    map(0),
    origMap(0),
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

uint16_t DecodedMap::blockAt(const GbaRom &rom, uint32_t x, uint32_t y) const
{
    const uint32_t i=y*static_cast<uint32_t>(width)+x;
    if(!blocksOverride.empty())
    {
        if(i<blocksOverride.size())
            return blocksOverride[i];
        return 0;
    }
    return rom.u16(blocksPtr+i*2);
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
        // group/map ids are u8 (Gen3 warps address them as u8): never more
        if(groupPtr.size()>=256)
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
        if(count>=256)
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
        if(count>256)
            count=256;
        uint32_t mi=0;
        while(mi<count)
        {
            uint32_t header=rom_.pointer(start+mi*4,&ok);
            if(ok)
            {
                DecodedMap dm=decodeMap(static_cast<uint8_t>(gi),static_cast<uint8_t>(mi),header);
                dm.origMap=dm.map;
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

void Decoder::finalizeMaps()
{
    resolveWarpDestinations();
    pruneConnections();
    splitOversizedMaps();
}

// Resolve every warp's (destMap,destWarpId) into absolute (destX,destY) while
// the warp lists are still the unsplit ROM ones (a split renumbers/moves them).
void Decoder::resolveWarpDestinations()
{
    size_t mi=0;
    while(mi<maps_.size())
    {
        DecodedMap &m=maps_[mi];
        size_t wi=0;
        while(wi<m.warps.size())
        {
            DecodedWarp &w=m.warps[wi];
            const DecodedMap *dest=find(w.destGroup,w.destMap);
            w.destX=0;
            w.destY=0;
            if(dest!=nullptr)
            {
                if(w.destWarpId<dest->warps.size())
                {
                    w.destX=dest->warps[w.destWarpId].x;
                    w.destY=dest->warps[w.destWarpId].y;
                }
                else if(!dest->warps.empty())
                {
                    w.destX=dest->warps[0].x;
                    w.destY=dest->warps[0].y;
                }
            }
            wi++;
        }
        mi++;
    }
}

// The shared edge length of a connection (how many tiles the two maps touch):
// this map covers [0,thisDim), the neighbour [O, O+neighDim) on the edge axis.
static int connectionOverlap(int thisDim, int neighDim, int O)
{
    int lo=(O>0)?O:0;
    int hi=(thisDim<neighDim+O)?thisDim:neighDim+O;
    return hi-lo;
}

static uint8_t oppositeDirection(uint8_t d)
{
    if(d==1) return 2;
    if(d==2) return 1;
    if(d==3) return 4;
    return 3;
}

// The CC engine has ONE border slot per map side: keep, per side, the
// connection with the largest shared edge, then drop the leftovers AND any
// one-way survivor (both ends must keep each other, else the engine logs an
// asymmetric-border warning and the walk-back is broken anyway).
void Decoder::pruneConnections()
{
    // (group<<8|map)<<3 | direction -> index of the kept connection
    std::unordered_map<uint32_t,uint32_t> kept;
    size_t mi=0;
    while(mi<maps_.size())
    {
        DecodedMap &m=maps_[mi];
        uint8_t d=1;
        while(d<=4)
        {
            int best=-1;
            int bestOv=-1;
            size_t ci=0;
            while(ci<m.connections.size())
            {
                const DecodedConnection &c=m.connections[ci];
                if(c.direction==d)
                {
                    const DecodedMap *dest=find(c.destGroup,c.destMap);
                    if(dest!=nullptr && dest->width>0)
                    {
                        const bool horizontal=(d==1 || d==2); // edge axis = x
                        const int thisDim=horizontal?static_cast<int>(m.width):static_cast<int>(m.height);
                        const int neighDim=horizontal?static_cast<int>(dest->width):static_cast<int>(dest->height);
                        const int ov=connectionOverlap(thisDim,neighDim,c.offset);
                        if(ov>bestOv)
                        {
                            bestOv=ov;
                            best=static_cast<int>(ci);
                        }
                    }
                }
                ci++;
            }
            if(best>=0)
                kept[(static_cast<uint32_t>((m.group<<8)|m.map)<<3)|d]=static_cast<uint32_t>(best);
            d++;
        }
        mi++;
    }
    // decide every verdict against the ORIGINAL lists first (filtering while
    // checking would make the stored indices point at the wrong slots), then
    // apply the filtering in a second sweep
    std::vector<std::vector<bool> > keepFlag;
    keepFlag.resize(maps_.size());
    int dropped=0;
    mi=0;
    while(mi<maps_.size())
    {
        const DecodedMap &m=maps_[mi];
        keepFlag[mi].resize(m.connections.size(),true);
        size_t ci=0;
        while(ci<m.connections.size())
        {
            const DecodedConnection &c=m.connections[ci];
            if(c.direction>=1 && c.direction<=4)
            {
                bool keep=false;
                std::unordered_map<uint32_t,uint32_t>::const_iterator me=
                    kept.find((static_cast<uint32_t>((m.group<<8)|m.map)<<3)|c.direction);
                if(me!=kept.cend() && me->second==ci)
                {
                    // reciprocity: the neighbour must keep its way back to us
                    std::unordered_map<uint32_t,uint32_t>::const_iterator back=
                        kept.find((static_cast<uint32_t>((c.destGroup<<8)|c.destMap)<<3)|oppositeDirection(c.direction));
                    if(back!=kept.cend())
                    {
                        const DecodedMap *dest=find(c.destGroup,c.destMap);
                        if(dest!=nullptr && back->second<dest->connections.size())
                        {
                            const DecodedConnection &r=dest->connections[back->second];
                            if(r.destGroup==m.group && r.destMap==m.map)
                                keep=true;
                        }
                    }
                }
                keepFlag[mi][ci]=keep;
                if(!keep)
                    dropped++;
            }
            ci++;
        }
        mi++;
    }
    mi=0;
    while(mi<maps_.size())
    {
        DecodedMap &m=maps_[mi];
        std::vector<DecodedConnection> survivors;
        size_t ci=0;
        while(ci<m.connections.size())
        {
            if(keepFlag[mi][ci])
                survivors.push_back(m.connections[ci]);
            ci++;
        }
        m.connections=survivors;
        mi++;
    }
    if(dropped>0)
        std::cout << "Decoder: " << dropped << " connection(s) dropped (one border per side + reciprocity)" << std::endl;
}

// CC map coordinates are 0..127 (COORD_TYPE): split every larger map into
// chunks along the oversized axis.  Chunk 0 keeps the (group,map) identity;
// the others get free map ids in the same group.  Per round: plan the chunk
// ids, then retarget every existing warp/connection at the right chunk, then
// build the chunks from the (already retargeted) big maps — the synthetic
// inter-chunk links use final ids and are never retargeted.
void Decoder::splitOversizedMaps()
{
    // Loop: a map oversized on BOTH axes is split once per round.
    while(true)
    {
        struct Chunk { uint8_t id; int start; int len; };
        struct Split { bool vertical; std::vector<Chunk> chunks; };
        std::unordered_map<uint16_t,Split> splits; // original (group<<8|map) -> plan
        std::unordered_map<uint16_t,int> reserved; // ids claimed this round
        // planned per-chunk sizes, keyed (group<<8|chunkId): phase C needs a
        // same-round-split neighbour's CHUNK length, but find() there returns
        // either nullptr (chunk not built yet) or the still-unsplit original
        // depending on maps_ order — consult the plan first
        std::unordered_map<uint16_t,int> plannedW;
        std::unordered_map<uint16_t,int> plannedH;

        // phase A: plan every oversized map's chunks and claim free ids
        size_t mi=0;
        while(mi<maps_.size())
        {
            const DecodedMap &m=maps_[mi];
            mi++;
            const bool vertical=(m.height>127);
            if(!vertical && m.width<=127)
                continue;
            const int len=vertical?static_cast<int>(m.height):static_cast<int>(m.width);
            const int n=(len+126)/127;
            const int chunkLen=(len+n-1)/n;
            Split plan;
            plan.vertical=vertical;
            int c=0;
            int nextFree=0;
            bool idsOk=true;
            while(c<n)
            {
                Chunk ch;
                ch.start=c*chunkLen;
                ch.len=(c==n-1)?(len-ch.start):chunkLen;
                if(c==0)
                    ch.id=m.map;
                else
                {
                    while(nextFree<256 &&
                          (index_.find(static_cast<uint16_t>((m.group<<8)|nextFree))!=index_.cend() ||
                           reserved.find(static_cast<uint16_t>((m.group<<8)|nextFree))!=reserved.cend()))
                        nextFree++;
                    if(nextFree>255)
                    {
                        idsOk=false;
                        break;
                    }
                    ch.id=static_cast<uint8_t>(nextFree);
                    reserved[static_cast<uint16_t>((m.group<<8)|nextFree)]=1;
                    nextFree++;
                }
                plan.chunks.push_back(ch);
                c++;
            }
            if(!idsOk)
            {
                std::cerr << "Decoder: no free map id in group " << static_cast<int>(m.group)
                          << " to split oversized map " << static_cast<int>(m.map) << std::endl;
                continue;
            }
            std::cout << "Decoder: split map group " << static_cast<int>(m.group)
                      << " map " << static_cast<int>(m.map) << " (" << m.width << "x" << m.height
                      << ") into " << n << (vertical?" vertical":" horizontal") << " chunk(s)" << std::endl;
            splits[static_cast<uint16_t>((m.group<<8)|m.map)]=plan;
            size_t pc=0;
            while(pc<plan.chunks.size())
            {
                const Chunk &ch=plan.chunks[pc];
                const uint16_t key=static_cast<uint16_t>((m.group<<8)|ch.id);
                plannedW[key]=vertical?static_cast<int>(m.width):ch.len;
                plannedH[key]=vertical?ch.len:static_cast<int>(m.height);
                pc++;
            }
        }
        if(splits.empty())
            break;

        // phase B: retarget every warp/connection that points into a split map
        // (the big maps themselves included — their lists are copied into the
        // chunks afterwards)
        mi=0;
        while(mi<maps_.size())
        {
            DecodedMap &m=maps_[mi];
            mi++;
            size_t wi=0;
            while(wi<m.warps.size())
            {
                DecodedWarp &w=m.warps[wi];
                wi++;
                std::unordered_map<uint16_t,Split>::const_iterator sp=
                    splits.find(static_cast<uint16_t>((w.destGroup<<8)|w.destMap));
                if(sp==splits.cend())
                    continue;
                const int pos=sp->second.vertical?w.destY:w.destX;
                size_t c=0;
                while(c+1<sp->second.chunks.size() && pos>=sp->second.chunks[c].start+sp->second.chunks[c].len)
                    c++;
                w.destMap=sp->second.chunks[c].id;
                if(sp->second.vertical)
                    w.destY=static_cast<uint8_t>(pos-sp->second.chunks[c].start);
                else
                    w.destX=static_cast<uint8_t>(pos-sp->second.chunks[c].start);
            }
            size_t ci=0;
            while(ci<m.connections.size())
            {
                DecodedConnection &conn=m.connections[ci];
                ci++;
                if(conn.direction<1 || conn.direction>4)
                    continue;
                std::unordered_map<uint16_t,Split>::const_iterator sp=
                    splits.find(static_cast<uint16_t>((conn.destGroup<<8)|conn.destMap));
                if(sp==splits.cend())
                    continue;
                const Split &plan=sp->second;
                const bool destEdgeAlongSplit=plan.vertical?(conn.direction==1 || conn.direction==2)
                                                           :(conn.direction==3 || conn.direction==4);
                if(destEdgeAlongSplit)
                {
                    // we touch the split map's first edge when it is BELOW/RIGHT
                    // of us (direction 1/4), i.e. we see its top/left chunk
                    const bool firstChunk=(conn.direction==1 || conn.direction==4);
                    conn.destMap=firstChunk?plan.chunks.front().id:plan.chunks.back().id;
                    // offset axis is the unsplit one: unchanged
                }
                else
                {
                    // we run alongside the split map: pick the chunk we overlap.
                    // conn.offset O = the split map\'s origin in OUR frame, so we
                    // span [ -O, -O+ourLen ) in ITS frame.
                    const int ourLen=plan.vertical?static_cast<int>(m.height):static_cast<int>(m.width);
                    const int total=plan.chunks.back().start+plan.chunks.back().len;
                    int mid=-conn.offset+ourLen/2;
                    if(mid<0) mid=0;
                    if(mid>=total) mid=total-1;
                    size_t c=0;
                    while(c+1<plan.chunks.size() && mid>=plan.chunks[c].start+plan.chunks[c].len)
                        c++;
                    conn.destMap=plan.chunks[c].id;
                    conn.offset=conn.offset+plan.chunks[c].start;
                }
            }
        }

        // phase C: build the chunks from the retargeted big maps
        mi=0;
        while(mi<maps_.size())
        {
            std::unordered_map<uint16_t,Split>::const_iterator sp=
                splits.find(static_cast<uint16_t>((maps_[mi].group<<8)|maps_[mi].map));
            if(sp==splits.cend())
            {
                mi++;
                continue;
            }
            const DecodedMap m=maps_[mi]; // copy: the slot is overwritten with chunk 0
            const Split &plan=sp->second;
            const bool vertical=plan.vertical;
            const int len=vertical?static_cast<int>(m.height):static_cast<int>(m.width);
            int c=0;
            while(c<static_cast<int>(plan.chunks.size()))
            {
                const Chunk &ch=plan.chunks[c];
                DecodedMap cm;
                cm.group=m.group;
                cm.map=ch.id;
                cm.origMap=m.origMap;
                cm.width=vertical?m.width:ch.len;
                cm.height=vertical?ch.len:m.height;
                cm.blocksPtr=0;
                cm.primaryTileset=m.primaryTileset;
                cm.secondaryTileset=m.secondaryTileset;
                cm.regionSection=m.regionSection;
                cm.mapType=m.mapType;
                cm.cave=m.cave;
                cm.music=m.music;
                const int ox=vertical?0:ch.start;
                const int oy=vertical?ch.start:0;
                cm.blocksOverride.resize(static_cast<size_t>(cm.width)*static_cast<size_t>(cm.height));
                int y=0;
                while(y<cm.height)
                {
                    int x=0;
                    while(x<cm.width)
                    {
                        cm.blocksOverride[static_cast<size_t>(y)*cm.width+x]=
                            m.blockAt(rom_,static_cast<uint32_t>(x+ox),static_cast<uint32_t>(y+oy));
                        x++;
                    }
                    y++;
                }
                // warps physically inside the chunk, source-shifted (their
                // destinations were already retargeted in phase B)
                size_t wi=0;
                while(wi<m.warps.size())
                {
                    const DecodedWarp &w=m.warps[wi];
                    if(w.x>=ox && w.x<ox+cm.width && w.y>=oy && w.y<oy+cm.height)
                    {
                        DecodedWarp cw=w;
                        cw.x=static_cast<uint8_t>(w.x-ox);
                        cw.y=static_cast<uint8_t>(w.y-oy);
                        cm.warps.push_back(cw);
                    }
                    wi++;
                }
                size_t ni=0;
                while(ni<m.npcs.size())
                {
                    const DecodedNpc &np=m.npcs[ni];
                    if(np.x>=ox && np.x<ox+cm.width && np.y>=oy && np.y<oy+cm.height)
                    {
                        DecodedNpc cn=np;
                        cn.x=static_cast<int16_t>(np.x-ox);
                        cn.y=static_cast<int16_t>(np.y-oy);
                        cm.npcs.push_back(cn);
                    }
                    ni++;
                }
                size_t si=0;
                while(si<m.signs.size())
                {
                    const DecodedSign &sg=m.signs[si];
                    if(sg.x>=ox && sg.x<ox+cm.width && sg.y>=oy && sg.y<oy+cm.height)
                    {
                        DecodedSign cs=sg;
                        cs.x=static_cast<int16_t>(sg.x-ox);
                        cs.y=static_cast<int16_t>(sg.y-oy);
                        cm.signs.push_back(cs);
                    }
                    si++;
                }
                // original connections: the edge ones along the split axis go to
                // the first/last chunk; the perpendicular ones go to the chunk
                // overlapping the neighbour (offset re-based to the chunk)
                size_t ci=0;
                while(ci<m.connections.size())
                {
                    const DecodedConnection &conn=m.connections[ci];
                    ci++;
                    if(conn.direction<1 || conn.direction>4)
                    {
                        if(c==0)
                            cm.connections.push_back(conn);
                        continue;
                    }
                    const bool alongSplit=vertical?(conn.direction==1 || conn.direction==2)
                                                  :(conn.direction==3 || conn.direction==4);
                    if(alongSplit)
                    {
                        // top/left edge -> chunk 0, bottom/right edge -> last
                        const bool firstEdge=(conn.direction==2 || conn.direction==3);
                        if((firstEdge && c==0) || (!firstEdge && c==static_cast<int>(plan.chunks.size())-1))
                            cm.connections.push_back(conn); // offset axis is unsplit
                    }
                    else
                    {
                        int neighLen=1;
                        const std::unordered_map<uint16_t,int> &planned=vertical?plannedH:plannedW;
                        std::unordered_map<uint16_t,int>::const_iterator pl=
                            planned.find(static_cast<uint16_t>((conn.destGroup<<8)|conn.destMap));
                        if(pl!=planned.cend())
                            neighLen=pl->second;
                        else
                        {
                            const DecodedMap *dest=find(conn.destGroup,conn.destMap);
                            if(dest!=nullptr)
                                neighLen=vertical?static_cast<int>(dest->height):static_cast<int>(dest->width);
                        }
                        int mid=conn.offset+neighLen/2;
                        if(mid<0) mid=0;
                        if(mid>=len) mid=len-1;
                        if(mid>=ch.start && mid<ch.start+ch.len)
                        {
                            DecodedConnection cc=conn;
                            cc.offset=conn.offset-ch.start;
                            cm.connections.push_back(cc);
                        }
                    }
                }
                // synthetic links between consecutive chunks (final ids, built
                // after phase B so they are never retargeted)
                if(c>0)
                {
                    DecodedConnection up;
                    up.direction=vertical?2:3; // towards the previous chunk
                    up.offset=0;
                    up.destGroup=m.group;
                    up.destMap=plan.chunks[c-1].id;
                    cm.connections.push_back(up);
                }
                if(c<static_cast<int>(plan.chunks.size())-1)
                {
                    DecodedConnection down;
                    down.direction=vertical?1:4; // towards the next chunk
                    down.offset=0;
                    down.destGroup=m.group;
                    down.destMap=plan.chunks[c+1].id;
                    cm.connections.push_back(down);
                }
                if(c==0)
                    maps_[mi]=cm; // keeps its index_ slot
                else
                {
                    index_[static_cast<uint16_t>((cm.group<<8)|cm.map)]=maps_.size();
                    maps_.push_back(cm);
                }
                c++;
            }
            mi++;
        }
    }
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
    return gi.behavior(gi.attributeAt(rom_,base+idx*gi.attributeSize()));
}

std::string Decoder::warpClassAt(const DecodedMap &m, const DecodedWarp &w) const
{
    if((m.blocksPtr==0 && m.blocksOverride.empty()) || w.x>=m.width || w.y>=m.height)
        return std::string();
    uint16_t meta=static_cast<uint16_t>(m.blockAt(rom_,w.x,w.y) & 0x3FF);
    return warpType(metatileBehavior(m,meta));
}
