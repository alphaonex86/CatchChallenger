#ifndef GBA2CC_FULLWRITER_HPP
#define GBA2CC_FULLWRITER_HPP

// Writes a SELF-CONTAINED datapack DB (monsters/type.xml, monsters/skill/skill.xml,
// monsters/monster.xml, items/items.xml) at the datapack ROOT from the decoded
// Gen3 game data.  Used only in --all mode so the wild encounters / trainer
// parties the map writer emits by NAME resolve to real monsters.
// OWNER RULE: --all only FILLS GAPS at the root — an existing non-empty file
// (a curated items library, hand-made plants/buffs/layers/start...) is KEPT;
// only monsters/monster.xml + skill.xml + sprites are converter-owned.  When
// the target defines its own items, item refs (TM byitem, evolution items)
// resolve BY NAME into the target's ids via the ItemResolver.

#include "CCWriter.hpp"
#include "Gen3Data.hpp"
#include "Gba.hpp"

#include <string>

class FullWriter {
public:
    FullWriter(const GbaRom &rom, const Gen3Data &data, const std::string &outRoot,
               const ItemResolver &items);
    bool writeAll();

private:
    bool writeTypes();
    bool writeSkills();
    bool writeMonsters();
    bool writeItems();
    void writeSprites();        // missing sprites are non-fatal (counted + reported)
    bool writeCompleteness();   // start/reputation/event/layers/visualcategory/...
    bool writePlayerSkin();
    int firstSpeciesId() const; // a valid starter species id

    // Target item id for a gen3 item id: the gen3 id itself when
    // self-contained, else the name-resolved target id (-1 = unresolved).
    int targetItemId(int gen3Id) const;
    // True when the target defines its own monster DB (any monsters/*.xml
    // besides our monster.xml with <monster> entries) — its numbering and art
    // then own monsters/skills/sprites; maps reference species by NAME.
    bool targetCuratesMonsters() const;

    const GbaRom &rom_;
    const Gen3Data &data_;
    std::string outRoot_;
    const ItemResolver &items_;
};

#endif // GBA2CC_FULLWRITER_HPP
