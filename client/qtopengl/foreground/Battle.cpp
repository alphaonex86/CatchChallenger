#include "Battle.hpp"
#include "../CCprogressbar.hpp"
#include "../CustomButton.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../cc/QtDatapackClientLoader.hpp"
#include <QGraphicsProxyWidget>
#include <QListWidget>
#include <QListWidgetItem>

Battle::Battle()
{
    frameFightBottom=new ImagesStrechMiddle(28,":/CC/images/interface/b1.png",this);
    labelFightBottomName=new QGraphicsTextItem(frameFightBottom);
    progressBarFightBottomHP=new CCprogressbar(frameFightBottom);

    frameFightTop=new ImagesStrechMiddle(28,":/CC/images/interface/b1.png",this);
    labelFightTopName=new QGraphicsTextItem(frameFightTop);
    progressBarFightTopHP=new CCprogressbar(frameFightTop);

    labelFightBackground=new QGraphicsPixmapItem(this);
    labelFightBackgroundPix=QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+"/map/fight/grass/background.png");
    labelFightForeground=new QGraphicsPixmapItem(this);
    labelFightForegroundPix=QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+"/map/fight/grass/foreground.png");

    labelFightMonsterAttackBottom=new QGraphicsPixmapItem(this);
    labelFightMonsterAttackTop=new QGraphicsPixmapItem(this);
    labelFightMonsterBottom=new QGraphicsPixmapItem(this);
    labelFightMonsterTop=new QGraphicsPixmapItem(this);
    labelFightPlateformBottom=new QGraphicsPixmapItem(this);
    labelFightPlateformTop=new QGraphicsPixmapItem(this);
    labelFightTrap=new QGraphicsPixmapItem(this);

    stackedWidgetFightBottomBar=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    labelFightEnter=new QGraphicsTextItem(stackedWidgetFightBottomBar);
    pushButtonFightEnterNext=new CustomButton(":/CC/images/interface/greenbutton.png",stackedWidgetFightBottomBar);

    pushButtonFightAttack=new CustomButton(":/CC/images/interface/greenbutton.png",stackedWidgetFightBottomBar);
    pushButtonFightBag=new CustomButton(":/CC/images/interface/button.png",stackedWidgetFightBottomBar);
    pushButtonFightMonster=new CustomButton(":/CC/images/interface/button.png",stackedWidgetFightBottomBar);
    toolButtonFightQuit=new CustomButton(":/CC/images/interface/button.png",stackedWidgetFightBottomBar);

    pushButtonFightAttackConfirmed=new CustomButton(":/CC/images/interface/greenbutton.png",stackedWidgetFightBottomBar);
    pushButtonFightReturn=new CustomButton(":/CC/images/interface/button.png",stackedWidgetFightBottomBar);
    labelFightAttackDetails=new QGraphicsTextItem(stackedWidgetFightBottomBar);
    listWidgetFightAttackProxy=new QGraphicsProxyWidget(stackedWidgetFightBottomBar);
    listWidgetFightAttack=new QListWidget();
    listWidgetFightAttackProxy->setWidget(listWidgetFightAttack);

    m_stackedWidgetFightBottomBarIndex=0;

    if(!connect(pushButtonFightEnterNext,&CustomButton::clicked,this,&Battle::on_pushButtonFightEnterNext_clicked))
        abort();
    if(!connect(toolButtonFightQuit,&CustomButton::clicked,this,&Battle::on_toolButtonFightQuit_clicked))
        abort();
    if(!connect(pushButtonFightAttack,&CustomButton::clicked,this,&Battle::on_pushButtonFightAttack_clicked))
        abort();
    if(!connect(pushButtonFightMonster,&CustomButton::clicked,this,&Battle::on_pushButtonFightMonster_clicked))
        abort();
    if(!connect(pushButtonFightAttackConfirmed,&CustomButton::clicked,this,&Battle::on_pushButtonFightAttackConfirmed_clicked))
        abort();
    if(!connect(pushButtonFightReturn,&CustomButton::clicked,this,&Battle::on_pushButtonFightReturn_clicked))
        abort();
    if(!connect(listWidgetFightAttack,&QListWidget::itemSelectionChanged,this,&Battle::on_listWidgetFightAttack_itemSelectionChanged))
        abort();
    if(!connect(listWidgetFightAttack,&QListWidget::itemActivated,this,&Battle::on_listWidgetFightAttack_itemActivated))
        abort();

    newLanguage();
}

Battle::~Battle()
{
}

void Battle::stackedWidgetFightBottomBarsetCurrentWidget(int index)
{
    m_stackedWidgetFightBottomBarIndex=index;
    switch(m_stackedWidgetFightBottomBarIndex)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    default:
        abort();
        break;
    }

    labelFightEnter->setVisible(m_stackedWidgetFightBottomBarIndex==0);
    pushButtonFightEnterNext->setVisible(m_stackedWidgetFightBottomBarIndex==0);

    pushButtonFightAttack->setVisible(m_stackedWidgetFightBottomBarIndex==1);
    pushButtonFightBag->setVisible(m_stackedWidgetFightBottomBarIndex==1);
    pushButtonFightMonster->setVisible(m_stackedWidgetFightBottomBarIndex==1);
    toolButtonFightQuit->setVisible(m_stackedWidgetFightBottomBarIndex==1);

    pushButtonFightAttackConfirmed->setVisible(m_stackedWidgetFightBottomBarIndex==2);
    pushButtonFightReturn->setVisible(m_stackedWidgetFightBottomBarIndex==2);
    labelFightAttackDetails->setVisible(m_stackedWidgetFightBottomBarIndex==2);
    listWidgetFightAttackProxy->setVisible(m_stackedWidgetFightBottomBarIndex==2);
    listWidgetFightAttack->setVisible(m_stackedWidgetFightBottomBarIndex==2);
}

void Battle::newLanguage()
{
    //labelFightBottomLevel->setText(tr("Level 100"));
    //labelFightTopLevel->setText(tr("Level 100"));
    //labelFightEnter->setText("");
    //labelFightAttackDetails->setText("");

    pushButtonFightEnterNext->setText(tr("Next"));

    pushButtonFightAttack->setText(tr("Attack"));
    pushButtonFightBag->setText(tr("Bag"));
    pushButtonFightMonster->setText(tr("Monster"));
    toolButtonFightQuit->setText(tr("Escape"));

    pushButtonFightAttackConfirmed->setText(tr("Attack"));
    pushButtonFightReturn->setText(tr("Return"));
}

QRectF Battle::boundingRect() const
{
    return QRectF();
}

void Battle::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
//    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    /*if(w->height()>300)
        fontSize=w->height()/6;*/
    if(w->width()<900 || w->height()<600)
    {
        pushButtonFightEnterNext->setSize(148,61);
        pushButtonFightEnterNext->setPixelSize(23);

        pushButtonFightAttack->setSize(148,61);
        pushButtonFightAttack->setPixelSize(23);
        pushButtonFightBag->setSize(148,61);
        pushButtonFightBag->setPixelSize(23);
        pushButtonFightMonster->setSize(148,61);
        pushButtonFightMonster->setPixelSize(23);
        toolButtonFightQuit->setSize(148,61);
        toolButtonFightQuit->setPixelSize(23);

        pushButtonFightAttackConfirmed->setSize(148,61);
        pushButtonFightAttackConfirmed->setPixelSize(23);
        pushButtonFightReturn->setSize(148,61);
        pushButtonFightReturn->setPixelSize(23);
        multiItemH=75;
    }
    else {
        space=30;
        pushButtonFightEnterNext->setSize(223,92);
        pushButtonFightEnterNext->setPixelSize(35);

        pushButtonFightAttack->setSize(223,92);
        pushButtonFightAttack->setPixelSize(35);
        pushButtonFightBag->setSize(223,92);
        pushButtonFightBag->setPixelSize(35);
        pushButtonFightMonster->setSize(223,92);
        pushButtonFightMonster->setPixelSize(35);
        toolButtonFightQuit->setSize(223,92);
        toolButtonFightQuit->setPixelSize(35);

        pushButtonFightAttackConfirmed->setSize(223,92);
        pushButtonFightAttackConfirmed->setPixelSize(35);
        pushButtonFightReturn->setSize(223,92);
        pushButtonFightReturn->setPixelSize(35);
    }

    unsigned int minRatio=w->width()/220;
    unsigned int tempRatio=w->height()/150;
    if(minRatio>tempRatio)
        minRatio=tempRatio;
    unsigned int finalWidth=220*minRatio;
    unsigned int finalHeight=150*minRatio;
    unsigned int finalX=(w->width()-finalWidth)/2;
    unsigned int finalY=(w->height()-finalHeight)/2;
    if(labelFightBackground->pixmap().width()!=labelFightBackgroundPix.width()*(int)minRatio)
    {
        QPixmap t=labelFightBackgroundPix;
        t.scaled(labelFightBackgroundPix.width()*minRatio,labelFightBackgroundPix.height()*minRatio);
        labelFightBackground->setPixmap(t);
    }
    labelFightBackground->setPos(finalX,finalY);

    frameFightBottom->setPos(480,310);
    frameFightBottom->setSize(300,88);
    //labelFightBottomName->setSize(10,10);
    progressBarFightBottomHP->setSize(10,10+24+10);

    frameFightTop->setPos(70,70);
    frameFightTop->setSize(300,88);
    //labelFightTopName->setSize(10,10);
    progressBarFightTopHP->setSize(10,10+24+10);

    labelFightMonsterAttackBottom->setPos(finalX+60*minRatio,finalY+280*minRatio);
    //labelFightMonsterAttackBottom->setSize(160*minRatio,160*minRatio);
    labelFightMonsterAttackTop->setPos(finalX+510*minRatio,finalY+90*minRatio);
    //labelFightMonsterAttackTop->setSize(160*minRatio,160*minRatio);
    labelFightMonsterBottom->setPos(finalX+60*minRatio,finalY+280*minRatio);
    //labelFightMonsterBottom->setSize(160*minRatio,160*minRatio);
    labelFightMonsterTop->setPos(finalX+510*minRatio,finalY+90*minRatio);
    //labelFightMonsterTop->setSize(160*minRatio,160*minRatio);
    labelFightPlateformBottom->setPos(finalX+30*minRatio,finalY+350*minRatio);
    //labelFightPlateformBottom->setSize(230*minRatio,90*minRatio);
    labelFightPlateformTop->setPos(finalX+460*minRatio,finalY+170*minRatio);
    //labelFightPlateformTop->setSize(260*minRatio,90*minRatio);
    labelFightTrap->setPos(finalX+280*minRatio,finalY+230*minRatio);
    //labelFightTrap->setSize(160*minRatio,160*minRatio);

    stackedWidgetFightBottomBar->setSize(w->width(),160*minRatio);
    stackedWidgetFightBottomBar->setPos(0,w->height()-160*minRatio);
    switch(m_stackedWidgetFightBottomBarIndex)
    {
        case 0:
            labelFightEnter->setPos(space,stackedWidgetFightBottomBar->height()/2-24/2);
            pushButtonFightEnterNext->setPos(stackedWidgetFightBottomBar->width()-space-pushButtonFightEnterNext->width(),stackedWidgetFightBottomBar->height()/2-pushButtonFightEnterNext->height()/2);
        break;
        case 1:
            pushButtonFightAttack->setPos(stackedWidgetFightBottomBar->width()/2-space/2-pushButtonFightAttack->width(),stackedWidgetFightBottomBar->height()/2-space/2-pushButtonFightAttack->height());
            pushButtonFightBag->setPos(stackedWidgetFightBottomBar->width()/2+space/2+pushButtonFightBag->width(),stackedWidgetFightBottomBar->height()/2-space/2-pushButtonFightBag->height());
            pushButtonFightMonster->setPos(stackedWidgetFightBottomBar->width()/2-space/2-pushButtonFightMonster->width(),stackedWidgetFightBottomBar->height()/2+space/2+pushButtonFightMonster->height());
            toolButtonFightQuit->setPos(stackedWidgetFightBottomBar->width()/2+space/2+toolButtonFightQuit->width(),stackedWidgetFightBottomBar->height()/2+space/2+toolButtonFightQuit->height());
        break;
        case 2:
            pushButtonFightReturn->setPos(stackedWidgetFightBottomBar->width()-space-pushButtonFightReturn->width(),stackedWidgetFightBottomBar->height()-space-pushButtonFightReturn->height());
            pushButtonFightAttackConfirmed->setPos(stackedWidgetFightBottomBar->width()-space-pushButtonFightAttackConfirmed->width(),stackedWidgetFightBottomBar->height()-space-pushButtonFightReturn->height()-space-pushButtonFightAttackConfirmed->height());
            labelFightAttackDetails->setPos(stackedWidgetFightBottomBar->width()/2+space/2,space);
            listWidgetFightAttackProxy->setPos(space,space);
            listWidgetFightAttack->setFixedSize(stackedWidgetFightBottomBar->width()/2-space-space/2,stackedWidgetFightBottomBar->height()-space*2);
        break;
    }
}

void Battle::mousePressEventXY(const QPointF &p, bool &pressValidated, bool &callParentClass)
{
    pushButtonFightEnterNext->mousePressEventXY(p,pressValidated);

    pushButtonFightAttack->mousePressEventXY(p,pressValidated);
    pushButtonFightBag->mousePressEventXY(p,pressValidated);
    pushButtonFightMonster->mousePressEventXY(p,pressValidated);
    toolButtonFightQuit->mousePressEventXY(p,pressValidated);

    pushButtonFightAttackConfirmed->mousePressEventXY(p,pressValidated);
    pushButtonFightReturn->mousePressEventXY(p,pressValidated);
}

void Battle::mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    pushButtonFightEnterNext->mouseReleaseEventXY(p,pressValidated);

    pushButtonFightAttack->mouseReleaseEventXY(p,pressValidated);
    pushButtonFightBag->mouseReleaseEventXY(p,pressValidated);
    pushButtonFightMonster->mouseReleaseEventXY(p,pressValidated);
    toolButtonFightQuit->mouseReleaseEventXY(p,pressValidated);

    pushButtonFightAttackConfirmed->mouseReleaseEventXY(p,pressValidated);
    pushButtonFightReturn->mouseReleaseEventXY(p,pressValidated);
}


void Battle::on_monsterList_itemActivated(QListWidgetItem *item)
{
    if(monsterspos_items_graphical.find(item)==monsterspos_items_graphical.cend())
        return;
    uint8_t monsterPosition=monsterspos_items_graphical.at(item);
    if(inSelection)
    {
       objectSelection(true,monsterPosition);
       return;
    }
    const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
    unsigned int index=monsterPosition;
    const PlayerMonster &monster=playerMonster.at(index);
    const QtDatapackClientLoader::MonsterExtra &monsterExtraInfo=QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster);
    const QtDatapackClientLoader::QtMonsterExtra &QtmonsterExtraInfo=QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster);
    const Monster &monsterGeneralInfo=CommonDatapack::commonDatapack.monsters.at(monster.monster);
    const Monster::Stat &stat=CommonFightEngine::getStat(monsterGeneralInfo,monster.level);
    if(monsterGeneralInfo.type.empty())
        ui->monsterDetailsType->setText(QString());
    else
    {
        QStringList typeList;
        unsigned int sub_index=0;
        while(sub_index<monsterGeneralInfo.type.size())
        {
            const auto &typeSub=monsterGeneralInfo.type.at(sub_index);
            if(QtDatapackClientLoader::datapackLoader->typeExtra.find(typeSub)!=QtDatapackClientLoader::datapackLoader->typeExtra.cend())
            {
                const QtDatapackClientLoader::TypeExtra &typeExtra=QtDatapackClientLoader::datapackLoader->typeExtra.at(typeSub);
                if(!typeExtra.name.empty())
                {
                    /*if(typeExtra.color.isValid())
                        typeList.push_back("<span style=\"background-color:"+typeExtra.color.name()+";\">"+QString::fromStdString(typeExtra.name)+"</span>");
                    else*/
                        typeList.push_back(QString::fromStdString(typeExtra.name));
                }
            }
            sub_index++;
        }
        QStringList extraInfo;
        if(!typeList.isEmpty())
            extraInfo << tr("Type: %1").arg(typeList.join(QStringLiteral(", ")));
        if(Ultimate::ultimate.isUltimate())
        {
            if(!monsterExtraInfo.kind.empty())
                extraInfo << tr("Kind: %1").arg(QString::fromStdString(monsterExtraInfo.kind));
            if(!monsterExtraInfo.habitat.empty())
                extraInfo << tr("Habitat: %1").arg(QString::fromStdString(monsterExtraInfo.habitat));
        }
        ui->monsterDetailsType->setText(extraInfo.join(QStringLiteral("<br />")));
    }
    ui->monsterDetailsName->setText(QString::fromStdString(monsterExtraInfo.name));
    ui->monsterDetailsDescription->setText(QString::fromStdString(monsterExtraInfo.description));
    {
        QPixmap front=QtmonsterExtraInfo.front;
        front=front.scaled(160,160);
        ui->monsterDetailsImage->setPixmap(front);
    }
    {
        QPixmap catchPixmap;
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(monster.catched_with)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(monster.catched_with).image;
            ui->monsterDetailsCatched->setToolTip(tr("catched with %1")
                                                  .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra
                                                                              .at(monster.catched_with).name)));
        }
        else
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
            ui->monsterDetailsCatched->setToolTip(tr("catched with unknown item: %1").arg(monster.catched_with));
        }
        catchPixmap=catchPixmap.scaled(48,48);
        ui->monsterDetailsCatched->setPixmap(catchPixmap);
    }
    if(monster.gender==Gender_Male)
    {
        ui->monsterDetailsGender->setPixmap(QPixmap(":/CC/images/interface/male.png").scaled(48,48));
        ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Male")));
    }
    else if(monster.gender==Gender_Female)
    {
        ui->monsterDetailsGender->setPixmap(QPixmap(":/CC/images/interface/female.png").scaled(48,48));
        ui->monsterDetailsGender->setToolTip(tr("Gender: %1").arg(tr("Female")));
    }
    else
    {
        ui->monsterDetailsGender->setPixmap(QPixmap());
        ui->monsterDetailsGender->setToolTip(QString());
    }
    const uint32_t &maxXp=monsterGeneralInfo.level_to_xp.at(monster.level-1);
    ui->monsterDetailsLevel->setText(tr("Level %1").arg(monster.level));
    if(Ultimate::ultimate.isUltimate())
    {
        if(monster.hp>(stat.hp/2))
            ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#1A8307\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
        else if(monster.hp>(stat.hp/4))
            ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#B99C09\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
        else
            ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("<span style=\"color:#BF0303\">%1/%2</span>").arg(monster.hp).arg(stat.hp));
    }
    else
        ui->monsterDetailsStatHeal->setText(tr("Heal: ")+QString("%1/%2").arg(monster.hp).arg(stat.hp));
    ui->monsterDetailsStatSpeed->setText(tr("Speed: %1").arg(stat.speed));
    ui->monsterDetailsStatXp->setText(tr("Xp: %1/%2").arg(monster.remaining_xp).arg(maxXp));
    ui->monsterDetailsStatAttack->setText(tr("Attack: %1").arg(stat.attack));
    ui->monsterDetailsStatDefense->setText(tr("Defense: %1").arg(stat.defense));
    ui->monsterDetailsStatXpBar->setMaximum(maxXp);
    if(monster.remaining_xp<=maxXp)
        ui->monsterDetailsStatXpBar->setValue(monster.remaining_xp);
    else
        ui->monsterDetailsStatXpBar->setValue(maxXp);
    ui->monsterDetailsStatXpBar->repaint();
    ui->monsterDetailsStatAttackSpe->setText(tr("Special attack: %1").arg(stat.special_attack));
    ui->monsterDetailsStatDefenseSpe->setText(tr("Special defense: %1").arg(stat.special_defense));
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        ui->monsterDetailsStatSp->setVisible(true);
        ui->monsterDetailsStatSp->setText(tr("Skill point: %1").arg(monster.sp));
    }
    else
        ui->monsterDetailsStatSp->setVisible(false);
    //skill
    ui->monsterDetailsSkills->clear();
    {
        unsigned int indexSkill=0;
        while(indexSkill<monster.skills.size())
        {
            const PlayerMonster::PlayerSkill &monsterSkill=monster.skills.at(indexSkill);
            QListWidgetItem *item;
            if(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.find(monsterSkill.skill)==
                    QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.cend())
                item=new QListWidgetItem(tr("Unknown skill"));
            else
            {
                if(monsterSkill.level>1)
                    item=new QListWidgetItem(tr("%1 at level %2").arg(
                                                 QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra
                                                                        .at(monsterSkill.skill).name)).arg(monsterSkill.level));
                else
                    item=new QListWidgetItem(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(monsterSkill.skill).name));
                const Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(monsterSkill.skill);
                item->setText(item->text()+QStringLiteral(" (%1/%2)")
                        .arg(monsterSkill.endurance)
                        .arg(skill.level.at(monsterSkill.level-1).endurance)
                        );
                if(skill.type!=255)
                    if(QtDatapackClientLoader::datapackLoader->typeExtra.find(skill.type)!=QtDatapackClientLoader::datapackLoader->typeExtra.cend())
                        if(!QtDatapackClientLoader::datapackLoader->typeExtra.at(skill.type).name.empty())
                            item->setText(item->text()+", "+tr("Type: %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->typeExtra
                                                                                                      .at(skill.type).name)));
                item->setText(item->text()+"\n"+QString::fromStdString(
                                  QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(monsterSkill.skill).description));
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(monsterSkill.skill).description));
            }
            ui->monsterDetailsSkills->addItem(item);
            indexSkill++;
        }
    }
    //do the buff
    {
        ui->monsterDetailsBuffs->clear();
        unsigned int index=0;
        while(index<monster.buffs.size())
        {
            const PlayerBuff &buffEffect=monster.buffs.at(index);
            QListWidgetItem *item=new QListWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(buffEffect.buff)==QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
            {
                item->setToolTip(tr("Unknown buff"));
                item->setIcon(QIcon(":/CC/images/interface/buff.png"));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra.at(buffEffect.buff).icon);
                if(buffEffect.level<=1)
                    item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name));
                else
                    item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(
                          QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name)).arg(buffEffect.level));
                item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra
                                                                             .at(buffEffect.buff).description));
            }
            ui->monsterDetailsBuffs->addItem(item);
            index++;
        }
    }

    ui->stackedWidget->setCurrentWidget(ui->page_monsterdetails);
}

//need correct match, else tp to die can failed and do mistake
bool Battle::check_monsters()
{
    unsigned int index=0;
    unsigned int sub_size;
    unsigned int sub_index;
    while(index<client->player_informations.playerMonster.size())
    {
        const PlayerMonster &monster=client->player_informations.playerMonster.at(index);
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
        {
            error(QStringLiteral("the monster %1 is not into monster list").arg(monster.monster).toStdString());
            return false;
        }
        const Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster);
        if(monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            error(QStringLiteral("the level %1 is greater than max level").arg(monster.level).toStdString());
            return false;
        }
        Monster::Stat stat=ClientFightEngine::getStat(monsterDef,monster.level);
        if(monster.hp>stat.hp)
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(stat.hp).arg(monster.level).toStdString());
            return false;
        }
        if(monster.remaining_xp>monsterDef.level_to_xp.at(monster.level-1))
        {
            error(QStringLiteral("the hp %1 is greater than max hp for the level %2").arg(monster.remaining_xp).arg(monster.level).toStdString());
            return false;
        }
        sub_size=static_cast<uint32_t>(monster.buffs.size());
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(monster.buffs.at(sub_index).buff)==CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
            {
                error(QStringLiteral("the buff %1 is not into the buff list").arg(monster.buffs.at(sub_index).buff).toStdString());
                return false;
            }
            if(monster.buffs.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.at(monster.buffs.at(sub_index).buff).level.size())
            {
                error(QStringLiteral("the buff have not the level %1 is not into the buff list").arg(monster.buffs.at(sub_index).level).toStdString());
                return false;
            }
            sub_index++;
        }
        sub_size=static_cast<uint32_t>(monster.skills.size());
        sub_index=0;
        while(sub_index<sub_size)
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(monster.skills.at(sub_index).skill)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
            {
                error(QStringLiteral("the skill %1 is not into the skill list").arg(monster.skills.at(sub_index).skill).toStdString());
                return false;
            }
            if(monster.skills.at(sub_index).level>CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(monster.skills.at(sub_index).skill).level.size())
            {
                error(QStringLiteral("the skill have not the level %1 is not into the skill list").arg(monster.skills.at(sub_index).level).toStdString());
                return false;
            }
            sub_index++;
        }
        index++;
    }
    //fightEngine.setPlayerMonster(client->player_informations.playerMonster);
    return true;
}

void Battle::load_monsters()
{
    const std::vector<PlayerMonster> &playerMonsters=fightEngine.getPlayerMonster();
    ui->monsterList->clear();
    monsterspos_items_graphical.clear();
    if(playerMonsters.empty())
        return;
    const uint8_t &currentMonsterPosition=fightEngine.getCurrentSelectedMonsterNumber();
    unsigned int index=0;
    while(index<playerMonsters.size())
    {
        const PlayerMonster &monster=playerMonsters.at(index);
        if(inSelection)
        {
            if(waitedObjectType==ObjectType_MonsterToFight && index==currentMonsterPosition)
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
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(objectInUsing.back())==
                            CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend())
                    {
                        index++;
                        continue;
                    }
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(objectInUsing.back()).find(monster.monster)==
                            CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.at(objectInUsing.back()).cend())
                    {
                        index++;
                        continue;
                    }
                break;
                case ObjectType_ItemLearnOnMonster:
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.find(objectInUsing.back())==
                            CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.cend())
                    {
                        index++;
                        continue;
                    }
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.at(objectInUsing.back()).find(monster.monster)==
                            CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.at(objectInUsing.back()).cend())
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
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(monster.monster)!=
                CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
        {
            Monster::Stat stat=CatchChallenger::ClientFightEngine::getStat(
                        CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster),monster.level);

            QListWidgetItem *item=new QListWidgetItem();
            item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).description));
            if(!QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).thumb.isNull())
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).thumb);
            else
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).front);

            if(waitedObjectType==ObjectType_MonsterToLearn && inSelection)
            {
                QHash<uint32_t,uint8_t> skillToDisplay;
                unsigned int sub_index=0;
                while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size())
                {
                    Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
                    if(learn.learnAtLevel<=monster.level)
                    {
                        unsigned int sub_index2=0;
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
                                (sub_index2<monster.skills.size() && (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
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
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name))
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("No skill to learn"))
                            );
                else
                    item->setText(tr("%1, level: %2\nHP: %3/%4\n%5")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name))
                            .arg(monster.level)
                            .arg(monster.hp)
                            .arg(stat.hp)
                            .arg(tr("%n skill(s) to learn","",skillToDisplay.size()))
                            );
            }
            else
            {
                item->setText(tr("%1, level: %2\nHP: %3/%4")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name))
                        .arg(monster.level)
                        .arg(monster.hp)
                        .arg(stat.hp)
                        );
            }
            if(monster.hp==0)
                item->setBackgroundColor(QColor(255,220,220,255));
            else if(index==currentMonsterPosition)
            {
                if(!fightEngine.isInFight())
                    item->setBackgroundColor(QColor(200,255,255,255));
            }
            ui->monsterList->addItem(item);
            monsterspos_items_graphical[item]=static_cast<uint8_t>(index);
        }
        else
        {
            error(QStringLiteral("Unknown monster: %1").arg(monster.monster).toStdString());
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

void Battle::wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y)
{
    if(!fightCollision(map,x,y))
        return;
    prepareFight();
    battleType=BattleType_Wild;
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at wildFightCollision()");
        return;
    }
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("A other %1 is in front of you!")
                                 .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name)));
    ui->pushButtonFightEnterNext->setVisible(false);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    resetPosition(true);
    moveFightMonsterBoth();
}

void Battle::prepareFight()
{
    escape=false;
    escapeSuccess=false;
}

void Battle::botFight(const uint16_t &fightId)
{
    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
    {
        emit error("fight id not found at collision");
        return;
    }
    botFightList.push_back(fightId);
    if(botFightList.size()>1 || !battleInformationsList.empty())
        return;
    botFightFull(fightId);
}

void Battle::botFightFull(const uint16_t &fightId)
{
    this->fightId=fightId;
    botFightTimer.start();
}

void Battle::botFightFullDiffered()
{
    prepareFight();
    ui->frameFightTop->setVisible(false);
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->botFightsExtra.at(fightId).start));
    ui->pushButtonFightEnterNext->setVisible(false);
    std::vector<PlayerMonster> botFightMonstersTransformed;
    const std::vector<BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId).monsters;
    unsigned int index=0;
    while(index<monsters.size())
    {
        const BotFight::BotFightMonster &botFightMonster=monsters.at(index);
        botFightMonstersTransformed.push_back(FacilityLib::botFightMonsterToPlayerMonster(
                                           monsters.at(index),
                                           ClientFightEngine::getStat(
                                               CatchChallenger::CommonDatapack::commonDatapack.monsters.at(botFightMonster.id),
                                               monsters.at(index).level
                                               )
                                           ));
        index++;
    }
    fightEngine.setBotMonster(botFightMonstersTransformed,fightId);
    init_environement_display(mapController->getMapObject(),mapController->getX(),mapController->getY());
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    ui->pushButtonFightEnterNext->setVisible(false);
    battleType=BattleType_Bot;
    QPixmap botImage;
    if(actualBot.properties.find("skin")!=actualBot.properties.cend())
        botImage=getFrontSkin(actualBot.properties.at("skin"));
    else
        botImage=QPixmap(QStringLiteral(":/CC/images/player_default/front.png"));
    ui->labelFightMonsterTop->setPixmap(botImage.scaled(160,160));
    qDebug() << QStringLiteral("The bot %1 is a front of you").arg(fightId);
    battleStep=BattleStep_Presentation;
    moveType=MoveType_Enter;
    resetPosition(true);
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
}

bool Battle::fightCollision(CatchChallenger::Map_client *map, const uint8_t &x, const uint8_t &y)
{
    if(!fightEngine.isInFight())
    {
        emit error("is in fight but without monster");
        return false;
    }
    init_environement_display(map,x,y);
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    init_current_monster_display();
    init_other_monster_display();
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster==NULL)
    {
        emit error("NULL pointer for other monster at fightCollision()");
        return false;
    }
    qDebug() << QStringLiteral("You are in front of monster id: %1 level: %2 with hp: %3").arg(otherMonster->monster).arg(otherMonster->level).arg(otherMonster->hp);
    return true;
}

void Battle::init_environement_display(Map_client *map, const uint8_t &x, const uint8_t &y)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    Q_UNUSED(x);
    Q_UNUSED(y);
    //map not located
    if(map==NULL)
    {
        ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png")).scaled(800,440));
        ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png")).scaled(800,440));
        ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-front.png")).scaled(260,90));
        ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-background.png")).scaled(230,90));
        return;
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*map,x,y);
    unsigned int index=0;
    while(index<monstersCollisionValue.walkOn.size())
    {
        const CatchChallenger::MonstersCollision &monstersCollision=
                CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
        const CatchChallenger::MonstersCollisionTemp &monstersCollisionTemp=
                CatchChallenger::CommonDatapack::commonDatapack.monstersCollisionTemp.at(monstersCollisionValue.walkOn.at(index));
        if(monstersCollision.item==0 || playerInformations.items.find(monstersCollision.item)!=playerInformations.items.cend())
        {
            if(!monstersCollisionTemp.background.empty())
            {
                const QString &baseSearch=QString::fromStdString(client->datapackPathBase())+DATAPACK_BASE_PATH_MAPBASE+
                        QString::fromStdString(monstersCollisionTemp.background);
                if(QFile(baseSearch+"/background.png").exists())
                    ui->labelFightBackground->setPixmap(QPixmap(baseSearch+QStringLiteral("/background.png")).scaled(800,440));
                else if(QFile(baseSearch+"/background.jpg").exists() &&
                        (supportedImageFormats.find("jpeg")!=supportedImageFormats.cend() ||
                                                                         supportedImageFormats.find("jpg")!=supportedImageFormats.cend()))
                    ui->labelFightBackground->setPixmap(QPixmap(baseSearch+QStringLiteral("/background.jpg")).scaled(800,440));
                else
                    ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png")).scaled(800,440));

                if(QFile(baseSearch+"/foreground.png").exists())
                    ui->labelFightForeground->setPixmap(QPixmap(baseSearch+QStringLiteral("/foreground.png")).scaled(800,440));
                else if(QFile(baseSearch+"/foreground.gif").exists())
                    ui->labelFightForeground->setPixmap(QPixmap(baseSearch+QStringLiteral("/foreground.gif")).scaled(800,440));
                else
                    ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png")).scaled(800,440));

                if(QFile(baseSearch+"/plateform-front.png").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-front.png")).scaled(260,90));
                else if(QFile(baseSearch+"/plateform-front.gif").exists())
                    ui->labelFightPlateformTop->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-front.gif")).scaled(260,90));
                else
                    ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-front.png")).scaled(260,90));

                if(QFile(baseSearch+"/plateform-background.png").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-background.png")).scaled(230,90));
                else if(QFile(baseSearch+"/plateform-background.gif").exists())
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(baseSearch+QStringLiteral("/plateform-background.gif")).scaled(230,90));
                else
                    ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-background.png")).scaled(230,90));
            }
            else
            {
                ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png")).scaled(800,440));
                ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png")).scaled(800,440));
                ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-front.png")).scaled(260,90));
                ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-background.png")).scaled(230,90));
            }
            return;
        }
        index++;
    }
    ui->labelFightBackground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/background.png")).scaled(800,440));
    ui->labelFightForeground->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/foreground.png")).scaled(800,440));
    ui->labelFightPlateformTop->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-front.png")).scaled(260,90));
    ui->labelFightPlateformBottom->setPixmap(QPixmap(QStringLiteral(":/CC/images/interface/fight/plateform-background.png")).scaled(230,90));
}

void Battle::init_other_monster_display()
{
    updateOtherMonsterInformation();
}

void Battle::init_current_monster_display(PlayerMonster *fightMonster)
{
    if(fightMonster==NULL)
        fightMonster=fightEngine.getCurrentMonster();
    if(fightMonster!=NULL)
    {
        //current monster
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->stackedWidget->setCurrentWidget(ui->page_battle);
        ui->pushButtonFightEnterNext->setVisible(true);
        ui->frameFightBottom->setVisible(false);
        ui->labelFightBottomName->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(fightMonster->monster).name));
        ui->labelFightBottomLevel->setText(tr("Level %1").arg(fightMonster->level));
        Monster::Stat fightStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(fightMonster->monster),fightMonster->level);
        ui->progressBarFightBottomHP->setMaximum(fightStat.hp);
        ui->progressBarFightBottomHP->setValue(fightMonster->hp);
        ui->progressBarFightBottomHP->repaint();
        ui->labelFightBottomHP->setText(QStringLiteral("%1/%2").arg(fightMonster->hp).arg(fightStat.hp));
        const Monster &monsterGenericInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(fightMonster->monster);
        int level_to_xp=monsterGenericInfo.level_to_xp.at(fightMonster->level-1);
        ui->progressBarFightBottomExp->setMaximum(level_to_xp);
        ui->progressBarFightBottomExp->setValue(fightMonster->remaining_xp);
        ui->progressBarFightBottomExp->repaint();
        //do the buff
        {
            buffToGraphicalItemBottom.clear();
            ui->bottomBuff->clear();
            unsigned int index=0;
            while(index<fightMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=fightMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(buffEffect.buff)==
                        QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/CC/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra.at(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name));
                    else
                        item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(
                                                                                             buffEffect.buff).name)).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(
                                                                                     buffEffect.buff).description));
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

void Battle::on_pushButtonFightEnterNext_clicked()
{
    ui->pushButtonFightEnterNext->setVisible(false);
    switch(battleStep)
    {
        case BattleStep_Presentation:
            if(fightEngine.isInFightWithWild())
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
            stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
            PlayerMonster *monster=fightEngine.getCurrentMonster();
            if(monster!=NULL)
                ui->labelFightEnter->setText(tr("Protect me %1!").arg(QString::fromStdString(
                                                                          QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster).name)));
            else
            {
                qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                ui->labelFightEnter->setText(tr("Protect me %1!").arg("(Unknow monster)"));
            }
        break;
    }
}

void Battle::moveFightMonsterBottom()
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
                PlayerMonster *monster=fightEngine.getCurrentMonster();
                if(monster==NULL)
                {
                    newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                             "NULL pointer at updateCurrentMonsterInformation()");
                    return;
                }
                ui->labelFightEnter->setText(tr("Go %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster).name)));
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
            fightEngine.dropKOCurrentMonster();
            if(fightEngine.haveAnotherMonsterOnThePlayerToFight())
            {
                if(fightEngine.isInFight())
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
                    displayText(tr("You win!").toStdString());
                    doNextActionStep=DoNextActionStep_Win;
                }
            }
            else
            {
                #ifdef DEBUG_CLIENT_BATTLE
                qDebug() << "You lose";
                #endif
                displayText(tr("You lose!").toStdString());
                doNextActionStep=DoNextActionStep_Loose;
            }
        }
    }
}

void Battle::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(fightEngine.currentMonsterIsKO() && !fightEngine.haveAnotherMonsterOnThePlayerToFight())//then is dead, is teleported to the last rescue point
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

void Battle::updateCurrentMonsterInformationXp()
{
    PlayerMonster *monster=fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    ui->progressBarFightBottomExp->setValue(monster->remaining_xp);
    ui->progressBarFightBottomExp->repaint();
    ui->labelFightBottomLevel->setText(tr("Level %1").arg(monster->level));
    const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster->monster);
    uint32_t maxXp=monsterInformations.level_to_xp.at(monster->level-1);
    ui->progressBarFightBottomExp->setMaximum(maxXp);
}

void Battle::updateCurrentMonsterInformation()
{
    if(!fightEngine.getAbleToFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Try update the monster when have not any ready monster");
        return;
    }
    PlayerMonster *monster=fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    qDebug() << "Now visible monster have hp:" << monster->hp;
    #endif
    QPoint p;
    p.setX(60);
    p.setY(280);
    ui->labelFightMonsterBottom->move(p);
    ui->labelFightMonsterBottom->setPixmap(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster->monster).back.scaled(160,160));
    ui->frameFightBottom->setVisible(true);
    ui->frameFightBottom->show();
    currentMonsterLevel=monster->level;
    updateAttackList();
}

void Battle::updateAttackList()
{
    if(!fightEngine.getAbleToFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Try update the monster when have not any ready monster");
        return;
    }
    PlayerMonster *monster=fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    //list the attack
    fight_attacks_graphical.clear();
    ui->listWidgetFightAttack->clear();
    unsigned int index=0;
    useTheRescueSkill=true;
    while(index<monster->skills.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const PlayerMonster::PlayerSkill &skill=monster->skills.at(index);
        if(skill.level>1)
            item->setText(QStringLiteral("%1, level %2 (remaining endurance: %3)")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(skill.skill).name))
                    .arg(skill.level)
                    .arg(skill.endurance)
                    );
        else
            item->setText(QStringLiteral("%1 (remaining endurance: %2)")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(skill.skill).name))
                    .arg(skill.endurance)
                    );
        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(skill.skill).description));
        item->setData(99,skill.endurance);
        item->setData(98,skill.skill);
        if(skill.endurance>0)
            useTheRescueSkill=false;
        fight_attacks_graphical[item]=skill.skill;
        ui->listWidgetFightAttack->addItem(item);
        index++;
    }
    if(useTheRescueSkill && CommonDatapack::commonDatapack.monsterSkills.find(0)!=CommonDatapack::commonDatapack.monsterSkills.cend())
        if(!CommonDatapack::commonDatapack.monsterSkills.at(0).level.empty())
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QStringLiteral("Rescue skill: %1").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(0).name)));
            item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(0).description));
            item->setData(99,0);
            fight_attacks_graphical[item]=0;
            ui->listWidgetFightAttack->addItem(item);
        }
    on_listWidgetFightAttack_itemSelectionChanged();
}

void Battle::moveFightMonsterTop()
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
            fightEngine.dropKOOtherMonster();
            if(fightEngine.isInFight())
            {
                if(fightEngine.isInBattle() && !fightEngine.haveBattleOtherMonster())
                {
                    ui->pushButtonFightEnterNext->setVisible(false);
                    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                    ui->labelFightEnter->setText(tr("In waiting of other monster selection"));
                    return;
                }
                ui->pushButtonFightEnterNext->setVisible(false);
                init_other_monster_display();
                ui->frameFightTop->setVisible(false);
                updateOtherMonsterInformation();
                stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
                PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
                if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(otherMonster->monster)!=
                        QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
                    ui->labelFightEnter->setText(tr("The other player call %1").arg(
                                                     QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name)));
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
                    PlayerMonster *currentMonster=fightEngine.getCurrentMonster();
                    if(currentMonster!=NULL)
                    {
                        ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
                        ui->progressBarFightBottomExp->repaint();
                        ui->labelFightBottomLevel->setText(tr("Level %1").arg(currentMonster->level));
                    }
                    displayText(tr("You win!").toStdString());
                }
                else
                {
                    if(escapeSuccess)
                        displayText(tr("Your escape is successful").toStdString());
                    else
                        displayText(tr("Your escape have failed but you win").toStdString());
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

void Battle::moveFightMonsterBoth()
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
                PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
                if(otherMonster!=NULL)
                    text+=tr("The other player call %1!").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                                                                                         otherMonster->monster).name));
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("The other player call %1!").arg("(Unknow monster)");
                }
                text+="<br />";
                PublicPlayerMonster *monster=fightEngine.getCurrentMonster();
                if(monster!=NULL)
                    text+=tr("You call %1!").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster).name));
                else
                {
                    qDebug() << "on_pushButtonFightEnterNext_clicked(): NULL pointer for current monster";
                    text+=tr("You call %1!").arg("(Unknow monster)");
                }
                stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
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

void Battle::updateOtherMonsterInformation()
{
    if(!fightEngine.isInFight())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"updateOtherMonsterInformation() but have not other monter");
        return;
    }
    QPoint p;
    p.setX(510);
    p.setY(90);
    ui->labelFightMonsterTop->move(p);
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
    {
        ui->labelFightMonsterTop->setPixmap(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(otherMonster->monster).front.scaled(160,160));
        //other monster
        ui->frameFightTop->setVisible(true);
        ui->frameFightTop->show();
        ui->labelFightTopName->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name));
        ui->labelFightTopLevel->setText(tr("Level %1").arg(otherMonster->level));
        Monster::Stat otherStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(otherMonster->monster),otherMonster->level);
        ui->progressBarFightTopHP->setMaximum(otherStat.hp);
        ui->progressBarFightTopHP->setValue(otherMonster->hp);
        ui->progressBarFightTopHP->repaint();
        //do the buff
        {
            buffToGraphicalItemTop.clear();
            ui->topBuff->clear();
            unsigned int index=0;
            while(index<otherMonster->buffs.size())
            {
                const PlayerBuff &buffEffect=otherMonster->buffs.at(index);
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(buffEffect.buff)==
                        QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
                {
                    item->setToolTip(tr("Unknown buff"));
                    item->setIcon(QIcon(":/CC/images/interface/buff.png"));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra.at(buffEffect.buff).icon);
                    if(buffEffect.level<=1)
                        item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name));
                    else
                        item->setToolTip(tr("%1 at level %2").arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(
                                                                                             buffEffect.buff).name)).arg(buffEffect.level));
                    item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff)
                                                                                 .description));
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
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Unable to load the other monster");
        return;
    }
}

void Battle::on_toolButtonFightQuit_clicked()
{
    if(!fightEngine.canDoFightAction())
    {
        QMessageBox::warning(this,"Communication problem",tr("Sorry but the client wait more data from the server to do it"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=true;
    qDebug() << "Battle::on_toolButtonFightQuit_clicked(): fight engine tryEscape()";
    if(fightEngine.tryEscape())
        escapeSuccess=true;
    else
        escapeSuccess=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    doNextAction();
}

void Battle::on_pushButtonFightAttack_clicked()
{
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageAttack);
}

void Battle::on_pushButtonFightMonster_clicked()
{
    selectObject(ObjectType_MonsterToFight);
}

void Battle::on_pushButtonFightAttackConfirmed_clicked()
{
    if(!fightEngine.canDoFightAction())
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
    const uint8_t skillEndurance=static_cast<uint8_t>(itemsList.first()->data(99).toUInt());
    const uint16_t skillUsed=fight_attacks_graphical.at(itemsList.first());
    if(skillEndurance<=0 && !useTheRescueSkill)
    {
        QMessageBox::warning(this,tr("No endurance"),tr("You have no more endurance to use this skill"));
        return;
    }
    doNextActionStep=DoNextActionStep_Start;
    escape=false;
    haveDisplayCurrentAttackSuccess=false;
    haveDisplayOtherAttackSuccess=false;
    PlayerMonster *monster=fightEngine.getCurrentMonster();
    if(monster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"NULL pointer at updateCurrentMonsterInformation()");
        return;
    }
    if(useTheRescueSkill)
        fightEngine.useSkill(0);
    else
        fightEngine.useSkill(skillUsed);
    updateAttackList();
    displayAttackProgression=0;
    attack_quantity_changed=0;
    if(battleType!=BattleType_OtherPlayer)
        doNextAction();
    else
    {
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("In waiting of the other player action"));
        ui->pushButtonFightEnterNext->hide();
    }
}

void Battle::on_pushButtonFightReturn_clicked()
{
    ui->listWidgetFightAttack->reset();
    on_listWidgetFightAttack_itemSelectionChanged();
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
}

void Battle::on_listWidgetFightAttack_itemSelectionChanged()
{
    QList<QListWidgetItem *> itemsList=ui->listWidgetFightAttack->selectedItems();
    if(itemsList.size()!=1)
    {
        ui->labelFightAttackDetails->setText(tr("Select an attack"));
        return;
    }
    uint16_t skillId=fight_attacks_graphical.at(itemsList.first());
    ui->labelFightAttackDetails->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(skillId).description));
}

void Battle::checkEvolution()
{
    PlayerMonster *monster=fightEngine.evolutionByLevelUp();
    if(monster!=NULL)
    {
        const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(monster->monster);
        const QtDatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster->monster);
        unsigned int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Level && evolution.data.level==monster->level)
            {
                monsterEvolutionPostion=fightEngine.getPlayerMonsterPosition(monster);
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.at(evolution.evolveTo);
                const QtDatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                            evolution.evolveTo);
                //create animation widget
                if(animationWidget!=NULL)
                    delete animationWidget;
                if(qQuickViewContainer!=NULL)
                    delete qQuickViewContainer;
                animationWidget=new QQuickView();
                qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
                qQuickViewContainer->setMinimumSize(QSize(width(),height()));
                qQuickViewContainer->setMaximumSize(QSize(width(),height()));
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
                if(!connect(evolutionControl,&EvolutionControl::cancel,this,&Battle::evolutionCanceled,Qt::QueuedConnection))
                    abort();
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(true));
                animationWidget->rootContext()->setContextProperty("itemEvolution",QString());
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const QString datapackQmlFile=QString::fromStdString(client->datapackPathBase())+"qml/evolution-animation.qml";
                if(QFile(datapackQmlFile).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
                return;
            }
            index++;
        }
    }
    while(!tradeEvolutionMonsters.empty())
    {
        const uint8_t monsterPosition=tradeEvolutionMonsters.at(0);
        PlayerMonster * const playerMonster=fightEngine.monsterByPosition(monsterPosition);
        if(playerMonster==NULL)
        {
            tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
            continue;
        }
        const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.at(playerMonster->monster);
        const QtDatapackClientLoader::MonsterExtra &monsterInformationsExtra=QtDatapackClientLoader::datapackLoader->monsterExtra.at(playerMonster->monster);
        unsigned int index=0;
        while(index<monsterInformations.evolutions.size())
        {
            const Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
            if(evolution.type==Monster::EvolutionType_Trade)
            {
                monsterEvolutionPostion=monsterPosition;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.at(evolution.evolveTo);
                const QtDatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=QtDatapackClientLoader::datapackLoader->monsterExtra
                        .at(evolution.evolveTo);
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
                if(!connect(evolutionControl,&EvolutionControl::cancel,this,&Battle::evolutionCanceled,Qt::QueuedConnection))
                    abort();
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(true));
                animationWidget->rootContext()->setContextProperty("itemEvolution",QString());
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const QString datapackQmlFile=QString::fromStdString(client->datapackPathBase())+"qml/evolution-animation.qml";
                if(QFile(datapackQmlFile).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
                tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
                return;
            }
            index++;
        }
        tradeEvolutionMonsters.erase(tradeEvolutionMonsters.begin());
    }
}

void Battle::finalFightTextQuit()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    checkEvolution();
}

void Battle::loose()
{
    qDebug() << "loose()";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString()
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString()
                       );
            return;
        }
    #endif
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
        return;
    fightEngine.healAllMonsters();
    fightEngine.fightFinished();
    mapController->unblock();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(botFightList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.erase(botFightList.cbegin());
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.erase(battleInformationsList.cbegin());
        break;
        default:
        break;
    }
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
    checkEvolution();
}

void Battle::win()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString());
            return;
        }
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString());
            return;
        }
    #endif
    fightEngine.fightFinished();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (win end && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString());
            return;
        }
    #endif
    mapController->unblock();
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
                if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightId)==CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                {
                    emit error("fight id not found at collision");
                    return;
                }
                addCash(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId).cash);
                mapController->addBeatenBotFight(fightId);
            }
            if(botFightList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.erase(botFightList.cbegin());
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.erase(battleInformationsList.cbegin());
        break;
        default:
        break;
    }
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        const uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
    checkEvolution();
}

void Battle::pageChanged()
{
    if(ui->stackedWidget->currentWidget()==ui->page_map)
        mapController->setFocus();
    if(ui->stackedWidget->currentWidget()==ui->page_plants)
        load_plant_inventory();
}

void Battle::displayExperienceGain()
{
    PlayerMonster * currentMonster=fightEngine.getCurrentMonster();
    if(currentMonster==NULL)
    {
        mLastGivenXP=0;
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: unable to get the current monster");
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
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part0: mLastGivenXP is negative").toStdString());
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("par0: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                     .toStdString());
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        #endif
        if(currentMonsterLevel==currentMonster->level && (uint32_t)ui->progressBarFightBottomExp->value()>currentMonster->remaining_xp)
        {
            message(QStringLiteral("part0: displayed xp greater than the real xp: %1>%2, auto fix")
                    .arg(ui->progressBarFightBottomExp->value()).arg(currentMonster->remaining_xp).toStdString());
            ui->progressBarFightBottomExp->setValue(currentMonster->remaining_xp);
            mLastGivenXP=0;
            doNextAction();
            return;
        }
        doNextAction();
        return;
    }

    //if start, display text
    if(displayAttackProgression==0)
    {
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        ui->labelFightEnter->setText(tr("You %1 gain %2 of experience").arg(
                                         QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(currentMonster->monster).name))
                                     .arg(mLastGivenXP));
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part1: mLastGivenXP is negative").toStdString());
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part1: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level).toStdString());
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    uint32_t xp_to_change;
    if(currentMonsterLevel<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
    {
        xp_to_change=ui->progressBarFightBottomExp->maximum()/200;//0.5%
        if(xp_to_change==0)
            xp_to_change=1;
        if(xp_to_change>mLastGivenXP)
            xp_to_change=mLastGivenXP;
    }
    else
    {
        xp_to_change=0;
        mLastGivenXP=0;
        doNextAction();
        return;
    }

    uint32_t maxXp=ui->progressBarFightBottomExp->maximum();
    if(((uint32_t)ui->progressBarFightBottomExp->value()+xp_to_change)>=(uint32_t)maxXp)
    {
        xp_to_change=maxXp-ui->progressBarFightBottomExp->value();
        if(xp_to_change>mLastGivenXP)
            xp_to_change=mLastGivenXP;
        if(mLastGivenXP>xp_to_change)
            mLastGivenXP-=xp_to_change;
        else
            mLastGivenXP=0;

        const Monster::Stat &oldStat=fightEngine.getStat(CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonsterLevel);
        const Monster::Stat &newStat=fightEngine.getStat(CommonDatapack::commonDatapack.monsters.at(currentMonster->monster),currentMonsterLevel+1);
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
        ui->progressBarFightBottomExp->setMaximum(CommonDatapack::commonDatapack.monsters.at(currentMonster->monster).level_to_xp.at(currentMonsterLevel-1));
        ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->maximum());
        ui->progressBarFightBottomExp->repaint();
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mLastGivenXP>4294000000)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part2: mLastGivenXP is negative").toStdString());
            doNextAction();
            return;
        }
        if(currentMonsterLevel>currentMonster->level)
        {
            newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                     QStringLiteral("part2: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                     .toStdString());
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
        if(currentMonsterLevel<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
        {
            ui->progressBarFightBottomExp->setValue(ui->progressBarFightBottomExp->value()+xp_to_change);
            ui->progressBarFightBottomExp->repaint();
        }
        else
        {
            mLastGivenXP=0;
            doNextAction();
            return;
        }
    }
    if(mLastGivenXP>xp_to_change)
        mLastGivenXP-=xp_to_change;
    else
        mLastGivenXP=0;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(mLastGivenXP>4294000000)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part3: mLastGivenXP is negative").toStdString());
        doNextAction();
        return;
    }
    if(currentMonsterLevel>currentMonster->level)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),
                 QStringLiteral("part3: displayed level greater than the real level: %1>%2").arg(currentMonsterLevel).arg(currentMonster->level)
                 .toStdString());
        mLastGivenXP=0;
        doNextAction();
        return;
    }
    #endif

    displayExpTimer.start();
    displayAttackProgression++;
}

void Battle::displayTrap()
{
    PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
    PublicPlayerMonster * currentMonster=fightEngine.getCurrentMonster();
    if(otherMonster==NULL)
    {
        error("displayAttack(): crash: unable to get the other monster to use trap");
        doNextAction();
        return;
    }
    if(currentMonster==NULL)
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"displayAttack(): crash: unable to get the current monster");
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
        QString skillAnimation=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_ITEM;
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
        ui->labelFightEnter->setText(QStringLiteral("Try catch the wild %1")
                      .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(otherMonster->monster).name)));
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
        displayTrapProgression=1;
        ui->labelFightTrap->show();
    }
    uint32_t animationTime;
    if(displayTrapProgression==1)
        animationTime=1500;
    else
        animationTime=2000;
    if(displayTrapProgression==1 && (uint32_t)updateTrapTime.elapsed()<animationTime)
        ui->labelFightTrap->move(
                    ui->labelFightMonsterBottom->pos().x()+(ui->labelFightMonsterTop->pos().x()-ui->labelFightMonsterBottom->pos().x())*updateTrapTime.elapsed()/animationTime,
                    ui->labelFightMonsterBottom->pos().y()-(ui->labelFightMonsterBottom->pos().y()-ui->labelFightMonsterTop->pos().y())*updateTrapTime.elapsed()/animationTime
                    );
    else
        ui->labelFightTrap->move(ui->labelFightMonsterTop->pos().x(),ui->labelFightMonsterTop->pos().y());
    if(!fightEngine.playerMonster_catchInProgress.empty())
        return;
    if((uint32_t)updateTrapTime.elapsed()>animationTime)
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
            QString skillAnimation=QString::fromStdString(QtDatapackClientLoader::datapackLoader->getDatapackPath())+DATAPACK_BASE_PATH_ITEM;
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
                fightEngine.catchIsDone();//why? do at the screen change to return on the map, not here!
                displayText(tr("You have catched the wild %1").arg(QString::fromStdString(
                                                                       QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                                                                           otherMonster->monster).name)).toStdString());
            }
            else
                displayText(tr("You have failed the catch of the wild %1").arg(QString::fromStdString(
                                                                                   QtDatapackClientLoader::datapackLoader->monsterExtra.at(
                                                                                       otherMonster->monster).name)).toStdString());
            ui->labelFightTrap->hide();
            return;
        }
    }
    displayTrapTimer.start();
}

void Battle::displayText(const std::string &text)
{
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(QString::fromStdString(text));
    doNextActionTimer.start();
}

void Battle::resetPosition(bool outOfScreen,bool topMonster,bool bottomMonster)
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

void Battle::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    tradeOtherMonsters.push_back(monster);
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
    if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(monster.monster)!=
            QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).front);
        item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name));
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    else
    {
        item->setIcon(QIcon(":/CC/images/monsters/default/front.png"));
        item->setText(tr("Unknown"));
        item->setToolTip(QStringLiteral("Level: %1, Gender: %2").arg(monster.level).arg(genderString));
    }
    ui->tradeOtherMonsters->addItem(item);
}

//learn
bool Battle::showLearnSkillByPosition(const uint8_t &monsterPosition)
{
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->learnAttackList->clear();
    attack_to_learn_graphical.clear();
    std::vector<PlayerMonster> playerMonster=fightEngine.getPlayerMonster();
    //get the right monster
    QHash<uint16_t,uint8_t> skillToDisplay;
    unsigned int index=monsterPosition;
    PlayerMonster monster=playerMonster.at(index);
    ui->learnMonster->setPixmap(QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster).front.scaled(160,160));
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        ui->learnSP->setVisible(true);
        ui->learnSP->setText(tr("SP: %1").arg(monster.sp));
    }
    else
        ui->learnSP->setVisible(false);
    if(Ultimate::ultimate.isUltimate())
        ui->learnInfo->setText(tr("<b>%1</b><br />Level %2").arg(
                               QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster).name))
                           .arg(monster.level));
    unsigned int sub_index=0;
    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.size())
    {
        Monster::AttackToLearn learn=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster).learn.at(sub_index);
        if(learn.learnAtLevel<=monster.level)
        {
            unsigned int sub_index2=0;
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
                    (sub_index2<monster.skills.size() && (monster.skills.at(sub_index2).level+1)==learn.learnSkillLevel)
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
    QHashIterator<uint16_t,uint8_t> i(skillToDisplay);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        const uint32_t &level=i.value();
        if(CommonSettingsServer::commonSettingsServer.useSP)
        {
            if(level<=1)
                item->setText(tr("%1\nSP cost: %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name))
                            .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
            else
                item->setText(tr("%1 level %2\nSP cost: %3")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name))
                            .arg(level)
                            .arg(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn)
                        );
        }
        else
        {
            if(level<=1)
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name));
            else
                item->setText(tr("%1 level %2")
                            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterSkillsExtra.at(i.key()).name)
                            .arg(level)
                        ));
        }
        if(CommonSettingsServer::commonSettingsServer.useSP && CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(i.key()).level.at(i.value()-1).sp_to_learn>monster.sp)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
            item->setToolTip(tr("You need more sp"));
        }
        attack_to_learn_graphical[item]=i.key();
        ui->learnAttackList->addItem(item);
    }
    if(attack_to_learn_graphical.empty())
        ui->learnDescription->setText(tr("No more attack to learn"));
    else
        ui->learnDescription->setText(tr("Select attack to learn"));
    return true;
}

void Battle::sendBattleReturn(const std::vector<Skill::AttackReturn> &attackReturn)
{
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=fightEngine.getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster was hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster was hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    fightEngine.addAndApplyAttackReturnList(attackReturn);
    #ifdef CATCHCHALLENGER_DEBUG_FIGHT
    {
        const PlayerMonster * currentMonster=fightEngine.getCurrentMonster();
        if(currentMonster!=NULL)
            qDebug() << "currentMonster have hp" << currentMonster->hp << "and buff" << currentMonster->buffs.size();
        const PublicPlayerMonster * otherMonster=fightEngine.getOtherMonster();
        if(currentMonster!=NULL)
            qDebug() << "otherMonster have hp" << otherMonster->hp << "and buff" << otherMonster->buffs.size();
    }
    #endif
    doNextAction();
}

void Battle::on_listWidgetFightAttack_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    on_pushButtonFightAttackConfirmed_clicked();
}

void Battle::useTrap(const uint16_t &itemId)
{
    updateTrapTime.restart();
    displayTrapProgression=0;
    trapItemId=itemId;
    if(!fightEngine.tryCatchClient(itemId))
    {
        std::cerr << "!fightEngine.tryCatchClient(itemId)" << std::endl;
        abort();
    }
    displayTrap();
    //need wait the server reply, monsterCatch(const bool &success)
}

void Battle::monsterCatch(const bool &success)
{
    if(fightEngine.playerMonster_catchInProgress.empty())
    {
        emit error("Internal bug: cupture monster list is emtpy");
        return;
    }
    if(!success)
    {
        trapSuccess=false;
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        emit message(QStringLiteral("catch is failed"));
        #endif
        fightEngine.generateOtherAttack();
    }
    else
    {
        trapSuccess=true;
        #ifdef CATCHCHALLENGER_DEBUG_FIGHT
        emit message(QStringLiteral("catch is success"));
        #endif
        //fightEngine.playerMonster_catchInProgress.first().id=newMonsterId;
        if(fightEngine.getPlayerMonster().size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
        {
            Player_private_and_public_informations &playerInformations=client->get_player_informations();
            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
            {
                QMessageBox::warning(this,tr("Error"),tr("You have already the maximum number of monster into you warehouse"));
                return;
            }
            playerInformations.warehouse_playerMonster.push_back(fightEngine.playerMonster_catchInProgress.front());
        }
        else
        {
            fightEngine.addPlayerMonster(fightEngine.playerMonster_catchInProgress.front());
            load_monsters();
        }
    }
    fightEngine.playerMonster_catchInProgress.erase(fightEngine.playerMonster_catchInProgress.cbegin());
    displayTrap();
}

void Battle::battleAcceptedByOther(const std::string &pseudo,const uint8_t &skinId,const std::vector<uint8_t> &stat,
                                       const uint8_t &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    BattleInformations battleInformations;
    battleInformations.pseudo=pseudo;
    battleInformations.skinId=skinId;
    battleInformations.stat=stat;
    battleInformations.monsterPlace=monsterPlace;
    battleInformations.publicPlayerMonster=publicPlayerMonster;
    battleInformationsList.push_back(battleInformations);
    if(battleInformationsList.size()>1 || !botFightList.empty())
        return;
    battleAcceptedByOtherFull(battleInformations);
}

void Battle::battleAcceptedByOtherFull(const BattleInformations &battleInformations)
{
    if(fightEngine.isInFight())
    {
        qDebug() << "already in fight";
        client->battleRefused();
        return;
    }
    prepareFight();
    battleType=BattleType_OtherPlayer;
    ui->stackedWidget->setCurrentWidget(ui->page_battle);

    //skinFolderList=CatchChallenger::FacilityLib::skinIdList(client->get_datapack_base_name()+QStringLiteral(DATAPACK_BASE_PATH_SKIN));
    QPixmap otherFrontImage=getFrontSkin(battleInformations.skinId);

    //reset the other player info
    ui->labelFightMonsterTop->setPixmap(otherFrontImage.scaled(160,160));
    //ui->battleOtherPseudo->setText(lastBattleQuery.first().first);
    ui->frameFightTop->hide();
    ui->frameFightBottom->hide();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage.scaled(160,160));
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("%1 wish fight with you").arg(QString::fromStdString(battleInformations.pseudo)));
    ui->pushButtonFightEnterNext->hide();

    resetPosition(true);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
    fightEngine.setBattleMonster(battleInformations.stat,battleInformations.monsterPlace,battleInformations.publicPlayerMonster);

    init_environement_display(mapController->getMapObject(),mapController->getX(),mapController->getY());
}

void Battle::battleCanceledByOther()
{
    fightEngine.fightFinished();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled the battle").toStdString());
    load_monsters();
    if(battleInformationsList.empty())
    {
        emit error("battle info not found at collision");
        return;
    }
    battleInformationsList.erase(battleInformationsList.cbegin());
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        const uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
}

void Battle::doNextAction()
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
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
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
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
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
    stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
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

bool Battle::displayFirstAttackText(bool firstText)
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
        stackedWidgetFightBottomBarsetCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
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

void Battle::displayAttack()
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
