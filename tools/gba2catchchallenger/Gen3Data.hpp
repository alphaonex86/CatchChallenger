#ifndef GBA2CC_GEN3DATA_HPP
#define GBA2CC_GEN3DATA_HPP

// Decodes the Gen3 GAME DATABASE (species base stats, moves, items, the type
// chart, evolutions and level-up learnsets) from a ROM.  Used by the --all mode
// to emit a SELF-CONTAINED datapack (monsters/skill/type/items), so the wild
// encounters and trainer parties the map writer emits by NAME actually resolve.
//
// Tables are AUTO-LOCATED by anchoring on known values (Bulbasaur's stats, the
// move "POUND", etc.) instead of hard-coded per-ROM offsets, so the same code
// works on every retail Gen3 ROM and verifies itself.

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct Gen3Species {
    int id;                 // internal species index (matches gSpeciesNames)
    std::string name;       // display name (matches the map wild/trainer refs)
    int hp, attack, defense, speed, spAttack, spDefense;
    int type1, type2;       // 0..17 Gen3 type ids
    int catchRate;
    int baseExp;
    int genderRatio;        // 0..254 (% of 255 that is female-ish), 255 genderless, 254 all female...
    int eggCycles;
    int growthRate;
    std::vector<std::pair<int,int> > learnset;   // (level, moveId)
    // evolutions: (kind, param, targetSpecies); kind: 0 none, 1 level, 2 item
    std::vector<std::pair<int,std::pair<int,int> > > evolutions;
    std::string kind;        // Pokedex category ("Seed"), "" when unavailable
    std::string description; // Pokedex flavour text, "" when unavailable
    std::vector<std::pair<int,int> > tmLearn;    // (TM/HM itemId, moveId)
};

struct Gen3Move {
    int id;
    std::string name;
    std::string description; // "" when unavailable
    int effect, power, type, accuracy, pp, priority;
};

struct Gen3Item {
    int id;
    std::string name;
    std::string description; // "" when unavailable
    int price;
};

// One directed matchup atk->def with a multiplier (0, 0.5 or 2).
struct Gen3TypeMatch {
    int atk, def;
    double mult;
};

class Gen3Data {
public:
    Gen3Data();
    // Decode everything; returns false if the core tables can't be located.
    bool decode(const GbaRom &rom);

    const std::vector<Gen3Species> &species() const { return species_; }
    const std::vector<Gen3Move> &moves() const { return moves_; }
    const std::vector<Gen3Item> &items() const { return items_; }
    const std::vector<Gen3TypeMatch> &typeChart() const { return typeChart_; }
    static const char *typeName(int t);  // 0..17 -> "normal".."dark"
    static int typeCount();

private:
    std::vector<Gen3Species> species_;
    std::vector<Gen3Move> moves_;
    std::vector<Gen3Item> items_;
    std::vector<Gen3TypeMatch> typeChart_;
};

#endif // GBA2CC_GEN3DATA_HPP
