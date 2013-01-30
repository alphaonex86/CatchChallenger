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
    int size=Pokecraft::Api_client_real::client->player_informations.playerMonster.size();
    int index=0;
    int sub_size;
    int sub_index;
    while(index<size)
    {
        const PlayerMonster &monster=Pokecraft::Api_client_real::client->player_informations.playerMonster.at(index);
        if(!Pokecraft::FightEngine::fightEngine.monsters.contains(monster.monster))
        {
            error(QString("the monster %1 is not into monster list").arg(monster.monster));
            return false;
        }
        const Monster &monsterDef=Pokecraft::FightEngine::fightEngine.monsters[monster.monster];
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
            if(!Pokecraft::FightEngine::fightEngine.monsterBuffs.contains(monster.buffs.at(sub_index).buff))
            {
                error(QString("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff));
                return false;
            }
            if(monster.buffs.at(sub_index).level>Pokecraft::FightEngine::fightEngine.monsterBuffs[monster.buffs.at(sub_index).buff].level.size())
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
            if(!Pokecraft::FightEngine::fightEngine.monsterSkills.contains(monster.skills.at(sub_index).skill))
            {
                error(QString("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill));
                return false;
            }
            if(monster.skills.at(sub_index).level>Pokecraft::FightEngine::fightEngine.monsterSkills[monster.skills.at(sub_index).skill].level.size())
            {
                error(QString("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level));
                return false;
            }
            sub_index++;
        }
        index++;
    }
    Pokecraft::FightEngine::fightEngine.setPlayerMonster(Pokecraft::Api_client_real::client->player_informations.playerMonster);
    return true;
}

void BaseWindow::load_monsters()
{
    const QList<PlayerMonster> &playerMonster=Pokecraft::FightEngine::fightEngine.getPlayerMonster();
    ui->monsterList->clear();
    int index=0;
    int size=playerMonster.size();
    while(index<size)
    {
        const PlayerMonster &monster=playerMonster.at(index);
        if(Pokecraft::FightEngine::fightEngine.monsters.contains(monster.monster))
        {
            Monster::Stat stat=Pokecraft::FightEngine::getStat(Pokecraft::FightEngine::fightEngine.monsters[monster.monster],monster.level);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("%1, level: %2\nHP: %3/%4")
                    .arg(Pokecraft::FightEngine::fightEngine.monsterExtra[monster.monster].name)
                    .arg(monster.level)
                    .arg(monster.hp)
                    .arg(stat.hp)
                    );
            item->setToolTip(Pokecraft::FightEngine::fightEngine.monsterExtra[monster.monster].description);
            item->setIcon(Pokecraft::FightEngine::fightEngine.monsterExtra[monster.monster].front);
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

void BaseWindow::fightCollision(Pokecraft::Map_client *map, const quint8 &x, const quint8 &y)
{
    if(!Pokecraft::FightEngine::fightEngine.haveOtherMonster())
    {
        qDebug() << "is in fight but without monster";
        return;
    }
    if(MoveOnTheMap::isGrass(*map,x,y))
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
    else
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
    PlayerMonster otherMonster=Pokecraft::FightEngine::fightEngine.getOtherMonster();
    PlayerMonster fightMonster=Pokecraft::FightEngine::fightEngine.getFightMonster();
    qDebug() << QString("You are in front of monster id: %1 level: %2").arg(otherMonster.monster).arg(otherMonster.level);
    ui->labelFightEnter->setText(tr("A wild %1 is in front of you!").arg(Pokecraft::FightEngine::fightEngine.monsterExtra[otherMonster.monster].name));
    updateOtherMonsterInformation();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->stackedWidget->setCurrentWidget(ui->page_battle);
    ui->pushButtonFightEnterNext->setVisible(true);
    //current monster
    ui->frameFightBottom->setVisible(false);
    ui->labelFightBottomName->setText(Pokecraft::FightEngine::fightEngine.monsterExtra[fightMonster.monster].name);
    ui->labelFightBottomLevel->setText(tr("Level: %1").arg(fightMonster.level));
    Monster::Stat fightStat=Pokecraft::FightEngine::getStat(Pokecraft::FightEngine::fightEngine.monsters[fightMonster.monster],fightMonster.level);
    ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
    ui->progressBarFightBottomHP->setValue(fightMonster.hp);
    ui->labelFightBottomHP->setText(QString("%1/%2").arg(fightMonster.hp).arg(fightStat.hp));
    ui->progressBarFightBottopExp->setMaximum(Pokecraft::FightEngine::fightEngine.monsters[fightMonster.monster].level_to_xp.at(fightMonster.level-1));
    ui->progressBarFightBottopExp->setValue(fightMonster.remaining_xp);
}

void BaseWindow::on_pushButtonFightEnterNext_clicked()
{
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    moveFightMonsterBottom();
    PlayerMonster monster=Pokecraft::FightEngine::fightEngine.getFightMonster();
    ui->labelFightEnter->setText(tr("Protect me %1!").arg(Pokecraft::FightEngine::fightEngine.monsterExtra[monster.monster].name));
}

void BaseWindow::moveFightMonsterBottom()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().rx()>(-ui->labelFightMonsterBottom->size().width()))
            moveFightMonsterBottomTimer.start();
        else
            updateCurrentMonsterInformation();
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setY(p.ry()+4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().ry()<440)
            moveFightMonsterBottomTimer.start();
        else
        {
            Pokecraft::FightEngine::fightEngine.currentMonsterIsKO();
            if(Pokecraft::FightEngine::fightEngine.canDoFight())
                updateCurrentMonsterInformation();
            else
            {
                //no more monster, tp to last recue point
                finalFightText.start();
                ui->labelFightEnter->setText(tr("You lose!<br />Waiting the recue..."));
                ui->pushButtonFightEnterNext->setVisible(false);
                //heal all the monster
                Pokecraft::FightEngine::fightEngine.healAllMonsters();
                //update the view
                load_monsters();
                fightTimerFinish=false;
                timerFightEnd.start();
            }
        }
    }
}

void BaseWindow::teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(!Pokecraft::FightEngine::fightEngine.canDoFight())//then is dead, is teleported to the last rescue point
    {
        if(fightTimerFinish)
            ui->stackedWidget->setCurrentWidget(ui->page_map);
        else
            fightTimerFinish=true;
    }
}

void BaseWindow::fightEnd()
{
    if(!Pokecraft::FightEngine::fightEngine.canDoFight())//then is dead, is teleported to the last rescue point
    {
        if(fightTimerFinish)
            ui->stackedWidget->setCurrentWidget(ui->page_map);
        else
            fightTimerFinish=true;
    }
}

void BaseWindow::updateCurrentMonsterInformation()
{
    PlayerMonster monster=Pokecraft::FightEngine::fightEngine.getFightMonster();
    QPoint p;
    p.setX(60);
    p.setY(280);
    ui->labelFightMonsterBottom->move(p);
    ui->labelFightMonsterBottom->setPixmap(Pokecraft::FightEngine::fightEngine.monsterExtra[monster.monster].back.scaled(160,160));
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    ui->frameFightBottom->setVisible(true);
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    int index=0;
    while(index<monster.skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::Skill &skill=monster.skills.at(index);
        if(skill.level>1)
            item->setText(QString("%1, level %2").arg(Pokecraft::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].name).arg(skill.level));
        else
            item->setText(Pokecraft::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].name);
        item->setToolTip(Pokecraft::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].description);
        fight_attacks_graphical[item]=skill;
        ui->listWidgetFightAttack->addItem(item);
        index++;
    }
    on_listWidgetFightAttack_itemSelectionChanged();
}

void BaseWindow::moveFightMonsterTop()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800)
            moveFightMonsterTopTimer.start();
        else
            updateCurrentMonsterInformation();
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800)
            moveFightMonsterTopTimer.start();
        else
        {
            Pokecraft::FightEngine::fightEngine.dropKOWildMonster();
            if(Pokecraft::FightEngine::fightEngine.haveOtherMonster())
                updateOtherMonsterInformation();
            else
            {
                //have win, return to the map
                finalFightText.start();
                ui->labelFightEnter->setText(tr("You win!"));
                ui->pushButtonFightEnterNext->setVisible(false);
                load_monsters();
            }
        }
    }
}

void BaseWindow::updateOtherMonsterInformation()
{
    QPoint p;
    p.setX(510);
    p.setY(90);
    ui->labelFightMonsterTop->move(p);
    PlayerMonster otherMonster=Pokecraft::FightEngine::fightEngine.getOtherMonster();
    ui->labelFightMonsterTop->setPixmap(Pokecraft::FightEngine::fightEngine.monsterExtra[otherMonster.monster].front.scaled(160,160));
    //other monster
    ui->frameFightTop->setVisible(true);
    ui->labelFightTopName->setText(Pokecraft::FightEngine::fightEngine.monsterExtra[otherMonster.monster].name);
    ui->labelFightTopLevel->setText(tr("Level: %1").arg(otherMonster.level));
    Monster::Stat otherStat=Pokecraft::FightEngine::getStat(Pokecraft::FightEngine::fightEngine.monsters[otherMonster.monster],otherMonster.level);
    ui->progressBarFightTopHP->setMaximum(otherStat.hp);
    ui->progressBarFightTopHP->setValue(otherMonster.hp);
}

void BaseWindow::otherMonsterAttackUpdate()
{
    otherMonsterAttackInt++;
    if(otherMonsterAttackInt%100 /* each 400ms */)
    {
        if(updateAttackTime.elapsed()<2000 /* 2000ms */)
            ui->labelFightMonsterBottom->setVisible(!ui->labelFightMonsterBottom->isVisible());
        else
            ui->labelFightMonsterBottom->setVisible(true);
    }
    //alternate the text
    if(updateAttackTime.elapsed()>1500 /* 2000ms */)
    {
        if(updateAttackTextTime.elapsed()>1500)
        {
            if(lifeEffectOtherMonsterIndex<Pokecraft::FightEngine::fightEngine.lifeEffectOtherMonster.size())
            {
                Monster::Skill::LifeEffectReturn effect=Pokecraft::FightEngine::fightEngine.lifeEffectOtherMonster.at(lifeEffectOtherMonsterIndex);
                if(effect.on==Monster::ApplyOn_AloneEnemy || effect.on==Monster::ApplyOn_AllEnemy)
                {
                    if(effect.quantity<0)
                        ui->labelFightEnter->setText(tr("The wild %1 take %2 of damage").arg(Pokecraft::FightEngine::fightEngine.monsterExtra[Pokecraft::FightEngine::fightEngine.getOtherMonster().monster].name).arg(-effect.quantity));
                    else
                        ui->labelFightEnter->setText(tr("The wild %1 is healed of %2").arg(Pokecraft::FightEngine::fightEngine.monsterExtra[Pokecraft::FightEngine::fightEngine.getOtherMonster().monster].name).arg(effect.quantity));
                }
                else
                {
                    if(effect.quantity<0)
                        ui->labelFightEnter->setText(tr("You take %1 of damage").arg(-effect.quantity));
                    else
                        ui->labelFightEnter->setText(tr("You are healed of %1").arg(effect.quantity));
                }

            }
            if(lifeEffectOtherMonsterIndex<=Pokecraft::FightEngine::fightEngine.lifeEffectOtherMonster.size())
                lifeEffectOtherMonsterIndex++;
            //show the buff
            updateAttackTextTime.restart();
        }
    }
    int hp_to_remove=ui->progressBarFightBottomHP->maximum()/200;//0.5%
    if(hp_to_remove==0)
        hp_to_remove=1;
    PlayerMonster currentMonster=Pokecraft::FightEngine::fightEngine.getFightMonster();
    if(updateAttackTime.elapsed()>3000 /*3000ms*/ && (lifeEffectOtherMonsterIndex)>Pokecraft::FightEngine::fightEngine.lifeEffectOtherMonster.size() &&
            (ui->progressBarFightBottomHP->value()<=hp_to_remove || (ui->progressBarFightBottomHP->value()-hp_to_remove)<=currentMonster.hp))
    {
        //attack is finish
        finalMonstersUpdate();
    }
    else
    {
        if((quint32)ui->progressBarFightBottomHP->value()>currentMonster.hp)
        {
            ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()-hp_to_remove);
            ui->labelFightBottomHP->setText(QString("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
        }
        otherMonsterAttack.start();
    }
}

void BaseWindow::finalMonstersUpdate()
{
    ui->labelFightMonsterBottom->setVisible(true);
    ui->labelFightMonsterTop->setVisible(true);
    if(!Pokecraft::FightEngine::fightEngine.currentMonsterIsKO())
    {
        Monster::Stat fightStat=Pokecraft::FightEngine::getStat(Pokecraft::FightEngine::fightEngine.monsters[Pokecraft::FightEngine::fightEngine.getFightMonster().monster],Pokecraft::FightEngine::fightEngine.getFightMonster().level);
        ui->labelFightBottomHP->setText(QString("%1/%2").arg(Pokecraft::FightEngine::fightEngine.getFightMonster().hp).arg(fightStat.hp));
        ui->progressBarFightBottomHP->setValue(Pokecraft::FightEngine::fightEngine.getFightMonster().hp);
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    }
    else
    {
        //current player monster is dead
        moveType=MoveType_Dead;
        moveFightMonsterBottom();
        ui->labelFightEnter->setText(tr("%1 have lost!").arg(Pokecraft::FightEngine::fightEngine.monsterExtra[Pokecraft::FightEngine::fightEngine.getFightMonster().monster].name));
    }
}

void BaseWindow::currentMonsterAttackUpdate()
{
}

void BaseWindow::on_toolButtonFightQuit_clicked()
{
    if(!Pokecraft::FightEngine::fightEngine.canDoFightAction())
    {
        QMessageBox::warning(this,"Server problem","Sorry but the client wait more data from the server to do it");
        return;
    }
    if(Pokecraft::FightEngine::fightEngine.tryEscape())
    {
        qDebug() << "escape is successful";
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    }
    else
    {//the other attack
        bool ok;
        quint32 attack=Pokecraft::FightEngine::fightEngine.generateOtherAttack(&ok);
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->pushButtonFightEnterNext->setVisible(false);
        if(ok)
            ui->labelFightEnter->setText(tr("The wild %1 do the attack %2")
                    .arg(Pokecraft::FightEngine::fightEngine.monsterExtra[Pokecraft::FightEngine::fightEngine.getOtherMonster().monster].name)
                    .arg(Pokecraft::FightEngine::fightEngine.monsterSkillsExtra[attack].name)
                    );
        else
        {
            ui->labelFightEnter->setText(tr("The wild %1 can't attack")
                    .arg(Pokecraft::FightEngine::fightEngine.monsterExtra[Pokecraft::FightEngine::fightEngine.getOtherMonster().monster].name)
                    );
        }
        lifeEffectOtherMonsterIndex=0;
        otherMonsterAttackInt=0;
        updateAttackTextTime.restart();
        updateAttackTime.restart();
        otherMonsterAttack.start();
    }
}

void BaseWindow::on_pushButtonFightAttack_clicked()
{
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageAttack);
}

void BaseWindow::on_pushButtonFightMonster_clicked()
{
}

void BaseWindow::on_pushButtonFightAttackConfirmed_clicked()
{
}

void BaseWindow::on_pushButtonFightReturn_clicked()
{
    ui->listWidgetFightAttack->reset();
    on_listWidgetFightAttack_itemSelectionChanged();
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
}

void BaseWindow::on_listWidgetFightAttack_itemSelectionChanged()
{
    QList<QListWidgetItem *> itemsList=ui->listWidgetFightAttack->selectedItems();
    if(itemsList.size()!=1)
    {
        ui->labelFightAttackDetails->setText(tr("Select an attack"));
        return;
    }
    quint32 skillId=fight_attacks_graphical[itemsList.first()].skill;
    ui->labelFightAttackDetails->setText(Pokecraft::FightEngine::fightEngine.monsterSkillsExtra[skillId].description);
}

void BaseWindow::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}
