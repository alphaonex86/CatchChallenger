#include "Warehouse.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../ConnexionManager.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include <QPainter>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <iostream>
#include <algorithm>

Warehouse::Warehouse() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    connexionManager=nullptr;

    label.setPixmap(QPixmap(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);

    QLinearGradient gradient1(0,0,0,100);
    gradient1.setColorAt(0.25,QColor(230,153,0));
    gradient1.setColorAt(0.75,QColor(255,255,255));
    QLinearGradient gradient2(0,0,0,100);
    gradient2.setColorAt(0,QColor(64,28,2));
    gradient2.setColorAt(1,QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    npcImage=new QGraphicsPixmapItem(this);
    npcNameText=new QGraphicsTextItem(this);
    npcNameText->setDefaultTextColor(Qt::white);

    playerMonstersTitle=new QGraphicsTextItem(this);
    playerMonstersTitle->setDefaultTextColor(Qt::white);
    playerMonsters=new QListWidget();
    playerMonstersProxy=new QGraphicsProxyWidget(this);
    playerMonstersProxy->setWidget(playerMonsters);

    warehouseMonstersTitle=new QGraphicsTextItem(this);
    warehouseMonstersTitle->setDefaultTextColor(Qt::white);
    warehouseMonsters=new QListWidget();
    warehouseMonstersProxy=new QGraphicsProxyWidget(this);
    warehouseMonstersProxy->setWidget(warehouseMonsters);

    depositBtn=new CustomButton(":/CC/images/interface/button.png",this);
    withdrawBtn=new CustomButton(":/CC/images/interface/button.png",this);
    validateBtn=new CustomButton(":/CC/images/interface/greenbutton.png",this);

    if(!connect(quit,&CustomButton::clicked,this,&Warehouse::removeAbove))
        abort();
    if(!connect(depositBtn,&CustomButton::clicked,this,&Warehouse::onDepositMonster))
        abort();
    if(!connect(withdrawBtn,&CustomButton::clicked,this,&Warehouse::onWithdrawMonster))
        abort();
    if(!connect(validateBtn,&CustomButton::clicked,this,&Warehouse::onValidate))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Warehouse::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Warehouse::~Warehouse()
{
    delete wdialog;
}

void Warehouse::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Warehouse::boundingRect() const
{
    return QRectF();
}

void Warehouse::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=900;
    int idealH=400;
    if(idealH<widget->height()-200)
        idealH=widget->height()-200;
    unsigned int space=30;
    if(widget->width()<800 || widget->height()<600)
    {
        idealW/=2;
        idealH/=2;
        space/=2;
    }
    if(idealW>widget->width())
        idealW=widget->width();
    int top=36;
    int bottom=94/2;
    if(widget->width()<800 || widget->height()<600)
    {
        top/=2;
        bottom/=2;
    }
    if((idealH+top+bottom)>widget->height())
        idealH=widget->height()-(top+bottom);

    int x=widget->width()/2-idealW/2;
    int y=widget->height()/2-idealH/2;

    wdialog->setPos(x,y);
    wdialog->setSize(idealW,idealH);

    auto font=npcNameText->font();
    int btnH=61;
    int btnW=148;
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        depositBtn->setSize(74,30);
        depositBtn->setPixelSize(10);
        withdrawBtn->setSize(74,30);
        withdrawBtn->setPixelSize(10);
        validateBtn->setSize(74,30);
        validateBtn->setPixelSize(10);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(10);
        btnH=30;
        btnW=74;
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        depositBtn->setSize(148,61);
        depositBtn->setPixelSize(23);
        withdrawBtn->setSize(148,61);
        withdrawBtn->setPixelSize(23);
        validateBtn->setSize(148,61);
        validateBtn->setPixelSize(23);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(16);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    // NPC image and name
    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    npcImage->setPos(x+wdialog->currentBorderSize()+space,contentY);
    npcNameText->setFont(font);
    npcNameText->setPos(x+wdialog->currentBorderSize()+space+52,contentY);

    int halfW=(idealW-space*3-wdialog->currentBorderSize()*2)/2;
    int listY=contentY+52+space/2;
    int listH=idealH-(listY-y)-btnH-space*2;
    if(listH<50) listH=50;

    // player monsters (left)
    playerMonstersTitle->setFont(font);
    playerMonstersTitle->setPos(x+wdialog->currentBorderSize()+space,listY-font.pixelSize()-2);
    playerMonsters->setFixedSize(halfW,listH);
    playerMonstersProxy->setPos(x+wdialog->currentBorderSize()+space,listY);

    // warehouse monsters (right)
    warehouseMonstersTitle->setFont(font);
    warehouseMonstersTitle->setPos(x+wdialog->currentBorderSize()+space+halfW+space,listY-font.pixelSize()-2);
    warehouseMonsters->setFixedSize(halfW,listH);
    warehouseMonstersProxy->setPos(x+wdialog->currentBorderSize()+space+halfW+space,listY);

    // buttons
    int btnY=listY+listH+space/2;
    depositBtn->setPos(x+wdialog->currentBorderSize()+space,btnY);
    withdrawBtn->setPos(x+wdialog->currentBorderSize()+space+halfW+space,btnY);
    validateBtn->setPos(x+idealW-wdialog->currentBorderSize()-space-validateBtn->width(),btnY);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Warehouse::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    depositBtn->mousePressEventXY(p,pressValidated);
    withdrawBtn->mousePressEventXY(p,pressValidated);
    validateBtn->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Warehouse::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    depositBtn->mouseReleaseEventXY(p,pressValidated);
    withdrawBtn->mouseReleaseEventXY(p,pressValidated);
    validateBtn->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Warehouse::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Warehouse::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Warehouse::newLanguage()
{
    title->setText(tr("Warehouse"));
    playerMonstersTitle->setHtml(tr("Your Monsters"));
    warehouseMonstersTitle->setHtml(tr("Stored Monsters"));
    depositBtn->setText(tr("Deposit"));
    withdrawBtn->setText(tr("Withdraw"));
    validateBtn->setText(tr("Validate"));
}

void Warehouse::setVar(ConnexionManager *connexionManager, const std::string &npcName, const uint8_t &skinId)
{
    this->connexionManager=connexionManager;

    npcNameText->setHtml(QString::fromStdString(npcName));
    const std::string skinPath=QtDatapackClientLoader::datapackLoader->getFrontSkinPath(skinId);
    QPixmap skin(QString::fromStdString(skinPath));
    if(!skin.isNull())
    {
        npcImage->setPixmap(skin.scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        npcImage->setVisible(true);
    }
    else
        npcImage->setVisible(false);

    updateContent();
}

void Warehouse::updateContent()
{
    playerMonsters->clear();
    warehouseMonsters->clear();
    if(connexionManager==nullptr)
        return;

    const std::vector<CatchChallenger::PlayerMonster> &playerMon=connexionManager->client->getPlayerMonster();
    unsigned int index=0;
    while(index<playerMon.size())
    {
        const CatchChallenger::PlayerMonster &monster=playerMon.at(index);
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=
                CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
        {
            QListWidgetItem *item=new QListWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(monster.monster)!=
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
            {
                const QtDatapackClientLoader::MonsterExtra &extra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster);
                item->setText(tr("%1, level: %2").arg(QString::fromStdString(extra.name)).arg(monster.level));
                item->setIcon(QPixmap(QString::fromStdString(extra.frontPath)));
            }
            else
                item->setText(tr("Monster %1, level: %2").arg(monster.monster).arg(monster.level));
            item->setData(98,0);// 0=from player
            item->setData(99,index);
            playerMonsters->addItem(item);
        }
        index++;
    }

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    index=0;
    while(index<playerInformations.warehouse_monsters.size())
    {
        const CatchChallenger::PlayerMonster &monster=playerInformations.warehouse_monsters.at(index);
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monster.monster)!=
                CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
        {
            QListWidgetItem *item=new QListWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(monster.monster)!=
                    QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
            {
                const QtDatapackClientLoader::MonsterExtra &extra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster);
                item->setText(tr("%1, level: %2").arg(QString::fromStdString(extra.name)).arg(monster.level));
                item->setIcon(QPixmap(QString::fromStdString(extra.frontPath)));
            }
            else
                item->setText(tr("Monster %1, level: %2").arg(monster.monster).arg(monster.level));
            item->setData(98,1);// 1=from warehouse
            item->setData(99,index);
            warehouseMonsters->addItem(item);
        }
        index++;
    }

    depositBtn->setEnabled(playerMonsters->count()>1);// must keep at least 1
    withdrawBtn->setEnabled(warehouseMonsters->count()>0);
}

void Warehouse::onDepositMonster()
{
    QList<QListWidgetItem *> items=playerMonsters->selectedItems();
    if(items.size()!=1)
        return;
    if(playerMonsters->count()<=1)
    {
        emit showTip(tr("You must keep at least one monster").toStdString());
        return;
    }
    if(warehouseMonsters->count()>=(int)CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
    {
        emit showTip(tr("Warehouse is full").toStdString());
        return;
    }

    QListWidgetItem *item=items.first();
    int row=playerMonsters->row(item);
    playerMonsters->takeItem(row);
    item->setData(98,0);// keep source marker
    warehouseMonsters->addItem(item);

    depositBtn->setEnabled(playerMonsters->count()>1);
    withdrawBtn->setEnabled(warehouseMonsters->count()>0);
}

void Warehouse::onWithdrawMonster()
{
    QList<QListWidgetItem *> items=warehouseMonsters->selectedItems();
    if(items.size()!=1)
        return;
    if(playerMonsters->count()>=(int)CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
    {
        emit showTip(tr("Your team is full").toStdString());
        return;
    }

    QListWidgetItem *item=items.first();
    int row=warehouseMonsters->row(item);
    warehouseMonsters->takeItem(row);
    item->setData(98,1);// keep source marker
    playerMonsters->addItem(item);

    depositBtn->setEnabled(playerMonsters->count()>1);
    withdrawBtn->setEnabled(warehouseMonsters->count()>0);
}

void Warehouse::onValidate()
{
    // collect indices for deposit/withdraw
    std::vector<uint8_t> depositIndices;
    std::vector<uint8_t> withdrawIndices;

    // items currently in warehouse list that came from player (data(98)==0)
    int count=warehouseMonsters->count();
    for(int i=0;i<count;i++)
    {
        QListWidgetItem *item=warehouseMonsters->item(i);
        if(item->data(98).toInt()==0)
            depositIndices.push_back(static_cast<uint8_t>(item->data(99).toUInt()));
    }

    // items currently in player list that came from warehouse (data(98)==1)
    count=playerMonsters->count();
    for(int i=0;i<count;i++)
    {
        QListWidgetItem *item=playerMonsters->item(i);
        if(item->data(98).toInt()==1)
            withdrawIndices.push_back(static_cast<uint8_t>(item->data(99).toUInt()));
    }

    // sort in reverse order for safe removal
    std::sort(depositIndices.rbegin(),depositIndices.rend());
    std::sort(withdrawIndices.rbegin(),withdrawIndices.rend());

    connexionManager->client->wareHouseStore(withdrawIndices,depositIndices);

    // server handles the actual state update via wareHouseStore

    emit showTip(tr("Warehouse updated").toStdString());
    emit setAbove(nullptr);
}
