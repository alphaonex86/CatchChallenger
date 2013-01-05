#include "../../base/interface/DatapackClientLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/fight/FightLoader.h"

#include <QDomElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parseBuff()
{
    fightEngine.monsterBuffs=Pokecraft::FightLoader::loadMonsterBuff(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"buff.xml");

    qDebug() << QString("%1 monster buff(s) loaded").arg(fightEngine.monsters.size());
}

void DatapackClientLoader::parseSkills()
{
    fightEngine.monsterSkills=Pokecraft::FightLoader::loadMonsterSkill(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml",fightEngine.monsterBuffs);

    qDebug() << QString("%1 monster skill(s) loaded").arg(fightEngine.monsterSkills.size());
}

void DatapackClientLoader::parseMonsters()
{
    fightEngine.monsters=Pokecraft::FightLoader::loadMonster(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml",fightEngine.monsterSkills);

    qDebug() << QString("%1 monster(s) loaded").arg(fightEngine.monsters.size());
}
