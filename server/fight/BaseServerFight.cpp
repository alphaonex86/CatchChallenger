#include "BaseServerFight.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/fight/FightLoader.h"
#include "../VariableServer.h"

using namespace Pokecraft;

void BaseServerFight::preload_monsters()
{
    GlobalData::serverPrivateVariables.monsters=FightLoader::loadMonster(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",GlobalData::serverPrivateVariables.monsterSkills);

    DebugClass::debugConsole(QString("%1 monster(s) loaded").arg(GlobalData::serverPrivateVariables.monsters.size()));
}

void BaseServerFight::preload_skills()
{
    GlobalData::serverPrivateVariables.monsterSkills=FightLoader::loadMonsterSkill(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml",GlobalData::serverPrivateVariables.monsterBuffs);

    DebugClass::debugConsole(QString("%1 skill(s) loaded").arg(GlobalData::serverPrivateVariables.monsterSkills.size()));
}

void BaseServerFight::preload_buff()
{
    GlobalData::serverPrivateVariables.monsterBuffs=FightLoader::loadMonsterBuff(GlobalData::serverPrivateVariables.datapack_basePath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");

    DebugClass::debugConsole(QString("%1 buff(s) loaded").arg(GlobalData::serverPrivateVariables.monsterBuffs.size()));
}

void BaseServerFight::unload_buff()
{
    GlobalData::serverPrivateVariables.monsterBuffs.clear();
}

void BaseServerFight::unload_skills()
{
    GlobalData::serverPrivateVariables.monsterSkills.clear();
}

void BaseServerFight::unload_monsters()
{
    GlobalData::serverPrivateVariables.monsters.clear();
}
