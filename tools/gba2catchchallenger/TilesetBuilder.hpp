#ifndef GBA2CC_TILESETBUILDER_HPP
#define GBA2CC_TILESETBUILDER_HPP

// Renders Gen3 metatiles into de-duplicated CatchChallenger tilesets.
//
// Each unique primary tileset and each (primary,secondary) pairing gets a "tile
// pool": every metatile actually used by a map is rendered to a 16x16 image
// (ground + the lifted "over" overlay), IDENTICAL images are stored once, and
// the unique tiles are packed into small fixed-size sheets (256 tiles =
// 256x256 px each).  Unused metatile slots are never emitted and unfilled sheet
// cells stay transparent, so there are no repeated tiles and no wasted area.
// A map references only the sheets of its primary + secondary pool (plus the
// marker), well under the 15-tileset budget.

#include "Decoder.hpp"
#include "Gba.hpp"

#include <QImage>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Naming;

// One decoded tileset pair: graphics, palettes, metatile + attribute arrays.
class Gen3Tileset {
public:
    Gen3Tileset(const GbaRom &rom, uint32_t primaryPtr, uint32_t secondaryPtr);

    QImage renderMetatile(uint16_t id) const;             // both sub-layers
    QImage renderUnder(uint16_t id) const;                // bottom sub-layer
    QImage renderOver(uint16_t id, const QImage &under) const; // top overlay
    uint16_t behavior(uint16_t id) const;
    // Metatile layer type (0 NORMAL, 1 COVERED, 2 SPLIT): decides whether the
    // top sub-layer draws above the player (NORMAL/SPLIT) or below (COVERED).
    uint8_t layerType(uint16_t id) const;
    // True when the metatile uses a tile slot of a LOADED ambient animation
    // (water/flower/shore edge/... — see GameInfo::ambientAnims).
    bool isAmbientAnimated(uint16_t id) const;
    // Frame count to render for an ambient-animated metatile: the max frames
    // among the sequences its subtiles touch (each shorter sequence cycles by
    // modulo inside renderMetatileFrame).  0 for a non-animated metatile.
    uint16_t ambientFrameCount(uint16_t id) const;
    // GUARD signal: metatile `id` is routed to the SECONDARY array
    // (id>=metatilesInPrimary) where it decodes EMPTY, yet the PRIMARY array holds
    // real graphics at the same absolute id — the signature of a wrong
    // metatilesInPrimary split, so the metatile renders as a backdrop "black hole".
    bool metatileMisrouted(uint16_t id) const;
    // Composite the metatile with the ambient-animated tiles swapped to
    // animation frame f (each sequence cycles by modulo of its own length).
    QImage renderMetatileFrame(uint16_t id, int frame) const;
    // Door-open animation frames (16x16) for a door metatile via the ROM's
    // gDoorAnimGraphicsTable: frame 0 = closed (the metatile), then the open
    // states.  Empty when the metatile is not in the door table.
    std::vector<QImage> renderDoorFrames(uint16_t id) const;

private:
    // DEFINED metatile count of the primary tileset: the metatile array is
    // immediately followed by its attribute array, so it is (attrPtr-metaPtr)/16.
    // primary[id] beyond this is out-of-bounds (reads the attribute array).
    uint32_t primaryMetatileCount() const;
    void drawSubtile(QImage &dst, int px, int py, uint16_t entry) const;
    const uint8_t *tilePixels(uint32_t tileIndex) const;
    uint32_t paletteColor(uint8_t palNum, uint8_t colorIndex) const;
    uint32_t metatileEntryOffset(uint16_t id, uint8_t subtile) const;
    uint32_t attributeOffset(uint16_t id) const;

    const GbaRom &rom_;
    const GameInfo &game_;
    uint32_t primaryPtr_;
    uint32_t secondaryPtr_;
    uint32_t primaryTilesPtr_;
    uint32_t secondaryTilesPtr_;
    uint32_t primaryPalPtr_;
    uint32_t secondaryPalPtr_;
    uint32_t primaryMetaPtr_;
    uint32_t secondaryMetaPtr_;
    uint32_t primaryAttrPtr_;
    uint32_t secondaryAttrPtr_;
    bool primaryCompressed_;
    bool secondaryCompressed_;
    std::vector<uint8_t> primaryTiles_;
    std::vector<uint8_t> secondaryTiles_;
    // ambientFrames_[seq][frame] = raw 4bpp gfx of GameInfo::ambientAnims[seq]
    // frame `frame`; a sequence with an unreadable pointer stays EMPTY (inactive).
    std::vector<std::vector<std::vector<uint8_t> > > ambientFrames_;
    mutable int currentAnimFrame_;                   // -1 = use base graphics
};

// A de-duplicated pool of unique tiles, packed into N fixed-capacity sheets.
struct TilePool {
    std::string baseName;     // sheet files are baseName + "_" + sheetIndex
    std::string subDir;       // region subfolder under tileset/ ("" = root)
    uint32_t uniqueCount;     // number of unique tiles
    uint32_t mainCount;       // ground+over tiles (== uniqueCount; anims live in the shared global tileset)
    uint32_t sheetCount;      // number of emitted sheet files
    uint32_t duplicateTiles;  // GUARD: redundant non-animation duplicate cells
    uint32_t adjacencyViolations; // GUARD: consistent map-neighbours not kept adjacent
    uint32_t tinyTiles;         // GUARD: tiles with 1..12 visible px (near-empty, rest transparent)
    uint32_t bgFgSplits;        // Pass-1b background/foreground splits applied (feature count)
    uint32_t layerSplitTiles;   // tiles split across 2 layers (under+over) the guard verified
    uint32_t layerSplitBad;     // GUARD: such tiles whose under+over != the ROM metatile
    uint32_t misroutedMetatiles; // GUARD: used metatiles empty in their routed array but present in the other (wrong primary/secondary split)
    std::unordered_map<uint16_t,int> groundCell; // metatile -> static ground cell (doors included; animated water absent)
    std::unordered_map<uint16_t,int> overCell;    // metatile -> pool cell, -1 none
    std::unordered_map<uint16_t,int> animGlobal;  // water/door metatile -> GLOBAL anim index
    TilePool();
};

class TilesetBuilder {
public:
    TilesetBuilder(const GbaRom &rom, const std::string &tilesetDir);
    ~TilesetBuilder();

    bool prepare(const std::vector<DecodedMap> &maps, const Naming &naming);

    // GID of the ground tile at map cell (x,y): the de-duplicated ground tile
    // (a DOOR keeps its STATIC ground tile), or the cell's animated tile for water.
    uint32_t groundGid(const DecodedMap &map, int x, int y) const;
    uint32_t overGid(const DecodedMap &map, uint16_t metatile) const;
    // GID of a door metatile's animated tile (for the door object) or 0.
    uint32_t doorGid(const DecodedMap &map, uint16_t metatile) const;
    uint32_t markerGid(const DecodedMap &map) const;
    // (firstgid, tsx base name) per sheet the map must reference (no marker).
    std::vector<std::pair<uint32_t,std::string> > tilesetRefs(const DecodedMap &map) const;

    uint16_t behavior(const DecodedMap &map, uint16_t metatileId) const;
    // True when the metatile's lifted "over" (WalkBehind) tile is fully opaque
    // (every pixel alpha 255) — i.e. it would completely hide a layer below it.
    // Used by the CCWriter per-layer visibility guard.
    bool overOpaque(const DecodedMap &map, uint16_t metatileId) const;

    static const uint32_t kCapacity;  // tiles per sheet
    static const int kColumns;

private:
    TilePool buildPool(uint32_t primaryPtr, uint32_t secondaryPtr,
                       const std::vector<uint16_t> &usedIds, const std::string &baseName,
                       const std::vector<const DecodedMap *> &poolMaps,
                       const std::string &subDir);
    Gen3Tileset &tilesetFor(const DecodedMap &map) const;
    static uint64_t pairKey(uint32_t primary, uint32_t secondary);
    const TilePool *primaryPool(const DecodedMap &map) const;
    const TilePool *secondaryPool(const DecodedMap &map) const;
    uint32_t secondaryBase(const DecodedMap &map) const;
    // First gid of the ONE global animation tileset for a map (after its primary +
    // secondary pools).  Water/door cells reference it; the marker follows it.
    uint32_t animBase(const DecodedMap &map) const;
    // Register an animation (its frames + "<ms>ms;<n>frames") in the global anim
    // collection (de-duplicated by frame content, laid out 1 animation per row);
    // returns the GLOBAL index of its first frame.
    // ambient = loops on its own (water/flowers/lava): gets a standard Tiled
    // <animation> in the .tsx so the editor previews it.  A door does NOT (it
    // only animates on player action).
    int registerGlobalAnim(const std::vector<QImage> &frames, const std::string &animStr, bool ambient);
    void emitGlobalAnim();

    const GbaRom &rom_;
    std::string tilesetDir_;
    std::unordered_map<uint32_t,TilePool> primaryPools_;   // by primary ptr
    std::unordered_map<uint64_t,TilePool> secondaryPools_; // by pair key
    std::vector<QImage> globalAnims_;                      // ONE shared anim tileset
    std::unordered_map<std::string,int> globalAnimSeen_;   // frame-sequence -> first index
    std::unordered_map<int,std::string> globalAnimStr_;    // first index -> "<ms>ms;<n>frames"
    std::unordered_set<int> globalAnimAmbient_;             // first indices that self-loop (not doors)
    int globalAnimCols_;                                   // anim sheet width (1 anim/row)
    bool ioError_;                                         // a sheet/.tsx/dir write failed
    mutable std::map<uint64_t,Gen3Tileset *> cache_;
    mutable std::map<std::pair<uint64_t,uint16_t>,bool> overOpaqueCache_;
};

#endif // GBA2CC_TILESETBUILDER_HPP
