#ifndef GBA2CC_FULLWRITER_HPP
#define GBA2CC_FULLWRITER_HPP

// Writes a SELF-CONTAINED datapack DB (monsters/type.xml, monsters/skill/skill.xml,
// monsters/monster.xml, items/items.xml) at the datapack ROOT from the decoded
// Gen3 game data.  Used only in --all mode so the wild encounters / trainer
// parties the map writer emits by NAME resolve to real monsters.

#include "Gen3Data.hpp"
#include "Gba.hpp"

#include <string>

class FullWriter {
public:
    FullWriter(const GbaRom &rom, const Gen3Data &data, const std::string &outRoot);
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

    const GbaRom &rom_;
    const Gen3Data &data_;
    std::string outRoot_;
};

#endif // GBA2CC_FULLWRITER_HPP
