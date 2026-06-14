#ifndef QUESTSTORE_HPP
#define QUESTSTORE_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Persistent storage of the quests of every mainDatapackCode of the
// datapack. The engine loaders (CommonDatapackServerSpec/loadQuests and
// DatapackClientLoader questsExtra) only keep data for the currently
// parsed main, truncate quest ids to uint8_t and drop quests whose
// bot="<id>" attribute has no map part — which drops ALL official quests.
// So, like MapStore, after each call to parseDatapackMainSub() we snapshot
// the quests of that main here, but from a dedicated XML parse of
// map/main/<mainCode>/quests/<id>/definition.xml that mirrors the old PHP
// generator semantics (full uint16 ids, map-less bot ids, en texts).
namespace QuestStore {

struct QuestItem
{
    uint16_t itemId;        // resolved item id (name or numeric attribute)
    bool itemOk;            // itemId is valid
    uint32_t quantity;      // default 1
    bool hasMonster;        // item is dropped by a monster
    uint16_t monsterId;     // first monster of the monster= attribute
    uint8_t rate;           // drop luck in percent (rate= attribute)
};

struct QuestStep
{
    std::string text;               // en step text
    std::vector<QuestItem> items;   // one entry per <item> element
    bool hasBot;                    // step has its own bot= attribute
    uint32_t botId;
    std::string mapTmxPath;         // resolved "dir/file.tmx" path, may be empty
};

struct ReputationRequirement
{
    std::string type;       // reputation code ("nation", ...)
    uint8_t level;
    bool positif;
};

struct ReputationReward
{
    std::string type;       // reputation code
    int32_t point;
};

struct RewardItem
{
    uint16_t itemId;
    bool itemOk;
    uint32_t quantity;
};

struct Quest
{
    uint16_t id;            // real id from the folder name (can be >=256)
    std::string name;       // en name from definition.xml
    bool repeatable;
    bool hasBot;            // quest has a starting bot id
    uint32_t botId;         // datapack-global bot id (bot= attribute)
    std::string mapTmxPath; // starting map if derivable from the quest xml
    std::vector<uint16_t> requirementQuests;
    std::vector<ReputationRequirement> requirementReputation;
    std::vector<QuestStep> steps;
    std::vector<RewardItem> rewardItems;
    std::vector<ReputationReward> rewardReputation;
    bool allowCreateClan;
};

// Best-effort bot identity, found by scanning the map xml files of the
// main for <bot id="..."> definitions matching the ids quests reference.
struct BotInfo
{
    std::string name;       // en name, may be empty
    std::string mapTmxPath; // map whose xml defines the bot
    bool ambiguous;         // same id defined in several maps: do not trust
};

struct MainCodeSet
{
    std::string mainCode;
    std::vector<Quest> quests;              // sorted by id
    std::map<uint32_t,BotInfo> bots;        // referenced bot id -> info
};

// Snapshot the quests of the currently parsed main. Must be called while
// the loader still holds this main's state (map list used to resolve the
// map= attributes). Calling it again with the same mainCode is a no-op.
void addFromCurrentLoader(const std::string &mainCode);

const std::vector<MainCodeSet> &sets();

// nullptr when the main code is unknown
const MainCodeSet *setForMainCode(const std::string &mainCode);

// nullptr when the quest is unknown in this set
const Quest *questById(const MainCodeSet &set, uint16_t id);

} // namespace QuestStore

#endif // QUESTSTORE_HPP
