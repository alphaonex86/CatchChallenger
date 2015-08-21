#ifndef COMMONDATAPACK_ServerSpec_H
#define COMMONDATAPACK_ServerSpec_H

#include <QObject>
#include <QMutex>
#include <unordered_map>
#include <string>

#include "GeneralStructures.h"
#include "CommonDatapack.h"

namespace CatchChallenger {
class CommonDatapackServerSpec
{
public:
    explicit CommonDatapackServerSpec();
public:
    void unload();
    void parseDatapack(const std::string &datapackPath, const std::string &mainDatapackCode);
public:
    std::unordered_map<quint16,BotFight> botFights;
    std::unordered_map<quint16,Quest> quests;
    std::unordered_map<quint32,Shop> shops;
    std::vector<ServerProfile> serverProfileList;
    static CommonDatapackServerSpec commonDatapackServerSpec;
private:
    QMutex inProgressSpec;
    bool isParsedSpec;
    std::string datapackPath;
    std::string mainDatapackCode;
private:
    void parseQuests();
    void parseBotFights();
    void parseShop();
    void parseServerProfileList();
    void parseIndustries();
};
}


#endif // COMMONDATAPACK_ServerSpec_H
