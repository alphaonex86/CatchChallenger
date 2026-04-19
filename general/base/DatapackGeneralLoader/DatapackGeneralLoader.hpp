#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

#include "../GeneralStructures.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../../tinyXML2/tinyxml2.hpp"
#endif
#include "../../../general/base/lib.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
#ifndef CATCHCHALLENGER_NOXML
public:
    static std::vector<std::string> loadSkins(const std::string &folder);
    static std::vector<Reputation> loadReputation(const std::string &file);//Player_private_and_public_informations, std::unordered_map<uint8_t,PlayerReputation> reputation;
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_QUEST, Quest> loadQuests(const std::string &folder, const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    static std::pair<bool,Quest> loadSingleQuest(const std::string &file,const catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_PLANT,Plant> loadPlants(const std::string &file);
    static std::pair<catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_CRAFTINGRECIPE,CraftingRecipe>,catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_CRAFTINGRECIPE> > loadCraftingRecipes(
            const std::string &file, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items, CATCHCHALLENGER_TYPE_CRAFTINGRECIPE &crafingRecipesMaxId);
    static void loadItems(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_ITEM> &tempNameToItemId,const std::string &folder, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_BUFF, Buff> &monsterBuffs,
            catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffect> > &monsterItemEffect,
            catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, std::vector<MonsterItemEffectOutOfFight> > &monsterItemEffectOutOfFight,
            catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &outItems,
            CATCHCHALLENGER_TYPE_ITEM &itemMaxId,
            catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, uint32_t> &repel,
            catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Trap> &trap);
    #endif
    DLL_PUBLIC static std::pair<std::vector<const tinyxml2::XMLElement *>, std::vector<Profile> > loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                      const std::vector<Reputation> &reputations);
    DLL_PUBLIC static std::vector<ServerSpecProfile> loadServerProfileList(
            const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file, const std::vector<Profile> &profileCommon,
            const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    static std::vector<ServerSpecProfile> loadServerProfileListInternal(
            const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file,
            const catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,std::vector<MonsterDrops> > loadMonsterDrop(
            const std::string &folder, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items,const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters);
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,std::vector<MonsterDrops> > loadMonsterDrop(
            const std::string &folder);
    static std::vector<MonstersCollisionTemp> loadMonstersCollision(
            const std::string &file, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items, const std::vector<Event> &events);
    static LayersOptions loadLayersOptions(const std::string &file);
    static std::vector<Event> loadEvents(const std::string &file);
    static catchchallenger_datapack_map<SHOP_TYPE,Shop> preload_shop(const std::string &file, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items);/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
    #endif
#endif
};
}

#endif // DATAPACKGENERALLOADER_H
