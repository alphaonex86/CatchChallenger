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

    // Sub-datapack overlay: emit ONLY what differs from the already-generated
    // main at mainDir (e.g. map/main/ruby).  The sub has no .tmx/tileset (geometry
    // is shared from main); it writes informations.xml plus, per map, a partial
    // <map>.xml carrying only the changed wild-encounter sections (<grass>/<water>)
    // — the version-exclusive Pokemon — matching map/main/test/sub/smallchange/.
    bool writeSubOverlay(const std::string &mainDir);

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
    // Render-based guard: reload every written .tmx with libtiled and render it
    // with all tile layers, then with each single tile layer hidden; a layer that
    // does not change the rendered pixels is invisible.  Ground truth for the
    // analytical layerVisibilityGuard.  Run once after every map is written.
    void renderVisibilityGuard();
    // Per-map layer-visibility guard: every tile layer must have >=1 cell not
    // hidden by a fully-opaque tile on a layer above it.  Accumulates violations.
    void layerVisibilityGuard(const DecodedMap &map,
                              const std::vector<uint32_t> &walkable,
                              const std::vector<uint32_t> &grass, bool anyGrass,
                              const std::vector<uint32_t> &water, bool anyWater,
                              const std::vector<uint32_t> &ledgeUp,
                              const std::vector<uint32_t> &ledgeDown,
                              const std::vector<uint32_t> &ledgeLeft,
                              const std::vector<uint32_t> &ledgeRight, bool anyLedge,
                              const std::vector<uint32_t> &collisions,
                              const std::vector<uint32_t> &walkbehind, bool anyOver);

    // GID layer data -> base64(zstd(LE u32 grid)).
    std::string encodeLayer(const std::vector<uint32_t> &gids) const;
    // Relative path from the directory of fromPath to toPath (both relative to
    // the region root, no extension).
    static std::string relPath(const std::string &fromPath, const std::string &toPath);
    // Pick a CatchChallenger skin string for a Gen3 overworld graphics id:
    // extract the sprite, reuse/add via the resolver (cached per graphics id).
    std::string skinFor(uint8_t graphicsId);
    // Write tileset/items.png|.tsx once (the 16x16 item-ball sprite ripped from
    // the ROM, like gen2's pokeball tile normal1.tsx#101) — the gid every
    // ground/hidden item object shows in the Tiled editor.
    void ensureItemTileset(uint8_t graphicsId);

    const GbaRom &rom_;
    const Decoder &decoder_;
    const TilesetBuilder &tilesets_;
    const Naming &naming_;
    const Wild &wild_;
    Gen3Script script_;
    std::string fireredDir_;
    SkinResolver &skins_;
    std::unordered_map<uint8_t,std::string> skinCache_;
    int guardLayers_;                          // tile layers checked
    int guardMasked_;                          // layers with no visible cell
    std::vector<std::string> guardMaskedList_; // first few offending layers
    int guardTopMaps_;                         // maps whose top layer was checked
    int guardTopCover_;                        // top layer = full 100%-opaque cover
    std::vector<std::string> guardTopCoverList_;
    std::vector<std::string> writtenTmx_;      // abs paths of every .tmx written
    int renderLayers_;                         // tile layers rendered-tested
    int renderInvisible_;                      // layers whose hide changed nothing
    std::vector<std::string> renderInvisibleList_;
    bool itemTilesetWritten_;                  // tileset/items.tsx emitted
    int itemsTotal_;                           // ground+hidden items emitted
};

#endif // GBA2CC_CCWRITER_HPP
