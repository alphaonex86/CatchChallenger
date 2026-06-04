#ifndef GBA2CC_NAMING_HPP
#define GBA2CC_NAMING_HPP

// Derives human folder/file names and the region/town/building/floor hierarchy
// for every map, from the ROM's section-name table + map types + the warp
// graph.  Outdoor maps (towns/routes) become <region>/<area>/<area>; indoor
// maps are grouped under their parent town via warps ("Generic + grouped").

#include "Decoder.hpp"
#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Naming {
public:
    Naming(const GbaRom &rom, const Decoder &decoder);

    void build();

    // Output path relative to the <label>/ root, no extension
    // (e.g. "kanto/pallet-town/pallet-town", "kanto/viridian-city/building-1/floor-0").
    const std::string &pathFor(uint8_t group, uint8_t map) const;
    // Area slug for the metadata zone attribute ("pallet-town"); "" if none.
    const std::string &zoneFor(uint8_t group, uint8_t map) const;
    // Display name for the metadata <name> ("Pallet Town").
    const std::string &displayFor(uint8_t group, uint8_t map) const;
    // Metadata type: "city" / "outdoor" / "indoor".
    std::string typeFor(uint8_t group, uint8_t map) const;
    // Set of distinct area zones that own at least one map (for zone/*.xml).
    const std::vector<std::pair<std::string,std::string> > &zones() const; // (slug, display)

private:
    static uint16_t keyOf(uint8_t group, uint8_t map);
    std::string sectionName(uint8_t sid) const; // decoded display name, "" if none
    bool isArea(const DecodedMap &m) const;
    // A small generic interior (<=12x10) with a teleport-on-push/it exit on the
    // bottom rows and no shop / trainer-fight bot — i.e. a plain "house".
    bool looksLikeHouse(const DecodedMap &m) const;
    // The map hosts a gym-leader trainer (a Fight bot of the leader class).
    bool hasGymLeader(const DecodedMap &m) const;
    // Section id of the named area a map belongs to (its own when it is a named
    // area map, else the nearest named area reachable by warps); -1 if none.
    int namedSidOf(uint16_t key) const;

    const GbaRom &rom_;
    const Decoder &decoder_;
    std::unordered_map<uint16_t,std::vector<uint16_t> > adj_; // warp adjacency
    std::unordered_map<uint16_t,std::string> path_;
    std::unordered_map<uint16_t,std::string> zone_;
    std::unordered_map<uint16_t,std::string> display_;
    std::vector<std::pair<std::string,std::string> > zones_;
    std::string empty_;
};

#endif // GBA2CC_NAMING_HPP
