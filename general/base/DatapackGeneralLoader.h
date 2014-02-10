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
    static QHash<QString, Reputation> loadReputation(const QString &file);
    static QHash<quint32, Quest> loadQuests(const QString &folder);
    static QPair<bool,Quest> loadSingleQuest(const QString &file);
    static QHash<quint8,Plant> loadPlants(const QString &file);
    static QPair<QHash<quint32,CrafingRecipe>,QHash<quint32,quint32> > loadCraftingRecipes(const QString &file, const QHash<quint32, Item> &items);
    static ItemFull loadItems(const QString &folder, const QHash<quint32, Buff> &monsterBuffs);
    static QHash<quint32,Industry> loadIndustries(const QString &folder,const QHash<quint32, Item> &items);
    static QHash<quint32,quint32> loadIndustriesLink(const QString &file,const QHash<quint32,Industry> &industries);
    static QPair<QList<QDomElement>, QList<Profile> > loadProfileList(const QString &datapackPath, const QString &file,const QHash<quint32, Item> &items,const QHash<quint32,Monster> &monsters,const QHash<QString, Reputation> &reputations);
protected:
    static QString text_list;
    static QString text_reputation;
    static QString text_type;
    static QString text_level;
    static QString text_point;
    static QString text_slashdefinitionxml;
    static QString text_quest;
    static QString text_repeatable;
    static QString text_yes;
    static QString text_true;
    static QString text_bot;
    static QString text_dotcomma;
    static QString text_requirements;
    static QString text_less;
    static QString text_id;
    static QString text_rewards;
    static QString text_allow;
    static QString text_clan;
    static QString text_step;
    static QString text_item;
    static QString text_quantity;
    static QString text_monster;
    static QString text_rate;
    static QString text_percent;
    static QString text_fight;
    static QString text_plants;
    static QString text_plant;
    static QString text_itemUsed;
    static QString text_grow;
    static QString text_fruits;
    static QString text_sprouted;
    static QString text_taller;
    static QString text_flowering;
    static QString text_recipes;
    static QString text_recipe;
    static QString text_itemToLearn;
    static QString text_doItemId;
    static QString text_success;
    static QString text_material;
    static QString text_industries;
    static QString text_industrialrecipe;
    static QString text_time;
    static QString text_cycletobefull;
    static QString text_resource;
    static QString text_product;
    static QString text_link;
    static QString text_price;
    static QString text_consumeAtUse;
    static QString text_false;
    static QString text_trap;
    static QString text_bonus_rate;
    static QString text_repel;
    static QString text_hp;
    static QString text_add;
    static QString text_all;
    static QString text_buff;
    static QString text_remove;
    static QString text_up;
    static QString text_start;
    static QString text_map;
    static QString text_file;
    static QString text_x;
    static QString text_y;
    static QString text_forcedskin;
    static QString text_cash;
    static QString text_itemId;
    static QString text_industry;
    static QString text_items;
    static QString text_value;
    static QString text_captured_with;
};
}

#endif // DATAPACKGENERALLOADER_H
