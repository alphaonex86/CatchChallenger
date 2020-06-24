#include "MonsterDetails.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../Language.hpp"
#include "../cc/QtDatapackClientLoader.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Ultimate.hpp"
#include "../../../general/fight/CommonFightEngineBase.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include <QPainter>

MonsterDetails::MonsterDetails()
{

    quit=new CustomButton(":/CC/images/interface/cancel.png",this);
    connect(quit,&CustomButton::clicked,this,&MonsterDetails::removeAbove);

    details=new QGraphicsTextItem(this);
    details->setDefaultTextColor(Qt::white);

    monsterDetailsImage=new QGraphicsPixmapItem(this);

    proxy=new QGraphicsProxyWidget(this);
    monsterDetailsSkills=new QListWidget();
    proxy->setWidget(monsterDetailsSkills);

    monsterDetailsCatched=new QGraphicsPixmapItem(this);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    monsterDetailsName=new CustomText(gradient1,gradient2,this);
    monsterDetailsLevel=new QGraphicsTextItem(this);
    monsterDetailsLevel->setDefaultTextColor(Qt::white);
    hp_text=new QGraphicsTextItem(this);
    hp_text->setDefaultTextColor(Qt::white);
    hp_back=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/mbb.png"),this);
    hp_bar=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png"),this);;
    xp_text=new QGraphicsTextItem(this);
    xp_text->setDefaultTextColor(Qt::white);
    xp_back=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/mbb.png"),this);;
    xp_bar=new QGraphicsPixmapItem(*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png"),this);;

    if(!connect(&Language::language,&Language::newLanguage,this,&MonsterDetails::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&MonsterDetails::removeAbove,Qt::QueuedConnection))
        abort();
    newLanguage();
}

QRectF MonsterDetails::boundingRect() const
{
    return QRectF();
}

void MonsterDetails::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    int min=widget->height();
    if(min>widget->width())
        min=widget->width();
    const int xLeft=(widget->width()-min)/2;
    const int xRight=widget->width()-min-xLeft;
    const int yTop=(widget->height()-min)/2;
    const int yBottom=widget->height()-min-yTop;
    p->drawPixmap(xLeft,yTop,min,min,*GameLoader::gameLoader->getImage(":/CC/images/interface/backm.png"));
    if(xLeft>0)
        p->drawPixmap(0,0,xLeft,widget->height(),*GameLoader::gameLoader->getImage(":/CC/images/interface/backm.png"),0,0,1,1);
    if(xRight>0)
        p->drawPixmap(widget->width()-xRight,0,xRight,widget->height(),*GameLoader::gameLoader->getImage(":/CC/images/interface/backm.png"),0,0,1,1);
    if(yTop>0)
        p->drawPixmap(0,0,widget->width(),yTop,*GameLoader::gameLoader->getImage(":/CC/images/interface/backm.png"),0,0,1,1);
    if(yBottom>0)
        p->drawPixmap(widget->height()-yBottom,0,yBottom,widget->width(),*GameLoader::gameLoader->getImage(":/CC/images/interface/backm.png"),0,0,1,1);

    auto font=details->font();
    unsigned int bigSpace=50;
    unsigned int space=30;
    qreal lineSize=2.0;

    if(widget->width()<800 || widget->height()<600)
    {
        quit->setSize(83/2,94/2);
        font.setPixelSize(30/2);
        bigSpace/=2;
        space=10;
        monsterDetailsCatched->setScale(1.0);
        hp_back->setScale(2.0);
        hp_bar->setScale(2.0);
        xp_back->setScale(2.0);
        xp_bar->setScale(2.0);
        lineSize=2.0;
        monsterDetailsName->setPixelSize(50/2);
    }
    else
    {
        quit->setSize(83,94);
        font.setPixelSize(30);
        monsterDetailsCatched->setScale(2.0);
        if(widget->width()<1400 || widget->height()<800)
        {
            hp_back->setScale(3.0);
            hp_bar->setScale(3.0);
            xp_back->setScale(3.0);
            xp_bar->setScale(3.0);
            lineSize=3.0;
        }
        else
        {
            hp_back->setScale(4.0);
            hp_bar->setScale(4.0);
            xp_back->setScale(4.0);
            xp_bar->setScale(4.0);
            lineSize=4.0;
        }
        monsterDetailsName->setPixelSize(50);
    }
    details->setFont(font);

    p->save();
    p->setPen(QPen(Qt::white, lineSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    //p->drawLine(widget->width()*1/6,widget->height()*1/6,widget->width()*2/6,widget->height()*1/6);
    p->drawLine(details->boundingRect().width()+space,widget->height()*1/6,widget->width()*2/6,widget->height()*1/6);

    p->drawLine(widget->width()*2/6,widget->height()*1/6,widget->width()/2,widget->height()*4/10);
    //p->drawLine(widget->width()*2/6,widget->height()*1/6,widget->width()*4/6,widget->height()*7/10);
    p->drawLine(widget->width()/2,widget->height()*5/10,widget->width()*4/6,widget->height()*7/10);

    p->drawLine(widget->width()*4/6,widget->height()*7/10,widget->width()*5/6,widget->height()*7/10);
    p->restore();

    monsterDetailsLevel->setFont(font);
    hp_text->setFont(font);
    xp_text->setFont(font);

    quit->setPos(widget->width()-quit->width()-bigSpace,bigSpace);
    details->setPos(bigSpace,bigSpace);

    int ratio=(min*2/3)/monsterDetailsImage->pixmap().width();
    if(ratio>0)
        monsterDetailsImage->setScale(ratio);
    monsterDetailsImage->setPos(widget->width()/2-monsterDetailsImage->pixmap().width()*monsterDetailsImage->scale()/2,
                                widget->height()/2-monsterDetailsImage->pixmap().height()*monsterDetailsImage->scale()/2
                                );

    monsterDetailsSkills->setFixedSize((widget->width()-bigSpace*4)/3,widget->height()/3);
    proxy->setPos(widget->width()-monsterDetailsSkills->width()-bigSpace,widget->height()/3);

    int tempY=widget->height()/2+monsterDetailsImage->pixmap().height()*monsterDetailsImage->scale()/2+space-100;
    monsterDetailsName->setPos(widget->width()/2-monsterDetailsName->boundingRect().width()/2,tempY);
    monsterDetailsCatched->setPos(widget->width()/2-monsterDetailsName->boundingRect().width()/2-space-monsterDetailsCatched->pixmap().width()*monsterDetailsCatched->scale(),
                                  tempY-(monsterDetailsCatched->pixmap().height()-monsterDetailsName->boundingRect().height())/2-18);
    monsterDetailsLevel->setPos(widget->width()/2+monsterDetailsName->boundingRect().width()/2+space,tempY+(monsterDetailsName->boundingRect().height()-monsterDetailsLevel->boundingRect().height())/2+15);

    tempY+=monsterDetailsCatched->pixmap().height()*monsterDetailsCatched->scale();
    unsigned int tempBarW=hp_text->boundingRect().width()+space+hp_back->pixmap().width()*hp_back->scale();
    unsigned int tempX=widget->width()/2-tempBarW/2+hp_text->boundingRect().width()+space;
    hp_text->setPos(tempX-hp_text->boundingRect().width()-space/3,tempY+10);
    int o=+(hp_text->boundingRect().height()-hp_back->pixmap().height())/2;
    hp_back->setPos(tempX,tempY+o);
    hp_bar->setPos(tempX,tempY+o);
    tempY+=hp_text->boundingRect().height();
    xp_text->setPos(tempX-xp_text->boundingRect().width()-space/3,tempY+10);
    o=+(hp_text->boundingRect().height()-hp_back->pixmap().height())/2;
    xp_back->setPos(tempX,tempY+o);
    xp_bar->setPos(tempX,tempY+o);
}

void MonsterDetails::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void MonsterDetails::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void MonsterDetails::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void MonsterDetails::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void MonsterDetails::newLanguage()
{
    hp_text->setHtml(tr("HP"));
    xp_text->setHtml(tr("XP"));
}

void MonsterDetails::setVar(const CatchChallenger::PlayerMonster &monster)
{
    /// \todo monsterDetailsBuffs

    QLinearGradient gradient1( 0, 0, 0, 100 );
    QLinearGradient gradient2( 0, 0, 0, 100 );
    switch (monster.gender) {
    case CatchChallenger::Gender::Gender_Male:
        gradient1.setColorAt( 0.25, QColor(180,200,255));
        gradient1.setColorAt( 0.75, QColor(255,255,255));
        gradient2.setColorAt( 0, QColor(2,28,64));
        gradient2.setColorAt( 1, QColor(2,28,64));
        break;
    case CatchChallenger::Gender::Gender_Female:
        gradient1.setColorAt( 0.25, QColor(255,200,200));
        gradient1.setColorAt( 0.75, QColor(255,255,255));
        gradient2.setColorAt( 0, QColor(64,28,2));
        gradient2.setColorAt( 1, QColor(64,28,2));
        break;
    default:
        gradient1.setColorAt( 0.25, QColor(230,230,230));
        gradient1.setColorAt( 0.75, QColor(255,255,255));
        gradient2.setColorAt( 0, QColor(50,50,50));
        gradient2.setColorAt( 1, QColor(50,50,50));
        break;
    }
    monsterDetailsName->setGradient(gradient2,gradient1);

    const QtDatapackClientLoader::MonsterExtra &monsterExtraInfo=QtDatapackClientLoader::datapackLoader->monsterExtra.at(monster.monster);
    const QtDatapackClientLoader::QtMonsterExtra &QtmonsterExtraInfo=QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster);
    const CatchChallenger::Monster &monsterGeneralInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster);
    const CatchChallenger::Monster::Stat &stat=CatchChallenger::CommonFightEngineBase::getStat(monsterGeneralInfo,monster.level);

    QString typeString;
    if(!monsterGeneralInfo.type.empty())
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
        typeString=extraInfo.join(QStringLiteral("<br />"));
    }
    monsterDetailsName->setText(QString::fromStdString(monsterExtraInfo.name));
    //monsterDetailsDescription->setText(QString::fromStdString(monsterExtraInfo.description));
    {
        QPixmap front=QtmonsterExtraInfo.front;
        front=front.scaled(160,160);
        monsterDetailsImage->setPixmap(front);
    }
    {
        QPixmap catchPixmap;
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(monster.catched_with)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->getImageExtra(monster.catched_with).image;
            monsterDetailsCatched->setToolTip(tr("catched with %1")
                                                  .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra
                                                                              .at(monster.catched_with).name)));
        }
        else
        {
            catchPixmap=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
            monsterDetailsCatched->setToolTip(tr("catched with unknown item: %1").arg(monster.catched_with));
        }
        catchPixmap=catchPixmap.scaled(48,48);
        monsterDetailsCatched->setPixmap(catchPixmap);
    }
    const uint32_t &maxXp=monsterGeneralInfo.level_to_xp.at(monster.level-1);
    monsterDetailsLevel->setHtml(QString::number(monster.level));

    QPixmap bar;
    if(monster.hp>(stat.hp/2))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png");
    else if(monster.hp>(stat.hp/4))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mborange.png");
    else
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbred.png");
    hp_bar->setPixmap(bar.scaled(bar.width()*monster.hp/stat.hp,bar.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png");
    xp_bar->setPixmap(bar.scaled(bar.width()*monster.remaining_xp/maxXp,bar.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));

    QString detailsString;
    detailsString+=tr("Speed: %1").arg(stat.speed)+"<br />";
    detailsString+=tr("Attack: %1").arg(stat.attack)+"<br />";
    detailsString+=tr("Defense: %1").arg(stat.defense)+"<br />";
    detailsString+=tr("SP attack: %1").arg(stat.special_attack)+"<br />";
    detailsString+=tr("SP defense: %1").arg(stat.special_defense)+"<br />";
    if(CommonSettingsServer::commonSettingsServer.useSP)
        detailsString+=tr("Skill point: %1").arg(monster.sp)+"<br />";
    if(!typeString.isEmpty())
        detailsString+="<br />"+typeString;
    details->setHtml(detailsString);
    //skill
    monsterDetailsSkills->clear();
    {
        unsigned int indexSkill=0;
        while(indexSkill<monster.skills.size())
        {
            const CatchChallenger::PlayerMonster::PlayerSkill &monsterSkill=monster.skills.at(indexSkill);
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
                const CatchChallenger::Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(monsterSkill.skill);
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
            monsterDetailsSkills->addItem(item);
            indexSkill++;
        }
    }
    //do the buff
    /*{
        monsterDetailsBuffs->clear();
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
            monsterDetailsBuffs->addItem(item);
            index++;
        }
    }*/
}
