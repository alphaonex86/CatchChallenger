#include "CCWriter.hpp"
#include "Gen3Text.hpp"
#include "OverworldSprite.hpp"

#include <QByteArray>
#include <QDir>
#include <QImage>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <zstd.h>

CCWriter::CCWriter(const GbaRom &rom,
                   const Decoder &decoder,
                   const TilesetBuilder &tilesets,
                   const Naming &naming,
                   const Wild &wild,
                   const std::string &fireredDir,
                   SkinResolver &skins) :
    rom_(rom),
    decoder_(decoder),
    tilesets_(tilesets),
    naming_(naming),
    wild_(wild),
    script_(rom),
    fireredDir_(fireredDir),
    skins_(skins)
{
}

std::string CCWriter::relPath(const std::string &fromPath, const std::string &toPath)
{
    // Split into components.
    std::vector<std::string> f,t;
    std::string cur;
    size_t i=0;
    while(i<=fromPath.size())
    {
        if(i==fromPath.size() || fromPath[i]=='/')
        {
            if(!cur.empty()) f.push_back(cur);
            cur.clear();
        }
        else
            cur.push_back(fromPath[i]);
        i++;
    }
    cur.clear();
    i=0;
    while(i<=toPath.size())
    {
        if(i==toPath.size() || toPath[i]=='/')
        {
            if(!cur.empty()) t.push_back(cur);
            cur.clear();
        }
        else
            cur.push_back(toPath[i]);
        i++;
    }
    // from's directory = all but the last component.
    if(!f.empty())
        f.pop_back();
    // common prefix.
    size_t p=0;
    while(p<f.size() && p<t.size() && f[p]==t[p])
        p++;
    std::string out;
    size_t up=p;
    while(up<f.size())
    {
        out+="../";
        up++;
    }
    size_t k=p;
    while(k<t.size())
    {
        out+=t[k];
        if(k+1<t.size())
            out+="/";
        k++;
    }
    return out;
}

std::string CCWriter::encodeLayer(const std::vector<uint32_t> &gids) const
{
    // Serialise as little-endian u32 (the Tiled on-disk layout).
    std::vector<uint8_t> raw;
    raw.resize(gids.size()*4);
    size_t i=0;
    while(i<gids.size())
    {
        uint32_t g=gids[i];
        raw[i*4+0]=static_cast<uint8_t>(g & 0xFF);
        raw[i*4+1]=static_cast<uint8_t>((g>>8) & 0xFF);
        raw[i*4+2]=static_cast<uint8_t>((g>>16) & 0xFF);
        raw[i*4+3]=static_cast<uint8_t>((g>>24) & 0xFF);
        i++;
    }
    size_t bound=ZSTD_compressBound(raw.size());
    std::vector<uint8_t> comp;
    comp.resize(bound);
    size_t got=ZSTD_compress(comp.data(),bound,raw.data(),raw.size(),22);
    if(ZSTD_isError(got))
    {
        std::cerr << "zstd compress failed: " << ZSTD_getErrorName(got) << std::endl;
        return std::string();
    }
    QByteArray ba(reinterpret_cast<const char *>(comp.data()),static_cast<int>(got));
    QByteArray b64=ba.toBase64();
    return std::string(b64.constData(),static_cast<size_t>(b64.size()));
}

std::string CCWriter::skinFor(uint8_t graphicsId)
{
    std::unordered_map<uint8_t,std::string>::const_iterator it=skinCache_.find(graphicsId);
    if(it!=skinCache_.cend())
        return it->second;
    // Extract the NPC's overworld sprite (only real 16x32 characters pass the
    // filters in OverworldSprite::render) and reuse-or-add via the resolver.
    // A null sprite means "not a character" -> empty, and the caller drops the
    // bot rather than placing a nonsense one.
    QImage sprite=OverworldSprite::render(rom_,graphicsId);
    std::string name;
    if(!sprite.isNull())
        name=skins_.resolve(sprite);
    skinCache_[graphicsId]=name;
    return name;
}

std::vector<CCWriter::MapBot> CCWriter::collectBots(const DecodedMap &map)
{
    std::vector<MapBot> bots;
    std::unordered_set<int> usedId;
    uint32_t w=static_cast<uint32_t>(map.width);
    uint32_t h=static_cast<uint32_t>(map.height);
    // Skinned NPCs keep their Gen3 local id.  A null skin means "not a real
    // character" (item/decoration object-event) and is dropped.
    size_t ni=0;
    while(ni<map.npcs.size())
    {
        const DecodedNpc &npc=map.npcs[ni];
        std::string skin=skinFor(npc.graphicsId);
        if(!skin.empty() && npc.x>=0 && npc.y>=0 &&
           static_cast<uint32_t>(npc.x)<w && static_cast<uint32_t>(npc.y)<h)
        {
            int id=npc.localId;
            while(usedId.find(id)!=usedId.cend())
                id++;
            usedId.insert(id);
            MapBot b;
            b.id=id;
            b.x=npc.x;
            b.y=npc.y;
            b.skin=skin;
            b.trainerType=npc.trainerType;
            b.scriptPtr=npc.scriptPtr;
            b.isSign=false;
            bots.push_back(b);
        }
        ni++;
    }
    // Script signs (the "press A" text panels, bg-event kind 0..4) become
    // skinless text bots, defined in the .xml so they are not dangling.
    size_t si=0;
    while(si<map.signs.size())
    {
        const DecodedSign &sign=map.signs[si];
        if(sign.kind<5 && sign.scriptPtr!=0 && sign.x>=0 && sign.y>=0 &&
           static_cast<uint32_t>(sign.x)<w && static_cast<uint32_t>(sign.y)<h)
        {
            int id=1;
            while(usedId.find(id)!=usedId.cend())
                id++;
            usedId.insert(id);
            MapBot b;
            b.id=id;
            b.x=sign.x;
            b.y=sign.y;
            b.skin.clear();
            b.trainerType=0;
            b.scriptPtr=sign.scriptPtr;
            b.isSign=true;
            bots.push_back(b);
        }
        si++;
    }
    return bots;
}

bool CCWriter::writeAll()
{
    const std::vector<DecodedMap> &maps=decoder_.maps();
    size_t i=0;
    while(i<maps.size())
    {
        writeMap(maps[i]);
        writeMapXml(maps[i]);
        i++;
    }
    writeInformations();
    writeStart();
    writeZones();
    std::cout << "CCWriter: wrote " << maps.size() << " maps to " << fireredDir_ << std::endl;
    return true;
}
void CCWriter::writeMap(const DecodedMap &map)
{
    uint32_t w=static_cast<uint32_t>(map.width);
    uint32_t h=static_cast<uint32_t>(map.height);
    uint32_t cells=w*h;
    // The semantic tile LAYERS (Collisions/Grass/Water/Ledges) re-use the REAL
    // map tile at each cell (the engine only needs a non-empty gid), so the
    // layer is visible AND the editor render matches the game.
    // The Moving/Object OBJECTS instead use the shared map/invisible.tsx markers
    // (objects are not tile layers, so a transparent marker is fine): tile 0
    // bot, 1 rescue, 2 teleport, 3 border-offset.  Doors reference their own
    // animated door tile.
    uint32_t invFirst=tilesets_.markerGid(map);
    if(invFirst==0)
    {
        std::cerr << "No tileset for map " << naming_.pathFor(map.group,map.map) << std::endl;
        return;
    }
    uint32_t botGid=invFirst+0;
    uint32_t rescueGid=invFirst+1;
    uint32_t teleportGid=invFirst+2;
    uint32_t borderGid=invFirst+3;
    // Semantic tile layers (Collisions/Grass/Water/Ledges) use a TRANSPARENT
    // invisible.tsx tile, not the cell's own Walkable tile: the engine only
    // needs a non-empty gid, and a transparent marker avoids putting the same
    // tile at the same position on two layers (and keeps the editor render ==
    // the game render).
    uint32_t semanticMk=invFirst+4;

    // Build the layers.
    std::vector<uint32_t> walkable(cells,0);
    std::vector<uint32_t> walkbehind(cells,0);
    bool anyOver=false;
    std::vector<uint32_t> collisions(cells,0);
    std::vector<uint32_t> grass(cells,0);
    std::vector<uint32_t> water(cells,0);
    std::vector<uint32_t> ledgeUp(cells,0);
    std::vector<uint32_t> ledgeDown(cells,0);
    std::vector<uint32_t> ledgeLeft(cells,0);
    std::vector<uint32_t> ledgeRight(cells,0);
    bool anyGrass=false,anyWater=false,anyLedge=false;

    uint32_t c=0;
    while(c<cells)
    {
        uint16_t block=rom_.u16(map.blocksPtr+c*2);
        uint16_t metatile=block & 0x3FF;
        uint8_t collisionBits=static_cast<uint8_t>((block>>10) & 0x3);
        walkable[c]=tilesets_.groundGid(map,static_cast<int>(c%map.width),static_cast<int>(c/map.width));
        uint32_t og=tilesets_.overGid(map,metatile);
        if(og!=0)
        {
            walkbehind[c]=og;
            anyOver=true;
        }

        uint16_t beh=tilesets_.behavior(map,metatile);
        Terrain t=decoder_.terrain(beh);
        bool isWater=(t==Terrain::Water);
        if(t==Terrain::Grass)
        {
            grass[c]=semanticMk;
            anyGrass=true;
        }
        else if(isWater)
        {
            water[c]=semanticMk;
            anyWater=true;
        }
        else if(t==Terrain::LedgeUp)
        {
            ledgeUp[c]=semanticMk; anyLedge=true;
        }
        else if(t==Terrain::LedgeDown)
        {
            ledgeDown[c]=semanticMk; anyLedge=true;
        }
        else if(t==Terrain::LedgeLeft)
        {
            ledgeLeft[c]=semanticMk; anyLedge=true;
        }
        else if(t==Terrain::LedgeRight)
        {
            ledgeRight[c]=semanticMk; anyLedge=true;
        }
        // Block on foot when the collision bit is set, except for surfable
        // water (kept out of Collisions; the Water layer's swim gating in
        // map/layers.xml is what stops on-foot walking there).
        if(collisionBits!=0 && !isWater)
            collisions[c]=semanticMk;
        c++;
    }

    // Assemble the file at its hierarchy path.
    std::string myPath=naming_.pathFor(map.group,map.map);
    if(myPath.empty())
        myPath=rom_.game().region+"/misc/g"+std::to_string(map.group)+"-m"+std::to_string(map.map);
    std::string path=fireredDir_+"/"+myPath+".tmx";
    std::string::size_type slash=path.find_last_of('/');
    QDir().mkpath(QString::fromStdString(path.substr(0,slash)));
    // "../" per directory level back to the region root (where tileset/ lives).
    int depth=0;
    std::string::size_type di=0;
    while(di<myPath.size())
    {
        if(myPath[di]=='/')
            depth++;
        di++;
    }
    std::string tsPre;
    while(depth>0)
    {
        tsPre+="../";
        depth--;
    }
    tsPre+="tileset/";

    std::ofstream out(path);
    if(!out)
    {
        std::cerr << "Cannot write map: " << path << std::endl;
        return;
    }

    int nextObjectId=1;

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<map version=\"1.10\" tiledversion=\"1.11.0\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\""
        << w << "\" height=\"" << h << "\" tilewidth=\"16\" tileheight=\"16\" infinite=\"0\">\n";
    std::vector<std::pair<uint32_t,std::string> > refs=tilesets_.tilesetRefs(map);
    size_t ri=0;
    while(ri<refs.size())
    {
        out << " <tileset firstgid=\"" << refs[ri].first << "\" source=\"" << tsPre << refs[ri].second << ".tsx\"/>\n";
        ri++;
    }
    // Shared map/invisible.tsx (object markers), at map/ root = tsPre minus the
    // trailing "tileset/" plus "../../" (main/<label>/ up to map/).
    std::string invRel=tsPre.substr(0,tsPre.size()>=8 ? tsPre.size()-8 : 0)+"../../invisible.tsx";
    out << " <tileset firstgid=\"" << invFirst << "\" source=\"" << invRel << "\"/>\n";

    int layerId=1;
    out << " <layer id=\"" << layerId++ << "\" name=\"Walkable\" width=\"" << w << "\" height=\"" << h << "\">\n";
    out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(walkable) << "</data>\n </layer>\n";
    if(anyGrass)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"Grass\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(grass) << "</data>\n </layer>\n";
    }
    if(anyWater)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"Water\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(water) << "</data>\n </layer>\n";
    }
    if(anyLedge)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesUp\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeUp) << "</data>\n </layer>\n";
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesDown\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeDown) << "</data>\n </layer>\n";
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesLeft\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeLeft) << "</data>\n </layer>\n";
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesRight\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeRight) << "</data>\n </layer>\n";
    }
    out << " <layer id=\"" << layerId++ << "\" name=\"Collisions\" width=\"" << w << "\" height=\"" << h << "\">\n";
    out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(collisions) << "</data>\n </layer>\n";
    // WalkBehind holds the lifted overlay tiles and must be the last tile layer:
    // the client inserts the player sprite just before it, so these draw above
    // the player (tree tops, roofs, ...).
    if(anyOver)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"WalkBehind\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(walkbehind) << "</data>\n </layer>\n";
    }

    // Moving group: warps -> door, connections -> border-*.
    out << " <objectgroup id=\"100\" name=\"Moving\">\n";
    size_t wi=0;
    while(wi<map.warps.size())
    {
        const DecodedWarp &warp=map.warps[wi];
        const DecodedMap *dest=decoder_.find(warp.destGroup,warp.destMap);
        if(dest!=nullptr && dest->width>0)
        {
            uint8_t tx=0,ty=0;
            if(warp.destWarpId<dest->warps.size())
            {
                tx=dest->warps[warp.destWarpId].x;
                ty=dest->warps[warp.destWarpId].y;
            }
            else if(!dest->warps.empty())
            {
                tx=dest->warps[0].x;
                ty=dest->warps[0].y;
            }
            int px=static_cast<int>(warp.x)*16;
            int py=(static_cast<int>(warp.y)+1)*16;
            // Classify the warp from the behaviour of the metatile it sits on:
            // building doors stay "door", arrow/edge tiles become "teleport on
            // push", plain step-on pads become "teleport on it".  All three use
            // the same map/x/y target properties in CatchChallenger.
            // "teleport on push"/"teleport on it" use invisible.tsx tile 2; a
            // real door references its own animated door tile (or the client
            // deletes it for lacking an animation).
            std::string warpKind="teleport on it";
            uint32_t warpGid=teleportGid;
            if(warp.x<w && warp.y<h)
            {
                uint16_t wmeta=static_cast<uint16_t>(rom_.u16(map.blocksPtr+(static_cast<uint32_t>(warp.y)*w+warp.x)*2) & 0x3FF);
                uint16_t wbeh=tilesets_.behavior(map,wmeta);
                warpKind=decoder_.warpType(wbeh);
                if(warpKind=="door")
                {
                    uint32_t dg=tilesets_.doorGid(map,wmeta);
                    if(dg!=0)
                        warpGid=dg;
                    else
                        warpKind="teleport on it"; // no anim tile -> keep the warp alive
                }
            }
            out << "  <object id=\"" << nextObjectId++ << "\" type=\"" << warpKind << "\" gid=\"" << warpGid
                << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            out << "   <properties>\n";
            out << "    <property name=\"map\" value=\"" << relPath(myPath,naming_.pathFor(dest->group,dest->map)) << "\"/>\n";
            out << "    <property name=\"x\" value=\"" << static_cast<int>(tx) << "\"/>\n";
            out << "    <property name=\"y\" value=\"" << static_cast<int>(ty) << "\"/>\n";
            out << "   </properties>\n  </object>\n";
        }
        wi++;
    }
    size_t ci=0;
    while(ci<map.connections.size())
    {
        const DecodedConnection &conn=map.connections[ci];
        const DecodedMap *dest=decoder_.find(conn.destGroup,conn.destMap);
        if(dest!=nullptr && dest->width>0 && conn.direction>=1 && conn.direction<=4)
        {
            // A border object marks one map edge and carries the alignment in
            // its position: CC reads object_x (top/bottom) or object_y
            // (left/right) as the offset.  Centre it on the edge and add the
            // connection's shift: offset = (thisDim+neighbourDim)/4 + halfO.
            // The /4 term is shared by both maps so it cancels in CC's
            // difference-based alignment, leaving exactly the Gen3 offset; for an
            // aligned same-width pair this lands on the edge centre (like gen2).
            std::string type;
            int O=conn.offset;
            int thisDim=0,neighDim=0,tx=0,ty=0;
            bool horizontal=false; // offset runs along X (top/bottom)
            if(conn.direction==1)      { type="border-bottom"; horizontal=true; thisDim=static_cast<int>(w); neighDim=dest->width;  ty=static_cast<int>(h)-1; }
            else if(conn.direction==2) { type="border-top";    horizontal=true; thisDim=static_cast<int>(w); neighDim=dest->width;  ty=0; }
            else if(conn.direction==3) { type="border-left";   thisDim=static_cast<int>(h); neighDim=dest->height; tx=0; }
            else                       { type="border-right";  thisDim=static_cast<int>(h); neighDim=dest->height; tx=static_cast<int>(w)-1; }
            int halfO=(O>=0) ? (O+1)/2 : -((-O)/2);
            int ov=(thisDim+neighDim)/4+halfO;
            if(ov<0)
                ov=0;
            if(horizontal)
                tx=ov;
            else
                ty=ov;
            int px=tx*16;
            int py=(ty+1)*16;
            out << "  <object id=\"" << nextObjectId++ << "\" type=\"" << type << "\" gid=\"" << borderGid
                << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            out << "   <properties>\n";
            out << "    <property name=\"map\" value=\"" << relPath(myPath,naming_.pathFor(dest->group,dest->map)) << "\"/>\n";
            out << "   </properties>\n  </object>\n";
        }
        ci++;
    }
    // Rescue point (respawn): place one on the walkable tile nearest the centre
    // of each town/city.  The heal-map door isn't reliably identifiable, so the
    // town centre stands in for "near the door to the heal point map".
    if(map.mapType==1 || map.mapType==2)
    {
        int bestx=-1,besty=-1;
        uint32_t bestd=0xFFFFFFFFu;
        uint32_t cx0=w/2,cy0=h/2;
        uint32_t y=0;
        while(y<h)
        {
            uint32_t x=0;
            while(x<w)
            {
                uint16_t block=rom_.u16(map.blocksPtr+(y*w+x)*2);
                if(((block>>10)&0x3)==0)
                {
                    uint32_t dx=(x>cx0)?x-cx0:cx0-x;
                    uint32_t dy=(y>cy0)?y-cy0:cy0-y;
                    uint32_t d=dx*dx+dy*dy;
                    if(d<bestd)
                    {
                        bestd=d;
                        bestx=static_cast<int>(x);
                        besty=static_cast<int>(y);
                    }
                }
                x++;
            }
            y++;
        }
        if(bestx>=0)
            out << "  <object id=\"" << nextObjectId++ << "\" type=\"rescue\" gid=\"" << rescueGid
                << "\" x=\"" << bestx*16 << "\" y=\"" << (besty+1)*16 << "\" width=\"16\" height=\"16\"/>\n";
    }
    out << " </objectgroup>\n";

    // Object group: bots = skinned NPCs + skinless script signs.  Every object
    // here has a matching <bot id> definition in the companion .xml.
    out << " <objectgroup id=\"101\" name=\"Object\">\n";
    std::vector<MapBot> bots=collectBots(map);
    size_t bi=0;
    while(bi<bots.size())
    {
        const MapBot &b=bots[bi];
        int px=b.x*16;
        int py=(b.y+1)*16;
        out << "  <object id=\"" << nextObjectId++ << "\" type=\"bot\" gid=\"" << botGid
            << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
        out << "   <properties>\n";
        out << "    <property name=\"id\" type=\"int\" value=\"" << b.id << "\"/>\n";
        // A skinless sign carries no skin (CatchChallenger renders it as an
        // invisible text bot); a real NPC carries its resolved skin + facing.
        if(!b.skin.empty())
        {
            out << "    <property name=\"lookAt\" value=\"bottom\"/>\n";
            out << "    <property name=\"skin\" value=\"" << b.skin << "\"/>\n";
        }
        out << "   </properties>\n  </object>\n";
        bi++;
    }
    out << " </objectgroup>\n";

    out << "</map>\n";
}

void CCWriter::writeInformations()
{
    std::string label=rom_.game().label;
    std::string display=label;
    if(!display.empty())
        display[0]=static_cast<char>(std::toupper(static_cast<unsigned char>(display[0])));
    std::string path=fireredDir_+"/informations.xml";
    std::ofstream out(path);
    if(!out)
    {
        std::cerr << "Cannot write " << path << std::endl;
        return;
    }
    out << "<?xml version='1.0'?>\n";
    out << "<informations color=\"#4FD9FF\">\n";
    out << " <name>" << display << "</name>\n";
    char initial=display.empty() ? 'F' : display[0];
    out << " <initial>" << initial << "</initial>\n";
    out << "</informations>\n";
}

void CCWriter::writeStart()
{
    // Choose a sizeable map and a guaranteed-walkable tile near its centre.
    const std::vector<DecodedMap> &maps=decoder_.maps();
    const DecodedMap *startMap=nullptr;
    size_t i=0;
    while(i<maps.size())
    {
        if(maps[i].width>=10 && maps[i].height>=10)
        {
            startMap=&maps[i];
            break;
        }
        i++;
    }
    if(startMap==nullptr && !maps.empty())
        startMap=&maps[0];
    if(startMap==nullptr)
        return;
    uint32_t w=static_cast<uint32_t>(startMap->width);
    uint32_t h=static_cast<uint32_t>(startMap->height);
    uint32_t sx=w/2;
    uint32_t sy=h/2;
    // search outward for a non-blocked cell
    uint32_t cy=0;
    bool found=false;
    while(cy<h && !found)
    {
        uint32_t cx=0;
        while(cx<w)
        {
            uint16_t block=rom_.u16(startMap->blocksPtr+(cy*w+cx)*2);
            uint8_t collisionBits=static_cast<uint8_t>((block>>10) & 0x3);
            if(collisionBits==0)
            {
                sx=cx; sy=cy; found=true;
                break;
            }
            cx++;
        }
        cy++;
    }
    std::string path=fireredDir_+"/start.xml";
    std::ofstream out(path);
    if(!out)
        return;
    out << "<profile>\n";
    out << " <start id=\"normal\">\n";
    out << "  <map file=\"" << naming_.pathFor(startMap->group,startMap->map) << "\" x=\"" << sx << "\" y=\"" << sy << "\" />\n";
    out << " </start>\n";
    out << "</profile>\n";
}

void CCWriter::writeZones()
{
    std::string dir=fireredDir_+"/zone";
    QDir().mkpath(QString::fromStdString(dir));
    const std::vector<std::pair<std::string,std::string> > &zones=naming_.zones();
    std::unordered_set<std::string> done;
    size_t i=0;
    while(i<zones.size())
    {
        const std::string &slug=zones[i].first;
        if(done.find(slug)==done.cend())
        {
            done.insert(slug);
            std::ofstream out(dir+"/"+slug+".xml");
            if(out)
            {
                out << "<zone>\n";
                out << " <name>" << zones[i].second << "</name>\n";
                out << "</zone>\n";
            }
        }
        i++;
    }
}

// Aggregate a per-slot wild list by species (sum slot weight -> luck, widen the
// level range) and emit a <grass>/<water> element.
static void emitWildList(std::ofstream &out, const char *tag, const std::vector<WildSlot> &slotList)
{
    std::vector<std::string> order;
    std::unordered_map<std::string,int> luck,minL,maxL;
    size_t s=0;
    while(s<slotList.size())
    {
        const WildSlot &sl=slotList[s];
        if(!sl.species.empty())
        {
            if(luck.find(sl.species)==luck.cend())
            {
                order.push_back(sl.species);
                luck[sl.species]=0;
                minL[sl.species]=sl.minLevel;
                maxL[sl.species]=sl.maxLevel;
            }
            luck[sl.species]+=sl.weight;
            if(sl.minLevel<minL[sl.species]) minL[sl.species]=sl.minLevel;
            if(sl.maxLevel>maxL[sl.species]) maxL[sl.species]=sl.maxLevel;
        }
        s++;
    }
    if(order.empty())
        return;
    out << " <" << tag << ">\n";
    size_t k=0;
    while(k<order.size())
    {
        const std::string &sp=order[k];
        out << "  <monster id=\"" << sp << "\" luck=\"" << luck[sp] << "\" ";
        if(minL[sp]==maxL[sp])
            out << "level=\"" << minL[sp] << "\"";
        else
            out << "minLevel=\"" << minL[sp] << "\" maxLevel=\"" << maxL[sp] << "\"";
        out << "/>\n";
        k++;
    }
    out << " </" << tag << ">\n";
}

// Companion <mapbase>.xml: the functional structure (type/zone/name) plus the
// wild-encounter data.  Bot/trainer dialogue is intentionally not transcribed.
void CCWriter::writeMapXml(const DecodedMap &map)
{
    std::string myPath=naming_.pathFor(map.group,map.map);
    if(myPath.empty())
        return;
    std::string path=fireredDir_+"/"+myPath+".xml";
    std::ofstream out(path);
    if(!out)
        return;
    std::string type=naming_.typeFor(map.group,map.map);
    std::string zone=naming_.zoneFor(map.group,map.map);
    std::string name=naming_.displayFor(map.group,map.map);
    out << "<map type=\"" << type << "\"";
    if(!zone.empty())
        out << " zone=\"" << zone << "\"";
    out << ">\n";
    out << " <name>" << name << "</name>\n";

    // Wild encounters ("monster on map").
    const WildSet *wild=wild_.find(map.group,map.map);
    if(wild!=nullptr)
    {
        emitWildList(out,"grass",wild->land);
        emitWildList(out,"water",wild->water);
    }

    // Bot definitions.  EVERY bot placed in the .tmx is defined here (matching
    // ids via collectBots), so there are no dangling references.  A trainer
    // emits its fight party, a merchant its shop list; a plain NPC or a script
    // sign emits a single text step.  Spoken dialogue prose is intentionally
    // left empty (only the function/structure is kept).
    std::vector<MapBot> bots=collectBots(map);
    size_t bi=0;
    while(bi<bots.size())
    {
        const MapBot &b=bots[bi];
        ScriptResult res;
        if(!b.isSign)
            res=script_.classify(b.trainerType,b.scriptPtr);
        else
            res.kind=BotKind::None;
        bool emitted=false;
        if(res.kind==BotKind::Fight)
        {
            std::vector<PartyMon> party=script_.party(res.trainerId);
            if(!party.empty())
            {
                out << " <bot id=\"" << b.id << "\">\n  <step type=\"fight\" id=\"1\">\n";
                size_t pi=0;
                while(pi<party.size())
                {
                    out << "   <monster id=\"" << party[pi].species << "\" level=\""
                        << static_cast<int>(party[pi].level) << "\"/>\n";
                    pi++;
                }
                out << "  </step>\n </bot>\n";
                emitted=true;
            }
        }
        else if(res.kind==BotKind::Mart)
        {
            std::string body;
            size_t ii=0;
            while(ii<res.itemIds.size())
            {
                std::string in=script_.itemName(res.itemIds[ii]);
                if(!in.empty())
                    body+="   <product item=\""+in+"\"/>\n";
                ii++;
            }
            if(!body.empty())
            {
                out << " <bot id=\"" << b.id << "\">\n  <step type=\"shop\" id=\"1\">\n"
                    << body << "  </step>\n </bot>\n";
                emitted=true;
            }
        }
        else if(res.kind==BotKind::Heal)
        {
            out << " <bot id=\"" << b.id << "\">\n  <step type=\"heal\" id=\"1\"/>\n </bot>\n";
            emitted=true;
        }
        else if(res.kind==BotKind::Pc)
        {
            out << " <bot id=\"" << b.id << "\">\n  <step type=\"pc\" id=\"1\"/>\n </bot>\n";
            emitted=true;
        }
        // SIGN -> the ROM sign string, split into [Next] pages (project owner's
        // call to include sign text).  Plain talking NPC -> one empty text step
        // (creative dialogue is not transcribed).
        if(!emitted)
        {
            std::vector<std::string> pages;
            if(b.isSign)
            {
                uint32_t tptr=script_.signTextOffset(b.scriptPtr);
                if(tptr!=0)
                    pages=Gen3Text::decodeSign(rom_,tptr,512);
            }
            if(!pages.empty())
            {
                out << " <bot id=\"" << b.id << "\">\n  <name><![CDATA[Sign]]></name>\n";
                size_t pi=0;
                while(pi<pages.size())
                {
                    std::string t=pages[pi];
                    if(pi+1<pages.size())
                        t+="<br /><a href=\"next\">[Next]</a>";
                    out << "  <step type=\"text\" id=\"" << (pi+1) << "\">\n   <text><![CDATA["
                        << t << "]]></text>\n  </step>\n";
                    pi++;
                }
                out << " </bot>\n";
            }
            else
                out << " <bot id=\"" << b.id << "\">\n  <step type=\"text\" id=\"1\">\n"
                    << "   <text><![CDATA[]]></text>\n  </step>\n </bot>\n";
        }
        bi++;
    }
    out << "</map>\n";
}
