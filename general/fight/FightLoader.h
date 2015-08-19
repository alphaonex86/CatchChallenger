#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <unordered_map>
#include <QString>

#include "../base/GeneralStructures.h"
#include "../base/DatapackGeneralLoader.h"

namespace CatchChallenger {
class FightLoader
{
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static QList<Type> loadTypes(const QString &file);
    #endif
    static std::unordered_map<quint16,Monster> loadMonster(const QString &folder, const std::unordered_map<quint16,Skill> &monsterSkills
                                              #ifndef CATCHCHALLENGER_CLASS_MASTER
                                              , const QList<Type> &types, const std::unordered_map<quint16, Item> &items
                                              #endif
                                              );
    static std::unordered_map<quint16,Skill> loadMonsterSkill(const QString &folder
                                                 #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                 , const std::unordered_map<quint8,Buff> &monsterBuffs
                                                 , const QList<Type> &types
                                                 #endif
                                                 );
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    static std::unordered_map<quint16/*item*/, std::unordered_map<quint16/*monster*/,quint16/*evolveTo*/> > loadMonsterEvolutionItems(const std::unordered_map<quint16,Monster> &monsters);
    static std::unordered_map<quint16/*item*/, QSet<quint16/*monster*/> > loadMonsterItemToLearn(const std::unordered_map<quint16,Monster> &monsters, const std::unordered_map<quint16/*item*/, std::unordered_map<quint16/*monster*/,quint16/*evolveTo*/> > &evolutionItem);
    static std::unordered_map<quint8,Buff> loadMonsterBuff(const QString &folder);
    static std::unordered_map<quint16,BotFight> loadFight(const QString &folder, const std::unordered_map<quint16,Monster> &monsters, const std::unordered_map<quint16, Skill> &monsterSkills, const std::unordered_map<quint16, Item> &items);
    #endif
    static QList<PlayerMonster::PlayerSkill> loadDefaultAttack(const quint16 &monsterId, const quint8 &level, const std::unordered_map<quint16,Monster> &monsters, const std::unordered_map<quint16, Skill> &monsterSkills);
    static QString text_type;
    static QString text_name;
    static QString text_id;
    static QString text_multiplicator;
    static QString text_number;
    static QString text_to;
    static QString text_dotcoma;
    static QString text_list;
    static QString text_monster;
    static QString text_monsters;
    static QString text_dotxml;
    static QString text_skills;
    static QString text_buffs;
    static QString text_egg_step;
    static QString text_xp_for_max_level;
    static QString text_xp_max;
    static QString text_hp;
    static QString text_attack;
    static QString text_defense;
    static QString text_special_attack;
    static QString text_special_defense;
    static QString text_speed;
    static QString text_give_sp;
    static QString text_give_xp;
    static QString text_catch_rate;
    static QString text_type2;
    static QString text_pow;
    static QString text_ratio_gender;
    static QString text_percent;
    static QString text_attack_list;
    static QString text_skill;
    static QString text_skill_level;
    static QString text_attack_level;
    static QString text_level;
    static QString text_byitem;
    static QString text_evolution;
    static QString text_evolutions;
    static QString text_trade;
    static QString text_evolveTo;
    static QString text_item;
    static QString text_fights;
    static QString text_fight;
    static QString text_gain;
    static QString text_cash;
    static QString text_sp;
    static QString text_effect;
    static QString text_endurance;
    static QString text_life;
    static QString text_applyOn;
    static QString text_aloneEnemy;
    static QString text_themself;
    static QString text_allEnemy;
    static QString text_allAlly;
    static QString text_nobody;
    static QString text_quantity;
    static QString text_more;
    static QString text_success;
    static QString text_buff;
    static QString text_capture_bonus;
    static QString text_duration;
    static QString text_Always;
    static QString text_NumberOfTurn;
    static QString text_durationNumberOfTurn;
    static QString text_ThisFight;
    static QString text_inFight;
    static QString text_inWalk;
    static QString text_steps;
};
bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2);
}

#endif // FIGHTLOADER_H
