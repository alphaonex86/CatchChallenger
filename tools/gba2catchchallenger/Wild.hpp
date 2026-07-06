#ifndef GBA2CC_WILD_HPP
#define GBA2CC_WILD_HPP

// Decodes the wild-encounter tables (gWildMonHeaders) — the "monster on map"
// data: which species appear in grass/water, at what levels, with what rate.
// Species are emitted by their ROM name (the datapack keys monsters by name).

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct WildSlot {
    std::string species; // decoded species name ("" = empty slot)
    uint8_t minLevel;
    uint8_t maxLevel;
    int weight;          // per-slot probability weight (sums ~100 per list)
};

struct WildSet {
    std::vector<WildSlot> land;
    std::vector<WildSlot> water;
};

class Wild {
public:
    explicit Wild(const GbaRom &rom);

    void build();
    const WildSet *find(uint8_t group, uint8_t map) const;

private:
    std::vector<WildSlot> decodeInfo(uint32_t infoPtr, int count, const int *weights);

    const GbaRom &rom_;
    std::unordered_map<uint16_t,WildSet> sets_;
};

#endif // GBA2CC_WILD_HPP
