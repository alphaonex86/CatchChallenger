#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QMultiHash>
#include <QIcon>
#include <QColor>

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
        QString imagePath;
        QPixmap image;
        QString name;
        QString description;
    };
    struct PlantExtra
    {
        Tiled::Tileset * tileset;
    };
    struct ReputationExtra
    {
        QString name;
        QStringList reputation_positive,reputation_negative;
    };
    struct QuestExtra
    {
        QString name;
        QStringList steps;
        bool showRewards;
        bool autostep;
    };
    struct ZoneExtra
    {
        QString name;
        QHash<QString,QString> audioAmbiance;
    };
    struct BotFightExtra
    {
        QString start;
        QString win;
    };
    struct QuestText
    {
        QHash<uint32_t,QString> text;
    };
    struct MonsterExtra
    {
        QString name;
        QString description;
        QString kind;
        QString habitat;
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
        struct Buff
        {
            QString name;
            QString description;
            QIcon icon;
        };
        struct Skill
        {
            QString name;
            QString description;
        };
        QString frontPath,backPath;
    };
    struct ProfileText
    {
        QString name;
        QString description;
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
        QList<VisualCategoryCondition> conditions;
    };
    struct TypeText
    {
        QString name;
    };
    struct PlantIndexContent
    {
        QString map;
        uint8_t x,y;
    };
    QHash<uint32_t,TypeText> typeExtra;
    QHash<uint32_t,MonsterExtra> monsterExtra;
    QHash<uint32_t,MonsterExtra::Buff> monsterBuffsExtra;
    QHash<uint32_t,MonsterExtra::Skill> monsterSkillsExtra;
    QHash<uint32_t,ItemExtra> itemsExtra;
    QHash<QString,ReputationExtra> reputationExtra;
    QHash<QString,int> reputationNameToId;
    QHash<uint32_t,uint8_t> itemToPlants;
    QHash<uint8_t,PlantExtra> plantExtra;
    QHash<uint32_t,QuestExtra> questsExtra;
    QHash<uint32_t,QuestText> questsText;
    QHash<QString,uint32_t> questsPathToId;
    QMultiHash<uint32_t,uint32_t> botToQuestStart;
    QHash<uint32_t,BotFightExtra> botFightsExtra;
    QHash<QString,ZoneExtra> zonesExtra;
    QHash<QString,QString> audioAmbiance;
    QHash<uint32_t,ProfileText> profileTextList;
    QHash<QString,VisualCategory> visualCategories;
    QString language;
    QStringList maps,skins;
    QHash<QString,uint32_t> mapToId;
    QHash<QString,uint32_t> fullMapPathToId;
    QHash<QString,QHash<QPair<uint8_t,uint8_t>,uint16_t> > itemOnMap;
    QHash<QString,QHash<QPair<uint8_t,uint8_t>,uint16_t> > plantOnMap;
    QHash<uint16_t,PlantIndexContent> plantIndexOfOnMap;
    QPixmap defaultInventoryImage();
    bool isParsingDatapack();
    QString getDatapackPath();
    QImage imagesInterfaceFightLabelBottom,imagesInterfaceFightLabelTop;
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
    void parseDatapackMainSub(const QString &mainDatapackCode, const QString &subDatapackCode);
signals:
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();
private:
    bool inProgress;
    QString datapackPath;
    QString mainDatapackCode;
    QString subDatapackCode;
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
    static const QString text_list;
    static const QString text_reputation;
    static const QString text_type;
    static const QString text_name;
    static const QString text_en;
    static const QString text_lang;
    static const QString text_level;
    static const QString text_point;
    static const QString text_text;
    static const QString text_id;
    static const QString text_image;
    static const QString text_description;
    static const QString text_item;
    static const QString text_slashdefinitiondotxml;
    static const QString text_quest;
    static const QString text_rewards;
    static const QString text_show;
    static const QString text_autostep;
    static const QString text_yes;
    static const QString text_true;
    static const QString text_step;
    static const QString text_bot;
    static const QString text_dotcomma;
    static const QString text_client_logic;
    static const QString text_map;
    static const QString text_items;
    static const QString text_zone;
    static const QString text_music;
    static const QString text_backgroundsound;

    static const QString text_monster;
    static const QString text_monsters;
    static const QString text_kind;
    static const QString text_habitat;
    static const QString text_slash;
    static const QString text_types;
    static const QString text_buff;
    static const QString text_skill;
    static const QString text_buffs;
    static const QString text_skills;
    static const QString text_fight;
    static const QString text_fights;
    static const QString text_start;
    static const QString text_win;
    static const QString text_dotxml;
    static const QString text_dottsx;
    static const QString text_visual;
    static const QString text_category;
    static const QString text_alpha;
    static const QString text_color;
    static const QString text_event;
    static const QString text_value;
    static const QString text_tileheight;
    static const QString text_tilewidth;
    static const QString text_x;
    static const QString text_y;
    static const QString text_object;
    static const QString text_objectgroup;
    static const QString text_Object;
    static const QString text_layer;
    static const QString text_Dirt;
    static const QString text_DATAPACK_BASE_PATH_MAPBASE;
    static QString text_DATAPACK_BASE_PATH_MAPMAIN;
    static QString text_DATAPACK_BASE_PATH_MAPSUB;
};

#endif // DATAPACKCLIENTLOADER_H
