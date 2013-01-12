#include "BaseServerFight.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/Map.h"
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

void BaseServerFight::check_monsters_map()
{
    quint32 monstersChecked=0,zoneChecked=0;
    int index;

    QHashIterator<QString,Map *> i(GlobalData::serverPrivateVariables.map_list);
    while (i.hasNext()) {
        i.next();
        if(!i.value()->grassMonster.isEmpty())
        {
            monstersChecked+=i.value()->grassMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->grassMonster.size())
            {
                if(!GlobalData::serverPrivateVariables.monsters.contains(i.value()->grassMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop grass monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->grassMonster.at(index).id));
                    GlobalData::serverPrivateVariables.map_list[i.key()]->grassMonster.clear();
                    break;
                }
                index++;
            }
        }
        if(!i.value()->waterMonster.isEmpty())
        {
            monstersChecked+=i.value()->waterMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->waterMonster.size())
            {
                if(!GlobalData::serverPrivateVariables.monsters.contains(i.value()->waterMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop water monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->waterMonster.at(index).id));
                    GlobalData::serverPrivateVariables.map_list[i.key()]->waterMonster.clear();
                    break;
                }
                index++;
            }
        }
        if(!i.value()->caveMonster.isEmpty())
        {
            monstersChecked+=i.value()->caveMonster.size();
            zoneChecked++;
            index=0;
            while(index<i.value()->grassMonster.size())
            {
                if(!GlobalData::serverPrivateVariables.monsters.contains(i.value()->caveMonster.at(index).id))
                {
                    DebugClass::debugConsole(QString("drop cave monster for the map: %1; because the monster %2 don't exists").arg(i.key()).arg(i.value()->caveMonster.at(index).id));
                    GlobalData::serverPrivateVariables.map_list[i.key()]->caveMonster.clear();
                    break;
                }
                index++;
            }
        }
    }

    DebugClass::debugConsole(QString("%1 monster(s) into %2 zone(s) for %3 map(s) checked").arg(monstersChecked).arg(zoneChecked).arg(GlobalData::serverPrivateVariables.map_list.size()));

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
