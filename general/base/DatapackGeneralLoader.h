#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <unordered_map>
#include <string>
#include <utility>
#include <vector>
#include <QDomElement>
#include <QDomDocument>
#include "GeneralStructures.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
public:
    static std::vector<std::string> loadSkins(const std::string &folder);
    static std::vector<Reputation> loadReputation(const std::string &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::unordered_map<quint16, Quest> loadQuests(const std::string &folder);
    static std::pair<bool,Quest> loadSingleQuest(const std::string &file);
    static std::unordered_map<quint8,Plant> loadPlants(const std::string &file);
    static std::pair<std::unordered_map<quint16,CrafingRecipe>,std::unordered_map<quint16,quint16> > loadCraftingRecipes(const std::string &file, const std::unordered_map<quint16, Item> &items);
    static ItemFull loadItems(const std::string &folder, const std::unordered_map<quint8, Buff> &monsterBuffs);
    static std::unordered_map<quint16,Industry> loadIndustries(const std::string &folder,const std::unordered_map<quint16, Item> &items);
    static std::unordered_map<quint16,IndustryLink> loadIndustriesLink(const std::string &file,const std::unordered_map<quint16,Industry> &industries);
    #endif
    static std::pair<std::vector<QDomElement>, std::vector<Profile> > loadProfileList(const std::string &datapackPath, const std::string &file,
                                                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<quint16, Item> &items,
                                                                      #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<quint16,Monster> &monsters,const std::vector<Reputation> &reputations);
    static std::vector<ServerProfile> loadServerProfileList(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file, const std::vector<Profile> &profileCommon);
    static std::vector<ServerProfile> loadServerProfileListInternal(const std::string &datapackPath, const std::string &mainDatapackCode, const std::string &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::vector<MonstersCollision> loadMonstersCollision(const std::string &file, const std::unordered_map<quint16, Item> &items, const std::vector<Event> &events);
    static LayersOptions loadLayersOptions(const std::string &file);
    static std::vector<Event> loadEvents(const std::string &file);
    static std::unordered_map<quint32,Shop> preload_shop(const std::string &file, const std::unordered_map<quint16, Item> &items);
    #endif
protected:
    static const std::string text_list;
    static const std::string text_dotxml;
    static const std::string text_dottmx;
    static const std::string text_reputation;
    static const std::string text_type;
    static const std::string text_level;
    static const std::string text_point;
    static const std::string text_slashdefinitionxml;
    static const std::string text_quest;
    static const std::string text_repeatable;
    static const std::string text_yes;
    static const std::string text_true;
    static const std::string text_bot;
    static const std::string text_dotcomma;
    static const std::string text_requirements;
    static const std::string text_less;
    static const std::string text_id;
    static const std::string text_rewards;
    static const std::string text_allow;
    static const std::string text_clan;
    static const std::string text_step;
    static const std::string text_item;
    static const std::string text_quantity;
    static const std::string text_monster;
    static const std::string text_rate;
    static const std::string text_percent;
    static const std::string text_fight;
    static const std::string text_plants;
    static const std::string text_plant;
    static const std::string text_itemUsed;
    static const std::string text_grow;
    static const std::string text_fruits;
    static const std::string text_sprouted;
    static const std::string text_taller;
    static const std::string text_flowering;
    static const std::string text_recipes;
    static const std::string text_recipe;
    static const std::string text_itemToLearn;
    static const std::string text_doItemId;
    static const std::string text_success;
    static const std::string text_material;
    static const std::string text_industries;
    static const std::string text_industrialrecipe;
    static const std::string text_time;
    static const std::string text_cycletobefull;
    static const std::string text_resource;
    static const std::string text_product;
    static const std::string text_link;
    static const std::string text_price;
    static const std::string text_consumeAtUse;
    static const std::string text_false;
    static const std::string text_trap;
    static const std::string text_bonus_rate;
    static const std::string text_repel;
    static const std::string text_hp;
    static const std::string text_add;
    static const std::string text_all;
    static const std::string text_buff;
    static const std::string text_remove;
    static const std::string text_up;
    static const std::string text_start;
    static const std::string text_map;
    static const std::string text_file;
    static const std::string text_x;
    static const std::string text_y;
    static const std::string text_forcedskin;
    static const std::string text_cash;
    static const std::string text_itemId;
    static const std::string text_industry;
    static const std::string text_items;
    static const std::string text_value;
    static const std::string text_captured_with;
    static const std::string text_monstersCollision;
    static const std::string text_monsterType;
    static const std::string text_walkOn;
    static const std::string text_actionOn;
    static const std::string text_layer;
    static const std::string text_tile;
    static const std::string text_background;
    static const std::string text_slash;
    static const std::string text_layers;
    static const std::string text_events;
    static const std::string text_event;
    static const std::string text_shop;
    static const std::string text_shops;
    static const std::string text_overridePrice;
    static const std::string text_inverse;
};
}

#endif // DATAPACKGENERALLOADER_H
