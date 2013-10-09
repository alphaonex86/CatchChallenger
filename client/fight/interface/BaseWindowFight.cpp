#include "../base/interface/BaseWindow.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../base/ClientVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralStructures.h"
#include "ClientFightEngine.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/CommonDatapack.h"

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
        if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster))
        {
            error(QString("the monster %1 is not into monster list").arg(monster.monster));
            return false;
        }
        const Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster];
        if(monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            error(QString("the level %1 is greater than max level").arg(monster.level));
            return false;
        }
        Monster::Stat stat=ClientFightEngine::getStat(monsterDef,monster.level);
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
            if(!CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.contains(monster.buffs.at(sub_index).buff))
            {
                error(QString("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff));
                return false;
            }
            if(monster.buffs.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs[monster.buffs.at(sub_index).buff].level.size())
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
            if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(monster.skills.at(sub_index).skill))
            {
                error(QString("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill));
                return false;
            }
            if(monster.skills.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[monster.skills.at(sub_index).skill].level.size())
            {
                error(QString("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level));
                return false;
            }
            sub_index++;
        }
        index++;
    }
    //CatchChallenger::ClientFightEngine::fightEngine.setPlayerMonster(CatchChallenger::Api_client_real::client->player_informations.playerMonster);
    return true;
}

void BaseWindow::load_monsters()
{
    const QList<PlayerMonster> &playerMonsters=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
    const PlayerMonster * currentMonster=ClientFightEngine::fightEngine.getCurrentMonster();
    ui->monsterList->clear();
    monsters_items_graphical.clear();
    int index=0;
    int size=playerMonsters.size();
    while(index<size)
    {
        const PlayerMonster &monster=playerMonsters.at(index);
        if(inSelection)
        {
            if(waitedObjectType==ObjectType_MonsterToFight && monster.id==currentMonster->id)
            {
                index++;
                continue;
            }
            switch(waitedObjectType)
            {
                case ObjectType_MonsterToFight:
                case ObjectType_MonsterToFightKO:
                    if(monster.hp==0 || monster.egg_step>0)
                    {
                        index++;
                        continue;
                    }
                break;
                default:
                break;
            }
        }
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster))
        {
            Monster::Stat stat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster],monster.level);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(tr("%1, level: %2\nHP: %3/%4")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].name)
                    .arg(monster.level)
                    .arg(monster.hp)
                    .arg(stat.hp)
                    );
            item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].description);
            if(!DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].thumb.isNull())
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].thumb);
            else
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].front);
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
    if(inSelection)
    {
       ui->monsterListMoveUp->setVisible(false);
       ui->monsterListMoveDown->setVisible(false);
       ui->toolButton_monster_list_quit->setVisible(waitedObjectType!=ObjectType_MonsterToFightKO);
    }
    else
    {
        ui->monsterListMoveUp->setVisible(true);
        ui->monsterListMoveDown->setVisible(true);
        ui->toolButton_monster_list_quit->setVisible(true);
        on_monsterList_itemSelectionChanged();
    }
    ui->selectMonster->setVisible(true);
}

void BaseWindow::wildFightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y)
{
    if(!fightCollision(map,x,y))
        return;
    battleType=BattleType_Wild;
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at wildFightCollision()");
        return;
    }
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("A other %1 is in front of you!").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    resetPosition(true);
    moveFightMonsterBoth();
}

void BaseWindow::botFight(const quint32 &fightId)
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightId))
    {
        emit error("fight id not found at collision");
        return;
    }
    botFightList << fightId;
    if(botFightList.size()>1 || !battleInformationsList.isEmpty())
        return;
    botFightFull(fightId);
}

void BaseWindow::botFightFull(const quint32 &fightId)
{
    ui->frameFightTop->setVisible(false);
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(DatapackClientLoader::datapackLoader.botFightsExtra[fightId].start);
    ui->pushButtonFightEnterNext->setVisible(false);
    QList<PlayerMonster> botFightMonstersTransformed;
    const QList<BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapack::commonDatapack.botFights[fightId].monsters;
    int index=0;
    while(index<monsters.size())
    {
        botFightMonstersTransformed << FacilityLib::botFightMonsterToPlayerMonster(monsters.at(index),ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[monsters.at(index).id],monsters.at(index).level));
        index++;
    }
    CatchChallenger::ClientFightEngine::fightEngine.setBotMonster(botFightMonstersTransformed);
    this->fightId=fightId;
    init_environement_display(MapController::mapController->getMapObject(),MapController::mapController->getX(),MapController::mapController->getY());
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    ui->pushButtonFightEnterNext->setVisible(false);
    battleType=BattleType_Bot;
    QPixmap botImage;
    if(actualBot.properties.contains("skin"))
        botImage=getFrontSkin(actualBot.properties["skin"]);
    else
        botImage=QPixmap(":/images/player_default/front.png");
    ui->labelFightMonsterTop->setPixmap(botImage.scaled(160,160));
    qDebug() << QString("The bot %1 is a front of you").arg(fightId);
    battleStep=BattleStep_Presentation;
    moveType=MoveType_Enter;
    resetPosition(true);
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
}

bool BaseWindow::fightCollision(CatchChallenger::Map_client *map, const quint8 &x, const quint8 &y)
{
    if(!CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        emit error("is in fight but without monster");
        return false;
    }
    init_environement_display(map,x,y);
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    init_other_monster_display();
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at fightCollision()");
        return false;
    }
    qDebug() << QString("You are in front of monster id: %1 level: %2").arg(otherMonster->monster).arg(otherMonster->level);
    return true;
}

void BaseWindow::init_environement_display(Map_client *map, const quint8 &x, const quint8 &y)
{
    //map not located
    if(map==NULL)
    {
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
        return;
    }
    //map is grass
    if(MoveOnTheMap::isGrass(*map,x,y))
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
    //map is other
    else
        ui->frameFightBackground->setStyleSheet("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}");
}

void BaseWindow::init_other_monster_display()
{
    updateOtherMonsterInformation();
}

void BaseWindow::init_current_monster_display()
{
    PlayerMonster *fightMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(fightMonster!=NULL)
    {
        //current monster
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->stackedWidget->setCurrentWidget(ui->page_battle);
        ui->pushButtonFightEnterNext->setVisible(true);
        ui->frameFightBottom->setVisible(false);
        ui->labelFightBottomName->setText(DatapackClientLoader::datapackLoader.monsterExtra[fightMonster->monster].name);
        ui->labelFightBottomLevel->setText(tr("Level: %1").arg(fightMonster->level));
        Monster::Stat fightStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[fightMonster->monster],fightMonster->level);
        ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
        ui->progressBarFightBottomHP->setValue(fightMonster->hp);
        ui->labelFightBottomHP->setText(QString("%1/%2").arg(fightMonster->hp).arg(fightStat.hp));
        const Monster &monsterGenericInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters[fightMonster->monster];
        int level_to_xp=monsterGenericInfo.level_to_xp.at(fightMonster->level-1);
        ui->progressBarFightBottomExp->setMaximum(level_to_xp);
        ui->progressBarFightBottomExp->setValue(fightMonster->remaining_xp);
        //do the buff
        {
            buffToGraphicalItemBottom.clear();
            ui->bottomBuff->clear();
            int index=0;
            while(index<fightMonster->buffs.size())
            {
                PlayerBuff buffEffect=fightMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].name);
                    else
                        item->setToolTip(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].name).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].description);
                }
                buffToGraphicalItemBottom[buffEffect.buff]=item;
                ui->bottomBuff->addItem(item);
                index++;
            }
        }
    }
    else
        emit error("Try fight without monster");
    battleStep=BattleStep_PresentationMonster;
}

void BaseWindow::on_pushButtonFightEnterNext_clicked()
{
    ui->pushButtonFightEnterNext->setVisible(false);
    switch(battleStep)
    {
        case BattleStep_Presentation:
            if(CatchChallenger::ClientFightEngine::fightEngine.isInFightWithWild())
            {
                moveType=MoveType_Leave;
                moveFightMonsterBottom();
            }
            else
            {
                moveType=MoveType_Leave;
                moveFightMonsterBoth();
            }
        break;
        case BattleStep_PresentationMonster:
        default:
            ui->frameFightTop->show();
            ui->frameFightBottom->show();
            moveType=MoveType_Enter;
            moveFightMonsterBottom();
            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
            PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
            if(monster!=NULL)
                ui->labelFightEnter->setText(tr("Protect me %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra[monster->monster].name));
            else
            {
                qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                ui->labelFightEnter->setText(tr("Protect me %1!").arg("(Unknow monster)"));
            }
        break;
    }
}

void BaseWindow::moveFightMonsterBottom()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().rx()<60)
            moveFightMonsterBottomTimer.start();
        else
        {
            updateCurrentMonsterInformation();
            doNextAction();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterBottom->pos().rx()>(-ui->labelFightMonsterBottom->size().width()))
            moveFightMonsterBottomTimer.start();
        else
        {
            updateCurrentMonsterInformation();
            if(battleStep==BattleStep_Presentation)
            {
                resetPosition(true,false,true);
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
                if(monster==NULL)
                {
                    newError(tr("Internal error"),"NULL pointer at updateCurrentMonsterInformation()");
                    return;
                }
                ui->labelFightEnter->setText(tr("Go %1").arg(DatapackClientLoader::datapackLoader.monsterExtra[monster->monster].name));
                moveFightMonsterBottom();
            }
        }
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
            CatchChallenger::ClientFightEngine::fightEngine.dropKOCurrentMonster();
            if(CatchChallenger::ClientFightEngine::fightEngine.haveAnotherMonsterOnThePlayerToFight())
            {
                #ifdef DEBUG_CLIENT_BATTLE
                qDebug() << "Your current monster is KO, select another";
                #endif
                selectObject(ObjectType_MonsterToFight);
            }
            else
            {
                #ifdef DEBUG_CLIENT_BATTLE
                qDebug() << "You lose";
                #endif
                displayText(tr("You lose!"));
                doNextActionStep=DoNextActionStep_Loose;
            }
        }
    }
}

void BaseWindow::teleportTo(const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(CatchChallenger::ClientFightEngine::fightEngine.currentMonsterIsKO() && !CatchChallenger::ClientFightEngine::fightEngine.haveAnotherMonsterOnThePlayerToFight())//then is dead, is teleported to the last rescue point
    {
        qDebug() << "tp on loose: " << fightTimerFinish;
        if(fightTimerFinish)
            loose();
        else
            fightTimerFinish=true;
    }
    else
        qDebug() << "normal tp";
}

void BaseWindow::updateCurrentMonsterInformation()
{
    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
    {
        newError(tr("Internal error"),"Try update the monster when have not any ready monster");
        return;
    }
    PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error"),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    QPoint p;
    p.setX(60);
    p.setY(280);
    ui->labelFightMonsterBottom->move(p);
    ui->labelFightMonsterBottom->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra[monster->monster].back.scaled(160,160));
    ui->frameFightBottom->setVisible(true);
    ui->frameFightBottom->show();
    const Monster &currentmonster=CommonDatapack::commonDatapack.monsters[monster->monster];
    ui->progressBarFightBottomExp->setMaximum(currentmonster.level_to_xp.at(monster->level-1));
    ui->progressBarFightBottomExp->setValue(monster->remaining_xp);
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    int index=0;
    while(index<monster->skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::PlayerSkill &skill=monster->skills.at(index);
        if(skill.level>1)
            item->setText(QString("%1, level %2 (remaining endurance: %3)")
                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[skill.skill].name)
                    .arg(skill.level)
                    .arg(skill.endurance)
                    );
        else
            item->setText(QString("%1 (remaining endurance: %2)")
                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[skill.skill].name)
                    .arg(skill.endurance)
                    );
        item->setToolTip(DatapackClientLoader::datapackLoader.monsterSkillsExtra[skill.skill].description);
        item->setData(99,skill.endurance);
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
        p.setX(p.rx()-4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()>510)
            moveFightMonsterTopTimer.start();
        else
        {
            updateOtherMonsterInformation();
            doNextAction();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800)
            moveFightMonsterTopTimer.start();
        else
        {
            updateOtherMonsterInformation();
            doNextAction();
        }
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
            CatchChallenger::ClientFightEngine::fightEngine.dropKOOtherMonster();
            if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
            {
                if(CatchChallenger::ClientFightEngine::fightEngine.isInBattle() && !CatchChallenger::ClientFightEngine::fightEngine.haveBattleOtherMonster())
                {
                    ui->pushButtonFightEnterNext->setVisible(false);
                    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                    ui->labelFightEnter->setText(tr("In waiting of other monster selection"));
                    return;
                }
                ui->pushButtonFightEnterNext->setVisible(false);
                init_other_monster_display();
                ui->frameFightTop->setVisible(false);
                updateOtherMonsterInformation();
                ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
                if(DatapackClientLoader::datapackLoader.monsterExtra.contains(otherMonster->monster))
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
                else
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(tr("(Unknown monster)")));
                resetPosition(true,true,false);
                moveType=MoveType_Enter;
                moveFightMonsterTop();
            }
            else
            {
                qDebug() << "doNextAction(): you win";
                doNextActionStep=DoNextActionStep_Win;
                if(!escape)
                {
                    PlayerMonster *currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
                    if(currentMonster!=NULL)
                        ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
                    displayText(tr("You win!"));
                }
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
            }
        }
    }
}

void BaseWindow::moveFightMonsterBoth()
{
    if(moveType==MoveType_Enter)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()-4);
        ui->labelFightMonsterTop->move(p);
        p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()+3);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterTop->pos().rx()>510 || ui->labelFightMonsterBottom->pos().rx()<(60))
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                resetPosition();
                ui->pushButtonFightEnterNext->show();
            }
            else if(battleStep==BattleStep_PresentationMonster)
            {
                doNextActionStep=DoNextActionStep_Start;
                doNextAction();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
    if(moveType==MoveType_Leave)
    {
        QPoint p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        p=ui->labelFightMonsterBottom->pos();
        p.setX(p.rx()-3);
        ui->labelFightMonsterBottom->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800 || ui->labelFightMonsterBottom->pos().rx()<(-ui->labelFightMonsterBottom->size().width()))
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                init_current_monster_display();
                ui->pushButtonFightEnterNext->setVisible(false);
                init_other_monster_display();
                ui->frameFightTop->setVisible(false);
                updateCurrentMonsterInformation();
                updateOtherMonsterInformation();
                resetPosition(true);
                QString text;
                PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
                if(otherMonster!=NULL)
                    text+=tr("The other player call %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name);
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("The other player call %1!").arg("(Unknow monster)");
                }
                text+="<br />";
                PublicPlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
                if(monster!=NULL)
                    text+=tr("You call %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra[monster->monster].name);
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("You call %1!").arg("(Unknow monster)");
                }
                ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                ui->labelFightEnter->setText(text);
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                moveFightMonsterBoth();
            }
            else if(battleStep==BattleStep_PresentationMonster)
            {
                doNextActionStep=DoNextActionStep_Start;
                doNextAction();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
    if(moveType==MoveType_Dead)
    {
        QPoint p=ui->labelFightMonsterBottom->pos();
        p.setY(p.ry()+4);
        ui->labelFightMonsterBottom->move(p);
        p=ui->labelFightMonsterTop->pos();
        p.setX(p.rx()+4);
        ui->labelFightMonsterTop->move(p);
        if(ui->labelFightMonsterTop->pos().rx()<800 || ui->labelFightMonsterBottom->pos().ry()<440)
            moveFightMonsterBothTimer.start();
        else
        {
            if(battleStep==BattleStep_Presentation)
            {
                ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
                init_current_monster_display();
                init_other_monster_display();
                updateCurrentMonsterInformation();
                updateOtherMonsterInformation();
                battleStep=BattleStep_PresentationMonster;
                moveType=MoveType_Enter;
                moveFightMonsterBoth();
                ui->frameFightTop->show();
                ui->frameFightBottom->show();
            }
            else
                ui->pushButtonFightEnterNext->show();
        }
    }
}

void BaseWindow::updateOtherMonsterInformation()
{
    if(!CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        newError(tr("Internal error"),"updateOtherMonsterInformation() but have not other monter");
        return;
    }
    QPoint p;
    p.setX(510);
    p.setY(90);
    ui->labelFightMonsterTop->move(p);
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
    {
        ui->labelFightMonsterTop->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].front.scaled(160,160));
        //other monster
        ui->frameFightTop->setVisible(true);
        ui->frameFightTop->show();
        ui->labelFightTopName->setText(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name);
        ui->labelFightTopLevel->setText(tr("Level: %1").arg(otherMonster->level));
        Monster::Stat otherStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[otherMonster->monster],otherMonster->level);
        ui->progressBarFightTopHP->setMaximum(otherStat.hp);
        ui->progressBarFightTopHP->setValue(otherMonster->hp);
        //do the buff
        {
            buffToGraphicalItemTop.clear();
            ui->topBuff->clear();
            int index=0;
            while(index<otherMonster->buffs.size())
            {
                PlayerBuff buffEffect=otherMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].name);
                    else
                        item->setToolTip(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].name).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff].description);
                }
                buffToGraphicalItemTop[buffEffect.buff]=item;
                ui->topBuff->addItem(item);
                index++;
            }
        }
    }
    else
    {
        newError(tr("Internal error"),"Unable to load the other monster");
        return;
    }
}

void BaseWindow::on_toolButtonFightQuit_clicked()
{
    if(!CatchChallenger::ClientFightEngine::fightEngine.canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=true;
    qDebug() << "BaseWindow::on_toolButtonFightQuit_clicked(): fight engine tryEscape()";
    if(CatchChallenger::ClientFightEngine::fightEngine.tryEscape())
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
    selectObject(ObjectType_MonsterToFight);
}

void BaseWindow::on_pushButtonFightAttackConfirmed_clicked()
{
    if(!CatchChallenger::ClientFightEngine::fightEngine.canDoFightAction())
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
    if(itemsList.first()->data(99).toUInt()<=0)
    {
        QMessageBox::warning(this,tr("No endurance"),tr("You have no more endurance to use this skill"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error"),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    CatchChallenger::ClientFightEngine::fightEngine.useSkill(fight_attacks_graphical[itemsList.first()]);
    if(battleType!=BattleType_OtherPlayer)
        doNextAction();
    else
    {
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("In waiting of the other player action"));
        ui->pushButtonFightEnterNext->hide();
    }
    updateCurrentMonsterInformation();
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
    ui->labelFightAttackDetails->setText(DatapackClientLoader::datapackLoader.monsterSkillsExtra[skillId].description);
}

void BaseWindow::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::loose()
{
    qDebug() << "loose()";
    CatchChallenger::ClientFightEngine::fightEngine.healAllMonsters();
    CatchChallenger::ClientFightEngine::fightEngine.fightFinished();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(botFightList.isEmpty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.removeFirst();
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.isEmpty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.removeFirst();
        break;
        default:
        break;
    }
    if(!battleInformationsList.isEmpty())
    {
        const BattleInformations &battleInformations=battleInformationsList.first();
        battleInformationsList.removeFirst();
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.isEmpty())
    {
        quint32 fightId=botFightList.first();
        botFightList.removeFirst();
        botFight(fightId);
    }
}

void BaseWindow::win()
{
    CatchChallenger::ClientFightEngine::fightEngine.fightFinished();
    if(zonecapture)
        ui->stackedWidget->setCurrentWidget(ui->page_zonecapture);
    else
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(!zonecapture)
            {
                if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightId))
                {
                    emit error("fight id not found at collision");
                    return;
                }
                addCash(CatchChallenger::CommonDatapack::commonDatapack.botFights[fightId].cash);
                MapController::mapController->addBeatenBotFight(fightId);
            }
            if(botFightList.isEmpty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.removeFirst();
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.isEmpty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.removeFirst();
        break;
        default:
        break;
    }
    if(!battleInformationsList.isEmpty())
    {
        const BattleInformations &battleInformations=battleInformationsList.first();
        battleInformationsList.removeFirst();
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.isEmpty())
    {
        quint32 fightId=botFightList.first();
        botFightList.removeFirst();
        botFight(fightId);
    }
}

void BaseWindow::doNextAction()
{
    ui->toolButtonFightQuit->setVisible(battleType==BattleType_Wild);
    if(escape)
    {
        doNextActionStep=DoNextActionStep_Start;
        if(!escapeSuccess)
        {//the other attack
            escape=false;//need be dropped to have text to escape
            fightTimerFinish=false;
            CatchChallenger::ClientFightEngine::fightEngine.generateOtherAttack();
            if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().empty())
            {
                qDebug() << "doNextAction(): escape failed: you take damage";
                displayText(tr("You have failed to escape!"));
                return;
            }
            else
            {
                PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
                if(otherMonster==NULL)
                {
                    emit error("NULL pointer for other monster at doNextAction()");
                    return;
                }
                qDebug() << "doNextAction(): escape fail but the wild monster can't attack";
                displayText(tr("The wild %1 can't attack").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
                return;
            }
        }
        else
        {
            qDebug() << "doNextAction(): escape success";
            win();
        }
        return;//useful to quit correctly
    }
    //apply the effect
    if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().empty())
    {
        if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success==false)
        {
            qDebug() << "doNextAction(): attack have failed";
            PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
            if(otherMonster==NULL)
            {
                emit error("NULL pointer for other monster at doNextAction()");
                return;
            }
            PublicPlayerMonster *currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
            if(currentMonster==NULL)
            {
                emit error("NULL pointer for other monster at doNextAction()");
                return;
            }
            quint32 attackId=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack;
            if(attackId==0)
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster)
                    displayText(tr("The wild %1 can't attack").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
                else
                    displayText(tr("Your %1 can't attack").arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name));
            }
            else
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster)
                    displayText(tr("The wild %1 have failed their attack %2")
                                .arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name)
                                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[attackId].name)
                            );
                else
                    displayText(tr("Your %1 have failed their attack %2")
                                .arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name)
                                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[attackId].name)
                            );
            }
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstLifeEffectAttackReturn();
            return;
        }
        qDebug() << "doNextAction(): apply the effect and display it";
        displayAttack();
        return;
    }

    if(doNextActionStep==DoNextActionStep_Loose)
    {
        qDebug() << "doNextAction(): do step loose";
        #ifdef DEBUG_CLIENT_BATTLE
        qDebug() << "doNextActionStep==DoNextActionStep_Loose, fightTimerFinish: " << fightTimerFinish;
        #endif
        if(fightTimerFinish)
            loose();
        else
            fightTimerFinish=true;
        return;
    }

    //if the current monster is KO
    if(CatchChallenger::ClientFightEngine::fightEngine.currentMonsterIsKO())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.isInFight())
        {
            loose();
            return;
        }
        qDebug() << "doNextAction(): remplace KO current monster";
        PublicPlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("Your %1 have lost!").arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name));
        doNextActionStep=DoNextActionStep_Start;
        moveType=MoveType_Dead;
        moveFightMonsterBottom();
        return;
    }

    //if the other monster is KO
    if(CatchChallenger::ClientFightEngine::fightEngine.otherMonsterIsKO())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.isInFight())
        {
            win();
            return;
        }
        qDebug() << "doNextAction(): remplace KO other monster";
        PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("No other monster into doNextAction()");
            return;
        }
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("The other %1 have lost!").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
        doNextActionStep=DoNextActionStep_Start;
        //current player monster is KO
        moveType=MoveType_Dead;
        moveFightMonsterTop();
        return;
    }

    //nothing to done, show the menu
    qDebug() << "doNextAction(): show the menu";
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    return;
}

void BaseWindow::displayAttack()
{
    PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to display an attack");
        doNextAction();
        return;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error"),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().isEmpty())
    {
        newError(tr("Internal error"),"displayAttack(): crash: display an empty attack return");
        doNextAction();
        return;
    }
    if(
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty() &&
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().buffLifeEffectMonster.isEmpty() &&
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.isEmpty() &&
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty()
            )
    {
        qDebug() << QString("displayAttack(): strange: display an empty lifeEffect list into attack return, attack: %1, doByTheCurrentMonster: %2, success: %3")
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack)
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster)
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success);
    }
    bool applyOnOtherMonster=true;
    if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty())
        applyOnOtherMonster=(
                    CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster &&
                    (CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().on==ApplyOn_AloneEnemy ||
                     CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().on==ApplyOn_AllEnemy)
            ) || (
                        !CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster &&
                        (CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().on==ApplyOn_Themself ||
                         CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().on==ApplyOn_AllAlly)
                );
    //if start, display text
    if(displayAttackProgression==0)
    {
        if(movie!=NULL)
        {
            movie->stop();
            delete movie;
        }
        movie=NULL;
        //do the buff
        {
            int index=0;
            while(index<CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.size())
            {
                Skill::BuffEffect buffEffect=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.at(index);
                bool buffApplyOnOtherMonster=(
                            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster &&
                            (buffEffect.on==ApplyOn_AloneEnemy ||
                             buffEffect.on==ApplyOn_AllEnemy)
                    ) || (
                                !CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster &&
                                (buffEffect.on==ApplyOn_Themself ||
                                 buffEffect.on==ApplyOn_AllAlly)
                        );

                if(buffApplyOnOtherMonster)
                {
                    if(buffToGraphicalItemTop.contains(buffEffect.buff))
                        delete buffToGraphicalItemTop[buffEffect.buff];
                    buffToGraphicalItemTop.remove(buffEffect.buff);
                }
                else
                {
                    if(buffToGraphicalItemBottom.contains(buffEffect.buff))
                        delete buffToGraphicalItemBottom[buffEffect.buff];
                    buffToGraphicalItemBottom.remove(buffEffect.buff);
                }
                QListWidgetItem *item=new QListWidgetItem();
                if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    const DatapackClientLoader::MonsterExtra::Buff &buffExtra=DatapackClientLoader::datapackLoader.monsterBuffsExtra[buffEffect.buff];
                    if(!buffExtra.icon.isNull())
                        item->setIcon(buffExtra.icon);
                    else
                        item->setIcon(QIcon(":/images/interface/buff.png"));
                    if(buffEffect.level<=1)
                        item->setToolTip(buffExtra.name);
                    else
                        item->setToolTip(tr("%1 at level %2").arg(buffExtra.name).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+buffExtra.description);
                }
                if(buffApplyOnOtherMonster)
                {
                    buffToGraphicalItemTop[buffEffect.buff]=item;
                    ui->topBuff->addItem(item);
                }
                else
                {
                    buffToGraphicalItemBottom[buffEffect.buff]=item;
                    ui->bottomBuff->addItem(item);
                }
                index++;
            }
        }
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty())
            attack_quantity_changed=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity;
        else
            attack_quantity_changed=0;
        ui->labelFightMonsterAttackTop->setMovie(NULL);
        ui->labelFightMonsterAttackBottom->setMovie(NULL);
        updateAttackTime.restart();
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->pushButtonFightEnterNext->setVisible(false);
        QString attackOwner;
        if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster)
            attackOwner=tr("Your %1 do the attack %2")
                .arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name)
                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack].name);
        else
            attackOwner=tr("The other %1 do the attack %2")
                .arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name)
                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack].name);
        QString damage;
        qint32 quantity;
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty())
            quantity=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity;
        else
            quantity=0;
        if(applyOnOtherMonster)
        {
            if(quantity>0)
                damage=tr("The other %1 is healed of %2")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name)
                    .arg(quantity);
            else if(quantity<0)
                damage=tr("The other %1 take %2 of damage")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name)
                    .arg(-quantity);
            if(quantity==0)
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.isEmpty() || CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty())
                    damage=tr("Do buff change");
                else
                    damage=tr("Have no effect");
            }
            else
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.isEmpty() || CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty())
                    damage+=tr(" and do buff change");
            }
            if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success)
            {
                quint32 attackId=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack;
                QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_SKILLANIMATION;
                QString fileAnimation=skillAnimation+QString("%1.mng").arg(attackId);
                if(QFile(fileAnimation).exists())
                {
                    movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightMonsterAttackTop);
                    movie->setScaledSize(QSize(ui->labelFightMonsterAttackTop->width(),ui->labelFightMonsterAttackTop->height()));
                    if(movie->isValid())
                    {
                        ui->labelFightMonsterAttackTop->setMovie(movie);
                        movie->start();
                    }
                    else
                        qDebug() << QString("movie loaded is not valid for: %1").arg(fileAnimation);
                }
            }
        }
        else
        {
            if(quantity>0)
                damage=tr("Your %1 is healed of %2")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name)
                    .arg(quantity);
            else if(quantity<0)
                damage=tr("Your %1 take %2 of damage")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra[currentMonster->monster].name)
                    .arg(-quantity);
            if(quantity==0)
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.isEmpty() || CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty())
                    damage=tr("Do buff change");
                else
                    damage=tr("Have no effect");
            }
            else
            {
                if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().addBuffEffectMonster.isEmpty() || CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty())
                    damage+=tr(" and do buff change");
            }
            if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success)
            {
                quint32 attackId=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack;
                QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_SKILLANIMATION;
                QString fileAnimation=skillAnimation+QString("%1.mng").arg(attackId);
                if(QFile(fileAnimation).exists())
                {
                    movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightMonsterAttackBottom);
                    movie->setScaledSize(QSize(ui->labelFightMonsterAttackBottom->width(),ui->labelFightMonsterAttackBottom->height()));
                    if(movie->isValid())
                    {
                        ui->labelFightMonsterAttackBottom->setMovie(movie);
                        movie->start();
                    }
                    else
                        qDebug() << QString("movie loaded is not valid for: %1").arg(fileAnimation);
                }
            }
        }
        ui->labelFightEnter->setText(QString("%1<br />%2").arg(attackOwner).arg(damage));
    }
    if(displayAttackProgression%100 /* each 400ms */ && attack_quantity_changed<0)
    {
        if(applyOnOtherMonster)
        {
            if(updateAttackTime.elapsed()<2000 /* 2000ms */)
                ui->labelFightMonsterTop->setVisible(!ui->labelFightMonsterTop->isVisible());
        }
        else
        {
            if(updateAttackTime.elapsed()<2000 /* 2000ms */)
                ui->labelFightMonsterBottom->setVisible(!ui->labelFightMonsterBottom->isVisible());
        }
    }
    if(updateAttackTime.elapsed()>2000)
    {
        ui->labelFightMonsterBottom->setVisible(true);
        ui->labelFightMonsterTop->setVisible(true);
        if(movie!=NULL)
        {
            movie->stop();
            delete movie;
        }
        movie=NULL;
        ui->labelFightMonsterAttackTop->setMovie(NULL);
        ui->labelFightMonsterAttackBottom->setMovie(NULL);
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
        displayAttackProgression=0;
        if(!CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty())
            if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity!=0)
            {
                if(applyOnOtherMonster)
                    ui->progressBarFightTopHP->setValue(ui->progressBarFightTopHP->value()+CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity);
                else
                {
                    ui->labelFightBottomHP->setText(QString("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
                    ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity);
                }
            }
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstLifeEffectAttackReturn();
        //attack is finish
        doNextAction();
    }
    else
    {
        if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.isEmpty())
            hp_to_change=0;
        else if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity<0)
        {
            hp_to_change=-hp_to_change;
            if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity>hp_to_change)
                hp_to_change=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity;
        }
        else if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity>0)
        {
            if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity<hp_to_change)
                hp_to_change=CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().lifeEffectMonster.first().quantity;
        }
        else
            hp_to_change=0;
        if(hp_to_change!=0)
        {
            CatchChallenger::ClientFightEngine::fightEngine.firstLifeEffectQuantityChange(-hp_to_change);
            if(applyOnOtherMonster)
                ui->progressBarFightTopHP->setValue(ui->progressBarFightTopHP->value()+hp_to_change);
            else
            {
                ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+hp_to_change);
                ui->labelFightBottomHP->setText(QString("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
            }
        }
        displayAttackTimer.start();
        displayAttackProgression++;
    }
}

void BaseWindow::displayTrap()
{
    PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to use trap");
        doNextAction();
        return;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error"),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    //if start, display text
    if(displayTrapProgression==0)
    {
        if(movie!=NULL)
        {
            movie->stop();
            delete movie;
        }
        QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_ITEM;
        QString fileAnimation=skillAnimation+QString("%1.mng").arg(trapItemId);
        movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
        movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
        if(movie->isValid())
        {
            ui->labelFightTrap->setMovie(movie);
            movie->start();
        }
        else
            qDebug() << QString("movie loaded is not valid for: %1").arg(fileAnimation);
        ui->labelFightEnter->setText(QString("Try capture the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        displayTrapProgression=1;
        ui->labelFightTrap->show();
    }
    quint32 animationTime;
    if(displayTrapProgression==1)
        animationTime=1500;
    else
        animationTime=2000;
    if(displayTrapProgression==1 && (quint32)updateTrapTime.elapsed()<animationTime)
        ui->labelFightTrap->move(
                    ui->labelFightMonsterBottom->pos().x()+(ui->labelFightMonsterTop->pos().x()-ui->labelFightMonsterBottom->pos().x())*updateTrapTime.elapsed()/animationTime,
                    ui->labelFightMonsterBottom->pos().y()-(ui->labelFightMonsterBottom->pos().y()-ui->labelFightMonsterTop->pos().y())*updateTrapTime.elapsed()/animationTime
                    );
    else
        ui->labelFightTrap->move(ui->labelFightMonsterTop->pos().x(),ui->labelFightMonsterTop->pos().y());
    if(!CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.isEmpty())
        return;
    if((quint32)updateTrapTime.elapsed()>animationTime)
    {
        updateTrapTime.restart();
        if(displayTrapProgression==1)
        {
            displayTrapProgression=2;
            if(movie!=NULL)
            {
                movie->stop();
                delete movie;
            }
            QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+DATAPACK_BASE_PATH_ITEM;
            QString fileAnimation;
            if(trapSuccess)
                fileAnimation=skillAnimation+QString("%1_success.mng").arg(trapItemId);
            else
                fileAnimation=skillAnimation+QString("%1_failed.mng").arg(trapItemId);
            movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
            movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
            if(movie->isValid())
            {
                ui->labelFightTrap->setMovie(movie);
                movie->start();
            }
            else
                qDebug() << QString("movie loaded is not valid for: %1").arg(fileAnimation);
        }
        else
        {
            if(trapSuccess)
                displayText(QString("You have captured the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
            else
                displayText(QString("You have failed the capture of the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra[otherMonster->monster].name));
            CatchChallenger::ClientFightEngine::fightEngine.captureIsDone();
            ui->labelFightTrap->hide();
            return;
        }
    }
    displayTrapTimer.start();
}

void BaseWindow::displayText(const QString &text)
{
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(text);
    doNextActionTimer.start();
}

void BaseWindow::resetPosition(bool outOfScreen,bool topMonster,bool bottomMonster)
{
    QPoint p;
    if(outOfScreen)
    {
        if(bottomMonster)
        {
            p.setX(-160);
            p.setY(280);
            ui->labelFightMonsterBottom->move(p);
        }
        if(topMonster)
        {
            p.setX(800);
            p.setY(90);
            ui->labelFightMonsterTop->move(p);
        }
    }
    else
    {
        if(bottomMonster)
        {
            p.setX(60);
            p.setY(280);
            ui->labelFightMonsterBottom->move(p);
        }
        if(topMonster)
        {
            p.setX(510);
            p.setY(90);
            ui->labelFightMonsterTop->move(p);
        }
    }
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
    if(DatapackClientLoader::datapackLoader.monsterExtra.contains(monster.monster))
    {
        item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].front);
        item->setText(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].name);
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

void BaseWindow::on_learnQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_learnValidate_clicked()
{
    QList<QListWidgetItem *> itemsList=ui->learnAttackList->selectedItems();
    if(itemsList.size()!=1)
    {
        QMessageBox::warning(this,tr("Error"),tr("Select an attack to learn"));
        return;
    }
    on_learnAttackList_itemActivated(itemsList.first());
}

void BaseWindow::on_learnAttackList_itemActivated(QListWidgetItem *item)
{
    if(!attack_to_learn_graphical.contains(item))
    {
        newError(tr("Internal error"),"Selected item wrong");
        return;
    }
    if(!learnSkill(monsterToLearn,attack_to_learn_graphical[item]))
    {
        ui->learnDescription->setText(tr("You can't learn this attack"));
        return;
    }
    if(!monsters_items_graphical.contains(item))
        return;
    showLearnSkill(monsterToLearn);
}

bool BaseWindow::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    if(!showLearnSkill(monsterId))
    {
        newError(tr("Internal error"),"Unable to load the right monster");
        return false;
    }
    if(ClientFightEngine::fightEngine.learnSkill(monsterId,skill))
    {
        CatchChallenger::Api_client_real::client->learnSkill(monsterId,skill);
        showLearnSkill(monsterId);
        return true;
    }
    return false;
}

//learn
bool BaseWindow::showLearnSkill(const quint32 &monsterId)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->learnAttackList->clear();
    attack_to_learn_graphical.clear();
    QList<PlayerMonster> playerMonster=ClientFightEngine::fightEngine.getPlayerMonster();
    //get the right monster
    QHash<quint32,quint8> skillToDisplay;
    int index=0;
    while(index<playerMonster.size())
    {
        if(playerMonster.at(index).id==monsterId)
        {
            PlayerMonster monster=playerMonster.at(index);
            ui->learnMonster->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra[monster.monster].front.scaled(160,160));
            ui->learnSP->setText(tr("SP: %1").arg(monster.sp));
            int sub_index=0;
            while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster].learn.size())
            {
                Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters[monster.monster].learn.at(sub_index);
                if(learn.learnAtLevel<=monster.level)
                {
                    int sub_index2=0;
                    while(sub_index2<monster.skills.size())
                    {
                        const PlayerMonster::PlayerSkill &skill=monster.skills.at(sub_index2);
                        if(skill.skill==learn.learnSkill)
                            break;
                        sub_index2++;
                    }
                    if(
                            //if skill not found
                            (sub_index2==monster.skills.size() && learn.learnSkillLevel==1)
                            ||
                            //if skill already found and need level up
                            (sub_index2<monster.skills.size() && (monster.skills[sub_index2].level+1)==learn.learnSkillLevel)
                    )
                    {
                        if(skillToDisplay.contains(learn.learnSkill))
                        {
                            if(skillToDisplay[learn.learnSkill]>learn.learnSkillLevel)
                                skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                        }
                        else
                            skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                    }
                }
                sub_index++;
            }
            QHashIterator<quint32,quint8> i(skillToDisplay);
            while (i.hasNext()) {
                i.next();
                QListWidgetItem *item=new QListWidgetItem();
                if(i.value()>1)
                    item->setText(tr("%1\nSP cost: %2")
                                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[i.key()].name)
                                .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[i.key()].level[i.value()-1].sp_to_learn)
                            );
                else
                    item->setText(tr("%1 level %2\nSP cost: %3")
                                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra[i.key()].name)
                                .arg(i.value())
                                .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[i.key()].level[i.value()-1].sp_to_learn)
                            );
                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[i.key()].level[i.value()-1].sp_to_learn>monster.sp)
                {
                    item->setFont(MissingQuantity);
                    item->setForeground(QBrush(QColor(200,20,20)));
                    item->setToolTip(tr("You need more sp"));
                }
                attack_to_learn_graphical[item]=i.key();
                ui->learnAttackList->addItem(item);
            }
            if(attack_to_learn_graphical.isEmpty())
                ui->learnDescription->setText(tr("No more attack to learn"));
            else
                ui->learnDescription->setText(tr("Select attack to learn"));
            return true;
        }
        index++;
    }
    return false;
}

void BaseWindow::sendBattleReturn(const QList<Skill::AttackReturn> &attackReturn)
{
    CatchChallenger::ClientFightEngine::fightEngine.addAndApplyAttackReturnList(attackReturn);
    doNextAction();
}

void BaseWindow::sendFullBattleReturn(const QList<Skill::AttackReturn> &attackReturn,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.haveBattleOtherMonster())
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.addBattleMonster(monsterPlace,publicPlayerMonster))
            return;
        sendBattleReturn(attackReturn);
    }
    else
    {
        if(!CatchChallenger::ClientFightEngine::fightEngine.addBattleMonster(monsterPlace,publicPlayerMonster))
            return;
        sendBattleReturn(attackReturn);
        init_other_monster_display();
        updateOtherMonsterInformation();
        battleStep=BattleStep_Middle;
        moveType=MoveType_Enter;
        moveFightMonsterTop();
    }
}


void CatchChallenger::BaseWindow::on_listWidgetFightAttack_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    on_pushButtonFightAttackConfirmed_clicked();
}

void CatchChallenger::BaseWindow::useTrap(const quint32 &itemId)
{
    updateTrapTime.restart();
    displayTrapProgression=0;
    trapItemId=itemId;
    CatchChallenger::ClientFightEngine::fightEngine.tryCaptureClient(itemId);
    displayTrap();
    remove_to_inventory(itemId);
}

void CatchChallenger::BaseWindow::monsterCatch(const quint32 &newMonsterId)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.isEmpty())
    {
        emit error("Internal bug: cupture monster list is emtpy");
        return;
    }
    if(newMonsterId==0x00000000)
    {
        ui->labelFightTrap->hide();
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("capture is failed"));
        #endif
        CatchChallenger::ClientFightEngine::fightEngine.generateOtherAttack();
        CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.removeFirst();
        displayAttack();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QString("capture is success"));
        #endif
        CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.first().id=newMonsterId;
        if(CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster().count()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
            warehouse_playerMonster << CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.first();
        else
        {
            CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.first());
            load_monsters();
        }
        CatchChallenger::ClientFightEngine::fightEngine.playerMonster_captureInProgress.removeFirst();
        displayTrap();
    }
}
