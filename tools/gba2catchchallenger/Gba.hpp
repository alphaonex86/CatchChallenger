#ifndef GBA2CC_GBA_HPP
#define GBA2CC_GBA_HPP

// GbaRom: raw access to a Gen3 GBA ROM plus the per-game constants needed to
// walk its map data.  Everything trademark-specific (real region/town names)
// is deliberately kept out — maps are addressed by numeric group/map index.
//
// All the offsets/constants below were validated empirically against the two
// target ROMs (FireRed BPRE v1, Ruby AXVE v1); see the per-game table in
// GameInfo::detect().

#include <cstdint>
#include <string>
#include <vector>

// Layout of the per-game Tileset struct differs between the FireRed/LeafGreen
// engine and the Ruby/Sapphire/Emerald engine (the callback and the
// metatileAttributes pointers are swapped).
enum class Engine : uint8_t {
    Frlg, // FireRed / LeafGreen
    Rse   // Ruby / Sapphire / Emerald
};

struct GameInfo {
    bool valid;
    std::string code;        // 4-char game code, e.g. "BPRE"
    uint8_t version;         // ROM version byte
    std::string label;       // neutral output label, e.g. "firered"
    Engine engine;
    uint32_t mapGroupsOffset;    // file offset of gMapGroups
    uint32_t tilesInPrimary;     // 8x8 tiles owned by the primary tileset
    uint32_t metatilesInPrimary; // metatiles owned by the primary tileset
    uint32_t palettesInPrimary;  // palette slots filled by the primary tileset
    uint32_t owGfxPointers;      // gObjectEventGraphicsInfoPointers
    uint32_t owGfxCount;         // valid graphics ids 0..owGfxCount-1
    uint32_t owPalettes;         // gObjectEventSpritePalettes (tag-keyed)
    std::string region;          // top-level region folder ("kanto"/"hoenn")
    // Optional secondary region (a separate minimap): maps whose section id is
    // in [region2MinSid,region2MaxSid] go under region2 instead of region.
    // Empty region2 disables the split (single-region games).
    std::string region2;         // e.g. "sevii-islands"; "" if none
    uint8_t region2MinSid;
    uint8_t region2MaxSid;
    uint32_t mapNameTable;       // section-name table base
    uint32_t mapNameStride;      // bytes per entry (4 FRLG, 8 RSE)
    uint32_t mapNameField;       // name-pointer offset within the entry (0/4)
    uint32_t mapNameMinSid;      // first section id with a valid name
    uint32_t mapNameMaxSid;      // last section id with a valid name
    uint32_t wildTable;          // gWildMonHeaders
    uint32_t speciesNames;       // gSpeciesNames (stride 11)
    uint32_t speciesToNatDex;    // gSpeciesToNationalPokedexNumber (u16 array)
    uint32_t trainers;           // gTrainers (stride 0x28)
    uint32_t trainersCount;
    uint8_t leaderClass;         // gym-leader trainer class (0xFF = unknown/none)
    uint32_t itemNames;          // gItems name records (stride 44, name[14]@+0)
    uint16_t healSpecial;        // special id that heals the party
    // Primary-tileset water animation (tiles animWaterTile..+animWaterTileCount).
    uint16_t animWaterTile;      // first animated 8x8 tile index (508)
    uint16_t animWaterTileCount; // animated tiles (4)
    uint16_t animWaterFrames;    // number of frames to cycle
    uint16_t animWaterMs;        // ms per frame
    uint32_t animWaterArray;     // file offset of u32 frame-pointer array
    // gDoorAnimGraphicsTable: array of 12-byte DoorGraphics records
    // {u16 metatileNum, u8 sound, u8 size, u32 tilesPtr(raw 4bpp), u32 palettesPtr},
    // zero-record terminated.  0 when unknown.
    uint32_t doorTable;

    GameInfo();

    // Inspect a loaded ROM and fill in the per-game constants.  Returns a
    // GameInfo with valid==false when the ROM is not recognised.
    static GameInfo detect(const std::vector<uint8_t> &rom);

    // Metatile attribute decode (engine dependent).
    uint16_t behavior(uint32_t attribute) const;
    uint8_t layerType(uint32_t attribute) const;
    uint8_t attributeSize() const; // 2 (RSE u16) or 4 (FRLG u32)
    // Region folder for a map's section id (region2 when in range, else region).
    std::string regionFor(uint8_t sid) const;
};

class GbaRom {
public:
    GbaRom();

    // Load the ROM file; returns false (and logs) on failure.
    bool load(const std::string &path);

    const GameInfo &game() const;
    bool isValid() const;

    // Raw little-endian reads at a file offset.  No bounds checking beyond a
    // size assertion guard — callers stay inside the ROM by construction.
    uint8_t u8(uint32_t offset) const;
    uint16_t u16(uint32_t offset) const;
    uint32_t u32(uint32_t offset) const;
    int32_t s32(uint32_t offset) const;
    int16_t s16(uint32_t offset) const;

    // Convert a GBA pointer (0x08000000..0x09FFFFFF) to a file offset.
    // Returns false in *ok for NULL/out-of-range pointers.
    uint32_t pointer(uint32_t offset, bool *ok) const;

    // True when value looks like a ROM pointer.
    bool isPointer(uint32_t value) const;

    uint32_t size() const;

    // Direct access to the ROM bytes (for tight pixel loops).  Returns nullptr
    // when [offset,offset+length) would leave the ROM.
    const uint8_t *raw(uint32_t offset, uint32_t length) const;

    // GBA BIOS LZ77 (compression type 0x10) decompression of the block at
    // offset.  Returns the decompressed bytes; empty on malformed input.
    std::vector<uint8_t> lz77(uint32_t offset) const;

private:
    std::vector<uint8_t> data_;
    GameInfo game_;
};

#endif // GBA2CC_GBA_HPP
