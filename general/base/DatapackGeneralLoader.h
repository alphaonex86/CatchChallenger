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
    static std::vector<std::basic_string<char>> loadSkins(const std::basic_string<char> &folder);
    static std::vector<Reputation> loadReputation(const std::basic_string<char> &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::unordered_map<quint16, Quest> loadQuests(const std::basic_string<char> &folder);
    static std::pair<bool,Quest> loadSingleQuest(const std::basic_string<char> &file);
    static std::unordered_map<quint8,Plant> loadPlants(const std::basic_string<char> &file);
    static std::pair<std::unordered_map<quint16,CrafingRecipe>,std::unordered_map<quint16,quint16> > loadCraftingRecipes(const std::basic_string<char> &file, const std::unordered_map<quint16, Item> &items);
    static ItemFull loadItems(const std::basic_string<char> &folder, const std::unordered_map<quint8, Buff> &monsterBuffs);
    static std::unordered_map<quint16,Industry> loadIndustries(const std::basic_string<char> &folder,const std::unordered_map<quint16, Item> &items);
    static std::unordered_map<quint16,IndustryLink> loadIndustriesLink(const std::basic_string<char> &file,const std::unordered_map<quint16,Industry> &industries);
    #endif
    static std::pair<std::vector<QDomElement>, std::vector<Profile> > loadProfileList(const std::basic_string<char> &datapackPath, const std::basic_string<char> &file,
                                                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<quint16, Item> &items,
                                                                      #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                      const std::unordered_map<quint16,Monster> &monsters,const std::vector<Reputation> &reputations);
    static std::vector<ServerProfile> loadServerProfileList(const std::basic_string<char> &datapackPath, const std::basic_string<char> &mainDatapackCode, const std::basic_string<char> &file, const std::vector<Profile> &profileCommon);
    static std::vector<ServerProfile> loadServerProfileListInternal(const std::basic_string<char> &datapackPath, const std::basic_string<char> &mainDatapackCode, const std::basic_string<char> &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::vector<MonstersCollision> loadMonstersCollision(const std::basic_string<char> &file, const std::unordered_map<quint16, Item> &items, const std::vector<Event> &events);
    static LayersOptions loadLayersOptions(const std::basic_string<char> &file);
    static std::vector<Event> loadEvents(const std::basic_string<char> &file);
    static std::unordered_map<quint32,Shop> preload_shop(const std::basic_string<char> &file, const std::unordered_map<quint16, Item> &items);
    #endif
protected:
    static const std::basic_string<char> text_list;
    static const std::basic_string<char> text_dotxml;
    static const std::basic_string<char> text_dottmx;
    static const std::basic_string<char> text_reputation;
    static const std::basic_string<char> text_type;
    static const std::basic_string<char> text_level;
    static const std::basic_string<char> text_point;
    static const std::basic_string<char> text_slashdefinitionxml;
    static const std::basic_string<char> text_quest;
    static const std::basic_string<char> text_repeatable;
    static const std::basic_string<char> text_yes;
    static const std::basic_string<char> text_true;
    static const std::basic_string<char> text_bot;
    static const std::basic_string<char> text_dotcomma;
    static const std::basic_string<char> text_requirements;
    static const std::basic_string<char> text_less;
    static const std::basic_string<char> text_id;
    static const std::basic_string<char> text_rewards;
    static const std::basic_string<char> text_allow;
    static const std::basic_string<char> text_clan;
    static const std::basic_string<char> text_step;
    static const std::basic_string<char> text_item;
    static const std::basic_string<char> text_quantity;
    static const std::basic_string<char> text_monster;
    static const std::basic_string<char> text_rate;
    static const std::basic_string<char> text_percent;
    static const std::basic_string<char> text_fight;
    static const std::basic_string<char> text_plants;
    static const std::basic_string<char> text_plant;
    static const std::basic_string<char> text_itemUsed;
    static const std::basic_string<char> text_grow;
    static const std::basic_string<char> text_fruits;
    static const std::basic_string<char> text_sprouted;
    static const std::basic_string<char> text_taller;
    static const std::basic_string<char> text_flowering;
    static const std::basic_string<char> text_recipes;
    static const std::basic_string<char> text_recipe;
    static const std::basic_string<char> text_itemToLearn;
    static const std::basic_string<char> text_doItemId;
    static const std::basic_string<char> text_success;
    static const std::basic_string<char> text_material;
    static const std::basic_string<char> text_industries;
    static const std::basic_string<char> text_industrialrecipe;
    static const std::basic_string<char> text_time;
    static const std::basic_string<char> text_cycletobefull;
    static const std::basic_string<char> text_resource;
    static const std::basic_string<char> text_product;
    static const std::basic_string<char> text_link;
    static const std::basic_string<char> text_price;
    static const std::basic_string<char> text_consumeAtUse;
    static const std::basic_string<char> text_false;
    static const std::basic_string<char> text_trap;
    static const std::basic_string<char> text_bonus_rate;
    static const std::basic_string<char> text_repel;
    static const std::basic_string<char> text_hp;
    static const std::basic_string<char> text_add;
    static const std::basic_string<char> text_all;
    static const std::basic_string<char> text_buff;
    static const std::basic_string<char> text_remove;
    static const std::basic_string<char> text_up;
    static const std::basic_string<char> text_start;
    static const std::basic_string<char> text_map;
    static const std::basic_string<char> text_file;
    static const std::basic_string<char> text_x;
    static const std::basic_string<char> text_y;
    static const std::basic_string<char> text_forcedskin;
    static const std::basic_string<char> text_cash;
    static const std::basic_string<char> text_itemId;
    static const std::basic_string<char> text_industry;
    static const std::basic_string<char> text_items;
    static const std::basic_string<char> text_value;
    static const std::basic_string<char> text_captured_with;
    static const std::basic_string<char> text_monstersCollision;
    static const std::basic_string<char> text_monsterType;
    static const std::basic_string<char> text_walkOn;
    static const std::basic_string<char> text_actionOn;
    static const std::basic_string<char> text_layer;
    static const std::basic_string<char> text_tile;
    static const std::basic_string<char> text_background;
    static const std::basic_string<char> text_slash;
    static const std::basic_string<char> text_layers;
    static const std::basic_string<char> text_events;
    static const std::basic_string<char> text_event;
    static const std::basic_string<char> text_shop;
    static const std::basic_string<char> text_shops;
    static const std::basic_string<char> text_overridePrice;
    static const std::basic_string<char> text_inverse;
};
}

#endif // DATAPACKGENERALLOADER_H
