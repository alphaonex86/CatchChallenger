// gba2catchchallenger — convert a Gen3 GBA ROM into a CatchChallenger datapack
// region (maps + rendered tilesets).  Trademark-free: maps are addressed by
// numeric group/map index, no in-game proper names are emitted.
//
//   gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>
//
// Output goes to <datapack-root>/map/main/<label>/ with tilesets under
// <label>/tileset/ (label is "firered", "ruby", ... derived from the ROM).

#include "CCWriter.hpp"
#include "DatapackBase.hpp"
#include "Decoder.hpp"
#include "Gba.hpp"
#include "Gen3Script.hpp"
#include "Naming.hpp"
#include "SkinResolver.hpp"
#include "TilesetBuilder.hpp"
#include "Wild.hpp"

#include <QDir>
#include <QGuiApplication>
#include <iostream>
#include <string>
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
                    uint16_t id=rom.u16(m.blocksPtr+static_cast<uint32_t>(y*m.width+x)*2)&0x3FF;
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

    if(datapack.empty() || gba.empty())
    {
        std::cerr << "Usage: gba2catchchallenger --datapack <datapack-root> --gba <rom.gba>" << std::endl;
        return 1;
    }

    GbaRom rom;
    if(!rom.load(gba))
    {
        std::cerr << "Failed to load/identify ROM: " << gba << std::endl;
        return 1;
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

    // Correct the tileset split for expanded-tileset hacks (no-op on retail ROMs):
    // decides primary/secondary metatile routing, so it must run before tilesets.
    autodetectTilesetSplit(rom,decoder.maps());

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

    if(gi.isSub())
    {
        // Sub overlay: no .tmx / tileset / skins — geometry is shared from the
        // main; only the changed wild-encounter sections are written.  TilesetBuilder
        // is constructed but never prepared (unused by the overlay path).
        TilesetBuilder tilesets(rom,tilesetDir);
        CCWriter writer(rom,decoder,tilesets,naming,wild,outDir,skins);
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
        CCWriter writer(rom,decoder,tilesets,naming,wild,outDir,skins);
        writer.writeAll();
        std::cout << "Skins: reused " << skins.reuseCount() << ", added " << skins.addedCount() << std::endl;
    }

    std::cout << "Done." << std::endl;
    return 0;
}
