#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <unordered_map>
#include <string>

#include "../base/GeneralStructures.h"
#include "../base/DatapackGeneralLoader.h"

namespace CatchChallenger {
class FightLoader
{
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::vector<Type> loadTypes(const std::string &file);
    #endif
    static std::unordered_map<uint16_t,Monster> loadMonster(const std::string &folder, const std::unordered_map<uint16_t,Skill> &monsterSkills
                                              #ifndef CATCHCHALLENGER_CLASS_MASTER
                                              , const std::vector<Type> &types, const std::unordered_map<uint16_t, Item> &items
                                              #endif
                                              );
    static std::unordered_map<uint16_t,Skill> loadMonsterSkill(const std::string &folder
                                                 #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                 , const std::unordered_map<uint8_t,Buff> &monsterBuffs
                                                 , const std::vector<Type> &types
                                                 #endif
                                                 );
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    static std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > loadMonsterEvolutionItems(const std::unordered_map<uint16_t,Monster> &monsters);
    static std::unordered_map<uint16_t/*item*/, std::unordered_set<uint16_t/*monster*/> > loadMonsterItemToLearn(const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > &evolutionItem);
    static std::unordered_map<uint8_t,Buff> loadMonsterBuff(const std::string &folder);
    static std::unordered_map<uint16_t,BotFight> loadFight(const std::string &folder, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills, const std::unordered_map<uint16_t, Item> &items);
    #endif
    static std::vector<PlayerMonster::PlayerSkill> loadDefaultAttack(const uint16_t &monsterId, const uint8_t &level, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills);
    static std::string text_type;
    static std::string text_name;
    static std::string text_id;
    static std::string text_multiplicator;
    static std::string text_number;
    static std::string text_to;
    static std::string text_dotcoma;
    static std::string text_list;
    static std::string text_monster;
    static std::string text_monsters;
    static std::string text_dotxml;
    static std::string text_skills;
    static std::string text_buffs;
    static std::string text_egg_step;
    static std::string text_xp_for_max_level;
    static std::string text_xp_max;
    static std::string text_hp;
    static std::string text_attack;
    static std::string text_defense;
    static std::string text_special_attack;
    static std::string text_special_defense;
    static std::string text_speed;
    static std::string text_give_sp;
    static std::string text_give_xp;
    static std::string text_catch_rate;
    static std::string text_type2;
    static std::string text_pow;
    static std::string text_ratio_gender;
    static std::string text_percent;
    static std::string text_attack_list;
    static std::string text_skill;
    static std::string text_skill_level;
    static std::string text_attack_level;
    static std::string text_level;
    static std::string text_byitem;
    static std::string text_evolution;
    static std::string text_evolutions;
    static std::string text_trade;
    static std::string text_evolveTo;
    static std::string text_item;
    static std::string text_fights;
    static std::string text_fight;
    static std::string text_gain;
    static std::string text_cash;
    static std::string text_sp;
    static std::string text_effect;
    static std::string text_endurance;
    static std::string text_life;
    static std::string text_applyOn;
    static std::string text_aloneEnemy;
    static std::string text_themself;
    static std::string text_allEnemy;
    static std::string text_allAlly;
    static std::string text_nobody;
    static std::string text_quantity;
    static std::string text_more;
    static std::string text_success;
    static std::string text_buff;
    static std::string text_capture_bonus;
    static std::string text_duration;
    static std::string text_Always;
    static std::string text_NumberOfTurn;
    static std::string text_durationNumberOfTurn;
    static std::string text_ThisFight;
    static std::string text_inFight;
    static std::string text_inWalk;
    static std::string text_steps;
};
bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2);
}

#endif // FIGHTLOADER_H
