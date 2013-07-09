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
    struct Item
    {
        QPixmap image;
        QString name;
        QString description;
        quint32 price;
    };
    QHash<quint32,Item> items;
    struct Plant
    {
        quint32 itemUsed;
        quint16 sprouted_seconds;
        quint16 taller_seconds;
        quint16 flowering_seconds;
        quint16 fruits_seconds;
        float quantity;
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
    QHash<QString,CatchChallenger::Reputation> reputation;
    QHash<QString,ReputationExtra> reputationExtra;
    QHash<quint8,Plant> plants;
    QHash<quint32,quint8> itemToPlants;
    QHash<quint32,quint32> itemToCrafingRecipes;
    QHash<quint32,CatchChallenger::CrafingRecipe> crafingRecipes;
    QHash<quint32,CatchChallenger::Quest> quests;
    QHash<quint32,QuestExtra> questsExtra;
    QHash<quint32,QuestText> questsText;
    QHash<QString,quint32> questsPathToId;
    QMultiHash<quint32,quint32> botToQuestStart;
    QHash<quint32,CatchChallenger::BotFight> botFights;
    QHash<quint32,BotFightExtra> botFightsExtra;
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
    void parseItems();
    void parseQuests();
    void parseMaps();
    void parsePlants();
    void parseCraftingRecipes();
    void parseBuff();
    void parseSkills();
    void parseMonsters();
    void parseReputation();
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseSkillsExtra();
    void parseQuestsExtra();
    void parseQuestsText();
    void parseQuestsLink();
    void parseSkins();
    void parseBotFights();
    void parseBotFightsExtra();
};

#endif // DATAPACKCLIENTLOADER_H
