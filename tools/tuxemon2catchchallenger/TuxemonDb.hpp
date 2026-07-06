#ifndef TUXEMON2CC_TUXEMONDB_HPP
#define TUXEMON2CC_TUXEMONDB_HPP

// In-memory representation of the Tuxemon "db" (the YAML game data) plus the
// shape stat table.  Only the fields the CatchChallenger datapack needs are
// kept; the rest of the (large) Tuxemon schema is ignored.  Loaded once from
// mods/tuxemon/db/ with yaml-cpp, then handed to DatapackWriter.

#include <string>
#include <vector>
#include <map>

#include <yaml-cpp/node/node.h>

namespace tuxemon {

// Body shape -> 6 base attributes on a 0..10 scale (shape/shapes.yaml).
struct Shape {
    int hp;
    int melee;
    int ranged;
    int armour;
    int dodge;
    int speed;
    Shape();
};

// One learnable move at a given trainer level.
struct MoveEntry {
    int level;            // level_learned (0 = starting move)
    std::string technique; // technique slug
    MoveEntry();
};

// One item that triggers an item evolution (Tuxemon `item:` is a {slug: prob} map).
struct EvolutionItem {
    std::string slug;     // item slug
    double prob;          // probability weight (informative)
    EvolutionItem();
};

// One evolution branch.
struct Evolution {
    int atLevel;          // at_level (>0) else 0
    std::string toSlug;   // monster_slug to evolve into
    std::vector<EvolutionItem> items; // item-triggered evolution (may coexist with atLevel)
    Evolution();
};

struct Monster {
    std::string slug;
    int txmnId;           // unique numeric id, reused as the CC monster id
    std::string species;
    std::vector<std::string> types; // element slugs (1 or 2 used)
    std::string shape;    // shape slug
    std::string stage;    // basic / stage1 / stage2
    int heightCm;         // height in cm (cosmetic)
    int weightKg;         // weight in kg (cosmetic)
    double catchRate;     // 0..100ish
    double lowerCatchResistance;
    double upperCatchResistance;
    double genderMale;    // gender weight
    double genderFemale;
    std::vector<std::string> tags;
    std::vector<std::string> terrains;
    std::vector<MoveEntry> moveset;
    std::vector<Evolution> evolutions;
    YAML::Node raw;
    Monster();
};

// A technique effect: {type, parameters[]}  (e.g. type=give params=[burn,enemy]).
struct Effect {
    std::string type;
    std::vector<std::string> params;
};

struct Technique {
    std::string slug;
    int techId;           // numeric id from the YAML
    double power;         // damage multiplier (~0.5..2)
    double accuracy;      // 0..1 (negative => unset, treat as 1)
    double potency;       // secondary-effect chance 0..1
    int recharge;         // turns to recharge
    std::string range;    // reliable / reach / ranged
    std::string sort;     // damage / meta / ...
    std::string category; // simple / basic / ...
    std::vector<std::string> types; // element slugs
    bool targetSelf;      // own_monster true
    std::vector<Effect> effects;
    YAML::Node raw;
    Technique();
};

struct Item {
    std::string slug;
    std::string category; // potion / capture / none ...
    std::string sort;     // utility / potion ...
    long cost;            // shop price (<0 => unset)
    std::string sprite;   // gfx/items/<x>.png (relative to the mod root)
    bool consumable;
    bool throwable;
    bool resellable;
    std::vector<Effect> effects;
    YAML::Node raw;
    Item();
};

// One directed matchup: this element attacking <against> deals <mult>x.
struct Matchup {
    std::string against;
    double mult;
};

struct Element {
    std::string slug;
    std::vector<Matchup> matchups;
    YAML::Node raw;
};

// One stat_modifiers entry of a status (e.g. speed {value:0.5, operation:'*'}).
struct StatModifier {
    std::string stat;     // hp/melee/ranged/armour/dodge/speed
    std::string op;       // operation ('*' is the only one used)
    double value;
    StatModifier();
};

struct Status {
    std::string slug;
    int condId;
    std::string category; // negative / positive / neutral
    std::string sort;     // meta / ...
    bool persists;        // persists_after_combat
    std::string stepType; // flat_damage / percent_damage / ""
    double stepValue;
    int stepInterval;
    std::vector<Effect> effects; // e.g. type=burnt params=[8,weakest]
    std::vector<StatModifier> statModifiers; // stat_modifiers map
    YAML::Node raw;
    Status();
};

// One wild-spawn entry of an encounter table.
struct EncounterMonster {
    std::string slug;
    int minLevel;
    int maxLevel;
    double rate;
    int daytime;          // 1 day-only, 0 night-only, -1 any
    EncounterMonster();
};

struct Encounter {
    std::string slug;
    std::vector<EncounterMonster> monsters;
};

// A Tuxemon NPC template (placed on maps via create_npc).
struct Npc {
    std::string slug;
    std::string spriteName;   // overworld sprite (sprites/<x>.png)
    std::string combatSheet;  // battle sheet (gfx/sprites/player/<x>.png)
};

// A shop product list (referenced by set_economy).
struct EconomyItem {
    std::string slug;
    int price;
};
struct Economy {
    std::string slug;
    std::vector<EconomyItem> items;
};

class TuxemonDb {
public:
    TuxemonDb();
    // modRoot = .../mods/tuxemon  (the folder holding db/, gfx/, l18n/, …)
    bool load(const std::string &modRoot);

    const std::vector<Monster>   &monsters()   const { return monsters_; }
    const std::vector<Technique> &techniques() const { return techniques_; }
    const std::vector<Item>      &items()      const { return items_; }
    const std::vector<Element>   &elements()   const { return elements_; }
    const std::vector<Status>    &statuses()   const { return statuses_; }
    const std::map<std::string,Shape> &shapes() const { return shapes_; }

    // NULL when the slug is unknown.
    const Encounter *encounter(const std::string &slug) const;
    const Npc *npc(const std::string &slug) const;
    const Economy *economy(const std::string &slug) const;

private:
    bool loadShapes(const std::string &file);
    void loadMonsters(const std::string &dir);
    void loadTechniques(const std::string &dir);
    void loadItems(const std::string &dir);
    void loadElements(const std::string &dir);
    void loadStatuses(const std::string &dir);
    void loadEncounters(const std::string &dir);
    void loadNpcs(const std::string &dir);
    void loadEconomy(const std::string &dir);

    std::vector<Monster>   monsters_;
    std::vector<Technique> techniques_;
    std::vector<Item>      items_;
    std::vector<Element>   elements_;
    std::vector<Status>    statuses_;
    std::map<std::string,Shape> shapes_;
    std::map<std::string,Encounter> encounters_;
    std::map<std::string,Npc> npcs_;
    std::map<std::string,Economy> economies_;
};

} // namespace tuxemon

#endif // TUXEMON2CC_TUXEMONDB_HPP
