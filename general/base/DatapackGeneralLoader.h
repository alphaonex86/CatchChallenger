#ifndef DATAPACKGENERALLOADER_H
#define DATAPACKGENERALLOADER_H

#include <QHash>
#include <QString>
#include <QPair>
#include <QList>
#include <QDomElement>
#include <QDomDocument>
#include "GeneralStructures.h"

namespace CatchChallenger {
class DatapackGeneralLoader
{
public:
    static QList<QString> loadSkins(const QString &folder);
    static QList<Reputation> loadReputation(const QString &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static QHash<quint16, Quest> loadQuests(const QString &folder);
    static QPair<bool,Quest> loadSingleQuest(const QString &file);
    static QHash<quint8,Plant> loadPlants(const QString &file);
    static QPair<QHash<quint16,CrafingRecipe>,QHash<quint16,quint16> > loadCraftingRecipes(const QString &file, const QHash<quint16, Item> &items);
    static ItemFull loadItems(const QString &folder, const QHash<quint8, Buff> &monsterBuffs);
    static QHash<quint16,Industry> loadIndustries(const QString &folder,const QHash<quint16, Item> &items);
    static QHash<quint16,IndustryLink> loadIndustriesLink(const QString &file,const QHash<quint16,Industry> &industries);
    #endif
    static QPair<QList<QDomElement>, QList<Profile> > loadProfileList(const QString &datapackPath, const QString &file,
                                                                      #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                                      const QHash<quint16, Item> &items,
                                                                      #endif // CATCHCHALLENGER_CLASS_MASTER
                                                                      const QHash<quint16,Monster> &monsters,const QList<Reputation> &reputations);
    static QList<ServerProfile> loadServerProfileList(const QString &datapackPath, const QString &mainDatapackCode, const QString &file, const QList<Profile> &profileCommon);
    static QList<ServerProfile> loadServerProfileListInternal(const QString &datapackPath, const QString &mainDatapackCode, const QString &file);
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static QList<MonstersCollision> loadMonstersCollision(const QString &file, const QHash<quint16, Item> &items, const QList<Event> &events);
    static LayersOptions loadLayersOptions(const QString &file);
    static QList<Event> loadEvents(const QString &file);
    static QHash<quint32,Shop> preload_shop(const QString &file, const QHash<quint16, Item> &items);
    #endif
protected:
    static const QString text_list;
    static const QString text_dotxml;
    static const QString text_dottmx;
    static const QString text_reputation;
    static const QString text_type;
    static const QString text_level;
    static const QString text_point;
    static const QString text_slashdefinitionxml;
    static const QString text_quest;
    static const QString text_repeatable;
    static const QString text_yes;
    static const QString text_true;
    static const QString text_bot;
    static const QString text_dotcomma;
    static const QString text_requirements;
    static const QString text_less;
    static const QString text_id;
    static const QString text_rewards;
    static const QString text_allow;
    static const QString text_clan;
    static const QString text_step;
    static const QString text_item;
    static const QString text_quantity;
    static const QString text_monster;
    static const QString text_rate;
    static const QString text_percent;
    static const QString text_fight;
    static const QString text_plants;
    static const QString text_plant;
    static const QString text_itemUsed;
    static const QString text_grow;
    static const QString text_fruits;
    static const QString text_sprouted;
    static const QString text_taller;
    static const QString text_flowering;
    static const QString text_recipes;
    static const QString text_recipe;
    static const QString text_itemToLearn;
    static const QString text_doItemId;
    static const QString text_success;
    static const QString text_material;
    static const QString text_industries;
    static const QString text_industrialrecipe;
    static const QString text_time;
    static const QString text_cycletobefull;
    static const QString text_resource;
    static const QString text_product;
    static const QString text_link;
    static const QString text_price;
    static const QString text_consumeAtUse;
    static const QString text_false;
    static const QString text_trap;
    static const QString text_bonus_rate;
    static const QString text_repel;
    static const QString text_hp;
    static const QString text_add;
    static const QString text_all;
    static const QString text_buff;
    static const QString text_remove;
    static const QString text_up;
    static const QString text_start;
    static const QString text_map;
    static const QString text_file;
    static const QString text_x;
    static const QString text_y;
    static const QString text_forcedskin;
    static const QString text_cash;
    static const QString text_itemId;
    static const QString text_industry;
    static const QString text_items;
    static const QString text_value;
    static const QString text_captured_with;
    static const QString text_monstersCollision;
    static const QString text_monsterType;
    static const QString text_walkOn;
    static const QString text_actionOn;
    static const QString text_layer;
    static const QString text_tile;
    static const QString text_background;
    static const QString text_slash;
    static const QString text_layers;
    static const QString text_events;
    static const QString text_event;
    static const QString text_shop;
    static const QString text_shops;
    static const QString text_overridePrice;
    static const QString text_inverse;
};
}

#endif // DATAPACKGENERALLOADER_H
