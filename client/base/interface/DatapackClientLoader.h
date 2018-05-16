#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <string>
#include <vector>
#include <unordered_map>
#include <QIcon>
#include <QColor>
#include <utility>

#include "../../../general/base/GeneralStructures.h"
#include "../../tiled/tiled_tileset.h"

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    static DatapackClientLoader datapackLoader;
    void resetAll();

    //static items
    struct ItemExtra
    {
        std::string imagePath;
        QPixmap image;
        std::string name;
        std::string description;
    };
    struct PlantExtra
    {
        Tiled::Tileset * tileset;
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
    struct QuestStepExtra
    {
        std::vector<QuestStepWithConditionExtra> texts;
    };
    struct QuestExtra
    {
        std::string name;
        std::vector<QuestStepExtra> steps;
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
    struct QuestText
    {
        std::unordered_map<uint32_t,std::string> text;
    };
    struct MonsterExtra
    {
        std::string name;
        std::string description;
        std::string kind;
        std::string habitat;
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
        struct Buff
        {
            std::string name;
            std::string description;
            QIcon icon;
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
    struct VisualCategory
    {
        QColor defaultColor;
        struct VisualCategoryCondition
        {
            uint8_t event;
            uint8_t eventValue;
            QColor color;
        };
        std::vector<VisualCategoryCondition> conditions;
    };
    struct TypeExtra
    {
        std::string name;
        QColor color;
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
    std::unordered_map<uint8_t,PlantExtra> plantExtra;
    std::unordered_map<uint16_t,QuestExtra> questsExtra;
    std::unordered_map<uint16_t,QuestText> questsText;
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
    QPixmap defaultInventoryImage();
    bool isParsingDatapack();
    std::string getDatapackPath();
    std::string getMainDatapackPath();
    std::string getSubDatapackPath();
    QImage imagesInterfaceFightLabelBottom,imagesInterfaceFightLabelTop;
protected:
    void run();
public slots:
    void parseDatapack(const std::string &datapackPath);
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode);
signals:
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();
private:
    bool inProgress;
    std::string datapackPath;
    std::string mainDatapackCode;
    std::string subDatapackCode;
    QPixmap *mDefaultInventoryImage;
    explicit DatapackClientLoader();
    ~DatapackClientLoader();
private slots:
    void parsePlantsExtra();
    void parseItemsExtra();
    void parseMaps();
    void parseVisualCategory();
    void parseTypesExtra();
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseSkillsExtra();
    void parseQuestsExtra();
    void parseQuestsText();
    void parseQuestsLink();
    void parseSkins();
    void parseBotFightsExtra();
    void parseAudioAmbiance();
    void parseZoneExtra();
    void parseTileset();
    void parseReputationExtra();
protected:
    static const std::string text_list;
    static const std::string text_reputation;
    static const std::string text_type;
    static const std::string text_name;
    static const std::string text_en;
    static const std::string text_lang;
    static const std::string text_level;
    static const std::string text_point;
    static const std::string text_text;
    static const std::string text_id;
    static const std::string text_image;
    static const std::string text_description;
    static const std::string text_item;
    static const std::string text_slashdefinitiondotxml;
    static const std::string text_quest;
    static const std::string text_rewards;
    static const std::string text_show;
    static const std::string text_autostep;
    static const std::string text_yes;
    static const std::string text_true;
    static const std::string text_step;
    static const std::string text_bot;
    static const std::string text_dotcomma;
    static const std::string text_client_logic;
    static const std::string text_map;
    static const std::string text_items;
    static const std::string text_zone;
    static const std::string text_music;
    static const std::string text_backgroundsound;

    static const std::string text_monster;
    static const std::string text_monsters;
    static const std::string text_kind;
    static const std::string text_habitat;
    static const std::string text_slash;
    static const std::string text_types;
    static const std::string text_buff;
    static const std::string text_skill;
    static const std::string text_buffs;
    static const std::string text_skills;
    static const std::string text_fight;
    static const std::string text_fights;
    static const std::string text_start;
    static const std::string text_win;
    static const std::string text_dotxml;
    static const std::string text_dottsx;
    static const std::string text_visual;
    static const std::string text_category;
    static const std::string text_alpha;
    static const std::string text_color;
    static const std::string text_event;
    static const std::string text_value;
    static const std::string text_tileheight;
    static const std::string text_tilewidth;
    static const std::string text_x;
    static const std::string text_y;
    static const std::string text_object;
    static const std::string text_objectgroup;
    static const std::string text_Object;
    static const std::string text_layer;
    static const std::string text_Dirt;
    static const std::string text_DATAPACK_BASE_PATH_MAPBASE;
    static std::string text_DATAPACK_BASE_PATH_MAPMAIN;
    static std::string text_DATAPACK_BASE_PATH_MAPSUB;
};

#endif // DATAPACKCLIENTLOADER_H
