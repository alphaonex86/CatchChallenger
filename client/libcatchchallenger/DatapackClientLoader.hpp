#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "../../general/base/lib.h"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/GeneralType.hpp"

class DLL_PUBLIC DatapackClientLoader
{
public:
    explicit DatapackClientLoader();
    ~DatapackClientLoader();
    void resetAll();

    //static items
    class ItemExtra
    {
    public:
        std::string imagePath;
        std::string name;
        std::string description;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << imagePath << name << description;
        }
        template <class B>
        void parse(B& buf) {
            buf >> imagePath >> name >> description;
        }
        #endif
    };
    class ReputationExtra
    {
    public:
        std::string name;
        std::vector<std::string> reputation_positive,reputation_negative;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << name << reputation_positive << reputation_negative;
        }
        template <class B>
        void parse(B& buf) {
            buf >> name >> reputation_positive >> reputation_negative;
        }
        #endif
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
        ZONE_TYPE id;
        std::string name;
        std::unordered_map<std::string,std::string/*relative to main datapack or base datapack*/> audioAmbiance;
    };
    struct BotFightExtra
    {
        std::string start;
        std::string win;
    };
    class MonsterExtra
    {
    public:
        std::string name;
        std::string description;
        std::string kind;
        std::string habitat;
        std::string frontPath,backPath;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << name << description << kind << habitat << frontPath << backPath;
        }
        template <class B>
        void parse(B& buf) {
            buf >> name >> description >> kind >> habitat >> frontPath >> backPath;
        }
        #endif
        class Buff
        {
        public:
            std::string name;
            std::string description;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << name << description;
            }
            template <class B>
            void parse(B& buf) {
                buf >> name >> description;
            }
            #endif
        };
        class Skill
        {
        public:
            std::string name;
            std::string description;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << name << description;
            }
            template <class B>
            void parse(B& buf) {
                buf >> name >> description;
            }
            #endif
        };
    };
    struct ProfileText
    {
        std::string name;
        std::string description;
    };
    class CCColor
    {
    public:
        int r,g,b,a;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << r << g << b << a;
        }
        template <class B>
        void parse(B& buf) {
            buf >> r >> g >> b >> a;
        }
        #endif
    };
    class VisualCategory
    {
    public:
        CCColor defaultColor;
        class VisualCategoryCondition
        {
        public:
            uint8_t event;
            uint8_t eventValue;
            CCColor color;
            #ifdef CATCHCHALLENGER_CACHE_HPS
            template <class B>
            void serialize(B& buf) const {
                buf << event << eventValue << color;
            }
            template <class B>
            void parse(B& buf) {
                buf >> event >> eventValue >> color;
            }
            #endif
        };
        std::vector<VisualCategoryCondition> conditions;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << defaultColor << conditions;
        }
        template <class B>
        void parse(B& buf) {
            buf >> defaultColor >> conditions;
        }
        #endif
    };
    class TypeExtra
    {
    public:
        std::string name;
        CCColor color;
        #ifdef CATCHCHALLENGER_CACHE_HPS
        template <class B>
        void serialize(B& buf) const {
            buf << name << color;
        }
        template <class B>
        void parse(B& buf) {
            buf >> name >> color;
        }
        #endif
    };
    struct PlantIndexContent
    {
        std::string map;
        uint8_t x,y;
    };
public:
    const std::unordered_map<uint8_t,TypeExtra> &get_typeExtra() const;
    const std::unordered_map<uint16_t,MonsterExtra> &get_monsterExtra() const;
    const std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,MonsterExtra::Buff> &get_monsterBuffsExtra() const;
    const std::unordered_map<CATCHCHALLENGER_TYPE_SKILL,MonsterExtra::Skill> &get_monsterSkillsExtra() const;
    const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,ItemExtra> &get_itemsExtra() const;
    const std::unordered_map<std::string,ReputationExtra> &get_reputationExtra() const;
    const std::unordered_map<std::string,uint8_t> &get_reputationNameToId() const;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint8_t> &get_itemToPlants() const;
    const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST,QuestExtra> &get_questsExtra() const;
    const std::unordered_map<std::string,uint16_t> &get_questsPathToId() const;
    const std::unordered_map<uint16_t,std::vector<CATCHCHALLENGER_TYPE_QUEST> > &get_botToQuestStart() const;
    const std::unordered_map<uint16_t,BotFightExtra> &get_botFightsExtra() const;
    const std::unordered_map<std::string,ZoneExtra> &get_zonesExtra() const;
    const std::unordered_map<std::string,std::string> &get_audioAmbiance() const;
    const std::unordered_map<uint32_t,ProfileText> &get_profileTextList() const;
    const std::unordered_map<std::string,VisualCategory> &get_visualCategories() const;
    const std::string &get_language() const;
    const std::vector<std::string> &get_maps() const;
    const std::vector<std::string> &get_skins() const;
    ///todo drop the full path and .tmx
    const std::unordered_map<std::string,uint32_t> &get_mapToId() const;
    const std::unordered_map<std::string,uint32_t> &get_fullMapPathToId() const;
    const std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > &get_itemOnMap() const;
    const std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > &get_plantOnMap() const;
    const std::unordered_map<uint16_t,PlantIndexContent> &get_plantIndexOfOnMap() const;
protected:
    std::unordered_map<uint8_t,TypeExtra> typeExtra;
    std::unordered_map<uint16_t,MonsterExtra> monsterExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,MonsterExtra::Buff> monsterBuffsExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_SKILL,MonsterExtra::Skill> monsterSkillsExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,ItemExtra> itemsExtra;
    std::unordered_map<std::string,ReputationExtra> reputationExtra;
    std::unordered_map<std::string,uint8_t> reputationNameToId;//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint8_t> itemToPlants;
    std::unordered_map<CATCHCHALLENGER_TYPE_QUEST,QuestExtra> questsExtra;
    std::unordered_map<std::string,uint16_t> questsPathToId;
    std::unordered_map<uint16_t,std::vector<CATCHCHALLENGER_TYPE_QUEST> > botToQuestStart;
    std::unordered_map<uint16_t,BotFightExtra> botFightsExtra;
    std::unordered_map<std::string,ZoneExtra> zonesExtra;
    std::unordered_map<std::string,std::string/*relative to main datapack or base datapack*/> audioAmbiance;
    std::unordered_map<uint32_t,ProfileText> profileTextList;
    std::unordered_map<std::string,VisualCategory> visualCategories;
    std::string language;
    std::vector<std::string> skins;
    /*std::vector<std::string> maps;
    ///todo drop the full path and .tmx
    std::unordered_map<std::string,uint32_t> mapToId;
    std::unordered_map<std::string,uint32_t> fullMapPathToId;
    std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > itemOnMap;
    std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > plantOnMap;
    std::unordered_map<uint16_t,PlantIndexContent> plantIndexOfOnMap;*/
public:
    bool isParsingDatapack() const;
    std::string getDatapackPath() const;
    std::string getMainDatapackPath() const;
    std::string getSubDatapackPath() const;
    void parseDatapack(const std::string &datapackPath, const std::string &cacheHash=std::string(), const std::string &language="en");
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode, const std::string &cacheHashMain=std::string(), const std::string &cacheHashBase=std::string());
    static CCColor namedColorToCCColor(const std::string &str,bool *ok);
    static std::vector<std::string> listFolderNotRecursive(const std::string& folder,const std::string& suffix);
protected:
    bool inProgress;
    std::string datapackPath;
    std::string mainDatapackCode;
    std::string subDatapackCode;
    static std::string text_DATAPACK_BASE_PATH_MAPMAIN;
    static std::string text_DATAPACK_BASE_PATH_MAPSUB;

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
};

#endif // DATAPACKCLIENTLOADER_H
