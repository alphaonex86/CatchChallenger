#ifndef COMMONDATAPACK_ServerSpec_H
#define COMMONDATAPACK_ServerSpec_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QMultiHash>

#include "GeneralStructures.h"
#include "CommonDatapack.h"

namespace CatchChallenger {
class CommonDatapackServerSpec
{
public:
    explicit CommonDatapackServerSpec();
public:
    void unload();
    void parseDatapack(const QString &datapackPath, const QString &mainDatapackCode);
public:
    QHash<quint16,BotFight> botFights;
    QHash<quint16,Quest> quests;
    QHash<quint32,Shop> shops;
    QList<ServerProfile> serverProfileList;
    static CommonDatapackServerSpec commonDatapackServerSpec;
private:
    QMutex inProgressSpec;
    bool isParsedSpec;
    QString datapackPath;
    QString mainDatapackCode;
private:
    void parseQuests();
    void parseBotFights();
    void parseShop();
    void parseServerProfileList();
    void parseIndustries();
};
}


#endif // COMMONDATAPACK_ServerSpec_H
