#include "CCWriter.hpp"
#include "Gen3Text.hpp"
#include "OverworldSprite.hpp"

#include <QByteArray>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <vector>
#include <zstd.h>

// libtiled — render the written .tmx exactly as map2png / the client do.
#include <mapreader.h>
#include <map.h>
#include <layer.h>
#include <tilelayer.h>
#include <maprenderer.h>
#include <orthogonalrenderer.h>

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
    skins_(skins),
    guardLayers_(0),
    guardMasked_(0),
    guardTopMaps_(0),
    guardTopCover_(0),
    renderLayers_(0),
    renderInvisible_(0)
{
}

// Render the map's tile layers (all of them, or all but hideIndex) with libtiled,
// exactly as the client/map2png do, into an ARGB image.  Object groups are
// skipped (only tile layers matter, and skipping them avoids map2png's known
// stale-back-pointer crash).  Same painter setup for every call so the resulting
// images are directly comparable.
static QImage renderTiledLayers(const Tiled::MapRenderer &renderer,
                                const QList<Tiled::Layer *> &layers,
                                const QRect &bounds, int hideIndex)
{
    QImage img(bounds.size(),QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    QPainter p(&img);
    p.translate(-bounds.topLeft());
    int i=0;
    while(i<layers.size())
    {
        Tiled::Layer *L=layers.at(i);
        if(i!=hideIndex && L->isTileLayer())
            renderer.drawTileLayer(&p,L->asTileLayer(),QRectF(bounds));
        i++;
    }
    p.end();
    return img;
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


void CCWriter::layerVisibilityGuard(const DecodedMap &map,
                                    const std::vector<uint32_t> &walkable,
                                    const std::vector<uint32_t> &grass, bool anyGrass,
                                    const std::vector<uint32_t> &water, bool anyWater,
                                    const std::vector<uint32_t> &ledgeUp,
                                    const std::vector<uint32_t> &ledgeDown,
                                    const std::vector<uint32_t> &ledgeLeft,
                                    const std::vector<uint32_t> &ledgeRight, bool anyLedge,
                                    const std::vector<uint32_t> &collisions,
                                    const std::vector<uint32_t> &walkbehind, bool anyOver)
{
    size_t cells=walkable.size();
    // The only layer that can fully hide a layer below it is WalkBehind, when its
    // lifted over-tile is 100% opaque (the semantic markers are semi-transparent,
    // and Walkable is the bottom layer).  Pre-mark those masking cells.
    std::vector<char> masking(cells,0);
    if(anyOver)
    {
        size_t c=0;
        while(c<cells)
        {
            if(walkbehind[c]!=0)
            {
                uint16_t mt=static_cast<uint16_t>(map.blockAt(rom_,c%static_cast<uint32_t>(map.width),c/static_cast<uint32_t>(map.width))&0x3FF);
                if(tilesets_.overOpaque(map,mt))
                    masking[c]=1;
            }
            c++;
        }
    }
    // For each tile layer (bottom -> top), require >=1 cell that has the layer's
    // tile AND is not masked by an opaque tile above (WalkBehind itself is top).
    struct LG { const char *name; const std::vector<uint32_t> *data; bool top; bool present; };
    LG layers[]={
        {"Walkable",&walkable,false,true},
        {"Grass",&grass,false,anyGrass},
        {"Water",&water,false,anyWater},
        {"LedgesUp",&ledgeUp,false,anyLedge},
        {"LedgesDown",&ledgeDown,false,anyLedge},
        {"LedgesLeft",&ledgeLeft,false,anyLedge},
        {"LedgesRight",&ledgeRight,false,anyLedge},
        {"Collisions",&collisions,false,true},
        {"WalkBehind",&walkbehind,true,anyOver},
    };
    std::string myPath=naming_.pathFor(map.group,map.map);
    size_t li=0;
    while(li<sizeof(layers)/sizeof(layers[0]))
    {
        const LG &L=layers[li];
        if(L.present)
        {
            bool hasTile=false,visible=false;
            size_t c=0;
            while(c<cells)
            {
                if((*L.data)[c]!=0)
                {
                    hasTile=true;
                    if(L.top || masking[c]==0)
                    {
                        visible=true;
                        break;
                    }
                }
                c++;
            }
            if(hasTile)
            {
                guardLayers_++;
                if(!visible)
                {
                    guardMasked_++;
                    if(guardMaskedList_.size()<20)
                        guardMaskedList_.push_back(myPath+" / "+L.name);
                }
            }
        }
        li++;
    }
    // Second guard: the last/top tile layer (WalkBehind in general) must NOT be a
    // FULL-SURFACE 100%-opaque cover — a fully-opaque over-tile on EVERY cell would
    // hide every layer below it (the whole map would render as just WalkBehind).
    if(anyOver)
    {
        guardTopMaps_++;
        bool fullCover=true;
        size_t c=0;
        while(c<cells)
        {
            // a cell breaks the cover if WalkBehind is empty there OR its over-tile
            // is not 100% opaque (masking[c] was set only for fully-opaque overs).
            if(walkbehind[c]==0 || masking[c]==0)
            {
                fullCover=false;
                break;
            }
            c++;
        }
        if(fullCover)
        {
            guardTopCover_++;
            if(guardTopCoverList_.size()<20)
                guardTopCoverList_.push_back(myPath+" / WalkBehind");
        }
    }
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
    if(guardMasked_==0)
        std::cout << "CCWriter GUARD layer-visibility: PASS (" << guardLayers_
                  << " tile layers, each visible when toggled)" << std::endl;
    else
    {
        std::cout << "CCWriter GUARD layer-visibility: FAIL (" << guardMasked_ << "/"
                  << guardLayers_ << " layers fully hidden by an opaque tile above):" << std::endl;
        size_t k=0;
        while(k<guardMaskedList_.size()){ std::cout << "  " << guardMaskedList_[k] << std::endl; k++; }
    }
    if(guardTopCover_==0)
        std::cout << "CCWriter GUARD top-layer-cover: PASS (" << guardTopMaps_
                  << " WalkBehind layers, none a full 100%-opaque cover)" << std::endl;
    else
    {
        std::cout << "CCWriter GUARD top-layer-cover: FAIL (" << guardTopCover_
                  << " WalkBehind layers fully cover the map with opaque tiles, hiding everything below):" << std::endl;
        size_t k=0;
        while(k<guardTopCoverList_.size()){ std::cout << "  " << guardTopCoverList_[k] << std::endl; k++; }
    }
    renderVisibilityGuard();
    return true;
}

// Per-thread accumulator + the shared render queue for the parallel guard.
struct RenderGuardResult {
    int layers;
    int invisible;
    std::vector<std::string> list;
    RenderGuardResult() : layers(0), invisible(0) {}
};
struct RenderGuardQueue {
    const std::vector<std::unique_ptr<Tiled::Map> > *maps;
    const std::vector<std::string> *tmxPaths;
    const std::string *fireredDir;
    std::atomic<size_t> next;
};

// One worker: pull the next map index off the shared queue, render it (baseline
// then each non-empty tile layer hidden) and record any layer whose hide does not
// change the pixels.  Reads only the shared (already-loaded) maps + cached
// tilesets and writes its OWN QImages + result, so no locking is needed.
static void renderGuardWorker(RenderGuardQueue *q, RenderGuardResult *out)
{
    while(true)
    {
        size_t idx=q->next.fetch_add(1);
        if(idx>=q->maps->size())
            break;
        Tiled::Map *map=(*q->maps)[idx].get();
        if(map==nullptr)
            continue;
        Tiled::OrthogonalRenderer renderer(map);
        QRect bounds=renderer.mapBoundingRect();
        if(bounds.width()<=0 || bounds.height()<=0)
            continue;
        const QList<Tiled::Layer *> &layers=map->layers();
        QImage baseline=renderTiledLayers(renderer,layers,bounds,-1);
        int i=0;
        while(i<layers.size())
        {
            Tiled::Layer *L=layers.at(i);
            if(L->isTileLayer() && !L->asTileLayer()->isEmpty())
            {
                out->layers++;
                QImage variant=renderTiledLayers(renderer,layers,bounds,i);
                if(variant==baseline)
                {
                    out->invisible++;
                    if(out->list.size()<30)
                    {
                        std::string rel=(*q->tmxPaths)[idx];
                        if(rel.compare(0,q->fireredDir->size(),*q->fireredDir)==0)
                            rel=rel.substr(q->fireredDir->size()+1);
                        out->list.push_back(rel+" / "+L->name().toStdString());
                    }
                }
            }
            i++;
        }
    }
}

void CCWriter::renderVisibilityGuard()
{
    if(writtenTmx_.empty())
        return;
    // Phase 1 (serial, main thread): load every written .tmx with libtiled.  The
    // load path is NOT thread-safe — it uses the global TilesetManager cache (so
    // each distinct tileset sheet loads exactly ONCE and is reused across maps)
    // and builds QPixmaps, which Qt only allows on the main thread.
    std::vector<std::unique_ptr<Tiled::Map> > maps;
    maps.reserve(writtenTmx_.size());
    size_t mi=0;
    while(mi<writtenTmx_.size())
    {
        Tiled::MapReader reader;
        maps.push_back(reader.readMap(QString::fromStdString(writtenTmx_[mi])));
        if(maps.back()==nullptr)
            std::cerr << "render-guard: could not load " << writtenTmx_[mi] << std::endl;
        mi++;
    }
    // Phase 2 (parallel): a render queue drained by one worker per CPU core.  Each
    // job renders one map's tile layers (all, then each hidden) and compares — the
    // rendering only READS the shared maps + cached tilesets, writing its own
    // images, so the cores run independently with no locking.
    unsigned int nthreads=std::thread::hardware_concurrency();
    if(nthreads<1)
        nthreads=1;
    if(static_cast<size_t>(nthreads)>maps.size())
        nthreads=static_cast<unsigned int>(maps.size());
    RenderGuardQueue q;
    q.maps=&maps;
    q.tmxPaths=&writtenTmx_;
    q.fireredDir=&fireredDir_;
    q.next.store(0);
    std::vector<RenderGuardResult> results(nthreads);
    std::vector<std::thread> pool;
    unsigned int t=0;
    while(t<nthreads)
    {
        pool.push_back(std::thread(renderGuardWorker,&q,&results[t]));
        t++;
    }
    t=0;
    while(t<pool.size())
    {
        pool[t].join();
        t++;
    }
    t=0;
    while(t<results.size())
    {
        renderLayers_+=results[t].layers;
        renderInvisible_+=results[t].invisible;
        size_t k=0;
        while(k<results[t].list.size() && renderInvisibleList_.size()<30)
        {
            renderInvisibleList_.push_back(results[t].list[k]);
            k++;
        }
        t++;
    }
    if(renderInvisible_==0)
        std::cout << "CCWriter GUARD render-visibility: PASS (" << renderLayers_
                  << " tile layers libtiled-rendered on " << nthreads
                  << " threads; hiding each changes the render)" << std::endl;
    else
    {
        std::cout << "CCWriter GUARD render-visibility: FAIL (" << renderInvisible_ << "/"
                  << renderLayers_ << " layers render IDENTICALLY whether shown or hidden):" << std::endl;
        size_t k=0;
        while(k<renderInvisibleList_.size()){ std::cout << "  " << renderInvisibleList_[k] << std::endl; k++; }
    }
}
void CCWriter::writeMap(const DecodedMap &map)
{
    uint32_t w=static_cast<uint32_t>(map.width);
    uint32_t h=static_cast<uint32_t>(map.height);
    uint32_t cells=w*h;
    // The Moving/Object OBJECTS use the shared map/invisible.tsx markers (objects
    // are added by the engine at runtime / invisible in the editor): tile 0 bot,
    // 1 rescue, 2 teleport, 3 border-offset.  Doors reference their animated tile.
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

    // Build the layers.
    std::vector<uint32_t> walkable(cells,0);
    std::vector<uint32_t> walkbehind(cells,0);
    bool anyOver=false;
    std::vector<uint32_t> collisions(cells,0);
    std::vector<uint32_t> collisions2(cells,0); // 2nd Collisions: the over of a
    bool anyCollisions2=false;                  // collidable cell (superposed below player)
    std::vector<uint32_t> grass(cells,0);
    std::vector<uint32_t> water(cells,0);
    std::vector<uint32_t> ledgeUp(cells,0);
    std::vector<uint32_t> ledgeDown(cells,0);
    std::vector<uint32_t> ledgeLeft(cells,0);
    std::vector<uint32_t> ledgeRight(cells,0);
    bool anyGrass=false,anyWater=false,anyLedge=false;
    // Per-direction ledge presence: a map with only (say) up-ledges must NOT emit
    // empty LedgesLeft/Right layers — an all-zero layer renders nothing, so hiding
    // it in Tiled changes no pixel (it would fail the render-visibility guard) and
    // just bloats the .tmx.  anyLedge stays the OR (used by the visibility guard).
    bool anyLedgeUp=false,anyLedgeDown=false,anyLedgeLeft=false,anyLedgeRight=false;

    // DISJOINT real-tile layers (no generated markers): every cell's REAL
    // below-player tile (groundGid) goes to EXACTLY ONE layer chosen by its
    // behaviour, so hiding any layer in Tiled removes its cells (visible change).
    // The above-player over-tile (overGid) goes to WalkBehind ONLY for cells the
    // player can stand on; for a COLLIDABLE cell the player is never there, so its
    // over stays BELOW the player, superposed on a 2nd "Collisions" layer (the
    // engine OR-merges the Collisions layers for blocking, so a 2nd one is free
    // superposition, NOT an extra tile above the player — that wrongly hid the
    // player behind whole building/tree bodies).  Walkable stays EMPTY at feature
    // cells (water/grass/ledge/collision): the engine still makes water/grass/lava
    // passable via the layer's walkOn monsterCollision (map/layers.xml), collisions
    // block (254), ledges 250-253.  (CatchChallenger re-orders tile layers at load,
    // so this must NOT depend on layer order — disjoint + OR-merge do not.)
    uint32_t c=0;
    while(c<cells)
    {
        uint16_t block=map.blockAt(rom_,c%static_cast<uint32_t>(map.width),c/static_cast<uint32_t>(map.width));
        uint16_t metatile=block & 0x3FF;
        uint8_t collisionBits=static_cast<uint8_t>((block>>10) & 0x3);
        uint32_t g=tilesets_.groundGid(map,static_cast<int>(c%map.width),static_cast<int>(c/map.width));
        uint32_t og=tilesets_.overGid(map,metatile);

        uint16_t beh=tilesets_.behavior(map,metatile);
        Terrain t=decoder_.terrain(beh);
        bool onCollisions=false;
        if(t==Terrain::Water)
        {
            water[c]=g; anyWater=true;
        }
        else if(t==Terrain::LedgeUp)
        {
            ledgeUp[c]=g; anyLedge=true; anyLedgeUp=true;
        }
        else if(t==Terrain::LedgeDown)
        {
            ledgeDown[c]=g; anyLedge=true; anyLedgeDown=true;
        }
        else if(t==Terrain::LedgeLeft)
        {
            ledgeLeft[c]=g; anyLedge=true; anyLedgeLeft=true;
        }
        else if(t==Terrain::LedgeRight)
        {
            ledgeRight[c]=g; anyLedge=true; anyLedgeRight=true;
        }
        else if(t==Terrain::Grass)
        {
            grass[c]=g; anyGrass=true;
        }
        else if(collisionBits!=0)
        {
            collisions[c]=g; onCollisions=true;
        }
        else
        {
            walkable[c]=g;
        }
        if(og!=0)
        {
            if(onCollisions)
            {
                collisions2[c]=og; anyCollisions2=true;
            }
            else
            {
                walkbehind[c]=og; anyOver=true;
            }
        }
        c++;
    }

    // Assemble the file at its hierarchy path.
    std::string myPath=naming_.pathFor(map.group,map.map);
    if(myPath.empty())
        myPath=rom_.game().region+"/misc/g"+std::to_string(map.group)+"-m"+std::to_string(map.map);
    layerVisibilityGuard(map,walkable,grass,anyGrass,water,anyWater,
                         ledgeUp,ledgeDown,ledgeLeft,ledgeRight,anyLedge,
                         collisions,walkbehind,anyOver);
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
    // Shared map/invisible.tsx (engine object markers — teleporter/bot/etc, things
    // invisible in the Tiled editor because CatchChallenger adds them at runtime).
    // Referenced READ-ONLY; the converter never writes outside map/main/<label>/.
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
    // Each ledge direction is emitted ONLY when it actually has tiles (a typical
    // town has no ledges at all; a route may have only one or two directions).
    if(anyLedgeUp)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesUp\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeUp) << "</data>\n </layer>\n";
    }
    if(anyLedgeDown)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesDown\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeDown) << "</data>\n </layer>\n";
    }
    if(anyLedgeLeft)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesLeft\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeLeft) << "</data>\n </layer>\n";
    }
    if(anyLedgeRight)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"LedgesRight\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(ledgeRight) << "</data>\n </layer>\n";
    }
    out << " <layer id=\"" << layerId++ << "\" name=\"Collisions\" width=\"" << w << "\" height=\"" << h << "\">\n";
    out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(collisions) << "</data>\n </layer>\n";
    // 2nd "Collisions" layer: the over-tiles of collidable cells, superposed on the
    // first.  The engine OR-merges layers named "Collisions" for blocking, so this
    // is purely visual stacking (the full building/tree renders BELOW the player);
    // it does not lift those tiles above the player like WalkBehind would.
    if(anyCollisions2)
    {
        out << " <layer id=\"" << layerId++ << "\" name=\"Collisions\" width=\"" << w << "\" height=\"" << h << "\">\n";
        out << "  <data encoding=\"base64\" compression=\"zstd\">" << encodeLayer(collisions2) << "</data>\n </layer>\n";
    }
    // WalkBehind holds the lifted overlay tiles and must be the last tile layer:
    // the client inserts the player sprite just before it, so these draw above
    // the player (tree tops, roofs, ...) — but ONLY for player-reachable cells.
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
        // A warp whose SOURCE is outside the map grid is unusable (RSE has a
        // few, e.g. the Slateport ferry pads at x==width): the loader would
        // drop the object with a warning, so skip it cleanly here.
        if(dest!=nullptr && dest->width>0 && warp.x<w && warp.y<h)
        {
            // Destination resolved by Decoder::finalizeMaps (split-safe: the
            // dest map's own warp indices are meaningless after a chunk split).
            // Clamp into the destination grid: a handful of ROM warps point a
            // tile past the edge (or at a 1x1 dummy layout) and the engine
            // rejects an out-of-range target ("tp is out of range").
            uint8_t tx=warp.destX;
            uint8_t ty=warp.destY;
            if(tx>=dest->width) tx=static_cast<uint8_t>(dest->width-1);
            if(ty>=dest->height) ty=static_cast<uint8_t>(dest->height-1);
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
                uint16_t wmeta=static_cast<uint16_t>(map.blockAt(rom_,warp.x,warp.y) & 0x3FF);
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
            // The marker must stay INSIDE this map (the loader drops an object
            // past the edge: "Object out of the map") AND the mirrored marker
            // (ov-O) inside the neighbour, so alignment difference stays = O.
            // Clamping into the shared validity interval is symmetric: both
            // maps compute the same interval shifted by O, and clamp commutes
            // with the shift — the pair difference survives.
            {
                int lo=(O>0)?O:0;
                int hi=thisDim-1;
                if(neighDim-1+O<hi)
                    hi=neighDim-1+O;
                if(lo<=hi)
                {
                    if(ov<lo) ov=lo;
                    if(ov>hi) ov=hi;
                }
            }
            if(ov<0)
                ov=0;
            if(ov>thisDim-1)
                ov=thisDim-1;
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
                uint16_t block=map.blockAt(rom_,x,y);
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
    out.close();
    // record for the render-based visibility guard (run after all maps written).
    writtenTmx_.push_back(path);
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
            uint16_t block=startMap->blockAt(rom_,cx,cy);
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
static void emitWildList(std::ostream &out, const char *tag, const std::vector<WildSlot> &slotList)
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
    // The engine REJECTS a list whose luck does not sum to exactly 100
    // (FightLoader "total luck is not egal to 100").  Slots with an empty
    // species are skipped above, so the slot weights may no longer reach 100:
    // rescale proportionally (min 1) and put the rounding remainder on the
    // biggest entry.
    {
        int total=0;
        size_t k=0;
        while(k<order.size()) { total+=luck[order[k]]; k++; }
        if(total!=100 && total>0)
        {
            int newTotal=0;
            size_t maxIdx=0;
            k=0;
            while(k<order.size())
            {
                int l=(luck[order[k]]*100+total/2)/total;
                if(l<1) l=1;
                luck[order[k]]=l;
                newTotal+=l;
                if(l>luck[order[maxIdx]]) maxIdx=k;
                k++;
            }
            luck[order[maxIdx]]+=100-newTotal;
        }
    }
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

// Read a whole text file; sets *exists=false when it cannot be opened.
static std::string readWholeFile(const std::string &path, bool *exists)
{
    std::ifstream in(path,std::ios::binary);
    if(!in)
    {
        if(exists!=nullptr) *exists=false;
        return std::string();
    }
    if(exists!=nullptr) *exists=true;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Extract the "<tag>...</tag>" block from xml text, or "" when absent.  "<water>"
// won't match "<waterOldRod>" (the trailing '>' differs), so plain tags are safe.
static std::string extractSection(const std::string &xml, const char *tag)
{
    std::string open=std::string("<")+tag+">";
    std::string close=std::string("</")+tag+">";
    size_t a=xml.find(open);
    if(a==std::string::npos)
        return std::string();
    size_t b=xml.find(close,a);
    if(b==std::string::npos)
        return std::string();
    return xml.substr(a,b+close.size()-a);
}

// Whitespace-stripped content key, so indentation differences never count as a diff.
static std::string stripWs(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    size_t i=0;
    while(i<s.size())
    {
        char c=s[i];
        if(c!=' ' && c!='\t' && c!='\n' && c!='\r')
            out.push_back(c);
        i++;
    }
    return out;
}

bool CCWriter::writeSubOverlay(const std::string &mainDir)
{
    const std::string &sub=rom_.game().subCode;
    std::string display=sub;
    if(!display.empty())
        display[0]=static_cast<char>(std::toupper(static_cast<unsigned char>(display[0])));
    // Sub informations.xml — the overlay's own display identity (its own name,
    // initial, colour), matching map/main/test/sub/smallchange/informations.xml.
    QDir().mkpath(QString::fromStdString(fireredDir_));
    {
        std::ofstream out(fireredDir_+"/informations.xml");
        if(out)
        {
            out << "<?xml version='1.0'?>\n";
            out << "<informations color=\"#FF8000\">\n";
            out << " <name>" << display << "</name>\n";
            out << " <initial>" << (display.empty()?'S':display[0]) << "</initial>\n";
            out << "</informations>\n";
        }
    }

    const std::vector<DecodedMap> &maps=decoder_.maps();
    int changed=0, shared=0, missing=0;
    size_t i=0;
    while(i<maps.size())
    {
        const DecodedMap &map=maps[i];
        const std::string &P=naming_.pathFor(map.group,map.map);
        if(P.empty())
        {
            i++;
            continue;
        }
        // The sibling's own wild blocks, produced by the SAME emitter as the main
        // run — so an unchanged encounter list is byte-identical to the main file.
        std::ostringstream gs,ws;
        const WildSet *wild=wild_.find(map.group,map.origMap);
        if(wild!=nullptr)
        {
            emitWildList(gs,"grass",wild->land);
            emitWildList(ws,"water",wild->water);
        }
        const std::string sibGrass=gs.str();
        const std::string sibWater=ws.str();
        // The main's version of this map: geometry is shared, so we only ever need
        // to override wild data.  If the main has no such map, this is a
        // sibling-exclusive map (e.g. Emerald's Battle Frontier) -> not part of a
        // wild-only overlay, skip it.
        bool exists=false;
        const std::string mainText=readWholeFile(mainDir+"/"+P+".xml",&exists);
        if(!exists)
        {
            missing++;
            i++;
            continue;
        }
        const std::string mainGrass=extractSection(mainText,"grass");
        const std::string mainWater=extractSection(mainText,"water");
        const bool emitG=(!sibGrass.empty()) && (stripWs(sibGrass)!=stripWs(mainGrass));
        const bool emitW=(!sibWater.empty()) && (stripWs(sibWater)!=stripWs(mainWater));
        if(!emitG && !emitW)
        {
            shared++;
            i++;
            continue;
        }
        const std::string outFile=fireredDir_+"/"+P+".xml";
        const std::string outDir=outFile.substr(0,outFile.find_last_of('/'));
        QDir().mkpath(QString::fromStdString(outDir));
        std::ofstream out(outFile);
        if(out)
        {
            // Partial map xml: only the changed wild sections.  The engine keeps
            // the main's geometry + every section we DON'T override.
            out << "<map>\n";
            if(emitG) out << sibGrass;
            if(emitW) out << sibWater;
            out << "</map>\n";
            changed++;
        }
        i++;
    }
    std::cout << "CCWriter sub-overlay '" << rom_.game().mainCode << "/sub/" << sub << "': "
              << changed << " map(s) with changed wild encounters, "
              << shared << " identical (shared from main), "
              << missing << " sibling-only (skipped); wrote to " << fireredDir_ << std::endl;
    return true;
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
    // per-map background music: the ROM's BGM id, ripped to music/song-<id>.opus
    // (the file is written by the --all / music pass; absent => no music).
    if(map.music!=0 && map.music!=0xFFFF)
        out << " backgroundsound=\"music/song-" << map.music << ".opus\"";
    out << ">\n";
    out << " <name>" << name << "</name>\n";

    // Wild encounters ("monster on map").
    const WildSet *wild=wild_.find(map.group,map.origMap);
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
                std::string tname=script_.trainerName(res.trainerId);
                out << " <bot id=\"" << b.id << "\">\n";
                if(!tname.empty())
                    out << "  <name><![CDATA[" << tname << "]]></name>\n";
                out << "  <step type=\"fight\" id=\"1\""
                    << (res.leader ? " leader=\"true\"" : "") << ">\n";
                if(res.introText!=0)
                {
                    std::vector<std::string> pg=Gen3Text::decodeSign(rom_,res.introText,512);
                    std::string s;
                    size_t k=0;
                    while(k<pg.size()){ if(k)s+="<br />"; s+=pg[k]; k++; }
                    if(!s.empty())
                        out << "   <start><![CDATA[" << s << "]]></start>\n";
                }
                if(res.defeatText!=0)
                {
                    std::vector<std::string> pg=Gen3Text::decodeSign(rom_,res.defeatText,512);
                    std::string s;
                    size_t k=0;
                    while(k<pg.size()){ if(k)s+="<br />"; s+=pg[k]; k++; }
                    if(!s.empty())
                        out << "   <win><![CDATA[" << s << "]]></win>\n";
                }
                size_t pi=0;
                while(pi<party.size())
                {
                    out << "   <monster id=\"" << party[pi].species << "\" level=\""
                        << static_cast<int>(party[pi].level) << "\"/>\n";
                    pi++;
                }
                // Prize money, Gen3 formula: lastMonLevel * classMoney * 4.  The
                // per-class money table isn't located in the ROM, so classMoney is
                // approximated — 25 for gym leaders (giving the canonical
                // level*100, e.g. a L40 ace -> 4000) and 15 for other trainers.
                int lastLevel=party.empty() ? 0 : static_cast<int>(party.back().level);
                int cash=lastLevel*(res.leader ? 100 : 60);
                if(cash>0)
                    out << "   <gain cash=\"" << cash << "\"/>\n";
                out << "  </step>\n </bot>\n";
                emitted=true;
            }
        }
        else if(res.kind==BotKind::Mart)
        {
            // Full seller bot: a text step offering Buy/Sell, the shop (buy) step
            // with the ROM's product list, and a sell step — matching the engine's
            // shop convention.  Ownership (martOwner_) already guarantees a single
            // seller per shop, so there are no duplicate sellers / phantom shops.
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
                out << " <bot id=\"" << b.id << "\">\n"
                    << "  <name><![CDATA[Seller]]></name>\n"
                    << "  <step type=\"text\" id=\"1\">\n"
                    << "   <text><![CDATA[Welcome!<br /><a href=\"2\">Buy</a><br /><a href=\"3\">Sell</a>]]></text>\n"
                    << "  </step>\n"
                    << "  <step type=\"shop\" id=\"2\">\n"
                    << body
                    << "  </step>\n"
                    << "  <step type=\"sell\" id=\"3\"/>\n"
                    << " </bot>\n";
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
        // SIGN or plain talking NPC -> the ROM text string, split into [Next]
        // pages (project owner's call to include the in-game text).  A sign also
        // gets a "Sign" name; an NPC keeps just its dialogue steps.
        if(!emitted)
        {
            std::vector<std::string> pages;
            uint32_t tptr=script_.signTextOffset(b.scriptPtr);
            if(tptr!=0)
                pages=Gen3Text::decodeSign(rom_,tptr,512);
            if(!pages.empty())
            {
                out << " <bot id=\"" << b.id << "\">\n";
                if(b.isSign)
                    out << "  <name><![CDATA[Sign]]></name>\n";
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
                // No decodable text and no fight/shop/heal/pc metadata: define
                // the bot (so the .tmx reference resolves) but emit NO step — an
                // empty <text> step is never written.
                out << " <bot id=\"" << b.id << "\"></bot>\n";
        }
        bi++;
    }
    out << "</map>\n";
}
