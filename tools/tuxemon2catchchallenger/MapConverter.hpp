#ifndef TUXEMON2CC_MAPCONVERTER_HPP
#define TUXEMON2CC_MAPCONVERTER_HPP

// Converts the Tuxemon Tiled maps (maps/*.tmx, 16px) into CatchChallenger maps
// under <out>/map/main/tuxemon/.  Tuxemon already ships proper external .tsx
// tilesets, so the tile gids are kept verbatim; the work is:
//   * decode each tile layer (csv or base64+zlib),
//   * rasterise the "Collisions" object-group rectangles to a blocked grid,
//   * re-emit each cell's tile on the CatchChallenger "Walkable" layer (passable
//     cells) or "Collisions" layer (blocked cells) so the engine's layer-name
//     collision model is satisfied, over-tiles on "WalkBehind",
//   * turn the "transition_teleport" events into CatchChallenger warp objects,
//   * copy the referenced tilesets (.tsx + image) once and repath them.
// Output tile layers use base64+zlib (the datapack convention).

#include "TuxemonDb.hpp"
#include "Localization.hpp"
#include "DatapackWriter.hpp"
#include "SkinGen.hpp"

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace tuxemon {

class MapConverter {
public:
    MapConverter(const std::string &modRoot, const std::string &outRoot,
                 const TuxemonDb &db, const DatapackWriter &dw,
                 const Localization &l10n, SkinGen &skins);
    // Convert every maps/*.tmx; returns the number of maps written.
    int convertAll();

    // A walkable start location chosen across the converted maps (for start.xml).
    const std::string &startMap() const { return startMap_; }
    int startX() const { return startX_; }
    int startY() const { return startY_; }

private:
    bool convertOne(const std::string &tmxPath);
    // Copy an externally-referenced tileset (.tsx + its image) into the shared
    // tileset dir, sanitising the filenames.  Returns the written (sanitised)
    // .tsx basename to reference from the map.
    std::string copyTileset(const std::string &tsxBasename);
    // Materialise an embedded (inline) <tileset> (no source attribute) as an
    // external .tsx + copied image, relative image source resolved against the
    // source map's directory.  Returns the written .tsx basename ("" on failure).
    std::string materializeInline(void *tilesetElement, const std::string &mapDirAbs);
    // Datapack filenames must be [a-z0-9._/-] only (the sync/checksum path drops
    // others).  Sanitise a basename and make it unique within the tileset dir.
    std::string uniqueTilesetFile(const std::string &origBasename);

    const TuxemonDb &db_;
    const DatapackWriter &dw_;
    const Localization &l10n_;
    SkinGen &skins_;
    std::string modRoot_;
    std::string outRoot_;
    std::string mapDir_;     // <out>/map/main/tuxemon
    std::string tilesetDir_; // <out>/map/main/tuxemon/tileset
    std::unordered_set<std::string> copiedTilesets_;
    std::unordered_map<std::string,std::string> tsxSan_;   // orig .tsx basename -> written name
    std::unordered_map<std::string,std::string> imgSan_;   // orig image basename -> written name
    std::unordered_set<std::string> usedTilesetFiles_;     // all written names (collision guard)
    std::unordered_map<std::string,std::pair<int,int> > mapDims_; // slug -> (w,h) for warp clamping
    int warpsTotal_;
    int collisionCells_;
    int botsTotal_;
    int encounterZones_;
    std::string startMap_;
    int startX_;
    int startY_;
    int startScore_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_MAPCONVERTER_HPP
