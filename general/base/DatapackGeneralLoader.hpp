#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

#include "GeneralStructures.hpp"
#include "../tinyXML2/tinyxml2.hpp"
#include "../../general/base/lib.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
public:
    static std::vector<std::string> loadSkins(const std::string &folder);
    static std::vector<Reputation> loadReputation(const std::string &file);//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::unordered_map<CATCHCHALLENGER_TYPE_QUEST, Quest> loadQuests(const std::string &folder, const std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    static std::pair<bool,Quest> loadSingleQuest(const std::string &file,const std::unordered_map<std::string,CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    static std::unordered_map<uint8_t,Plant> loadPlants(const std::string &file);
    static std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> > loadCraftingRecipes(
            const std::string &file, const std::unordered_map<uint16_t, Item> &items, uint16_t &crafingRecipesMaxId);
    static ItemFull loadItems(const std::string &folder, const std::unordered_map<uint8_t, Buff> &monsterBuffs);
    #endif
    DLL_PUBLIC static std::pair<std::vector<const tinyxml2::XMLElement *>, std::vector<Profile> > loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<uint16_t, Item> &items,
                                                                      #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<uint16_t,Monster> &monsters,const std::vector<Reputation> &reputations);
    DLL_PUBLIC static std::vector<ServerSpecProfile> loadServerProfileList(
            const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file, const std::vector<Profile> &profileCommon);
    static std::vector<ServerSpecProfile> loadServerProfileListInternal(
            const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::unordered_map<uint16_t,std::vector<MonsterDrops> > loadMonsterDrop(
            const std::string &folder, const std::unordered_map<uint16_t, Item> &items,const std::unordered_map<uint16_t,Monster> &monsters);
    static std::vector<MonstersCollisionTemp> loadMonstersCollision(
            const std::string &file, const std::unordered_map<uint16_t, Item> &items, const std::vector<Event> &events);
    static LayersOptions loadLayersOptions(const std::string &file);
    static std::vector<Event> loadEvents(const std::string &file);
    static std::unordered_map<uint16_t,Shop> preload_shop(const std::string &file, const std::unordered_map<uint16_t, Item> &items);/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    #endif
};
}

#endif // DATAPACKGENERALLOADER_H
