#include "../base/interface/BaseWindow.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralStructures.h"
#include "ui_BaseWindow.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>

/* show only the plant into the inventory */

using namespace Pokecraft;

//need correct match, else tp to die can failed and do mistake
bool BaseWindow::check_monsters()
{
    int size=client->player_informations.playerMonster.size();
    int index=0;
    int sub_size;
    int sub_index;
    while(index<size)
    {
        const PlayerMonster &monster=client->player_informations.playerMonster.at(index);
        if(!DatapackClientLoader::datapackLoader.fightEngine.monsters.contains(monster.monster))
        {
            error(QString("the monster %1 is not into monster list").arg(monster.monster));
            return false;
        }
        const Monster &monsterDef=DatapackClientLoader::datapackLoader.fightEngine.monsters[monster.monster];
        if(monster.level>POKECRAFT_MONSTER_LEVEL_MAX)
        {
            error(QString("the level %1 is greater than max level").arg(monster.level));
            return false;
        }
        Monster::Stat stat=FightEngine::getStat(monsterDef,monster.level);
        if(monster.hp>stat.hp)
        {
            error(QString("the hp %1 is greater than max hp for the level %2").arg(stat.hp).arg(monster.level));
            return false;
        }
        if(monster.remaining_xp>monsterDef.level_to_xp.at(monster.level-1))
        {
            error(QString("the hp %1 is greater than max hp for the level %2").arg(monster.remaining_xp).arg(monster.level));
            return false;
        }
        sub_size=monster.buffs.size();
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(!DatapackClientLoader::datapackLoader.fightEngine.monsterBuffs.contains(monster.buffs.at(sub_index).buff))
            {
                error(QString("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff));
                return false;
            }
            if(monster.buffs.at(sub_index).level>DatapackClientLoader::datapackLoader.fightEngine.monsterBuffs[monster.buffs.at(sub_index).buff].level.size())
            {
                error(QString("the buff have not the level %1 is not into the buff list").arg(monster.buffs.at(sub_index).level));
                return false;
            }
            sub_index++;
        }
        sub_size=monster.skills.size();
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(!DatapackClientLoader::datapackLoader.fightEngine.monsterSkills.contains(monster.skills.at(sub_index).skill))
            {
                error(QString("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill));
                return false;
            }
            if(monster.skills.at(sub_index).level>DatapackClientLoader::datapackLoader.fightEngine.monsterSkills[monster.skills.at(sub_index).skill].level.size())
            {
                error(QString("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level));
                return false;
            }
            sub_index++;
        }
        index++;
    }
    DatapackClientLoader::datapackLoader.fightEngine.setPlayerMonster(client->player_informations.playerMonster);
    return true;
}

void BaseWindow::load_monsters()
{
    const QList<PlayerMonster> &playerMonster=DatapackClientLoader::datapackLoader.fightEngine.getPlayerMonster();
    ui->monsterList->clear();
    int index=0;
    int size=playerMonster.size();
    while(index<size)
    {
        const PlayerMonster &monster=playerMonster.at(index);
        if(DatapackClientLoader::datapackLoader.fightEngine.monsters.contains(monster.monster))
        {
            Monster::Stat stat=Pokecraft::FightEngine::getStat(DatapackClientLoader::datapackLoader.fightEngine.monsters[monster.monster],monster.level);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("%1, level :%2\nHP: %3/%4")
                    .arg(DatapackClientLoader::datapackLoader.fightEngine.monsterExtra[monster.monster].name)
                    .arg(monster.level)
                    .arg(monster.hp)
                    .arg(stat.hp)
                    );
            item->setToolTip(DatapackClientLoader::datapackLoader.fightEngine.monsterExtra[monster.monster].description);
            item->setIcon(DatapackClientLoader::datapackLoader.fightEngine.monsterExtra[monster.monster].front);
            ui->monsterList->addItem(item);
        }
        else
        {
            error(QString("Unknown monster: %1").arg(monster.monster));
            return;
        }
        index++;
    }
}
