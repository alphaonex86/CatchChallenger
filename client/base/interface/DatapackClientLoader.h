#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QMultiHash>

#include "../../general/base/GeneralStructures.h"

#include "tileset.h"

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    static DatapackClientLoader datapackLoader;
    void resetAll();

    //static items
    struct ItemExtra
    {
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
        QStringList reputation_positive,reputation_negative;
    };
    struct QuestExtra
    {
        QString name;
        QStringList steps;
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
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
        struct Buff
        {
            QString name;
            QString description;
        };
        struct Skill
        {
            QString name;
            QString description;
        };
    };
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
    QHash<QString,QString> audioAmbiance;
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
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseSkillsExtra();
    void parseQuestsExtra();
    void parseQuestsText();
    void parseQuestsLink();
    void parseSkins();
    void parseBotFightsExtra();
    void parseAudioAmbiance();
};

#endif // DATAPACKCLIENTLOADER_H
