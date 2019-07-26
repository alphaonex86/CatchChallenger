#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "../../general/base/GeneralStructures.h"

class DatapackClientLoader
{
public:
    explicit DatapackClientLoader();
    ~DatapackClientLoader();
    void resetAll();

    //static items
    struct ItemExtra
    {
        std::string imagePath;
        std::string name;
        std::string description;
    };
    struct ReputationExtra
    {
        std::string name;
        std::vector<std::string> reputation_positive,reputation_negative;
    };

    enum QuestCondition
    {
        QuestCondition_queststep,
        QuestCondition_haverequirements
    };
    struct QuestConditionExtra
    {
        QuestCondition type;
        uint8_t value;
    };
    struct QuestStepWithConditionExtra
    {
        std::string text;
        std::vector<QuestConditionExtra> conditions;
    };
    struct QuestTextExtra
    {
        std::vector<QuestStepWithConditionExtra> texts;
    };
    struct QuestExtra
    {
        std::string name;
        std::unordered_map<uint32_t,QuestTextExtra> text;
        std::vector<std::string> steps;
        bool showRewards;
        bool autostep;
    };

    struct ZoneExtra
    {
        std::string name;
        std::unordered_map<std::string,std::string> audioAmbiance;
    };
    struct BotFightExtra
    {
        std::string start;
        std::string win;
    };
    struct MonsterExtra
    {
        std::string name;
        std::string description;
        std::string kind;
        std::string habitat;
        struct Buff
        {
            std::string name;
            std::string description;
        };
        struct Skill
        {
            std::string name;
            std::string description;
        };
        std::string frontPath,backPath;
    };
    struct ProfileText
    {
        std::string name;
        std::string description;
    };
    struct CCColor
    {
        int r,g,b,a;
    };
    struct VisualCategory
    {
        CCColor defaultColor;
        struct VisualCategoryCondition
        {
            uint8_t event;
            uint8_t eventValue;
            CCColor color;
        };
        std::vector<VisualCategoryCondition> conditions;
    };
    struct TypeExtra
    {
        std::string name;
        CCColor color;
    };
    struct PlantIndexContent
    {
        std::string map;
        uint8_t x,y;
    };
    std::unordered_map<uint8_t,TypeExtra> typeExtra;
    std::unordered_map<uint16_t,MonsterExtra> monsterExtra;
    std::unordered_map<uint8_t,MonsterExtra::Buff> monsterBuffsExtra;
    std::unordered_map<uint16_t,MonsterExtra::Skill> monsterSkillsExtra;
    std::unordered_map<uint16_t,ItemExtra> itemsExtra;
    std::unordered_map<std::string,ReputationExtra> reputationExtra;
    std::unordered_map<std::string,uint8_t> reputationNameToId;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::unordered_map<uint16_t,uint8_t> itemToPlants;
    std::unordered_map<uint16_t,QuestExtra> questsExtra;
    std::unordered_map<std::string,uint16_t> questsPathToId;
    std::unordered_map<uint16_t,std::vector<uint16_t> > botToQuestStart;
    std::unordered_map<uint16_t,BotFightExtra> botFightsExtra;
    std::unordered_map<std::string,ZoneExtra> zonesExtra;
    std::unordered_map<std::string,std::string> audioAmbiance;
    std::unordered_map<uint32_t,ProfileText> profileTextList;
    std::unordered_map<std::string,VisualCategory> visualCategories;
    std::string language;
    std::vector<std::string> maps,skins;
    ///todo drop the full path and .tmx
    std::unordered_map<std::string,uint32_t> mapToId;
    std::unordered_map<std::string,uint32_t> fullMapPathToId;
    std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> > itemOnMap;
    std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> > plantOnMap;
    std::unordered_map<uint16_t,PlantIndexContent> plantIndexOfOnMap;
    bool isParsingDatapack();
    std::string getDatapackPath();
    std::string getMainDatapackPath();
    std::string getSubDatapackPath();
public:
    void parseDatapack(const std::string &datapackPath);
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode);
    static CCColor namedColorToCCColor(const std::string &str,bool *ok);
protected:
    bool inProgress;
    std::string datapackPath;
    std::string mainDatapackCode;
    std::string subDatapackCode;

    //virtual void parsePlantsExtra();
    virtual void parseItemsExtra();
    //virtual void parseMonstersExtra();
    //virtual void parseBuffExtra();
private:
    virtual void emitdatapackParsed() = 0;
    virtual void emitdatapackParsedMainSub() = 0;
    virtual void emitdatapackChecksumError() = 0;
    virtual void parseTopLib() = 0;
    virtual std::string getLanguage() = 0;
private:
    void parseMaps();
    void parseVisualCategory();
    void parseTypesExtra();
    void parseSkillsExtra();
    void parseQuestsExtra();
    void parseQuestsText();
    void parseQuestsLink();
    void parseSkins();
    void parseBotFightsExtra();
    void parseAudioAmbiance();
    void parseZoneExtra();
    void parseReputationExtra();
    static std::string text_DATAPACK_BASE_PATH_MAPMAIN;
    static std::string text_DATAPACK_BASE_PATH_MAPSUB;
};

#endif // DATAPACKCLIENTLOADER_H
