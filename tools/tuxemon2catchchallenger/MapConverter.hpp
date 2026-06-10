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

#include <string>
#include <unordered_set>

namespace tuxemon {

class MapConverter {
public:
    MapConverter(const std::string &modRoot, const std::string &outRoot);
    // Convert every maps/*.tmx; returns the number of maps written.
    int convertAll();

    // A walkable start location chosen across the converted maps (for start.xml).
    const std::string &startMap() const { return startMap_; }
    int startX() const { return startX_; }
    int startY() const { return startY_; }

private:
    bool convertOne(const std::string &tmxPath);
    // Copy an externally-referenced tileset (.tsx + its image) into the shared
    // tileset dir.
    void copyTileset(const std::string &tsxBasename);
    // Materialise an embedded (inline) <tileset> (no source attribute) as an
    // external .tsx + copied image, relative image source resolved against the
    // source map's directory.  Returns the written .tsx basename ("" on failure).
    std::string materializeInline(void *tilesetElement, const std::string &mapDirAbs);

    std::string modRoot_;
    std::string outRoot_;
    std::string mapDir_;     // <out>/map/main/tuxemon
    std::string tilesetDir_; // <out>/map/main/tuxemon/tileset
    std::unordered_set<std::string> copiedTilesets_;
    int warpsTotal_;
    int collisionCells_;
    std::string startMap_;
    int startX_;
    int startY_;
    int startScore_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_MAPCONVERTER_HPP
