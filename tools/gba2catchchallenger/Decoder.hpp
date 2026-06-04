#ifndef GBA2CC_DECODER_HPP
#define GBA2CC_DECODER_HPP

// Gen3 map decoder: walks gMapGroups and turns every map into a plain
// DecodedMap struct (geometry + tileset pointers + warps/connections/NPCs).
// No rendering or output here — just structured data the rest of the tool
// consumes.

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Per-cell terrain classification derived from the metatile behaviour byte.
enum class Terrain : uint8_t {
    Normal,
    Grass,
    Water,
    LedgeUp,
    LedgeDown,
    LedgeLeft,
    LedgeRight
};

struct DecodedWarp {
    uint8_t x;
    uint8_t y;
    uint8_t destGroup;
    uint8_t destMap;
    uint8_t destWarpId;
};

struct DecodedConnection {
    uint8_t direction; // 1 down, 2 up, 3 left, 4 right (5 dive, 6 emerge)
    int32_t offset;
    uint8_t destGroup;
    uint8_t destMap;
};

struct DecodedNpc {
    uint8_t localId;
    uint8_t graphicsId;
    int16_t x;
    int16_t y;
    uint8_t movementType;
    uint16_t trainerType;
    uint32_t scriptPtr; // file offset of the script, 0 when none
    uint16_t flag;
};

struct DecodedSign {
    int16_t x;
    int16_t y;
    uint8_t kind;
    uint32_t scriptPtr; // file offset when kind is a script sign
    uint16_t itemId;    // when kind is a hidden item
};

struct DecodedMap {
    uint8_t group;
    uint8_t map;
    int32_t width;
    int32_t height;
    uint32_t blocksPtr;      // file offset of the width*height u16 block grid
    uint32_t primaryTileset;  // file offset of the primary Tileset struct
    uint32_t secondaryTileset;// file offset of the secondary Tileset struct (0 if none)
    uint8_t regionSection;
    uint8_t mapType;        // MapHeader+0x17 (1 town,2 city,3 route,8 indoor,...)
    uint16_t music;
    std::vector<DecodedWarp> warps;
    std::vector<DecodedConnection> connections;
    std::vector<DecodedNpc> npcs;
    std::vector<DecodedSign> signs;

    DecodedMap();

    // Relative datapack path (without extension), e.g. "group-03/map-00".
    std::string relativePath() const;
};

class Decoder {
public:
    explicit Decoder(const GbaRom &rom);

    // Decode every map.  Returns false when gMapGroups looks wrong.
    bool decodeAll();

    const std::vector<DecodedMap> &maps() const;

    // Look up a map by (group,map); returns nullptr when absent.
    const DecodedMap *find(uint8_t group, uint8_t map) const;

    // Terrain classification for a metatile behaviour value.
    Terrain terrain(uint16_t behavior) const;
    // Warp object class for a warp tile's metatile behaviour: "door" (you face
    // and open it), "teleport on push" (arrow/edge warp, you walk into it) or
    // "teleport on it" (step-on pad / stairs / ladder / hole).
    std::string warpType(uint16_t behavior) const;
    // Raw metatile behaviour at a map cell's metatile (engine-dependent attribute
    // decode), and the warp class of the tile a warp sits on.
    uint16_t metatileBehavior(const DecodedMap &m, uint16_t metatile) const;
    std::string warpClassAt(const DecodedMap &m, const DecodedWarp &w) const;

private:
    DecodedMap decodeMap(uint8_t group, uint8_t map, uint32_t headerOffset);
    void decodeEvents(uint32_t eventsOffset, DecodedMap &out);
    void decodeConnections(uint32_t connectionsOffset, DecodedMap &out);
    bool looksLikeLayout(uint32_t layoutOffset) const;

    const GbaRom &rom_;
    std::vector<DecodedMap> maps_;
    std::unordered_map<uint16_t,size_t> index_; // (group<<8|map) -> maps_ index
};

#endif // GBA2CC_DECODER_HPP
