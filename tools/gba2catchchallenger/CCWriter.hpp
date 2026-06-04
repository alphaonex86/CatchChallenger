#ifndef GBA2CC_CCWRITER_HPP
#define GBA2CC_CCWRITER_HPP

// Writes the decoded maps out as a CatchChallenger datapack region: one .tmx
// per map (tile layers + Moving/Object groups), plus informations.xml,
// start.xml and per-group zone files.  Tile layers are emitted in the standard
// Tiled base64 + zstd encoding the rest of the datapack uses.

#include "Decoder.hpp"
#include "Gba.hpp"
#include "Gen3Script.hpp"
#include "Naming.hpp"
#include "SkinResolver.hpp"
#include "TilesetBuilder.hpp"
#include "Wild.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class CCWriter {
public:
    CCWriter(const GbaRom &rom,
             const Decoder &decoder,
             const TilesetBuilder &tilesets,
             const Naming &naming,
             const Wild &wild,
             const std::string &fireredDir,
             SkinResolver &skins);

    bool writeAll();

    // One bot placed on a map.  A skinned bot is a Gen3 NPC; a skinless bot is
    // a script sign (the "press A" text panels), which CatchChallenger models
    // as a bot with no skin.  Built once and shared by the .tmx object emission
    // and the .xml definition emission so the ids always match.
    struct MapBot {
        int id;
        int x;
        int y;
        std::string skin;     // "" => skinless sign (text-only)
        uint16_t trainerType; // for skinned NPC classification
        uint32_t scriptPtr;
        bool isSign;
    };

private:
    void writeMap(const DecodedMap &map);
    void writeMapXml(const DecodedMap &map);
    // Build the unified, id-stable bot list for a map (NPCs + script signs).
    std::vector<MapBot> collectBots(const DecodedMap &map);
    void writeInformations();
    void writeStart();
    void writeZones();

    // GID layer data -> base64(zstd(LE u32 grid)).
    std::string encodeLayer(const std::vector<uint32_t> &gids) const;
    // Relative path from the directory of fromPath to toPath (both relative to
    // the region root, no extension).
    static std::string relPath(const std::string &fromPath, const std::string &toPath);
    // Pick a CatchChallenger skin string for a Gen3 overworld graphics id:
    // extract the sprite, reuse/add via the resolver (cached per graphics id).
    std::string skinFor(uint8_t graphicsId);

    const GbaRom &rom_;
    const Decoder &decoder_;
    const TilesetBuilder &tilesets_;
    const Naming &naming_;
    const Wild &wild_;
    Gen3Script script_;
    std::string fireredDir_;
    SkinResolver &skins_;
    std::unordered_map<uint8_t,std::string> skinCache_;
};

#endif // GBA2CC_CCWRITER_HPP
