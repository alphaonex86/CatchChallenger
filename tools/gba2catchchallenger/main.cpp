// gba2catchchallenger — convert a Gen3 GBA ROM into a CatchChallenger datapack
// region (maps + rendered tilesets).  Trademark-free: maps are addressed by
// numeric group/map index, no in-game proper names are emitted.
//
//   gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>
//
// Output goes to <datapack-root>/map/main/<label>/ with tilesets under
// <label>/tileset/ (label is "firered", "ruby", ... derived from the ROM).

#include "CCWriter.hpp"

#include <tinyxml2.h>
#include <unordered_set>
#include <cctype>
#include "Gen3Data.hpp"
#include "FullWriter.hpp"
#include "M4aRipper.hpp"
#include "GsfWriter.hpp"
#include "DatapackBase.hpp"
#include "Decoder.hpp"
#include "Gba.hpp"
#include "Gen3Script.hpp"
#include "Gen3Text.hpp"
#include "Naming.hpp"
#include "SkinResolver.hpp"
#include "TilesetBuilder.hpp"
#include "Wild.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QGuiApplication>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

static std::string argValue(const std::vector<std::string> &args, const std::string &key)
{
    size_t i=0;
    while(i+1<args.size())
    {
        if(args[i]==key)
            return args[i+1];
        i++;
    }
    return std::string();
}

// Presence-only flag (no value).
static bool argFlag(const std::vector<std::string> &args, const std::string &key)
{
    size_t i=0;
    while(i<args.size()) { if(args[i]==key) return true; i++; }
    return false;
}

// Run pngquant (lossy palette) then zopflipng (lossless deflate) over every PNG
// under dir, when the tools are installed (else skip with a note).  Shrinks the
// flat-colour ROM tiles a lot (like tools/datapack-explorer-generator-cli).  Runs
// AFTER the guards so it never affects them.  Uses QProcess with an explicit argv
// (NO shell) so a datapack path with odd characters can't break or inject.
static void optimizePngs(const std::string &dir)
{
    QStringList pngs;
    QDirIterator it(QString::fromStdString(dir), QStringList() << "*.png",
                    QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
        pngs << it.next();
    if(pngs.isEmpty())
        return;
    if(QFile::exists("/usr/bin/pngquant"))
    {
        std::cout << "Optimizing tileset PNGs (pngquant)..." << std::flush;
        int i=0;
        while(i<pngs.size())
        {
            QStringList a;
            a << "--force" << "--skip-if-larger" << "--ext" << ".png" << "--quality" << "65-95" << pngs.at(i);
            QProcess::execute("/usr/bin/pngquant", a);
            i++;
        }
        std::cout << " done" << std::endl;
    }
    else
        std::cout << "no /usr/bin/pngquant — install it for smaller tilesets" << std::endl;
    if(QFile::exists("/usr/bin/zopflipng"))
    {
        std::cout << "Optimizing tileset PNGs (zopflipng)..." << std::flush;
        int i=0;
        while(i<pngs.size())
        {
            QStringList a;
            a << "-y" << pngs.at(i) << pngs.at(i);
            QProcess::execute("/usr/bin/zopflipng", a);
            i++;
        }
        std::cout << " done" << std::endl;
    }
    else
        std::cout << "no /usr/bin/zopflipng — install zopfli for smaller tilesets" << std::endl;
}

// True when the metatile at (base,idx) has no graphics (every subtile id is 0).
static bool metatileEmpty(const GbaRom &rom, uint32_t base, uint32_t idx)
{
    int s=0;
    while(s<8)
    {
        if((rom.u16(base+idx*16+static_cast<uint32_t>(s)*2)&0x3FF)!=0)
            return false;
        s++;
    }
    return true;
}

static std::string upcaseStr(const std::string &s)
{
    std::string out=s;
    size_t i=0;
    while(i<out.size())
    {
        if(out[i]>='a' && out[i]<='z')
            out[i]=static_cast<char>(out[i]-'a'+'A');
        i++;
    }
    return out;
}

// True when a section name's KIND matches a map's type — a town/city map names a
// settlement, a route names a ROUTE, a cave/underground names a cave-like place.
// Used to score a candidate section-name table against the maps' own types.
static bool nameTypeMatches(const std::string &name, uint8_t mapType)
{
    std::string u=upcaseStr(name);
    if(mapType==1 || mapType==2)
        return u.find(" TOWN")!=std::string::npos || u.find(" CITY")!=std::string::npos
            || u.find("ISLAND")!=std::string::npos || u.find("VILLAGE")!=std::string::npos
            || u.find("RESORT")!=std::string::npos || u.find("PLATEAU")!=std::string::npos;
    if(mapType==3)
        return u.find("ROUTE ")!=std::string::npos || u.find(" PATH")!=std::string::npos;
    if(mapType==4)
        return u.find("CAVE")!=std::string::npos || u.find("TUNNEL")!=std::string::npos
            || u.find("WELL")!=std::string::npos || u.find("FOREST")!=std::string::npos
            || u.find("MT")!=std::string::npos || u.find("PATH")!=std::string::npos;
    return false;
}

// Recover a RELOCATED section-name table for an off-fingerprint hack whose
// detect() left it unknown (mapNameTable==0).  The table is a stride-8 array of
// pointers (name pointer at +4 — the Gen3 layout) to area-name strings; a
// relocated copy is located by scanning for the base whose entries best match the
// maps' OWN types (a town map must name a "...TOWN/CITY", a route a "ROUTE ...",
// etc.), so a wrong base or a generic string pool scores ~0.  Once set, the
// existing Naming logic yields real area names AND grouping (HnS's 844-map
// "area-0" overworld blob splits into New Bark Town, the routes, ...).
static void recoverMapNameTable(GbaRom &rom, const std::vector<DecodedMap> &maps)
{
    if(rom.game().mapNameTable!=0)
        return; // retail ROM / sub already has its table
    std::map<std::pair<uint8_t,uint8_t>,int> usage; // (sid,mapType) -> map count
    uint8_t maxSid=0;
    size_t i=0;
    while(i<maps.size())
    {
        usage[std::make_pair(maps[i].regionSection,maps[i].mapType)]++;
        if(maps[i].regionSection>maxSid)
            maxSid=maps[i].regionSection;
        i++;
    }
    const uint32_t stride=8, field=4;
    uint32_t romSize=rom.size();
    uint32_t best=0;
    int bestScore=0;
    uint32_t o=0x100000; // tables live in the data region, well past the header/code
    while(o+16<romSize)
    {
        bool ok0=false;
        uint32_t p0=rom.pointer(o+field,&ok0);
        // cheap filter: the first two entries must both be valid names
        if(ok0 && !Gen3Text::strictName(rom,p0).empty())
        {
            bool ok1=false;
            uint32_t p1=rom.pointer(o+stride+field,&ok1);
            if(ok1 && !Gen3Text::strictName(rom,p1).empty())
            {
                int score=0;
                std::map<std::pair<uint8_t,uint8_t>,int>::const_iterator it=usage.begin();
                while(it!=usage.end())
                {
                    bool okn=false;
                    uint32_t np=rom.pointer(o+static_cast<uint32_t>(it->first.first)*stride+field,&okn);
                    if(okn)
                    {
                        std::string nm=Gen3Text::strictName(rom,np);
                        if(!nm.empty() && nameTypeMatches(nm,it->first.second))
                            score+=it->second;
                    }
                    ++it;
                }
                if(score>bestScore)
                {
                    bestScore=score;
                    best=o;
                }
            }
        }
        o+=4;
    }
    if(best!=0 && bestScore>=8)
    {
        rom.setMapNameTable(best,stride,field,0,maxSid);
        std::cout << "Section-name table recovered at 0x" << std::hex << best << std::dec
                  << " (type-match score " << bestScore << ", sids 0.." << (int)maxSid
                  << ") -> real area names" << std::endl;
    }
}

// Auto-detect the primary/secondary tileset split (tiles/metatiles/palettes) from
// the map data.  The engine default (RSE 512/512/6, FRLG 640/640/7) is wrong for a
// ROM hack that kept one engine's MAP format but expanded its TILESETS to the
// other's size — e.g. HnS = Emerald maps + FRLG-size 640 tilesets.  A wrong
// metatilesInPrimary routes high-id metatiles to an empty array slot, so the cell
// renders as a backdrop "black hole" (a whole building lost).  We decode every map
// cell's metatile under each standard split and pick the one with the FEWEST
// all-empty metatiles; the split is overridden only on a STRICT improvement, so
// retail ROMs (already correct) are never touched.
static void autodetectTilesetSplit(GbaRom &rom, const std::vector<DecodedMap> &maps)
{
    static const uint32_t kCandidate[2]={512,640};
    uint32_t emptyCells[2]={0,0};
    size_t mi=0;
    while(mi<maps.size())
    {
        const DecodedMap &m=maps[mi];
        bool ok=false;
        uint32_t pmeta=rom.pointer(m.primaryTileset+0x0C,&ok);
        if(!ok)
            pmeta=0;
        uint32_t smeta=0;
        if(m.secondaryTileset!=0)
        {
            bool sok=false;
            uint32_t s=rom.pointer(m.secondaryTileset+0x0C,&sok);
            if(sok)
                smeta=s;
        }
        if(pmeta!=0 && m.width>0 && m.height>0)
        {
            int32_t y=0;
            while(y<m.height)
            {
                int32_t x=0;
                while(x<m.width)
                {
                    uint16_t id=m.blockAt(rom,static_cast<uint32_t>(x),static_cast<uint32_t>(y))&0x3FF;
                    int c=0;
                    while(c<2)
                    {
                        uint32_t np=kCandidate[c];
                        uint32_t base; uint32_t idx;
                        if(id<np) { base=pmeta; idx=id; }
                        else { base=smeta; idx=id-np; }
                        if(base!=0 && metatileEmpty(rom,base,idx))
                            emptyCells[c]++;
                        c++;
                    }
                    x++;
                }
                y++;
            }
        }
        mi++;
    }
    // index 1 (640) wins only when STRICTLY fewer empty cells than index 0 (512).
    int best=(emptyCells[1]<emptyCells[0]) ? 1 : 0;
    uint32_t curMeta=rom.game().metatilesInPrimary;
    if(kCandidate[best]!=curMeta && emptyCells[best]<emptyCells[best^1])
    {
        if(kCandidate[best]==640)
            rom.setTilesetSplit(640,640,7);
        else
            rom.setTilesetSplit(512,512,6);
        std::cout << "Tileset split auto-detected: metatilesInPrimary " << curMeta
                  << " -> " << kCandidate[best] << " (empty-cell count "
                  << emptyCells[best^1] << " -> " << emptyCells[best]
                  << "; expanded-tileset hack)" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM","offscreen");
    QGuiApplication app(argc,argv);

    std::vector<std::string> args;
    int i=1;
    while(i<argc)
    {
        args.push_back(std::string(argv[i]));
        i++;
    }

    std::string datapack=argValue(args,"--datapack");
    std::string gba=argValue(args,"--gba");
    if(gba.empty())
        gba=argValue(args,"--gda"); // accept the documented alias

    // --all <path>: write a SELF-CONTAINED datapack — the maps as usual PLUS the
    // full game DB (monsters/skill/type/items + music) at the datapack root.
    const std::string allPath=argValue(args,"--all");
    const bool fullDatapack=!allPath.empty();
    if(fullDatapack && datapack.empty())
        datapack=allPath;

    // --gsf: emit the maps' BGM as a standard GSF set (one <label>.gsflib + a
    // .minigsf per song) instead of opus.  Plays in external GSF players AND in
    // our client (which decodes the gsflib's ROM with the software MP2k synth).
    const bool gsfMode=argFlag(args,"--gsf");

    const std::string ripSong=argValue(args,"--ripsong");
    if(datapack.empty() && ripSong.empty())
    {
        std::cerr << "Usage: gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>" << std::endl;
        std::cerr << "       gba2catchchallenger --all <datapack-root> --gba <rom.gba>   (whole datapack)" << std::endl;
        return 1;
    }
    if(gba.empty())
    {
        std::cerr << "Missing --gba <rom.gba>" << std::endl;
        return 1;
    }

    GbaRom rom;
    if(!rom.load(gba))
    {
        std::cerr << "Failed to load/identify ROM: " << gba << std::endl;
        return 1;
    }

    // --ripsong <id>: debug — render one song to /tmp/song-<id>.opus and exit.
    if(!ripSong.empty())
    {
        M4aRipper m4a;
        if(!m4a.locate(rom)) { std::cerr << "no song table" << std::endl; return 1; }
        const int id=std::atoi(ripSong.c_str());
        const std::string out="/tmp/song-"+ripSong+".opus";
        std::cerr << "rip song " << id << " -> " << (m4a.writeOpus(rom,id,out) ? out : std::string("FAILED")) << std::endl;
        return 0;
    }
    std::cout << "ROM: " << rom.game().code << " v" << (int)rom.game().version
              << " -> label '" << rom.game().label << "'" << std::endl;

    // Load the base datapack via the client loader (skins for bot mapping).
    DatapackBase base;
    base.load(datapack);

    Decoder decoder(rom);
    if(!decoder.decodeAll())
    {
        std::cerr << "Map decoding failed" << std::endl;
        return 1;
    }

    // Engine-fit normalisation: resolve warp targets to coordinates, keep one
    // reciprocal connection per map side, split >127-tile maps into chunks.
    decoder.finalizeMaps();

    // Correct the tileset split for expanded-tileset hacks (no-op on retail ROMs):
    // decides primary/secondary metatile routing, so it must run before tilesets.
    autodetectTilesetSplit(rom,decoder.maps());

    // Recover a relocated section-name table for a hack (no-op on retail ROMs):
    // gives maps their real names + grouping, so it must run before Naming.
    recoverMapNameTable(rom,decoder.maps());

    // Resolve, across all maps, the single owner of each trainerbattle command so
    // overrunning NPCs/signs don't duplicate a gym leader (one leader per gym).
    Gen3Script::indexBattleOwners(rom,decoder.maps());

    // Output placement.  A base/standalone goes to map/main/<label>/.  A sibling
    // sub overlay goes to map/main/<mainCode>/sub/<subCode>/ and is diffed against
    // the already-generated main at map/main/<mainCode>/.
    const GameInfo &gi=rom.game();
    std::string mainDir=datapack+"/map/main/"+gi.mainCode;
    std::string outDir=gi.isSub() ? (mainDir+"/sub/"+gi.subCode)
                                   : (datapack+"/map/main/"+gi.label);
    std::string tilesetDir=outDir+"/tileset";

    // Wipe any previous output for this label/sub first: the tileset sheet count
    // and layout change between runs, so old <label>/tileset/*.png|*.tsx and maps
    // would otherwise accumulate as stale files (bloating the datapack and
    // showing tiles no current map references).  Guarded on a non-empty label so
    // this can never escape to map/main/ itself.
    if(!gi.label.empty())
    {
        if(QDir(QString::fromStdString(outDir)).removeRecursively())
            std::cout << "Cleaned previous output at " << outDir << std::endl;
    }

    // Naming first: the tileset builder names pools after the areas that use
    // them, so it needs the resolved area slugs.  The sub run uses the SAME
    // naming as its main (same section names + indexBattleOwners above), so the
    // sibling's map paths line up with the main's for the overlay diff.
    Naming naming(rom,decoder);
    naming.build();

    Wild wild(rom);
    wild.build();

    std::string skinBotDir=datapack;
    if(!skinBotDir.empty() && skinBotDir.back()=='/')
        skinBotDir.pop_back();
    skinBotDir+="/skin/bot";
    SkinResolver skins(skinBotDir,26);

    // A complete main carries the standard per-main folders even when empty, so
    // the datapack loader (and tools like datapack-explorer-generator) don't error
    // on a missing quests/ directory.  Subs inherit these from their main.
    if(!gi.isSub())
        QDir().mkpath(QString::fromStdString(outDir+"/quests"));

    // Item-on-map references (see ItemResolver): the ROM item NAMES drive the
    // resolution — numeric id when self-contained (--all), the base datapack's
    // id on a name match, else the lowercase name itself (visible in Tiled,
    // auto-resolves once the datapack's items.xml defines the item).
    Gen3Data gen3;
    const bool gen3ok=gen3.decode(rom);
    ItemResolver itemResolver;
    itemResolver.selfContained=(fullDatapack && gen3ok);
    {
        std::size_t ii=0;
        while(ii<gen3.items().size())
        {
            const Gen3Item &it=gen3.items()[ii];
            ii++;
            std::string lname=it.name;
            std::size_t ci=0;
            while(ci<lname.size())
            {
                lname[ci]=static_cast<char>(std::tolower(static_cast<unsigned char>(lname[ci])));
                ci++;
            }
            itemResolver.gen3Name[static_cast<uint16_t>(it.id)]=lname;
        }
    }
    if(!itemResolver.selfContained)
    {
        // base datapack item names, keyed the way the engine keys
        // tempNameToItemId: lowercase default-language <name>
        tinyxml2::XMLDocument d;
        if(d.LoadFile((datapack+"/items/items.xml").c_str())==tinyxml2::XML_SUCCESS && d.RootElement()!=nullptr)
        {
            tinyxml2::XMLElement *it=d.RootElement()->FirstChildElement("item");
            while(it!=nullptr)
            {
                const int id=it->IntAttribute("id",-1);
                tinyxml2::XMLElement *name=it->FirstChildElement("name");
                while(name!=nullptr && name->Attribute("lang")!=nullptr)
                    name=name->NextSiblingElement("name");
                if(id>=0 && name!=nullptr && name->GetText()!=nullptr)
                {
                    std::string lname=name->GetText();
                    std::size_t ci=0;
                    while(ci<lname.size())
                    {
                        lname[ci]=static_cast<char>(std::tolower(static_cast<unsigned char>(lname[ci])));
                        ci++;
                    }
                    itemResolver.baseByName[lname]=static_cast<uint16_t>(id);
                }
                it=it->NextSiblingElement("item");
            }
        }
    }

    if(gi.isSub())
    {
        // Sub overlay: no .tmx / tileset / skins — geometry is shared from the
        // main; only the changed wild-encounter sections are written.  TilesetBuilder
        // is constructed but never prepared (unused by the overlay path).
        TilesetBuilder tilesets(rom,tilesetDir);
        CCWriter writer(rom,decoder,tilesets,naming,wild,outDir,skins,itemResolver);
        writer.writeSubOverlay(mainDir);
    }
    else
    {
        TilesetBuilder tilesets(rom,tilesetDir);
        if(!tilesets.prepare(decoder.maps(),naming))
        {
            std::cerr << "Tileset build failed" << std::endl;
            return 1;
        }
        // Bot skins: extracted overworld sprites are matched against existing skins
        // (~10% per-channel tolerance, content-cropped) and reused, else added.
        skins.loadExisting();

        // Decide the music format BEFORE writing the maps, so each map's
        // backgroundsound ref matches what we actually produce.  --gsf builds a
        // standard GSF set ONLY if the engine functions + ROM free space are
        // located; otherwise fall back to opus, so a map never references a
        // .minigsf we couldn't write.
        M4aRipper m4a;
        const bool haveMusic=m4a.locate(rom);
        GsfWriter gsf;
        bool gsfReady=false;
        std::string musicExt="opus";
        if(gsfMode && haveMusic && gsf.prepare(rom,m4a)) { gsfReady=true; musicExt="minigsf"; }
        else if(gsfMode)
            std::cerr << "--gsf: could not build a GSF set (engine functions / ROM free space"
                         " not found); falling back to opus" << std::endl;

        CCWriter writer(rom,decoder,tilesets,naming,wild,outDir,skins,itemResolver);
        writer.setMusicExtension(musicExt);
        writer.writeAll();
        std::cout << "Skins: reused " << skins.reuseCount() << ", added " << skins.addedCount() << std::endl;

        // Rip the maps' background music into THIS label's own folder
        // map/main/<label>/music/<name>.<ext> (named after a representative map).
        if(haveMusic)
        {
            std::map<uint16_t,int> use;
            std::map<std::string,std::map<uint16_t,int> > byType;
            size_t mi=0;
            while(mi<decoder.maps().size())
            {
                const uint16_t mu=decoder.maps()[mi].music;
                const uint8_t mt=decoder.maps()[mi].mapType;
                if(mu!=0 && mu!=0xFFFF)
                {
                    use[mu]++;
                    std::string ct = (mt==1||mt==2)?"city":((mt>=8)?"indoor":"outdoor");
                    byType[ct][mu]++;
                }
                ++mi;
            }
            const std::string musicDir=outDir+"/music";
            QDir().mkpath(QString::fromStdString(musicDir));
            std::vector<std::pair<int,uint16_t> > order;
            std::map<uint16_t,int>::const_iterator it=use.begin();
            while(it!=use.end()) { order.push_back(std::make_pair(-it->second,it->first)); ++it; }
            std::sort(order.begin(),order.end());

            // --gsf (gsfReady): write the one shared gsflib for the label now;
            // each song then gets a tiny minigsf.  gsf was already prepared above.
            std::string libRel; // <label>.gsflib, dir-relative for the _lib tag
            if(gsfReady)
            {
                libRel=gi.label+".gsflib";
                gsf.writeLib(musicDir+"/"+libRel,gi.label);
            }
            int ripped=0; const int cap=64; size_t oi=0;
            while(oi<order.size() && ripped<cap)
            {
                const uint16_t id=order[oi].second; ++oi;
                const std::string out=musicDir+"/"+writer.musicFileBase(id)+"."+musicExt;
                bool wrote=false;
                if(gsfReady)
                    wrote = !libRel.empty() && gsf.writeMinigsf(out,id,libRel,writer.musicFileBase(id));
                else
                    wrote = m4a.writeOpus(rom,id,out,16.0);
                if(wrote) ++ripped;
            }
            std::cout << "Music: ripped " << ripped << "/" << use.size() << " "
                      << (gsfReady?"minigsf":"opus") << " song(s) to " << musicDir << std::endl;
            // type-fallback map/music.xml (dominant ripped BGM per coarse type).
            // Refs are label-root-relative ("music/<name>.<ext>"), resolved by the
            // client as datapackPathMain()+ref = "<label>/music/<name>.<ext>".
            QDir().mkpath(QString::fromStdString(datapack+"/map"));
            std::ofstream mx((datapack+"/map/music.xml").c_str());
            mx << "<musics>\n";
            const char *types[]={"city","indoor","outdoor","cave"};
            int ty=0;
            while(ty<4)
            {
                const std::string key = std::string(types[ty])=="cave"?"outdoor":types[ty];
                uint16_t best=0; int bc=-1;
                std::map<uint16_t,int>::const_iterator b=byType[key].begin();
                while(b!=byType[key].end()) { if(b->second>bc && QFile::exists(QString::fromStdString(musicDir+"/"+writer.musicFileBase(b->first)+"."+musicExt))) { bc=b->second; best=b->first; } ++b; }
                if(best!=0) mx << "    <map type=\"" << types[ty] << "\">" << writer.musicRefPrefix() << "/" << writer.musicFileBase(best) << "." << musicExt << "</map>\n";
                ++ty;
            }
            mx << "</musics>\n";
        }

        // Shrink the tileset PNGs (pngquant + zopflipng) — after the guards.
        optimizePngs(tilesetDir);
    }

    // --all: also emit the full game DB (monsters/skill/type/items) at the
    // datapack ROOT so the maps' by-name wild/trainer/shop references resolve.
    if(fullDatapack)
    {
        if(gen3ok)
        {
            FullWriter full(rom,gen3,datapack);
            full.writeAll();
        }
        else
            std::cerr << "Warning: --all could not decode the Gen3 game DB; wrote maps only." << std::endl;
    }

    std::cout << "Done." << std::endl;
    return 0;
}
