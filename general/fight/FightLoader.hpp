#ifndef FIGHTLOADER_H
#define FIGHTLOADER_H

#include <unordered_map>
#include <string>

#include "../base/GeneralStructures.hpp"
#ifndef CATCHCHALLENGER_NOXML
#include "../base/DatapackGeneralLoader/DatapackGeneralLoader.hpp"
#endif

namespace CatchChallenger {
class FightLoader
{
public:
    #ifndef CATCHCHALLENGER_CLASS_MASTER
    #ifndef CATCHCHALLENGER_NOXML
    static std::vector<Type> loadTypes(const std::string &file);
    static void loadMonsterName(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> &tempNameToMonsterId, const std::string &folder);
    #endif
    #endif
    #ifndef CATCHCHALLENGER_NOXML
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> loadMonster(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_MONSTER> &tempNameToMonsterId,const std::string &folder, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_SKILL,Skill> &monsterSkills
                                              #ifndef CATCHCHALLENGER_CLASS_MASTER
                                              , const std::vector<Type> &types, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items,
                                              CATCHCHALLENGER_TYPE_MONSTER &monstersMaxId
                                              #endif
                                              );
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_SKILL,Skill> loadMonsterSkill(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_SKILL> &tempNameToSkillId,const std::string &folder
                                                 #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                 , const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_BUFF,Buff> &monsterBuffs
                                                 , const std::vector<Type> &types
                                                 #endif
                                                 );
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,CATCHCHALLENGER_TYPE_MONSTER> > loadMonsterEvolutionItems(const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters);
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, catchchallenger_datapack_set<CATCHCHALLENGER_TYPE_MONSTER> > loadMonsterItemToEvolution(const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,CATCHCHALLENGER_TYPE_MONSTER> > &evolutionItem);
    #ifndef CATCHCHALLENGER_NOXML
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_BUFF,Buff> loadMonsterBuff(catchchallenger_datapack_map<std::string,CATCHCHALLENGER_TYPE_BUFF> &tempNameToBuffId, const std::string &folder);
    static catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_BOTID,BotFight> loadFight(const std::string &folder, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_MONSTER,Monster> &monsters, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_SKILL, Skill> &monsterSkills, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_ITEM, Item> &items, CATCHCHALLENGER_TYPE_BOTID &botFightsMaxId);
    #endif
    #endif
    static std::vector<PlayerMonster::PlayerSkill> loadDefaultAttack(const uint16_t &monsterId, const uint8_t &level, const catchchallenger_datapack_map<uint16_t,Monster> &monsters, const catchchallenger_datapack_map<CATCHCHALLENGER_TYPE_SKILL, Skill> &monsterSkills);
};
bool operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2);
}

#endif // FIGHTLOADER_H
