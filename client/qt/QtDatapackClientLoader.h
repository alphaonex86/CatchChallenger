#ifndef QtDATAPACKCLIENTLOADER_H
#define QtDATAPACKCLIENTLOADER_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#include <QPixmap>
#include <QHash>
#include <string>
#include <vector>
#include <unordered_map>
#include <QIcon>
#include <QColor>
#include <utility>

#include "../../general/base/GeneralStructures.h"
#include "../libcatchchallenger/DatapackClientLoader.h"
#include "tiled/tiled_tileset.h"

class QtDatapackClientLoader
        #ifndef NOTHREADS
        : public QThread
        #else
        : public QObject
        #endif
        , public DatapackClientLoader
{
    Q_OBJECT
public:
    static QtDatapackClientLoader datapackLoader;
    struct QtItemExtra
    {
        QPixmap image;
    };
    struct QtMonsterExtra
    {
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
        struct QtBuff
        {
            QIcon icon;
        };
    };

    /*
    void resetAll();

    //static items

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
    std::unordered_map<uint16_t,PlantIndexContent> plantIndexOfOnMap;*/
    std::unordered_map<uint16_t,QtItemExtra> QtitemsExtra;
    std::unordered_map<uint16_t,QtMonsterExtra> QtmonsterExtra;
    std::unordered_map<uint8_t,QtMonsterExtra::QtBuff> QtmonsterBuffsExtra;
    QPixmap defaultInventoryImage();
    bool isParsingDatapack();
    std::string getDatapackPath();
    std::string getMainDatapackPath();
    std::string getSubDatapackPath();
    QImage imagesInterfaceFightLabelBottom,imagesInterfaceFightLabelTop;
protected:
    void run();
    void emitdatapackParsed() override;
    void emitdatapackParsedMainSub() override;
    void emitdatapackChecksumError() override;
    void parseTopLib() override;
public slots:
    void parseDatapack(const std::string &datapackPath);
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode);
signals:
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();
private:
    QPixmap *mDefaultInventoryImage;
    explicit QtDatapackClientLoader();
    ~QtDatapackClientLoader();
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
};

#endif // DATAPACKCLIENTLOADER_H
