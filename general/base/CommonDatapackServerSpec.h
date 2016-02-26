#ifndef COMMONDATAPACK_ServerSpec_H
#define COMMONDATAPACK_ServerSpec_H

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
    std::unordered_map<uint16_t,BotFight> botFights;
    std::unordered_map<uint16_t,Quest> quests;
    std::unordered_map<uint32_t,Shop> shops;
    std::vector<ServerProfile> serverProfileList;
    static CommonDatapackServerSpec commonDatapackServerSpec;
private:
    bool isParsedSpec;
    bool parsingSpec;
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
