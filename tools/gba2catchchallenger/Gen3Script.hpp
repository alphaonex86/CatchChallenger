#ifndef GBA2CC_GEN3SCRIPT_HPP
#define GBA2CC_GEN3SCRIPT_HPP

// Classifies an NPC's script into a functional bot kind (trainer fight, mart,
// heal, PC) and decodes the functional payload (trainer party = species+level,
// mart = item list).  No dialogue text is read.

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <vector>

enum class BotKind : uint8_t { None, Fight, Mart, Heal, Pc };

struct PartyMon {
    std::string species;
    uint8_t level;
};

struct ScriptResult {
    BotKind kind;
    uint16_t trainerId;            // valid when kind==Fight
    std::vector<uint16_t> itemIds; // valid when kind==Mart
    ScriptResult();
};

class Gen3Script {
public:
    explicit Gen3Script(const GbaRom &rom);

    ScriptResult classify(uint16_t trainerType, uint32_t scriptOffset) const;
    // File offset of a sign script's displayed text (its loadpointer/msgbox
    // target), or 0 if none found.  Used only for SIGN bots, not NPC dialogue.
    uint32_t signTextOffset(uint32_t scriptOffset) const;
    std::vector<PartyMon> party(uint16_t trainerId) const;
    std::string itemName(uint16_t id) const;
    std::string speciesName(uint16_t internalId) const;

private:
    bool looksLikeItemList(uint32_t off, std::vector<uint16_t> &items) const;

    const GbaRom &rom_;
};

#endif // GBA2CC_GEN3SCRIPT_HPP
