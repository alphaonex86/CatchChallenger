#ifndef GBA2CC_GEN3SCRIPT_HPP
#define GBA2CC_GEN3SCRIPT_HPP

// Classifies an NPC's script into a functional bot kind (trainer fight, mart)
// and decodes the functional payload (trainer party = species+level,
// mart = item list).  No dialogue text is read.

#include "Gba.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct DecodedMap; // Decoder.hpp

enum class BotKind : uint8_t { None, Fight, Mart };

struct PartyMon {
    std::string species;
    uint8_t level;
};

struct ScriptResult {
    BotKind kind;
    uint16_t trainerId;            // valid when kind==Fight
    bool leader;                   // Fight: the trainer is a gym leader
    uint32_t introText;            // Fight: file offset of pre-battle text (0=none)
    uint32_t defeatText;           // Fight: file offset of defeat text (0=none)
    uint32_t matchOffset;          // Fight: file offset of the trainerbattle command
    std::vector<uint16_t> itemIds; // valid when kind==Mart
    ScriptResult();
};

class Gen3Script {
public:
    explicit Gen3Script(const GbaRom &rom);

    ScriptResult classify(uint16_t trainerType, uint32_t scriptOffset) const;
    // Pre-compute, across EVERY map's NPCs, the single owner of each trainerbattle
    // command: the NPC whose script starts closest before it.  classify() then
    // rejects a fight for any other NPC that only reaches the command by the scan
    // overrunning past its own (shorter) script.  Call once after decodeAll().
    static void indexBattleOwners(const GbaRom &rom, const std::vector<DecodedMap> &maps);
    // File offset of a sign script's displayed text (its loadpointer/msgbox
    // target), or 0 if none found.  Used only for SIGN bots, not NPC dialogue.
    uint32_t signTextOffset(uint32_t scriptOffset) const;
    // The item id when the script is the Gen3 item-ball "finditem" macro
    // (setorcopyvar VAR_0x8000,item; setorcopyvar VAR_0x8001,qty;
    // callstd STD_FIND_ITEM) — -1 otherwise.  Identifies ground item balls.
    static int findItemOf(const GbaRom &rom, uint32_t scriptOffset);
    std::vector<PartyMon> party(uint16_t trainerId) const;
    // Trainer's personal name (gTrainers[id]+0x04), Title-cased, or "" if blank.
    std::string trainerName(uint16_t trainerId) const;
    std::string itemName(uint16_t id) const;

private:
    bool looksLikeItemList(uint32_t off, std::vector<uint16_t> &items) const;
    // File offset of the first valid trainerbattle command in the script's linear
    // scan, or 0.  Sets *outType/*outTid (when non-null) to the battle type and
    // trainer id.  No ownership check — used by both indexBattleOwners and classify.
    uint32_t findBattle(uint32_t scriptOffset, uint8_t *outType, uint16_t *outTid) const;
    // File offset of the first valid mart command (0x86 + a plausible item list)
    // in the script's linear scan, or 0.  Fills outItems when non-null.
    uint32_t findMart(uint32_t scriptOffset, std::vector<uint16_t> *outItems) const;

    const GbaRom &rom_;
    // command offset -> owning NPC script start (global, set by indexBattleOwners).
    // Separate maps for fights and marts so a shop NPC and a trainer NPC that
    // happen to reach the same address for different command kinds don't clash.
    static std::unordered_map<uint32_t,uint32_t> battleOwner_;
    static std::unordered_map<uint32_t,uint32_t> martOwner_;
};

#endif // GBA2CC_GEN3SCRIPT_HPP
