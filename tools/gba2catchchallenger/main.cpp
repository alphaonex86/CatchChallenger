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
