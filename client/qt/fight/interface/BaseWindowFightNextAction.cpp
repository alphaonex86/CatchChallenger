#include "../../interface/BaseWindow.hpp"
#include "ui_BaseWindow.h"
#include "ClientFightEngine.hpp"

/* show only the plant into the inventory */

using namespace CatchChallenger;

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
            if(!fightEngine.getAttackReturnList().empty())
            {
                qDebug() << "doNextAction(): escape failed: you take damage";
                displayText(tr("You have failed to escape!").toStdString());
                return;
            }
            else
            {
                PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
                if(otherMonster==NULL)
                {
                    emit error("NULL pointer for other monster at doNextAction()");
                    return;
                }
                qDebug() << "doNextAction(): escape fail but the wild monster can't attack";
                displayText(tr("The wild %1 can't attack").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                                                                   otherMonster->monster).name)).toStdString());
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
    if(!fightEngine.getAttackReturnList().empty())
    {
        const Skill::AttackReturn returnAction=fightEngine.getAttackReturnList().front();
        PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
        if(otherMonster)
        {
            qDebug() << "doNextAction(): apply the effect and display it";
            displayAttack();
        }
        //do the monster change
        else if(returnAction.publicPlayerMonster.monster!=0)
        {
            if(!fightEngine.addBattleMonster(returnAction.monsterPlace,returnAction.publicPlayerMonster))
                return;
            //sendBattleReturn(returnAction);:already added because the information is into fightEngine.getAttackReturnList()
            init_other_monster_display();
            updateOtherMonsterInformation();
            resetPosition(true,true,false);
            battleStep=BattleStep_Middle;
            moveType=MoveType_Enter;
            moveFightMonsterTop();
        }
        else
            emit newError("Internal bug","No other monster to display attack");
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

    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
        return;
    //if the current monster is KO
    if(fightEngine.currentMonsterIsKO())
    {
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
            return;
        if(!fightEngine.isInFight())
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
                return;
            loose();
            return;
        }
        qDebug() << "doNextAction(): remplace KO current monster";
        PublicPlayerMonster * currentMonster=fightEngine.getCurrentMonster();
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("Your %1 have lost!")
                                     .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                                     );
        doNextActionStep=DoNextActionStep_Start;
        moveType=MoveType_Dead;
        moveFightMonsterBottom();
        return;
    }

    //if the other monster is KO
    if(fightEngine.otherMonsterIsKO())
    {
        uint32_t returnedLastGivenXP=fightEngine.lastGivenXP();
        if(returnedLastGivenXP>2*1000*1000)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     "returnedLastGivenXP is negative");
            doNextAction();
            return;
        }
        if(returnedLastGivenXP>0 || mLastGivenXP>0)
        {
            if(returnedLastGivenXP>0)
                mLastGivenXP=returnedLastGivenXP;
            displayAttackProgression=0;
            displayExperienceGain();
            return;
        }
        else
            updateCurrentMonsterInformation();
        if(!fightEngine.isInFight())
        {
            win();
            return;
        }
        updateCurrentMonsterInformationXp();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(fightEngine.getAttackReturnList().empty())
        {
            PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
            PublicPlayerMonster * currentMonster=fightEngine.getCurrentMonster();
            if(currentMonster!=NULL)
                if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
                {
                    emit error(QStringLiteral("Current monster hp don't match with the internal value (do next action): %1!=%2")
                               .arg(currentMonster->hp)
                               .arg(ui->progressBarFightBottomHP->value())
                               .toStdString());
                    return;
                }
            if(otherMonster!=NULL)
                if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
                {
                    emit error(QStringLiteral("Other monster hp don't match with the internal value (do next action): %1!=%2")
                               .arg(otherMonster->hp)
                               .arg(ui->progressBarFightTopHP->value())
                               .toStdString());
                    return;
                }
        }
        #endif
        qDebug() << "doNextAction(): remplace KO other monster";
        PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("No other monster into doNextAction()");
            return;
        }
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("The other %1 have lost!")
                                     .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name)));
        doNextActionStep=DoNextActionStep_Start;
        //current player monster is KO
        moveType=MoveType_Dead;
        moveFightMonsterTop();
        return;
    }

    //nothing to done, show the menu
    qDebug() << "doNextAction(): show the menu";
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    /*if(ClientBase::public_and_private_informations_solo!=NULL)
    {
        if(ClientBase::public_and_private_informations_solo->playerMonster.size()!=fightEngine.getPlayerMonster().size())
        {
            emit error(QStringLiteral("both monster list don't have same size: %1!=%2").arg(ClientBase::public_and_private_informations_solo->playerMonster.size()).arg(fightEngine.getPlayerMonster().size()));
            return;
        }
        {
            unsigned int index=0;
            while(index<ClientBase::public_and_private_informations_solo->playerMonster.size())
            {
                if(ClientBase::public_and_private_informations_solo->playerMonster.at(index)!=fightEngine.getPlayerMonster().at(index))
                {
                    emit error(QStringLiteral("both monster at %1 is not same for monster %2!=%3")
                               .arg(index)
                               .arg(ClientBase::public_and_private_informations_solo->playerMonster.at(index).monster)
                               .arg(fightEngine.getPlayerMonster().at(index).monster)
                               );
                    abort();
                }
                index++;
            }
        }
    }*/
    #endif
    PublicPlayerMonster *currentMonster=fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
    {
        if(CommonDatapack::commonDatapack.monsters.find(currentMonster->monster)==CommonDatapack::commonDatapack.monsters.cend())
        {
            emit error(QStringLiteral("Current monster don't exists: %1").arg(currentMonster->monster).toStdString());
            return;
        }
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (doNextAction && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString());
            return;
        }
    }
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
    {
        if(CommonDatapack::commonDatapack.monsters.find(otherMonster->monster)==CommonDatapack::commonDatapack.monsters.cend())
        {
            emit error(QStringLiteral("Current monster don't exists: %1").arg(otherMonster->monster).toStdString());
            return;
        }
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Other monster damage don't match with the internal value (doNextAction && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString());
            return;
        }
    }
    #endif
    return;
}

bool BaseWindow::displayFirstAttackText(bool firstText)
{
    PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to display an attack");
        doNextAction();
        return false;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 "displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return false;
    }
    if(fightEngine.getAttackReturnList().empty())
    {
        emit error("Display text for not existant attack");
        return false;
    }
    const Skill::AttackReturn currentAttack=fightEngine.getFirstAttackReturn();

    if(movie!=NULL)
    {
        movie->stop();
        delete movie;
    }
    movie=NULL;

    qDebug() << QStringLiteral("displayFirstAttackText(): before display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                .arg(currentAttack.lifeEffectMonster.size())
                .arg(currentAttack.buffLifeEffectMonster.size())
                .arg(currentAttack.addBuffEffectMonster.size())
                .arg(currentAttack.removeBuffEffectMonster.size())
                .arg(fightEngine.getAttackReturnList().size());

    //in case of failed
    if(!currentAttack.success)
    {
        if(currentAttack.doByTheCurrentMonster)
            displayText(tr("Your %1 have failed the attack %2")
                          .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                          .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(currentAttack.attack).name))
                        .toStdString());
        else
            displayText(tr("The other %1 have failed the attack %2")
                          .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                          .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(currentAttack.attack).name))
                        .toStdString());
        fightEngine.removeTheFirstAttackReturn();
        return false;
    }

    QString attackOwner;
    if(firstText)
    {
        if(currentAttack.doByTheCurrentMonster)
            attackOwner=tr("Your %1 do the attack %2 and ")
                .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(currentAttack.attack).name));
        else
            attackOwner=tr("The other %1 do the attack %2 and ")
                .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(currentAttack.attack).name));
    }
    else
    {
        if(currentAttack.doByTheCurrentMonster)
            attackOwner=tr("Your %1 ").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name));
        else
            attackOwner=tr("The other %1 ").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name));
    }

    //the life effect
    if(!currentAttack.lifeEffectMonster.empty())
    {
        const Skill::LifeEffectReturn &lifeEffectReturn=currentAttack.lifeEffectMonster.front();
        if(currentAttack.doByTheCurrentMonster)
        {
            if((lifeEffectReturn.on & ApplyOn_AllEnemy) || (lifeEffectReturn.on & ApplyOn_AloneEnemy))
            {
                if(lifeEffectReturn.quantity>0)
                    attackOwner+=tr("heal of %2 the other %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt of %2 the other %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                        .arg(-lifeEffectReturn.quantity);
            }
            if((lifeEffectReturn.on & ApplyOn_AllAlly) || (lifeEffectReturn.on & ApplyOn_Themself))
            {
                if(lifeEffectReturn.quantity>0)
                    attackOwner+=tr("heal themself of %1")
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt themself of %1")
                        .arg(-lifeEffectReturn.quantity);
            }
        }
        else
        {
            if((lifeEffectReturn.on & ApplyOn_AllEnemy) || (lifeEffectReturn.on & ApplyOn_AloneEnemy))
            {
                if(lifeEffectReturn.quantity>0)
                    attackOwner+=tr("heal of %2 your %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt of %2 your %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                        .arg(-lifeEffectReturn.quantity);
            }
            if((lifeEffectReturn.on & ApplyOn_AllAlly) || (lifeEffectReturn.on & ApplyOn_Themself))
            {
                if(lifeEffectReturn.quantity>0)
                    attackOwner+=tr("heal themself of %1")
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt themself of %1")
                        .arg(-lifeEffectReturn.quantity);
            }
        }
        if(lifeEffectReturn.effective!=1 && lifeEffectReturn.critical)
        {
            if(lifeEffectReturn.effective>1)
                attackOwner+=QStringLiteral("<br />")+tr("It's very effective and critical throw");
            else
                attackOwner+=QStringLiteral("<br />")+tr("It's not very effective but it's critical throw");
        }
        else if(lifeEffectReturn.effective!=1)
        {
            if(lifeEffectReturn.effective>1)
                attackOwner+=QStringLiteral("<br />")+tr("It's very effective");
            else
                attackOwner+=QStringLiteral("<br />")+tr("It's not very effective");
        }
        else if(lifeEffectReturn.critical)
            attackOwner+=QStringLiteral("<br />")+tr("Critical throw");
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(attackOwner);
        qDebug() << "display the life effect";
        qDebug() << QStringLiteral("displayFirstAttackText(): after display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                    .arg(currentAttack.lifeEffectMonster.size())
                    .arg(currentAttack.buffLifeEffectMonster.size())
                    .arg(currentAttack.addBuffEffectMonster.size())
                    .arg(currentAttack.removeBuffEffectMonster.size())
                    .arg(fightEngine.getAttackReturnList().size());
        return true;
    }

    //the add buff
    if(!currentAttack.addBuffEffectMonster.empty())
    {
        const Skill::BuffEffect &addBuffEffectMonster=currentAttack.addBuffEffectMonster.front();
        std::unordered_map<uint8_t,QListWidgetItem *> *buffToGraphicalItemCurrentbar=NULL;
        QListWidget *listWidget=NULL;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        bool onBuffCurrentMonster=false;
        #endif
        if(currentAttack.doByTheCurrentMonster)
        {
            if((addBuffEffectMonster.on & ApplyOn_AllEnemy) || (addBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on the other %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(addBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
                listWidget=ui->topBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=false;
                #endif
            }
            if((addBuffEffectMonster.on & ApplyOn_AllAlly) || (addBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(addBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
                listWidget=ui->bottomBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=true;
                #endif
            }
        }
        else
        {
            if((addBuffEffectMonster.on & ApplyOn_AllEnemy) || (addBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on your %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(addBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
                listWidget=ui->bottomBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=true;
                #endif
            }
            if((addBuffEffectMonster.on & ApplyOn_AllAlly) || (addBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(addBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
                listWidget=ui->topBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=false;
                #endif
            }
        }
        displayText(attackOwner.toStdString());
        if(buffToGraphicalItemCurrentbar!=NULL)
        {
            //search the buff
            QListWidgetItem *item=NULL;
            int index=0;
            while(index<listWidget->count())
            {
                if(listWidget->item(index)->data(99).toUInt()==addBuffEffectMonster.buff)
                {
                    item=listWidget->item(index);
                    break;
                }
                index++;
            }
            //add new buff because don't exist
            if(index==listWidget->count())
            {
                item=new QListWidgetItem();
                item->setData(99,addBuffEffectMonster.buff);//to prevent duplicate buff, because add can be to re-enable an already enable buff (for larger period then)
            }
            if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(addBuffEffectMonster.buff)==
                    QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
            {
                item->setToolTip(tr("Unknown buff"));
                item->setIcon(QIcon(":/CC/images/interface/buff.png"));
            }
            else
            {
                const QtDatapackClientLoader::MonsterExtra::Buff &buffExtra=QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(addBuffEffectMonster.buff);
                const QtDatapackClientLoader::QtMonsterExtra::QtBuff &QtbuffExtra=QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra.at(addBuffEffectMonster.buff);
                if(!QtbuffExtra.icon.isNull())
                    item->setIcon(QtbuffExtra.icon);
                else
                    item->setIcon(QIcon(":/CC/images/interface/buff.png"));
                if(addBuffEffectMonster.level<=1)
                    item->setToolTip(QString::fromStdString(buffExtra.name));
                else
                    item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(buffExtra.name)).arg(addBuffEffectMonster.level));
                item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(buffExtra.description));
            }
            (*buffToGraphicalItemCurrentbar)[addBuffEffectMonster.buff]=item;
            if(index==listWidget->count())
                listWidget->addItem(item);
        }
        fightEngine.removeTheFirstAddBuffEffectAttackReturn();
        if(!fightEngine.firstAttackReturnHaveMoreEffect())
            fightEngine.removeTheFirstAttackReturn();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!onBuffCurrentMonster && otherMonster!=NULL)
        {
            if(otherMonster->buffs.size()!=(uint32_t)listWidget->count())
            {
                error("displayFirstAttackText(): bug: buffs number displayed != than real buff count on other monter");
                doNextAction();
                return false;
            }
        }
        if(onBuffCurrentMonster && currentMonster!=NULL)
        {
            if(currentMonster->buffs.size()!=(uint32_t)listWidget->count())
            {
                error("displayFirstAttackText(): bug: buffs number displayed != than real buff count on current monter");
                doNextAction();
                return false;
            }
        }
        #endif
        qDebug() << "display the add buff";
        qDebug() << QStringLiteral("displayFirstAttackText(): after display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                    .arg(currentAttack.lifeEffectMonster.size())
                    .arg(currentAttack.buffLifeEffectMonster.size())
                    .arg(currentAttack.addBuffEffectMonster.size())
                    .arg(currentAttack.removeBuffEffectMonster.size())
                    .arg(fightEngine.getAttackReturnList().size());
        return false;
    }

    //the remove buff
    if(!currentAttack.removeBuffEffectMonster.empty())
    {
        const Skill::BuffEffect &removeBuffEffectMonster=currentAttack.removeBuffEffectMonster.front();
        std::unordered_map<uint8_t,QListWidgetItem *> *buffToGraphicalItemCurrentbar=NULL;
        if(currentAttack.doByTheCurrentMonster)
        {
            if((removeBuffEffectMonster.on & ApplyOn_AllEnemy) || (removeBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on the other %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(removeBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
            }
            if((removeBuffEffectMonster.on & ApplyOn_AllAlly) || (removeBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(removeBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
            }
        }
        else
        {
            if((removeBuffEffectMonster.on & ApplyOn_AllEnemy) || (removeBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on your %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(removeBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
            }
            if((removeBuffEffectMonster.on & ApplyOn_AllAlly) || (removeBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(removeBuffEffectMonster.buff).name));
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
            }
        }
        displayText(attackOwner.toStdString());
        if(buffToGraphicalItemCurrentbar!=NULL)
        {
            if(buffToGraphicalItemTop.find(removeBuffEffectMonster.buff)!=buffToGraphicalItemTop.cend())
                delete buffToGraphicalItemTop.at(removeBuffEffectMonster.buff);
            buffToGraphicalItemTop.erase(removeBuffEffectMonster.buff);
        }
        fightEngine.removeTheFirstRemoveBuffEffectAttackReturn();
        if(!fightEngine.firstAttackReturnHaveMoreEffect())
            fightEngine.removeTheFirstAttackReturn();
        qDebug() << "display the remove buff";
        qDebug() << QStringLiteral("displayFirstAttackText(): after display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                    .arg(currentAttack.lifeEffectMonster.size())
                    .arg(currentAttack.buffLifeEffectMonster.size())
                    .arg(currentAttack.addBuffEffectMonster.size())
                    .arg(currentAttack.removeBuffEffectMonster.size())
                    .arg(fightEngine.getAttackReturnList().size());
        return false;
    }

    //the apply buff effect
    if(!currentAttack.buffLifeEffectMonster.empty())
    {
        const Skill::LifeEffectReturn &buffLifeEffectMonster=currentAttack.buffLifeEffectMonster.front();
        if(currentAttack.doByTheCurrentMonster)
        {
            if((buffLifeEffectMonster.on & ApplyOn_AllEnemy) || (buffLifeEffectMonster.on & ApplyOn_AloneEnemy))
            {
                if(buffLifeEffectMonster.quantity>0)
                    attackOwner+=tr("heal of %2 the other %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt of %2 the other %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name))
                        .arg(-buffLifeEffectMonster.quantity);
            }
            if((buffLifeEffectMonster.on & ApplyOn_AllAlly) || (buffLifeEffectMonster.on & ApplyOn_Themself))
            {
                if(buffLifeEffectMonster.quantity>0)
                    attackOwner+=tr("heal themself of %1")
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt themself of %1")
                        .arg(-buffLifeEffectMonster.quantity);
            }
        }
        else
        {
            if((buffLifeEffectMonster.on & ApplyOn_AllEnemy) || (buffLifeEffectMonster.on & ApplyOn_AloneEnemy))
            {
                if(buffLifeEffectMonster.quantity>0)
                    attackOwner+=tr("heal of %2 your %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt of %2 your %1")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                        .arg(-buffLifeEffectMonster.quantity);
            }
            if((buffLifeEffectMonster.on & ApplyOn_AllAlly) || (buffLifeEffectMonster.on & ApplyOn_Themself))
            {
                if(buffLifeEffectMonster.quantity>0)
                    attackOwner+=tr("heal themself of %1")
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt themself of %1")
                        .arg(-buffLifeEffectMonster.quantity);
            }
        }
        if(buffLifeEffectMonster.effective!=1 && buffLifeEffectMonster.critical)
        {
            if(buffLifeEffectMonster.effective>1)
                attackOwner+=QStringLiteral("<br />")+tr("It's very effective and critical throw");
            else
                attackOwner+=QStringLiteral("<br />")+tr("It's not very effective but it's critical throw");
        }
        else if(buffLifeEffectMonster.effective!=1)
        {
            if(buffLifeEffectMonster.effective>1)
                attackOwner+=QStringLiteral("<br />")+tr("It's very effective");
            else
                attackOwner+=QStringLiteral("<br />")+tr("It's not very effective");
        }
        else if(buffLifeEffectMonster.critical)
            attackOwner+=QStringLiteral("<br />")+tr("Critical throw");
        qDebug() << "display the buff effect";
        qDebug() << QStringLiteral("displayFirstAttackText(): after display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                    .arg(currentAttack.lifeEffectMonster.size())
                    .arg(currentAttack.buffLifeEffectMonster.size())
                    .arg(currentAttack.addBuffEffectMonster.size())
                    .arg(currentAttack.removeBuffEffectMonster.size())
                    .arg(fightEngine.getAttackReturnList().size());
        return true;
    }
    emit error("Can't display text without effect");
    return false;
}

void BaseWindow::displayAttack()
{
    PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to display an attack");
        doNextAction();
        return;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    if(fightEngine.getAttackReturnList().empty())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: display an empty attack return");
        doNextAction();
        return;
    }
    if(
            fightEngine.getAttackReturnList().front().lifeEffectMonster.empty() &&
            fightEngine.getAttackReturnList().front().buffLifeEffectMonster.empty() &&
            fightEngine.getAttackReturnList().front().addBuffEffectMonster.empty() &&
            fightEngine.getAttackReturnList().front().removeBuffEffectMonster.empty() &&
            fightEngine.getAttackReturnList().front().success
            )
    {
        qDebug() << QStringLiteral("displayAttack(): strange: display an empty lifeEffect list into attack return, attack: %1, doByTheCurrentMonster: %2, success: %3")
                    .arg(fightEngine.getAttackReturnList().front().attack)
                    .arg(fightEngine.getAttackReturnList().front().doByTheCurrentMonster)
                    .arg(fightEngine.getAttackReturnList().front().success);
        fightEngine.removeTheFirstAttackReturn();
        doNextAction();
        return;
    }

    //if start, display text
    if(displayAttackProgression==0)
    {
        updateAttackTime.restart();
        if(!displayFirstAttackText(true))
            return;
    }

    const Skill::AttackReturn &attackReturn=fightEngine.getFirstAttackReturn();
    //get the life effect to display
    Skill::LifeEffectReturn lifeEffectReturn;
    if(!attackReturn.lifeEffectMonster.empty())
        lifeEffectReturn=attackReturn.lifeEffectMonster.front();
    else if(!attackReturn.buffLifeEffectMonster.empty())
        lifeEffectReturn=attackReturn.buffLifeEffectMonster.front();
    else
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("displayAttack(): strange: nothing to display, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, AttackReturnList: %5")
                    .arg(attackReturn.lifeEffectMonster.size())
                    .arg(attackReturn.buffLifeEffectMonster.size())
                    .arg(attackReturn.addBuffEffectMonster.size())
                    .arg(attackReturn.removeBuffEffectMonster.size())
                    .arg(fightEngine.getAttackReturnList().size())
                 .toStdString()
                 );
        fightEngine.removeTheFirstAttackReturn();
        //doNextAction();
        return;
    }

    bool applyOnOtherMonster=false;
    if(attackReturn.doByTheCurrentMonster)
    {
        if((lifeEffectReturn.on==ApplyOn_AloneEnemy) || (lifeEffectReturn.on==ApplyOn_AllEnemy))
            applyOnOtherMonster=true;
    }
    else
    {
        if((lifeEffectReturn.on==ApplyOn_Themself) || (lifeEffectReturn.on==ApplyOn_AllAlly))
            applyOnOtherMonster=true;
    }
    QLabel * attackMovie;
    if(applyOnOtherMonster)
        attackMovie=ui->labelFightMonsterAttackTop;
    else
        attackMovie=ui->labelFightMonsterAttackBottom;

    //attack animation
    {
        uint32_t attackId=attackReturn.attack;
        QString skillAnimation=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_SKILLANIMATION;
        QString fileAnimation=skillAnimation+QStringLiteral("%1.gif").arg(attackId);
        if(QFile(fileAnimation).exists())
        {
            movie=new QMovie(fileAnimation,QByteArray(),attackMovie);
            movie->setScaledSize(QSize(attackMovie->width(),attackMovie->height()));
            if(movie->isValid())
            {
                attackMovie->setMovie(movie);
                movie->start();
            }
            else
                qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
        }
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
        //only if passe here before have updated all the stats
        {
            int hp_to_change;
            if(applyOnOtherMonster)
                hp_to_change=ui->progressBarFightTopHP->maximum();
            else
                hp_to_change=ui->progressBarFightBottomHP->maximum();
            if(attackReturn.lifeEffectMonster.empty())
                hp_to_change=0;
            else if(attackReturn.lifeEffectMonster.front().quantity!=0)
                hp_to_change=attackReturn.lifeEffectMonster.front().quantity;
            else
                hp_to_change=0;
            if(hp_to_change!=0)
            {
                fightEngine.firstLifeEffectQuantityChange(-hp_to_change);
                if(applyOnOtherMonster)
                {
                    ui->progressBarFightTopHP->setValue(ui->progressBarFightTopHP->value()+hp_to_change);
                    ui->progressBarFightTopHP->repaint();
                }
                else
                {
                    ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+hp_to_change);
                    ui->progressBarFightBottomHP->repaint();
                    ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
                }
            }
        }
        displayAttackProgression=0;
        attack_quantity_changed=0;
        if(!attackReturn.lifeEffectMonster.empty())
            fightEngine.removeTheFirstLifeEffectAttackReturn();
        else
            fightEngine.removeTheFirstBuffEffectAttackReturn();
        if(!fightEngine.firstAttackReturnHaveMoreEffect())
        {
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            {
                qDebug() << "after display attack: currentMonster have hp" << ui->progressBarFightBottomHP->value() << "and buff" << ui->bottomBuff->count();
                qDebug() << "after display attack: otherMonster have hp" << ui->progressBarFightTopHP->value() << "and buff" << ui->topBuff->count();
            }
            #endif
            fightEngine.removeTheFirstAttackReturn();
        }
        //attack is finish
        doNextAction();
    }
    else
    {
        if(attackReturn.lifeEffectMonster.empty())
            hp_to_change=0;
        else if(attackReturn.lifeEffectMonster.front().quantity<0)
        {
            hp_to_change=-hp_to_change;
            if(abs(hp_to_change)>abs(attackReturn.lifeEffectMonster.front().quantity))
                hp_to_change=attackReturn.lifeEffectMonster.front().quantity;
        }
        else if(attackReturn.lifeEffectMonster.front().quantity>0)
        {
            if(hp_to_change>attackReturn.lifeEffectMonster.front().quantity)
                hp_to_change=attackReturn.lifeEffectMonster.front().quantity;
        }
        else
            hp_to_change=0;
        if(hp_to_change!=0)
        {
            fightEngine.firstLifeEffectQuantityChange(-hp_to_change);
            if(applyOnOtherMonster)
            {
                ui->progressBarFightTopHP->setValue(ui->progressBarFightTopHP->value()+hp_to_change);
                ui->progressBarFightTopHP->repaint();
            }
            else
            {
                ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+hp_to_change);
                ui->progressBarFightBottomHP->repaint();
                ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
            }
        }
        displayAttackTimer.start();
        displayAttackProgression++;
    }
}
