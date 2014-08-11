#include "../../base/interface/BaseWindow.h"
#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/ClientVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralStructures.h"
#include "ClientFightEngine.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/CommonDatapack.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QQmlContext>

/* show only the plant into the inventory */

using namespace CatchChallenger;

void BaseWindow::on_monsterList_itemActivated(QListWidgetItem *item)
{
    if(!monsters_items_graphical.contains(item))
        return;
    quint32 monsterId=monsters_items_graphical.value(item);
    if(inSelection)
    {
       objectSelection(true,monsterId);
       return;
    }
    QList<PlayerMonster> playerMonster=ClientFightEngine::fightEngine.getPlayerMonster();
    int index=0;
    int size=playerMonster.size();
    while(index<size)
    {
        const PlayerMonster &monster=playerMonster.at(index);
        if(monster.id==monsterId)
        {
            const DatapackClientLoader::MonsterExtra &monsterExtraInfo=DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster);
            const Monster &monsterGeneralInfo=CommonDatapack::commonDatapack.monsters.value(monster.monster);
            const Monster::Stat &stat=CommonFightEngine::getStat(monsterGeneralInfo,monster.level);
            if(monsterGeneralInfo.type.isEmpty())
                ui->monsterDetailsType->setText(QString());
            else
            {
                QStringList typeList;
                int sub_index=0;
                while(sub_index<monsterGeneralInfo.type.size())
                {
                    if(DatapackClientLoader::datapackLoader.typeExtra.contains(monsterGeneralInfo.type.at(sub_index)))
                        if(!DatapackClientLoader::datapackLoader.typeExtra.value(monsterGeneralInfo.type.at(sub_index)).name.isEmpty())
                            typeList << DatapackClientLoader::datapackLoader.typeExtra.value(monsterGeneralInfo.type.at(sub_index)).name;
                    sub_index++;
                }
                QStringList extraInfo;
                if(!typeList.isEmpty())
                    extraInfo << tr("Type: %1").arg(typeList.join(QStringLiteral(", ")));
                #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
                if(!monsterExtraInfo.kind.isEmpty())
                    extraInfo << tr("Kind: %1").arg(monsterExtraInfo.kind);
                if(!monsterExtraInfo.habitat.isEmpty())
                    extraInfo << tr("Habitat: %1").arg(monsterExtraInfo.habitat);
                #endif
                ui->monsterDetailsType->setText(extraInfo.join(QStringLiteral("<br />")));
            }
            ui->monsterDetailsName->setText(monsterExtraInfo.name);
            ui->monsterDetailsDescription->setText(monsterExtraInfo.description);
            {
                QPixmap front=monsterExtraInfo.front;
                front=front.scaled(160,160);
                ui->monsterDetailsImage->setPixmap(front);
            }
            {
                QPixmap catchPixmap;
                if(DatapackClientLoader::datapackLoader.itemsExtra.contains(monster.catched_with))
                {
                    catchPixmap=DatapackClientLoader::datapackLoader.itemsExtra.value(monster.catched_with).image;
                    ui->monsterDetailsCatched->setToolTip(tr("catched with %1").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(monster.catched_with).name));
                }
                else
                {
                    catchPixmap=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                    ui->monsterDetailsCatched->setToolTip(tr("catched with unknown item: %1").arg(monster.catched_with));
                }
                catchPixmap=catchPixmap.scaled(48,48);
                ui->monsterDetailsCatched->setPixmap(catchPixmap);
            }
            if(monster.gender==Gender_Male)
            {
                ui->monsterDetailsGender->setPixmap(QPixmap(":/images/interface/male.png").scaled(48,48));
                ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Male")));
            }
            else if(monster.gender==Gender_Female)
            {
                ui->monsterDetailsGender->setPixmap(QPixmap(":/images/interface/female.png").scaled(48,48));
                ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Female")));
            }
            else
            {
                ui->monsterDetailsGender->setPixmap(QPixmap());
                ui->monsterDetailsGender->setToolTip(QString());
            }
            ui->monsterDetailsLevel->setText(tr("Level %1").arg(monster.level));
            ui->monsterDetailsStatHeal->setText(tr("Heal: %1/%2").arg(monster.hp).arg(stat.hp));
            ui->monsterDetailsStatSpeed->setText(tr("Speed: %1").arg(stat.speed));
            ui->monsterDetailsStatXp->setText(tr("Xp: %1").arg(monster.remaining_xp));
            ui->monsterDetailsStatAttack->setText(tr("Attack: %1").arg(stat.attack));
            ui->monsterDetailsStatDefense->setText(tr("Defense: %1").arg(stat.defense));
            ui->monsterDetailsStatXpBar->setValue(monster.remaining_xp);
            ui->monsterDetailsStatXpBar->repaint();
            ui->monsterDetailsStatXpBar->setMaximum(monsterGeneralInfo.level_to_xp.at(monster.level-1));
            ui->monsterDetailsStatAttackSpe->setText(tr("Special attack: %1").arg(stat.special_attack));
            ui->monsterDetailsStatDefenseSpe->setText(tr("Special defense: %1").arg(stat.special_defense));
            if(CommonSettings::commonSettings.useSP)
            {
                ui->monsterDetailsStatSp->setVisible(true);
                ui->monsterDetailsStatSp->setText(tr("Skill point: %1").arg(monster.sp));
            }
            else
                ui->monsterDetailsStatSp->setVisible(false);
            //skill
            ui->monsterDetailsSkills->clear();
            int index=0;
            while(index<monster.skills.size())
            {
                const PlayerMonster::PlayerSkill &monsterSkill=monster.skills.at(index);
                QListWidgetItem *item;
                if(!DatapackClientLoader::datapackLoader.monsterSkillsExtra.contains(monsterSkill.skill))
                    item=new QListWidgetItem(tr("Unknown skill"));
                else
                {
                    if(monsterSkill.level>1)
                        item=new QListWidgetItem(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(monsterSkill.skill).name).arg(monsterSkill.level));
                    else
                        item=new QListWidgetItem(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(monsterSkill.skill).name);
                    const Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(monsterSkill.skill);
                    item->setText(item->text()+QStringLiteral(" (%1/%2)")
                            .arg(monsterSkill.endurance)
                            .arg(skill.level.at(monsterSkill.level-1).endurance)
                            );
                    if(skill.type!=255)
                        if(DatapackClientLoader::datapackLoader.typeExtra.contains(skill.type))
                            if(!DatapackClientLoader::datapackLoader.typeExtra.value(skill.type).name.isEmpty())
                                item->setText(item->text()+", "+tr("Type: %1").arg(DatapackClientLoader::datapackLoader.typeExtra.value(skill.type).name));
                    item->setText(item->text()+"\n"+DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(monsterSkill.skill).description);
                    item->setToolTip(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(monsterSkill.skill).description);
                }
                ui->monsterDetailsSkills->addItem(item);
                index++;
            }
            //do the buff
            {
                ui->monsterDetailsBuffs->clear();
                int index=0;
                while(index<monster.buffs.size())
                {
                    const PlayerBuff &buffEffect=monster.buffs.at(index);
                    QListWidgetItem *item=new QListWidgetItem();
                    if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                    {
                        item->setToolTip(tr("Unknown buff"));
                        item->setIcon(QIcon(":/images/interface/buff.png"));
                    }
                    else
                    {
                        item->setIcon(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).icon);
                        if(buffEffect.level<=1)
                            item->setToolTip(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name);
                        else
                            item->setToolTip(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name).arg(buffEffect.level));
                        item->setToolTip(item->toolTip()+"\n"+DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).description);
                    }
                    ui->monsterDetailsBuffs->addItem(item);
                    index++;
                }
            }

            ui->stackedWidget->setCurrentWidget(ui->page_monsterdetails);
            return;
        }
        index++;
    }
    if(index==size)
    {
        QMessageBox::warning(this,tr("Error"),tr("No details on the selected monster found"));
        return;
    }
}

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
            error(QStringLiteral("the monster %1 is not into monster list").arg(monster.monster));
            return false;
        }
        const Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster);
        if(monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            error(QStringLiteral("the level %1 is greater than max level").arg(monster.level));
            return false;
        }
        Monster::Stat stat=ClientFightEngine::getStat(monsterDef,monster.level);
        if(monster.hp>stat.hp)
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(stat.hp).arg(monster.level));
            return false;
        }
        if(monster.remaining_xp>monsterDef.level_to_xp.at(monster.level-1))
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(monster.remaining_xp).arg(monster.level));
            return false;
        }
        sub_size=monster.buffs.size();
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(!CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.contains(monster.buffs.at(sub_index).buff))
            {
                error(QStringLiteral("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff));
                return false;
            }
            if(monster.buffs.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.value(monster.buffs.at(sub_index).buff).level.size())
            {
                error(QStringLiteral("the buff have not the level %1 is not into the buff list").arg(monster.buffs.at(sub_index).level));
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
                error(QStringLiteral("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill));
                return false;
            }
            if(monster.skills.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(monster.skills.at(sub_index).skill).level.size())
            {
                error(QStringLiteral("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level));
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
    ui->monsterList->clear();
    monsters_items_graphical.clear();
    if(playerMonsters.isEmpty())
        return;
    const PlayerMonster * currentMonster=ClientFightEngine::fightEngine.getCurrentMonster();
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
                case ObjectType_ItemEvolutionOnMonster:
                    if(monster.hp==0 || monster.egg_step>0)
                    {
                        index++;
                        continue;
                    }
                    if(!CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(objectInUsing.last()))
                    {
                        index++;
                        continue;
                    }
                    if(!CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(objectInUsing.last()).contains(monster.monster))
                    {
                        index++;
                        continue;
                    }
                break;
                case ObjectType_ItemLearnOnMonster:
                    if(!CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.contains(objectInUsing.last()))
                    {
                        index++;
                        continue;
                    }
                    if(!CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.value(objectInUsing.last()).contains(monster.monster))
                    {
                        index++;
                        continue;
                    }
                break;
                case ObjectType_MonsterToFight:
                case ObjectType_MonsterToFightKO:
                case ObjectType_ItemOnMonster:
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
            Monster::Stat stat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster),monster.level);

            QListWidgetItem *item=new QListWidgetItem();
            item->setToolTip(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).description);
            if(!DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).thumb.isNull())
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).thumb);
            else
                item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).front);

            if(waitedObjectType==ObjectType_MonsterToLearn && inSelection)
            {
                QHash<quint32,quint8> skillToDisplay;
                int sub_index=0;
                while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.size())
                {
                    Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.at(sub_index);
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
                                (sub_index2<monster.skills.size() && (monster.skills.value(sub_index2).level+1)==learn.learnSkillLevel)
                        )
                        {
                            if(skillToDisplay.contains(learn.learnSkill))
                            {
                                if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
                                    skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                            }
                            else
                                skillToDisplay[learn.learnSkill]=learn.learnSkillLevel;
                        }
                    }
                    sub_index++;
                }
                if(skillToDisplay.isEmpty())
                    item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                            .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name)
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("No skill to learn"))
                            );
                else
                    item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                            .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name)
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("%n skill(s) to learn","",skillToDisplay.size()))
                            );
            }
            else
            {
                item->setText(tr("%1, level: %2\nHP: %3/%4")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name)
                        .arg(monster.level)
                        .arg(monster.hp)
                        .arg(stat.hp)
                        );
            }
            ui->monsterList->addItem(item);
            monsters_items_graphical[item]=monster.id;
        }
        else
        {
            error(QStringLiteral("Unknown monster: %1").arg(monster.monster));
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
    prepareFight();
    battleType=BattleType_Wild;
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at wildFightCollision()");
        return;
    }
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("A other %1 is in front of you!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    resetPosition(true);
    moveFightMonsterBoth();
}

void BaseWindow::prepareFight()
{
    escape=false;
    escapeSuccess=false;
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
    prepareFight();
    ui->frameFightTop->setVisible(false);
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(DatapackClientLoader::datapackLoader.botFightsExtra.value(fightId).start);
    ui->pushButtonFightEnterNext->setVisible(false);
    QList<PlayerMonster> botFightMonstersTransformed;
    const QList<BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapack::commonDatapack.botFights.value(fightId).monsters;
    int index=0;
    while(index<monsters.size())
    {
        botFightMonstersTransformed << FacilityLib::botFightMonsterToPlayerMonster(monsters.at(index),ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monsters.at(index).id),monsters.at(index).level));
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
    if(actualBot.properties.contains(QStringLiteral("skin")))
        botImage=getFrontSkin(actualBot.properties.value(QStringLiteral("skin")));
    else
        botImage=QPixmap(QStringLiteral(":/images/player_default/front.png"));
    ui->labelFightMonsterTop->setPixmap(botImage.scaled(160,160));
    qDebug() << QStringLiteral("The bot %1 is a front of you").arg(fightId);
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
    qDebug() << QStringLiteral("You are in front of monster id: %1 level: %2 with hp: %3").arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonster->hp);
    return true;
}

void BaseWindow::init_environement_display(Map_client *map, const quint8 &x, const quint8 &y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    //map not located
    if(map==NULL)
    {
        ui->frameFightBackground->setStyleSheet(QStringLiteral("#frameFightBackground{background-image: url(:/images/interface/fight/background.jpg);}"));
        return;
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*map,x,y);
    int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
        if(monstersCollision.item==0 || items.contains(monstersCollision.item))
        {
            if(!monstersCollision.background.isEmpty())
            {
                const QString &baseSearch=CatchChallenger::Api_client_real::client->datapackPath()+QLatin1Literal(DATAPACK_BASE_PATH_MAP)+monstersCollision.background;
                if(QFile(baseSearch+"/background.png").exists())
                    ui->frameFightBackground->setStyleSheet(QLatin1Literal("#frameFightBackground{background-image: url('")+baseSearch+QLatin1Literal("/background.png');}"));
                else if(QFile(baseSearch+"/background.jpg").exists() && (supportedImageFormats.contains(QLatin1Literal("jpeg")) || supportedImageFormats.contains(QLatin1Literal("jpg"))))
                    ui->frameFightBackground->setStyleSheet(QLatin1Literal("#frameFightBackground{background-image: url('")+baseSearch+QLatin1Literal("/background.jpg');}"));
                else
                    ui->frameFightBackground->setStyleSheet(QLatin1Literal("#frameFightBackground{background-image: url(:/images/interface/fight/background.jpg);}"));

                if(QFile(baseSearch+"/plateform-front.png").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QLatin1Literal("/plateform-front.png")));
                else if(QFile(baseSearch+"/plateform-front.gif").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QLatin1Literal("/plateform-front.gif")));
                else
                    ui->labelFightPlateformTop->setPixmap(QPixmap(QLatin1Literal(":/images/interface/fight/plateform-front.png")));

                if(QFile(baseSearch+"/plateform-background.png").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QLatin1Literal("/plateform-background.png")));
                else if(QFile(baseSearch+"/plateform-background.gif").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QLatin1Literal("/plateform-background.gif")));
                else
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(QLatin1Literal(":/images/interface/fight/plateform-background.png")));
            }
            else
            {
                ui->frameFightBackground->setStyleSheet(QStringLiteral("#frameFightBackground{background-image: url(:/images/interface/fight/background.jpg);}"));
                ui->labelFightPlateformTop->setPixmap(QPixmap(":/images/interface/fight/plateform-front.png"));
                ui->labelFightPlateformBottom->setPixmap(QPixmap(":/images/interface/fight/plateform-background.png"));
            }
            return;
        }
        index++;
    }
    ui->frameFightBackground->setStyleSheet(QStringLiteral("#frameFightBackground{background-image: url(:/images/interface/fight/background.jpg);}"));
    ui->labelFightPlateformTop->setPixmap(QPixmap(":/images/interface/fight/plateform-front.png"));
    ui->labelFightPlateformBottom->setPixmap(QPixmap(":/images/interface/fight/plateform-background.png"));
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
        ui->labelFightBottomName->setText(DatapackClientLoader::datapackLoader.monsterExtra.value(fightMonster->monster).name);
        ui->labelFightBottomLevel->setText(tr("Level %1").arg(fightMonster->level));
        Monster::Stat fightStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(fightMonster->monster),fightMonster->level);
        ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
        ui->progressBarFightBottomHP->setValue(fightMonster->hp);
        ui->progressBarFightBottomHP->repaint();
        ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(fightMonster->hp).arg(fightStat.hp));
        const Monster &monsterGenericInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(fightMonster->monster);
        int level_to_xp=monsterGenericInfo.level_to_xp.at(fightMonster->level-1);
        ui->progressBarFightBottomExp->setMaximum(level_to_xp);
        ui->progressBarFightBottomExp->setValue(fightMonster->remaining_xp);
        ui->progressBarFightBottomExp->repaint();
        //do the buff
        {
            buffToGraphicalItemBottom.clear();
            ui->bottomBuff->clear();
            int index=0;
            while(index<fightMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=fightMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name);
                    else
                        item->setToolTip(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).description);
                }
                buffToGraphicalItemBottom[buffEffect.buff]=item;
                item->setData(99,buffEffect.buff);//to prevent duplicate buff, because add can be to re-enable an already enable buff (for larger period then)
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
                ui->labelFightEnter->setText(tr("Protect me %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster->monster).name));
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
            updateCurrentMonsterInformationXp();
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
                ui->labelFightEnter->setText(tr("Go %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster->monster).name));
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
                if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
                {
                    #ifdef DEBUG_CLIENT_BATTLE
                    qDebug() << "Your current monster is KO, select another";
                    #endif
                    selectObject(ObjectType_MonsterToFightKO);
                }
                else
                {
                    #ifdef DEBUG_CLIENT_BATTLE
                    qDebug() << "You win";
                    #endif
                    displayText(tr("You win!"));
                    doNextActionStep=DoNextActionStep_Win;
                }
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

void BaseWindow::updateCurrentMonsterInformationXp()
{
    PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error"),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    ui->progressBarFightBottomExp->setValue(monster->remaining_xp);
    ui->progressBarFightBottomExp->repaint();
    ui->labelFightBottomLevel->setText(tr("Level %1").arg(monster->level));
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster->monster);
    quint32 maxXp=monsterInformations.level_to_xp.at(monster->level-1);
    ui->progressBarFightBottomExp->setMaximum(maxXp);
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
    ui->labelFightMonsterBottom->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra.value(monster->monster).back.scaled(160,160));
    ui->frameFightBottom->setVisible(true);
    ui->frameFightBottom->show();
    currentMonsterLevel=monster->level;
    updateAttackList();
}

void BaseWindow::updateAttackList()
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
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    int index=0;
    useTheRescueSkill=true;
    while(index<monster->skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::PlayerSkill &skill=monster->skills.at(index);
        if(skill.level>1)
            item->setText(QStringLiteral("%1, level %2 (remaining endurance: %3)")
                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(skill.skill).name)
                    .arg(skill.level)
                    .arg(skill.endurance)
                    );
        else
            item->setText(QStringLiteral("%1 (remaining endurance: %2)")
                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(skill.skill).name)
                    .arg(skill.endurance)
                    );
        item->setToolTip(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(skill.skill).description);
        item->setData(99,skill.endurance);
        item->setData(98,skill.skill);
        if(skill.endurance>0)
            useTheRescueSkill=false;
        fight_attacks_graphical[item]=skill.skill;
        ui->listWidgetFightAttack->addItem(item);
        index++;
    }
    if(useTheRescueSkill && CommonDatapack::commonDatapack.monsterSkills.contains(0))
        if(!CommonDatapack::commonDatapack.monsterSkills.value(0).level.isEmpty())
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QStringLiteral("Rescue skill: %1").arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(0).name));
            item->setToolTip(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(0).description);
            item->setData(99,0);
            fight_attacks_graphical[item]=0;
            ui->listWidgetFightAttack->addItem(item);
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
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
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
                    {
                        ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
                        ui->progressBarFightBottomExp->repaint();
                        ui->labelFightBottomLevel->setText(tr("Level %1").arg(currentMonster->level));
                    }
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
                    text+=tr("The other player call %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name);
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("The other player call %1!").arg("(Unknow monster)");
                }
                text+="<br />";
                PublicPlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
                if(monster!=NULL)
                    text+=tr("You call %1!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster->monster).name);
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
        ui->labelFightMonsterTop->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).front.scaled(160,160));
        //other monster
        ui->frameFightTop->setVisible(true);
        ui->frameFightTop->show();
        ui->labelFightTopName->setText(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name);
        ui->labelFightTopLevel->setText(tr("Level %1").arg(otherMonster->level));
        Monster::Stat otherStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.value(otherMonster->monster),otherMonster->level);
        ui->progressBarFightTopHP->setMaximum(otherStat.hp);
        ui->progressBarFightTopHP->setValue(otherMonster->hp);
        ui->progressBarFightTopHP->repaint();
        //do the buff
        {
            buffToGraphicalItemTop.clear();
            ui->topBuff->clear();
            int index=0;
            while(index<otherMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=otherMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(buffEffect.buff))
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name);
                    else
                        item->setToolTip(tr("%1 at level %2").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).name).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(buffEffect.buff).description);
                }
                buffToGraphicalItemTop[buffEffect.buff]=item;
                item->setData(99,buffEffect.buff);//to prevent duplicate buff, because add can be to re-enable an already enable buff (for larger period then)
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
    const quint32 skillEndurance=itemsList.first()->data(99).toUInt();
    const quint32 skillUsed=fight_attacks_graphical.value(itemsList.first());
    if(skillEndurance<=0 && !useTheRescueSkill)
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
    if(useTheRescueSkill)
        CatchChallenger::ClientFightEngine::fightEngine.useSkill(0);
    else
        CatchChallenger::ClientFightEngine::fightEngine.useSkill(skillUsed);
    updateAttackList();
    displayAttackProgression=0;
    attack_quantity_changed=0;
    if(battleType!=BattleType_OtherPlayer)
        doNextAction();
    else
    {
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("In waiting of the other player action"));
        ui->pushButtonFightEnterNext->hide();
    }
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
    quint32 skillId=fight_attacks_graphical.value(itemsList.first());
    ui->labelFightAttackDetails->setText(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(skillId).description);
}

void BaseWindow::checkEvolution()
{
    PlayerMonster *monster=CatchChallenger::ClientFightEngine::fightEngine.evolutionByLevelUp();
    if(monster!=NULL)
    {
        const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster->monster);
        const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(monster->monster);
        int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Level && evolution.level==monster->level)
            {
                idMonsterEvolution=monster->id;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.value(evolution.evolveTo);
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(evolution.evolveTo);
                //create animation widget
                if(animationWidget!=NULL)
                    delete animationWidget;
                if(qQuickViewContainer!=NULL)
                    delete qQuickViewContainer;
                animationWidget=new QQuickView();
                qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
                qQuickViewContainer->setMinimumSize(QSize(800,600));
                qQuickViewContainer->setMaximumSize(QSize(800,600));
                qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
                ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                if(baseMonsterEvolution!=NULL)
                    delete baseMonsterEvolution;
                if(targetMonsterEvolution!=NULL)
                    delete targetMonsterEvolution;
                baseMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformations,monsterInformationsExtra);
                targetMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                if(evolutionControl!=NULL)
                    delete evolutionControl;
                evolutionControl=new EvolutionControl(monsterInformations,monsterInformationsExtra,monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                connect(evolutionControl,&EvolutionControl::cancel,this,&BaseWindow::evolutionCanceled,Qt::QueuedConnection);
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(true));
                animationWidget->rootContext()->setContextProperty("itemEvolution",QString());
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const QString datapackQmlFile=CatchChallenger::Api_client_real::client->datapackPath()+"qml/evolution-animation.qml";
                if(QFile(datapackQmlFile).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
                return;
            }
            index++;
        }
    }
    while(!tradeEvolutionMonsters.isEmpty())
    {
        const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(tradeEvolutionMonsters.first().monster);
        const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(tradeEvolutionMonsters.first().monster);
        int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Trade)
            {
                idMonsterEvolution=monster->id;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.value(evolution.evolveTo);
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(evolution.evolveTo);
                //create animation widget
                if(animationWidget!=NULL)
                    delete animationWidget;
                if(qQuickViewContainer!=NULL)
                    delete qQuickViewContainer;
                animationWidget=new QQuickView();
                qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
                qQuickViewContainer->setMinimumSize(QSize(800,600));
                qQuickViewContainer->setMaximumSize(QSize(800,600));
                qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
                ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
                //show the animation
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                if(baseMonsterEvolution!=NULL)
                    delete baseMonsterEvolution;
                if(targetMonsterEvolution!=NULL)
                    delete targetMonsterEvolution;
                baseMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformations,monsterInformationsExtra);
                targetMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                if(evolutionControl!=NULL)
                    delete evolutionControl;
                evolutionControl=new EvolutionControl(monsterInformations,monsterInformationsExtra,monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                connect(evolutionControl,&EvolutionControl::cancel,this,&BaseWindow::evolutionCanceled,Qt::QueuedConnection);
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(true));
                animationWidget->rootContext()->setContextProperty("itemEvolution",QString());
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const QString datapackQmlFile=CatchChallenger::Api_client_real::client->datapackPath()+"qml/evolution-animation.qml";
                if(QFile(datapackQmlFile).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
                tradeEvolutionMonsters.removeFirst();
                return;
            }
            index++;
        }
        tradeEvolutionMonsters.removeFirst();
    }
}

void BaseWindow::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    checkEvolution();
}

void BaseWindow::loose()
{
    qDebug() << "loose()";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       );
            return;
        }
    #endif
    CatchChallenger::ClientFightEngine::fightEngine.healAllMonsters();
    CatchChallenger::ClientFightEngine::fightEngine.fightFinished();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
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
    checkEvolution();
}

void BaseWindow::win()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       );
            return;
        }
    #endif
    CatchChallenger::ClientFightEngine::fightEngine.fightFinished();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win end && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       );
            return;
        }
    #endif
    if(zonecatch)
        ui->stackedWidget->setCurrentWidget(ui->page_zonecatch);
    else
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(!zonecatch)
            {
                if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightId))
                {
                    emit error("fight id not found at collision");
                    return;
                }
                addCash(CatchChallenger::CommonDatapack::commonDatapack.botFights.value(fightId).cash);
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
    checkEvolution();
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
                displayText(tr("The wild %1 can't attack").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
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
        PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
        if(otherMonster)
        {
            qDebug() << "doNextAction(): apply the effect and display it";
            displayAttack();
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
        ui->labelFightEnter->setText(tr("Your %1 have lost!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name));
        doNextActionStep=DoNextActionStep_Start;
        moveType=MoveType_Dead;
        moveFightMonsterBottom();
        return;
    }

    //if the other monster is KO
    if(CatchChallenger::ClientFightEngine::fightEngine.otherMonsterIsKO())
    {
        quint32 returnedLastGivenXP=CatchChallenger::ClientFightEngine::fightEngine.lastGivenXP();
        if(returnedLastGivenXP>2*1000*1000)
        {
            newError(tr("Internal error"),QStringLiteral("returnedLastGivenXP is negative"));
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
        if(!CatchChallenger::ClientFightEngine::fightEngine.isInFight())
        {
            win();
            return;
        }
        updateCurrentMonsterInformationXp();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().isEmpty())
        {
            PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
            PublicPlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
            if(currentMonster!=NULL)
                if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
                {
                    emit error(QStringLiteral("Current monster hp don't match with the internal value (do next action): %1!=%2")
                               .arg(currentMonster->hp)
                               .arg(ui->progressBarFightBottomHP->value())
                               );
                    return;
                }
            if(otherMonster!=NULL)
                if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
                {
                    emit error(QStringLiteral("Other monster hp don't match with the internal value (do next action): %1!=%2")
                               .arg(otherMonster->hp)
                               .arg(ui->progressBarFightTopHP->value())
                               );
                    return;
                }
        }
        #endif
        qDebug() << "doNextAction(): remplace KO other monster";
        PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
        if(otherMonster==NULL)
        {
            emit error("No other monster into doNextAction()");
            return;
        }
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("The other %1 have lost!").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
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
    PublicPlayerMonster *currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (doNextAction && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Other monster damage don't match with the internal value (doNextAction && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       );
            return;
        }
    #endif
    return;
}

void BaseWindow::pageChanged()
{
    if(ui->stackedWidget->currentWidget()==ui->page_map)
        MapController::mapController->setFocus();
}

bool BaseWindow::displayFirstAttackText(bool firstText)
{
    PublicPlayerMonster * otherMonster=CatchChallenger::ClientFightEngine::fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to display an attack");
        doNextAction();
        return false;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error"),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return false;
    }
    if(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().isEmpty())
    {
        emit error("Display text for not existant attack");
        return false;
    }
    const Skill::AttackReturn currentAttack=CatchChallenger::ClientFightEngine::fightEngine.getFirstAttackReturn();

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
                .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size());

    //in case of failed
    if(!currentAttack.success)
    {
        if(currentAttack.doByTheCurrentMonster)
            displayText(tr("Your %1 have failed the attack %2")
                          .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                          .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(currentAttack.attack).name));
        else
            displayText(tr("The other %1 have failed the attack %2")
                          .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                          .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(currentAttack.attack).name));
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
        return false;
    }

    QString attackOwner;
    if(firstText)
    {
        if(currentAttack.doByTheCurrentMonster)
            attackOwner=tr("Your %1 do the attack %2 and ")
                .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(currentAttack.attack).name);
        else
            attackOwner=tr("The other %1 do the attack %2 and ")
                .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(currentAttack.attack).name);
    }
    else
    {
        if(currentAttack.doByTheCurrentMonster)
            attackOwner=tr("Your %1 ").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name);
        else
            attackOwner=tr("The other %1 ").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name);
    }

    //the life effect
    if(!currentAttack.lifeEffectMonster.isEmpty())
    {
        const Skill::LifeEffectReturn &lifeEffectReturn=currentAttack.lifeEffectMonster.first();
        if(currentAttack.doByTheCurrentMonster)
        {
            if((lifeEffectReturn.on & ApplyOn_AllEnemy) || (lifeEffectReturn.on & ApplyOn_AloneEnemy))
            {
                if(lifeEffectReturn.quantity>0)
                    attackOwner+=tr("heal of %2 the other %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt of %2 the other %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
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
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                        .arg(lifeEffectReturn.quantity);
                else if(lifeEffectReturn.quantity<0)
                    attackOwner+=tr("hurt of %2 your %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
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
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size());
        return true;
    }

    //the add buff
    if(!currentAttack.addBuffEffectMonster.isEmpty())
    {
        const Skill::BuffEffect &addBuffEffectMonster=currentAttack.addBuffEffectMonster.first();
        QHash<quint32,QListWidgetItem *> *buffToGraphicalItemCurrentbar=NULL;
        QListWidget *listWidget=NULL;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        bool onBuffCurrentMonster;
        #endif
        if(currentAttack.doByTheCurrentMonster)
        {
            if((addBuffEffectMonster.on & ApplyOn_AllEnemy) || (addBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on the other %1")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(addBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
                listWidget=ui->topBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=false;
                #endif
            }
            if((addBuffEffectMonster.on & ApplyOn_AllAlly) || (addBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(addBuffEffectMonster.buff).name);
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
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(addBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
                listWidget=ui->bottomBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=true;
                #endif
            }
            if((addBuffEffectMonster.on & ApplyOn_AllAlly) || (addBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(addBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
                listWidget=ui->topBuff;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                onBuffCurrentMonster=false;
                #endif
            }
        }
        displayText(attackOwner);
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
            if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(addBuffEffectMonster.buff))
            {
                item->setToolTip(tr("Unknown buff"));
                item->setIcon(QIcon(":/images/interface/buff.png"));
            }
            else
            {
                const DatapackClientLoader::MonsterExtra::Buff &buffExtra=DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(addBuffEffectMonster.buff);
                if(!buffExtra.icon.isNull())
                    item->setIcon(buffExtra.icon);
                else
                    item->setIcon(QIcon(":/images/interface/buff.png"));
                if(addBuffEffectMonster.level<=1)
                    item->setToolTip(buffExtra.name);
                else
                    item->setToolTip(tr("%1 at level %2").arg(buffExtra.name).arg(addBuffEffectMonster.level));
                item->setToolTip(item->toolTip()+"\n"+buffExtra.description);
            }
            (*buffToGraphicalItemCurrentbar)[addBuffEffectMonster.buff]=item;
            if(index==listWidget->count())
                listWidget->addItem(item);
        }
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAddBuffEffectAttackReturn();
        if(!CatchChallenger::ClientFightEngine::fightEngine.firstAttackReturnHaveMoreEffect())
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!onBuffCurrentMonster && otherMonster!=NULL)
        {
            if(otherMonster->buffs.size()!=listWidget->count())
            {
                error("displayFirstAttackText(): bug: buffs number displayed != than real buff count on other monter");
                doNextAction();
                return false;
            }
        }
        if(onBuffCurrentMonster && currentMonster!=NULL)
        {
            if(currentMonster->buffs.size()!=listWidget->count())
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
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size());
        return false;
    }

    //the remove buff
    if(!currentAttack.removeBuffEffectMonster.isEmpty())
    {
        const Skill::BuffEffect &removeBuffEffectMonster=currentAttack.removeBuffEffectMonster.first();
        QHash<quint32,QListWidgetItem *> *buffToGraphicalItemCurrentbar=NULL;
        if(currentAttack.doByTheCurrentMonster)
        {
            if((removeBuffEffectMonster.on & ApplyOn_AllEnemy) || (removeBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on the other %1")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(removeBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
            }
            if((removeBuffEffectMonster.on & ApplyOn_AllAlly) || (removeBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(removeBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
            }
        }
        else
        {
            if((removeBuffEffectMonster.on & ApplyOn_AllEnemy) || (removeBuffEffectMonster.on & ApplyOn_AloneEnemy))
            {
                attackOwner+=tr("add the buff %2 on your %1")
                    .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(removeBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemBottom;
            }
            if((removeBuffEffectMonster.on & ApplyOn_AllAlly) || (removeBuffEffectMonster.on & ApplyOn_Themself))
            {
                attackOwner+=tr("add the buff %1 on themself")
                    .arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.value(removeBuffEffectMonster.buff).name);
                buffToGraphicalItemCurrentbar=&buffToGraphicalItemTop;
            }
        }
        displayText(attackOwner);
        if(buffToGraphicalItemCurrentbar!=NULL)
        {
            if(buffToGraphicalItemTop.contains(removeBuffEffectMonster.buff))
                delete buffToGraphicalItemTop.value(removeBuffEffectMonster.buff);
            buffToGraphicalItemTop.remove(removeBuffEffectMonster.buff);
        }
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstRemoveBuffEffectAttackReturn();
        if(!CatchChallenger::ClientFightEngine::fightEngine.firstAttackReturnHaveMoreEffect())
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
        qDebug() << "display the remove buff";
        qDebug() << QStringLiteral("displayFirstAttackText(): after display text, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, attackReturnList.size(): %5")
                    .arg(currentAttack.lifeEffectMonster.size())
                    .arg(currentAttack.buffLifeEffectMonster.size())
                    .arg(currentAttack.addBuffEffectMonster.size())
                    .arg(currentAttack.removeBuffEffectMonster.size())
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size());
        return false;
    }

    //the apply buff effect
    if(!currentAttack.buffLifeEffectMonster.isEmpty())
    {
        const Skill::LifeEffectReturn &buffLifeEffectMonster=currentAttack.buffLifeEffectMonster.first();
        if(currentAttack.doByTheCurrentMonster)
        {
            if((buffLifeEffectMonster.on & ApplyOn_AllEnemy) || (buffLifeEffectMonster.on & ApplyOn_AloneEnemy))
            {
                if(buffLifeEffectMonster.quantity>0)
                    attackOwner+=tr("heal of %2 the other %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt of %2 the other %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name)
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
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
                        .arg(buffLifeEffectMonster.quantity);
                else if(buffLifeEffectMonster.quantity<0)
                    attackOwner+=tr("hurt of %2 your %1")
                        .arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name)
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
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size());
        return true;
    }
    emit error("Can't display text without effect");
    return false;
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
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().removeBuffEffectMonster.isEmpty() &&
            CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success
            )
    {
        qDebug() << QStringLiteral("displayAttack(): strange: display an empty lifeEffect list into attack return, attack: %1, doByTheCurrentMonster: %2, success: %3")
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().attack)
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().doByTheCurrentMonster)
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().first().success);
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
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

    const Skill::AttackReturn &attackReturn=CatchChallenger::ClientFightEngine::fightEngine.getFirstAttackReturn();
    //get the life effect to display
    Skill::LifeEffectReturn lifeEffectReturn;
    if(!attackReturn.lifeEffectMonster.isEmpty())
        lifeEffectReturn=attackReturn.lifeEffectMonster.first();
    else if(!attackReturn.buffLifeEffectMonster.isEmpty())
        lifeEffectReturn=attackReturn.buffLifeEffectMonster.first();
    else
    {
        newError(tr("Internal error"),QStringLiteral("displayAttack(): strange: nothing to display, lifeEffectMonster.size(): %1, buffLifeEffectMonster.size(): %2, addBuffEffectMonster.size(): %3, removeBuffEffectMonster.size(): %4, AttackReturnList: %5")
                    .arg(attackReturn.lifeEffectMonster.size())
                    .arg(attackReturn.buffLifeEffectMonster.size())
                    .arg(attackReturn.addBuffEffectMonster.size())
                    .arg(attackReturn.removeBuffEffectMonster.size())
                    .arg(CatchChallenger::ClientFightEngine::fightEngine.getAttackReturnList().size()));
        CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
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
        quint32 attackId=attackReturn.attack;
        QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+QStringLiteral(DATAPACK_BASE_PATH_SKILLANIMATION);
        QString fileAnimation=skillAnimation+QStringLiteral("%1.mng").arg(attackId);
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
        else
        {
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
            if(attackReturn.lifeEffectMonster.isEmpty())
                hp_to_change=0;
            else if(attackReturn.lifeEffectMonster.first().quantity!=0)
                hp_to_change=attackReturn.lifeEffectMonster.first().quantity;
            else
                hp_to_change=0;
            if(hp_to_change!=0)
            {
                CatchChallenger::ClientFightEngine::fightEngine.firstLifeEffectQuantityChange(-hp_to_change);
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
        if(!attackReturn.lifeEffectMonster.isEmpty())
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstLifeEffectAttackReturn();
        else
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstBuffEffectAttackReturn();
        if(!CatchChallenger::ClientFightEngine::fightEngine.firstAttackReturnHaveMoreEffect())
        {
            #ifdef CATCHCHALLENGER_DEBUG_FIGHT
            {
                qDebug() << "after display attack: currentMonster have hp" << ui->progressBarFightBottomHP->value() << "and buff" << ui->bottomBuff->count();
                qDebug() << "after display attack: otherMonster have hp" << ui->progressBarFightTopHP->value() << "and buff" << ui->topBuff->count();
            }
            #endif
            CatchChallenger::ClientFightEngine::fightEngine.removeTheFirstAttackReturn();
        }
        //attack is finish
        doNextAction();
    }
    else
    {
        if(attackReturn.lifeEffectMonster.isEmpty())
            hp_to_change=0;
        else if(attackReturn.lifeEffectMonster.first().quantity<0)
        {
            hp_to_change=-hp_to_change;
            if(abs(hp_to_change)>abs(attackReturn.lifeEffectMonster.first().quantity))
                hp_to_change=attackReturn.lifeEffectMonster.first().quantity;
        }
        else if(attackReturn.lifeEffectMonster.first().quantity>0)
        {
            if(hp_to_change>attackReturn.lifeEffectMonster.first().quantity)
                hp_to_change=attackReturn.lifeEffectMonster.first().quantity;
        }
        else
            hp_to_change=0;
        if(hp_to_change!=0)
        {
            CatchChallenger::ClientFightEngine::fightEngine.firstLifeEffectQuantityChange(-hp_to_change);
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

void BaseWindow::displayExperienceGain()
{
    PlayerMonster * currentMonster=CatchChallenger::ClientFightEngine::fightEngine.getCurrentMonster();
    if(currentMonster==NULL)
    {
        mLastGivenXP=0;
        newError(tr("Internal error"),"displayAttack(): crash: unable to get the current monster");
        doNextAction();
        return;
    }
    if(ui->progressBarFightBottomExp->value()==ui->progressBarFightBottomExp->maximum())
    {
        ui->progressBarFightBottomExp->setValue(0);
        ui->progressBarFightBottomExp->repaint();
    }
    //animation finished
    if(mLastGivenXP<=0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mLastGivenXP>4294000000)
        {
            newError(tr("Internal error"),QStringLiteral("part0: mLastGivenXP is negative"));
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error"),QStringLiteral("par0: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level));
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        if(currentMonsterLevel==currentMonster->level && (quint32)ui->progressBarFightBottomExp->value()>currentMonster->remaining_xp)
        {
            newError(tr("Internal error"),QStringLiteral("part0: displayed xp greater than the real xp: %1>%2").arg(ui->progressBarFightBottomExp->value()).arg(currentMonster->remaining_xp));
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        #endif
        doNextAction();
        return;
    }

    //if start, display text
    if(displayAttackProgression==0)
    {
        ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("You %1 gain %2 of experience").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(currentMonster->monster).name).arg(mLastGivenXP));
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error"),QStringLiteral("part1: mLastGivenXP is negative"));
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error"),QStringLiteral("part1: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level));
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    quint32 xp_to_change;
    xp_to_change=ui->progressBarFightBottomExp->maximum()/200;//0.5%
    if(xp_to_change==0)
        xp_to_change=1;
    if(xp_to_change>mLastGivenXP)
        xp_to_change=mLastGivenXP;

    quint32 maxXp=ui->progressBarFightBottomExp->maximum();
    if(((quint32)ui->progressBarFightBottomExp->value()+xp_to_change)>=(quint32)maxXp)
    {
        xp_to_change=maxXp-ui->progressBarFightBottomExp->value();
        if(xp_to_change>mLastGivenXP)
            xp_to_change=mLastGivenXP;
        if(mLastGivenXP>xp_to_change)
            mLastGivenXP-=xp_to_change;
        else
            mLastGivenXP=0;

        const Monster::Stat &oldStat=CatchChallenger::ClientFightEngine::fightEngine.getStat(CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonsterLevel);
        const Monster::Stat &newStat=CatchChallenger::ClientFightEngine::fightEngine.getStat(CommonDatapack::commonDatapack.monsters.value(currentMonster->monster),currentMonsterLevel+1);
        if(oldStat.hp<newStat.hp)
        {
            qDebug() << QStringLiteral("Now the old hp: %1/%2 increased of %3 for the old level %4").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()).arg(newStat.hp-oldStat.hp).arg(currentMonsterLevel);
            ui->progressBarFightBottomHP->setMaximum(ui->progressBarFightBottomHP->maximum()+(newStat.hp-oldStat.hp));
            ui->progressBarFightBottomHP->setValue(ui->progressBarFightBottomHP->value()+(newStat.hp-oldStat.hp));
            ui->progressBarFightBottomHP->repaint();
            ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()));
        }
        else
            qDebug() << QStringLiteral("The hp at level change is untouched: %1/%2 for the old level %3").arg(ui->progressBarFightBottomHP->value()).arg(ui->progressBarFightBottomHP->maximum()).arg(currentMonsterLevel);
        ui->progressBarFightBottomExp->setMaximum(CommonDatapack::commonDatapack.monsters.value(currentMonster->monster).level_to_xp.at(currentMonsterLevel-1));
        ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->maximum());
        ui->progressBarFightBottomExp->repaint();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mLastGivenXP>4294000000)
        {
            newError(tr("Internal error"),QStringLiteral("part2: mLastGivenXP is negative"));
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error"),QStringLiteral("part2: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level));
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        #endif
        if(currentMonsterLevel>=CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        currentMonsterLevel++;
        ui->labelFightBottomLevel->setText(tr("Level %1").arg(currentMonsterLevel));
        displayExpTimer.start();
        displayAttackProgression++;
        return;
    }
    else
    {
        ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->value()+xp_to_change);
        ui->progressBarFightBottomExp->repaint();
    }
    if(mLastGivenXP>xp_to_change)
        mLastGivenXP-=xp_to_change;
    else
        mLastGivenXP=0;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error"),QStringLiteral("part3: mLastGivenXP is negative"));
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error"),QStringLiteral("part3: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level));
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    displayExpTimer.start();
    displayAttackProgression++;
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
        QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+QStringLiteral(DATAPACK_BASE_PATH_ITEM);
        QString fileAnimation=skillAnimation+QStringLiteral("%1.mng").arg(trapItemId);
        if(QFile(fileAnimation).exists())
        {
            movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
            movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
            if(movie->isValid())
            {
                ui->labelFightTrap->setMovie(movie);
                movie->start();
            }
            else
                qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
        }
        else
        {
            fileAnimation=skillAnimation+QStringLiteral("%1.gif").arg(trapItemId);
            if(QFile(fileAnimation).exists())
            {
                movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                if(movie->isValid())
                {
                    ui->labelFightTrap->setMovie(movie);
                    movie->start();
                }
                else
                    qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
            }
        }
        ui->labelFightEnter->setText(QStringLiteral("Try catch the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
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
    if(!CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.isEmpty())
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
            QString skillAnimation=DatapackClientLoader::datapackLoader.getDatapackPath()+QStringLiteral(DATAPACK_BASE_PATH_ITEM);
            QString fileAnimation;
            if(trapSuccess)
                fileAnimation=skillAnimation+QStringLiteral("%1_success.mng").arg(trapItemId);
            else
                fileAnimation=skillAnimation+QStringLiteral("%1_failed.mng").arg(trapItemId);
            if(QFile(fileAnimation).exists())
            {
                movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                if(movie->isValid())
                {
                    ui->labelFightTrap->setMovie(movie);
                    movie->start();
                }
                else
                    qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
            }
            else
            {
                if(trapSuccess)
                    fileAnimation=skillAnimation+QStringLiteral("%1_success.gif").arg(trapItemId);
                else
                    fileAnimation=skillAnimation+QStringLiteral("%1_failed.gif").arg(trapItemId);
                if(QFile(fileAnimation).exists())
                {
                    movie=new QMovie(fileAnimation,QByteArray(),ui->labelFightTrap);
                    movie->setScaledSize(QSize(ui->labelFightTrap->width(),ui->labelFightTrap->height()));
                    if(movie->isValid())
                    {
                        ui->labelFightTrap->setMovie(movie);
                        movie->start();
                    }
                    else
                        qDebug() << QStringLiteral("movie loaded is not valid for: %1").arg(fileAnimation);
                }
            }
        }
        else
        {
            if(trapSuccess)
            {
                CatchChallenger::ClientFightEngine::fightEngine.catchIsDone();//why? do at the screen change to return on the map, not here!
                displayText(tr("You have catched the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
            }
            else
                displayText(tr("You have failed the catch of the wild %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(otherMonster->monster).name));
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
        item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).front);
        item->setText(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name);
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    else
    {
        item->setIcon(QIcon(":/images/monsters/default/front.png"));
        item->setText(tr("Unknown"));
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    ui->tradeOtherMonsters->addItem(item);
}

void BaseWindow::on_learnQuit_clicked()
{
    selectObject(ObjectType_MonsterToLearn);
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
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
    if(!learnSkill(monsterToLearn,attack_to_learn_graphical.value(item)))
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
            ui->learnMonster->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).front.scaled(160,160));
            if(CommonSettings::commonSettings.useSP)
            {
                ui->learnSP->setVisible(true);
                ui->learnSP->setText(tr("SP: %1").arg(monster.sp));
            }
            else
                ui->learnSP->setVisible(false);
            #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
            ui->learnInfo->setText(tr("<b>%1</b><br />Level %2").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster).name).arg(monster.level));
            #endif
            int sub_index=0;
            while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.size())
            {
                Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.value(monster.monster).learn.at(sub_index);
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
                            (sub_index2<monster.skills.size() && (monster.skills.value(sub_index2).level+1)==learn.learnSkillLevel)
                    )
                    {
                        if(skillToDisplay.contains(learn.learnSkill))
                        {
                            if(skillToDisplay.value(learn.learnSkill)>learn.learnSkillLevel)
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
                const quint32 &level=i.value();
                if(CommonSettings::commonSettings.useSP)
                {
                    if(level<=1)
                        item->setText(tr("%1\nSP cost: %2")
                                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(i.key()).name)
                                    .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(i.key()).level.value(i.value()-1).sp_to_learn)
                                );
                    else
                        item->setText(tr("%1 level %2\nSP cost: %3")
                                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(i.key()).name)
                                    .arg(level)
                                    .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(i.key()).level.value(i.value()-1).sp_to_learn)
                                );
                }
                else
                {
                    if(level<=1)
                        item->setText(tr("%1")
                                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(i.key()).name)
                                );
                    else
                        item->setText(tr("%1 level %2")
                                    .arg(DatapackClientLoader::datapackLoader.monsterSkillsExtra.value(i.key()).name)
                                    .arg(level)
                                );
                }
                if(CommonSettings::commonSettings.useSP && CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.value(i.key()).level.value(i.value()-1).sp_to_learn>monster.sp)
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
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=ClientFightEngine::fightEngine.getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster was hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=ClientFightEngine::fightEngine.getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster was hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    CatchChallenger::ClientFightEngine::fightEngine.addAndApplyAttackReturnList(attackReturn);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=ClientFightEngine::fightEngine.getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster have hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=ClientFightEngine::fightEngine.getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster have hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    doNextAction();
}

/*void BaseWindow::sendFullBattleReturn(const QList<Skill::AttackReturn> &attackReturn,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
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
}*/


void BaseWindow::on_listWidgetFightAttack_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    on_pushButtonFightAttackConfirmed_clicked();
}

void BaseWindow::useTrap(const quint32 &itemId)
{
    updateTrapTime.restart();
    displayTrapProgression=0;
    trapItemId=itemId;
    CatchChallenger::ClientFightEngine::fightEngine.tryCatchClient(itemId);
    displayTrap();
}

void BaseWindow::monsterCatch(const quint32 &newMonsterId)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.isEmpty())
    {
        emit error("Internal bug: cupture monster list is emtpy");
        return;
    }
    if(newMonsterId==0x00000000)
    {
        trapSuccess=false;
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QStringLiteral("catch is failed"));
        #endif
        CatchChallenger::ClientFightEngine::fightEngine.generateOtherAttack();
    }
    else
    {
        trapSuccess=true;
        #ifdef DEBUG_MESSAGE_CLIENT_FIGHT
        emit message(QStringLiteral("catch is success"));
        #endif
        CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.first().id=newMonsterId;
        if(CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster().count()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
            warehouse_playerMonster << CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.first();
        else
        {
            CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.first());
            load_monsters();
        }
    }
    CatchChallenger::ClientFightEngine::fightEngine.playerMonster_catchInProgress.removeFirst();
    displayTrap();
}

void BaseWindow::battleAcceptedByOther(const QString &pseudo,const quint8 &skinId,const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    BattleInformations battleInformations;
    battleInformations.pseudo=pseudo;
    battleInformations.skinId=skinId;
    battleInformations.stat=stat;
    battleInformations.monsterPlace=monsterPlace;
    battleInformations.publicPlayerMonster=publicPlayerMonster;
    battleInformationsList << battleInformations;
    if(battleInformationsList.size()>1 || !botFightList.isEmpty())
        return;
    battleAcceptedByOtherFull(battleInformations);
}

void BaseWindow::battleAcceptedByOtherFull(const BattleInformations &battleInformations)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "already in fight";
        CatchChallenger::Api_client_real::client->battleRefused();
        return;
    }
    prepareFight();
    battleType=BattleType_OtherPlayer;
    ui->stackedWidget->setCurrentWidget(ui->page_battle);

    //skinFolderList=CatchChallenger::FacilityLib::skinIdList(CatchChallenger::Api_client_real::client->get_datapack_base_name()+QStringLiteral(DATAPACK_BASE_PATH_SKIN));
    QPixmap otherFrontImage=getFrontSkin(battleInformations.skinId);

    //reset the other player info
    ui->labelFightMonsterTop->setPixmap(otherFrontImage);
    //ui->battleOtherPseudo->setText(lastBattleQuery.first().first);
    ui->frameFightTop->hide();
    ui->frameFightBottom->hide();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage);
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("%1 wish fight with you").arg(battleInformations.pseudo));
    ui->pushButtonFightEnterNext->hide();

    resetPosition(true);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
    CatchChallenger::ClientFightEngine::fightEngine.setBattleMonster(battleInformations.stat,battleInformations.monsterPlace,battleInformations.publicPlayerMonster);
}

void BaseWindow::battleCanceledByOther()
{
    CatchChallenger::ClientFightEngine::fightEngine.fightFinished();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled the battle"));
    load_monsters();
    if(battleInformationsList.isEmpty())
    {
        emit error("battle info not found at collision");
        return;
    }
    battleInformationsList.removeFirst();
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
