#include "../base/interface/BaseWindow.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralStructures.h"
#include "FightEngine.h"
#include "ui_BaseWindow.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>

/* show only the plant into the inventory */

using namespace CatchChallenger;

//need correct match, else tp to die can failed and do mistake
bool BaseWindow::check_monsters()
{
    int size=CatchChallenger::Api_client_real::client->player_informations.playerMonster.size();
    int index=0;
    int sub_size;
    int sub_index;
    while(index<size)
    {
        const PlayerMonster &monster=CatchChallenger::Api_client_real::client->player_informations.playerMonster.at(index);
        if(!CatchChallenger::FightEngine::fightEngine.monsters.contains(monster.monster))
        {
            error(QString("the monster %1 is not into monster list").arg(monster.monster));
            return false;
        }
        const Monster &monsterDef=CatchChallenger::FightEngine::fightEngine.monsters[monster.monster];
        if(monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
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
            if(!CatchChallenger::FightEngine::fightEngine.monsterBuffs.contains(monster.buffs.at(sub_index).buff))
            {
                error(QString("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff));
                return false;
            }
            if(monster.buffs.at(sub_index).level>CatchChallenger::FightEngine::fightEngine.monsterBuffs[monster.buffs.at(sub_index).buff].level.size())
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
            if(!CatchChallenger::FightEngine::fightEngine.monsterSkills.contains(monster.skills.at(sub_index).skill))
            {
                error(QString("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill));
                return false;
            }
            if(monster.skills.at(sub_index).level>CatchChallenger::FightEngine::fightEngine.monsterSkills[monster.skills.at(sub_index).skill].level.size())
            {
                error(QString("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level));
                return false;
            }
            sub_index++;
        }
        index++;
    }
    CatchChallenger::FightEngine::fightEngine.setPlayerMonster(CatchChallenger::Api_client_real::client->player_informations.playerMonster);
    return true;
}

void BaseWindow::load_monsters()
{
    const QList<PlayerMonster> &playerMonster=CatchChallenger::FightEngine::fightEngine.getPlayerMonster();
    ui->monsterList->clear();
    monsters_items_graphical.clear();
    int index=0;
    int size=playerMonster.size();
    while(index<size)
    {
        const PlayerMonster &monster=playerMonster.at(index);
        if(CatchChallenger::FightEngine::fightEngine.monsters.contains(monster.monster))
        {
            Monster::Stat stat=CatchChallenger::FightEngine::getStat(CatchChallenger::FightEngine::fightEngine.monsters[monster.monster],monster.level);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(tr("%1, level: %2\nHP: %3/%4")
                    .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].name)
                    .arg(monster.level)
                    .arg(monster.hp)
                    .arg(stat.hp)
                    );
            item->setToolTip(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].description);
            item->setIcon(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].front);
            ui->monsterList->addItem(item);
            monsters_items_graphical[item]=monster.id;
        }
        else
        {
            error(QString("Unknown monster: %1").arg(monster.monster));
            return;
        }
        index++;
    }
}

void BaseWindow::fightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y)
{
    if(!CatchChallenger::FightEngine::fightEngine.haveOtherMonster())
    {
        qDebug() << "is in fight but without monster";
        return;
    }
    if(MoveOnTheMap::isGrass(*map,x,y))
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
    else
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
    PlayerMonster otherMonster=CatchChallenger::FightEngine::fightEngine.getOtherMonster();
    PlayerMonster fightMonster=CatchChallenger::FightEngine::fightEngine.getFightMonster();
    qDebug() << QString("You are in front of monster id: %1 level: %2").arg(otherMonster.monster).arg(otherMonster.level);
    ui->labelFightEnter->setText(tr("A wild %1 is in front of you!").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[otherMonster.monster].name));
    updateOtherMonsterInformation();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->stackedWidget->setCurrentWidget(ui->page_battle);
    ui->pushButtonFightEnterNext->setVisible(true);
    //current monster
    ui->frameFightBottom->setVisible(false);
    ui->labelFightBottomName->setText(CatchChallenger::FightEngine::fightEngine.monsterExtra[fightMonster.monster].name);
    ui->labelFightBottomLevel->setText(tr("Level: %1").arg(fightMonster.level));
    Monster::Stat fightStat=CatchChallenger::FightEngine::getStat(CatchChallenger::FightEngine::fightEngine.monsters[fightMonster.monster],fightMonster.level);
    ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
    ui->progressBarFightBottomHP->setValue(fightMonster.hp);
    ui->labelFightBottomHP->setText(QString("%1/%2").arg(fightMonster.hp).arg(fightStat.hp));
    ui->progressBarFightBottopExp->setMaximum(CatchChallenger::FightEngine::fightEngine.monsters[fightMonster.monster].level_to_xp.at(fightMonster.level-1));
    ui->progressBarFightBottopExp->setValue(fightMonster.remaining_xp);
}

void BaseWindow::on_pushButtonFightEnterNext_clicked()
{
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    moveFightMonsterBottom();
    PlayerMonster monster=CatchChallenger::FightEngine::fightEngine.getFightMonster();
    ui->labelFightEnter->setText(tr("Protect me %1!").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].name));
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
            doNextAction();
    }
}

void BaseWindow::teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(CatchChallenger::FightEngine::fightEngine.canDoFight())//then is dead, is teleported to the last rescue point
    {
        if(fightTimerFinish)
            lose();
        else
            fightTimerFinish=true;
    }
}

void BaseWindow::updateCurrentMonsterInformation()
{
    PlayerMonster monster=CatchChallenger::FightEngine::fightEngine.getFightMonster();
    QPoint p;
    p.setX(60);
    p.setY(280);
    ui->labelFightMonsterBottom->move(p);
    ui->labelFightMonsterBottom->setPixmap(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].back.scaled(160,160));
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    ui->frameFightBottom->setVisible(true);
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    int index=0;
    while(index<monster.skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::PlayerSkill &skill=monster.skills.at(index);
        if(skill.level>1)
            item->setText(QString("%1, level %2").arg(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].name).arg(skill.level));
        else
            item->setText(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].name);
        item->setToolTip(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[skill.skill].description);
        fight_attacks_graphical[item]=skill.skill;
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
            doNextAction();
    }
}

void BaseWindow::updateOtherMonsterInformation()
{
    QPoint p;
    p.setX(510);
    p.setY(90);
    ui->labelFightMonsterTop->move(p);
    PlayerMonster otherMonster=CatchChallenger::FightEngine::fightEngine.getOtherMonster();
    ui->labelFightMonsterTop->setPixmap(CatchChallenger::FightEngine::fightEngine.monsterExtra[otherMonster.monster].front.scaled(160,160));
    //other monster
    ui->frameFightTop->setVisible(true);
    ui->labelFightTopName->setText(CatchChallenger::FightEngine::fightEngine.monsterExtra[otherMonster.monster].name);
    ui->labelFightTopLevel->setText(tr("Level: %1").arg(otherMonster.level));
    Monster::Stat otherStat=CatchChallenger::FightEngine::getStat(CatchChallenger::FightEngine::fightEngine.monsters[otherMonster.monster],otherMonster.level);
    ui->progressBarFightTopHP->setMaximum(otherStat.hp);
    ui->progressBarFightTopHP->setValue(otherMonster.hp);
}

void BaseWindow::on_toolButtonFightQuit_clicked()
{
    if(!CatchChallenger::FightEngine::fightEngine.canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=true;
    if(CatchChallenger::FightEngine::fightEngine.tryEscape())
        escapeSuccess=true;
    else
        escapeSuccess=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    doNextAction();
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
    if(!CatchChallenger::FightEngine::fightEngine.canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    QList<QListWidgetItem *> itemsList=ui->listWidgetFightAttack->selectedItems();
    if(itemsList.size()!=1)
    {
        QMessageBox::warning(this,tr("Selection error"),tr("You need select an attack"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    CatchChallenger::FightEngine::fightEngine.useSkill(fight_attacks_graphical[itemsList.first()]);
    doNextAction();
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
    quint32 skillId=fight_attacks_graphical[itemsList.first()];
    ui->labelFightAttackDetails->setText(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[skillId].description);
}

void BaseWindow::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::lose()
{
    CatchChallenger::FightEngine::fightEngine.healAllMonsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
}

void BaseWindow::win()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
}

void BaseWindow::doNextAction()
{
    if(escape)
    {
        if(!escapeSuccess)
        {//the other attack
            escape=false;
            CatchChallenger::FightEngine::fightEngine.generateOtherAttack();
            if(!CatchChallenger::FightEngine::fightEngine.attackReturnList.empty())
                displayText(tr("You have failed!"));
            else
                displayText(tr("The wild %1 can't attack").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getOtherMonster().monster].name));
        }
    }
    if(doNextActionStep==DoNextActionStep_Start || !CatchChallenger::FightEngine::fightEngine.attackReturnList.empty())
    {
        fightTimerFinish=false;
        displayAttackProgression=0;
    }
    //apply the effect
    if(!CatchChallenger::FightEngine::fightEngine.attackReturnList.empty())
    {
        displayAttack();
        return;
    }
    //if the current monster is KO
    if(CatchChallenger::FightEngine::fightEngine.canDoFight() && CatchChallenger::FightEngine::fightEngine.currentMonsterIsKO())
    {
        CatchChallenger::FightEngine::fightEngine.dropKOCurrentMonster();
        doNextActionStep=DoNextActionStep_Start;
        //current player monster is KO
        moveType=MoveType_Dead;
        moveFightMonsterBottom();
        ui->labelFightEnter->setText(tr("%1 have lost!").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getFightMonster().monster].name));
        return;
    }
    //if the current monster is KO
    if(CatchChallenger::FightEngine::fightEngine.isInFight() && CatchChallenger::FightEngine::fightEngine.wildMonsterIsKO())
    {
        ui->labelFightEnter->setText(tr("The wild %1 have lost!").arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getOtherMonster().monster].name));
        CatchChallenger::FightEngine::fightEngine.dropKOWildMonster();
        doNextActionStep=DoNextActionStep_Start;
        //current player monster is KO
        moveType=MoveType_Dead;
        moveFightMonsterTop();
        return;
    }
    if(doNextActionStep==DoNextActionStep_Lose)
    {
        if(fightTimerFinish)
            lose();
        else
            fightTimerFinish=true;
        return;
    }
    //if lose
    if(!CatchChallenger::FightEngine::fightEngine.canDoFight())
        if(doNextActionStep<DoNextActionStep_Lose)
        {
            displayText(tr("You lose!"));
            doNextActionStep=DoNextActionStep_Lose;
            return;
        }
    if(doNextActionStep==DoNextActionStep_Win)
    {
        win();
        return;
    }
    //if win
    if(CatchChallenger::FightEngine::fightEngine.wildMonsters.empty())
    {
        doNextActionStep=DoNextActionStep_Win;
        if(!escape)
            displayText(tr("You win!"));
        else
        {
            if(escapeSuccess)
                displayText(tr("Your escape is successful"));
            else
                displayText(tr("Your escape have failed but you win"));
        }
        if(escape)
        {
            fightTimerFinish=true;
            doNextAction();
        }
        return;
    }
    //replace the KO monster
    if(ui->progressBarFightTopHP->value()==0)
    {
        QMessageBox::information(this,"Todo","Part todo");
    }
    if(ui->progressBarFightBottomHP->value()==0)
    {
        CatchChallenger::FightEngine::fightEngine.currentMonsterIsKO();
        updateCurrentMonsterInformation();
    }
    //nothing to done, show the menu
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    return;
}

void BaseWindow::displayAttack()
{
    bool applyOnOtherMonster=(
                CatchChallenger::FightEngine::fightEngine.attackReturnList.first().doByTheCurrentMonster &&
                (CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().on==ApplyOn_AloneEnemy ||
                 CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().on==ApplyOn_AllEnemy)
        ) || (
                    !CatchChallenger::FightEngine::fightEngine.attackReturnList.first().doByTheCurrentMonster &&
                    (CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().on==ApplyOn_Themself ||
                     CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().on==ApplyOn_AllAlly)
            );
    //if start, display text
    if(displayAttackProgression==0)
    {
        updateAttackTime.restart();
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->pushButtonFightEnterNext->setVisible(false);
        QString attackOwner;
        if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().doByTheCurrentMonster)
            attackOwner=tr("Your %1 do the attack %2")
                .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getFightMonster().monster].name)
                .arg(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[CatchChallenger::FightEngine::fightEngine.attackReturnList.first().attack].name);
        else
            attackOwner=tr("The wild %1 do the attack %2")
                .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getOtherMonster().monster].name)
                .arg(CatchChallenger::FightEngine::fightEngine.monsterSkillsExtra[CatchChallenger::FightEngine::fightEngine.attackReturnList.first().attack].name);
        QString damage;
        qint32 quantity=CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity;
        if(applyOnOtherMonster)
        {
            if(quantity>0)
                damage=tr("The wild %1 is healed of %2")
                    .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getOtherMonster().monster].name)
                    .arg(quantity);
            else
                damage=tr("The wild %1 take %2 of damage")
                    .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getOtherMonster().monster].name)
                    .arg(-quantity);
        }
        else
        {
            if(quantity>0)
                damage=tr("Your %1 is healed of %2")
                    .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getFightMonster().monster].name)
                    .arg(quantity);
            else
                damage=tr("Your %1 take %2 of damage")
                    .arg(CatchChallenger::FightEngine::fightEngine.monsterExtra[CatchChallenger::FightEngine::fightEngine.getFightMonster().monster].name)
                    .arg(-quantity);
        }
        ui->labelFightEnter->setText(QString("%1<br />%2").arg(attackOwner).arg(damage));
    }
    if(displayAttackProgression%100 /* each 400ms */)
    {
        if(applyOnOtherMonster)
        {
            if(updateAttackTime.elapsed()<2000 /* 2000ms */)
                ui->labelFightMonsterTop->setVisible(!ui->labelFightMonsterTop->isVisible());
            else
                ui->labelFightMonsterTop->setVisible(true);
        }
        else
        {
            if(updateAttackTime.elapsed()<2000 /* 2000ms */)
                ui->labelFightMonsterBottom->setVisible(!ui->labelFightMonsterBottom->isVisible());
            else
                ui->labelFightMonsterBottom->setVisible(true);
        }
    }
    int hp_to_change;
    if(applyOnOtherMonster)
        hp_to_change=ui->progressBarFightTopHP->maximum()/200;//0.5%
    else
        hp_to_change=ui->progressBarFightBottomHP->maximum()/200;//0.5%
    if(hp_to_change==0)
        hp_to_change=1;
    if(updateAttackTime.elapsed()>3000 /*3000ms*/)
    {
        CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.removeFirst();
        if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.isEmpty())
            CatchChallenger::FightEngine::fightEngine.attackReturnList.removeFirst();
        //attack is finish
        doNextAction();
    }
    else
    {
        if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity<0)
        {
            hp_to_change=-hp_to_change;
            if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity>hp_to_change)
                hp_to_change=CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity;
        }
        else if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity>0)
        {
            hp_to_change=-hp_to_change;
            if(CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity<hp_to_change)
                hp_to_change=CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity;
        }
        else
            hp_to_change=0;
        if(hp_to_change!=0)
        {
            CatchChallenger::FightEngine::fightEngine.attackReturnList.first().lifeEffectMonster.first().quantity-=hp_to_change;
            if(applyOnOtherMonster)
                ui->progressBarFightTopHP->setValue(ui->progressBarFightTopHP->value()+hp_to_change);
            else
            {
                ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+hp_to_change);
                ui->labelFightBottomHP->setText(QString("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
            }
        }
        displayAttackTimer.start();
    }
    displayAttackProgression++;
}

void BaseWindow::displayText(const QString &text)
{
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(text);
    doNextActionTimer.start();
}

void BaseWindow::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    tradeOtherMonsters << monster;
    QString genderString;
    switch(monster.gender)
    {
        case 0x01:
        genderString=tr("Male");
        break;
        case 0x02:
        genderString=tr("Female");
        break;
        default:
        genderString=tr("Unknown");
        break;
    }
    QListWidgetItem *item=new QListWidgetItem();
    if(CatchChallenger::FightEngine::fightEngine.monsterExtra.contains(monster.monster))
    {
        item->setIcon(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].front);
        item->setText(CatchChallenger::FightEngine::fightEngine.monsterExtra[monster.monster].name);
        item->setToolTip(QString("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/front.png"));
        item->setText(tr("Unknown"));
        item->setToolTip(QString("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    ui->tradeOtherMonsters->addItem(item);
}
