#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <unordered_map>
#include <string>

#include "../base/GeneralStructures.hpp"
#include "../base/DatapackGeneralLoader/DatapackGeneralLoader.hpp"

namespace CatchChallenger {
class FightLoader
{
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    static std::vector<Type> loadTypes(const std::string &file);
    static void loadMonsterName(std::unordered_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> &tempNameToMonsterId, const std::string &folder);
    #endif
    static std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> loadMonster(std::unordered_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> &tempNameToMonsterId,const std::string &folder, const std::unordered_map<CATCHCHALLENGER_TYPE_SKILL,Skill> &monsterSkills
                                              #ifndef CATCHCHALLENGER_CLASS_MASTER
                                              , const std::vector<Type> &types, const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items,
                                              CATCHCHALLENGER_TYPE_MONSTER &monstersMaxId
                                              #endif
                                              );
    static std::unordered_map<CATCHCHALLENGER_TYPE_SKILL,Skill> loadMonsterSkill(std::unordered_map<std::string,CATCHCHALLENGER_TYPE_SKILL> &tempNameToSkillId,const std::string &folder
                                                 #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                 , const std::unordered_map<CATCHCHALLENGER_TYPE_BUFF,Buff> &monsterBuffs
                                                 , const std::vector<Type> &types
                                                 #endif
                                                 );
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    static std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,CATCHCHALLENGER_TYPE_MONSTER> > loadMonsterEvolutionItems(const std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters);
    static std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, std::unordered_set<CATCHCHALLENGER_TYPE_MONSTER> > loadMonsterItemToEvolution(const std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters, const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,CATCHCHALLENGER_TYPE_MONSTER> > &evolutionItem);
    static std::unordered_map<CATCHCHALLENGER_TYPE_BUFF,Buff> loadMonsterBuff(std::unordered_map<std::string,CATCHCHALLENGER_TYPE_BUFF> &tempNameToBuffId, const std::string &folder);
    static std::unordered_map<CATCHCHALLENGER_TYPE_BOTID,BotFight> loadFight(const std::string &folder, const std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters, const std::unordered_map<CATCHCHALLENGER_TYPE_SKILL, Skill> &monsterSkills, const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items, CATCHCHALLENGER_TYPE_BOTID &botFightsMaxId);
    #endif
    static std::vector<PlayerMonster::PlayerSkill> loadDefaultAttack(const uint16_t &monsterId, const uint8_t &level, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<CATCHCHALLENGER_TYPE_SKILL, Skill> &monsterSkills);
};
bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2);
}

#endif // FIGHTLOADER_H
