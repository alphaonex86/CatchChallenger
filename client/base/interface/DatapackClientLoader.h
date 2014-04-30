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

#include "../../general/base/GeneralStructures.h"

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
    };
    struct ZoneExtra
    {
        QString name;
    };
    struct BotFightExtra
    {
        QString start;
        QString win;
    };
    struct QuestText
    {
        QHash<quint32,QString> text;
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
            quint8 event;
            quint8 eventValue;
            QColor color;
        };
        QList<VisualCategoryCondition> conditions;
    };
    struct TypeText
    {
        QString name;
    };
    QHash<quint32,TypeText> typeExtra;
    QHash<quint32,MonsterExtra> monsterExtra;
    QHash<quint32,MonsterExtra::Buff> monsterBuffsExtra;
    QHash<quint32,MonsterExtra::Skill> monsterSkillsExtra;
    QHash<quint32,ItemExtra> itemsExtra;
    QHash<QString,ReputationExtra> reputationExtra;
    QHash<quint32,quint8> itemToPlants;
    QHash<quint8,PlantExtra> plantExtra;
    QHash<quint32,QuestExtra> questsExtra;
    QHash<quint32,QuestText> questsText;
    QHash<QString,quint32> questsPathToId;
    QMultiHash<quint32,quint32> botToQuestStart;
    QHash<quint32,BotFightExtra> botFightsExtra;
    QHash<QString,ZoneExtra> zonesExtra;
    QHash<QString,QString> audioAmbiance;
    QHash<quint32,ProfileText> profileTextList;
    QHash<QString,VisualCategory> visualCategories;
    QString language;
    QStringList maps,skins;
    QPixmap defaultInventoryImage();
    bool isParsingDatapack();
    QString getDatapackPath();
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
signals:
    void datapackParsed();
private:
    bool inProgress;
    QString datapackPath;
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
    static QString text_list;
    static QString text_reputation;
    static QString text_type;
    static QString text_name;
    static QString text_en;
    static QString text_lang;
    static QString text_level;
    static QString text_point;
    static QString text_text;
    static QString text_id;
    static QString text_image;
    static QString text_description;
    static QString text_item;
    static QString text_slashdefinitiondotxml;
    static QString text_quest;
    static QString text_rewards;
    static QString text_show;
    static QString text_true;
    static QString text_step;
    static QString text_bot;
    static QString text_dotcomma;
    static QString text_client_logic;
    static QString text_map;
    static QString text_items;
    static QString text_zone;

    static QString text_monster;
    static QString text_monsters;
    static QString text_kind;
    static QString text_habitat;
    static QString text_slash;
    static QString text_types;
    static QString text_buff;
    static QString text_skill;
    static QString text_buffs;
    static QString text_skills;
    static QString text_fight;
    static QString text_fights;
    static QString text_start;
    static QString text_win;
    static QString text_dotxml;
    static QString text_dottsx;
    static QString text_visual;
    static QString text_category;
    static QString text_alpha;
    static QString text_color;
    static QString text_event;
    static QString text_value;
};

#endif // DATAPACKCLIENTLOADER_H
