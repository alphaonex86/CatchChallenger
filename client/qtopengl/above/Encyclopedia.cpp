#include "Encyclopedia.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../ConnexionManager.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <iostream>
#include <algorithm>

Encyclopedia::Encyclopedia() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    showingMonsters=true;
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

    monsterTab=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    itemTab=new CustomButton(":/CC/images/interface/button.png",this);

    monsterList=new QListWidget();
    monsterListProxy=new QGraphicsProxyWidget(this);
    monsterListProxy->setWidget(monsterList);

    itemList=new QListWidget();
    itemListProxy=new QGraphicsProxyWidget(this);
    itemListProxy->setWidget(itemList);
    itemListProxy->setVisible(false);
    itemList->setVisible(false);

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    description=new QGraphicsTextItem(this);

    knownOnlyFilter=new CheckBox(this);
    knownOnlyFilter->setChecked(false);

    if(!connect(quit,&CustomButton::clicked,this,&Encyclopedia::removeAbove))
        abort();
    if(!connect(monsterTab,&CustomButton::clicked,this,&Encyclopedia::onMonsterTab))
        abort();
    if(!connect(itemTab,&CustomButton::clicked,this,&Encyclopedia::onItemTab))
        abort();
    if(!connect(monsterList,&QListWidget::itemSelectionChanged,this,&Encyclopedia::on_monsterList_itemSelectionChanged))
        abort();
    if(!connect(itemList,&QListWidget::itemSelectionChanged,this,&Encyclopedia::on_itemList_itemSelectionChanged))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Encyclopedia::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Encyclopedia::~Encyclopedia()
{
    delete wdialog;
}

void Encyclopedia::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Encyclopedia::boundingRect() const
{
    return QRectF();
}

void Encyclopedia::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=description->font();
    int tabH=61;
    int tabW=148;
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        monsterTab->setSize(148/2,61/2);
        monsterTab->setPixelSize(12);
        itemTab->setSize(148/2,61/2);
        itemTab->setPixelSize(12);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
        tabH=61/2;
        tabW=148/2;
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        monsterTab->setSize(148,61);
        monsterTab->setPixelSize(23);
        itemTab->setSize(148,61);
        itemTab->setPixelSize(23);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    // tabs
    int tabY=y+label.pixmap().height()/2*label.scale()+space/2;
    monsterTab->setPos(x+space,tabY);
    itemTab->setPos(x+space+tabW+space/2,tabY);

    // known filter
    knownOnlyFilter->setPos(x+idealW-space-100,tabY);

    // list
    int listY=tabY+tabH+space/2;
    int descH=font.pixelSize()*5+space;
    int listH=idealH-(listY-y)-descH-space;
    if(listH<50) listH=50;

    if(showingMonsters)
    {
        monsterList->setFixedSize(idealW-space*2-wdialog->currentBorderSize()*2,listH);
        monsterListProxy->setPos(x+wdialog->currentBorderSize()+space,listY);
    }
    else
    {
        itemList->setFixedSize(idealW-space*2-wdialog->currentBorderSize()*2,listH);
        itemListProxy->setPos(x+wdialog->currentBorderSize()+space,listY);
    }

    // description area
    int descY=listY+listH+space/2;
    descriptionBack->setPos(x+wdialog->currentBorderSize()+space,descY);
    descriptionBack->setSize(idealW-space*2-wdialog->currentBorderSize()*2,descH);
    description->setPos(x+wdialog->currentBorderSize()+space+descriptionBack->currentBorderSize(),
                        descY+descriptionBack->currentBorderSize());
    description->setTextWidth(idealW-space*2-wdialog->currentBorderSize()*2-descriptionBack->currentBorderSize()*2);
    description->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Encyclopedia::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    monsterTab->mousePressEventXY(p,pressValidated);
    itemTab->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Encyclopedia::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    monsterTab->mouseReleaseEventXY(p,pressValidated);
    itemTab->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Encyclopedia::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Encyclopedia::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Encyclopedia::newLanguage()
{
    title->setText(tr("Encyclopedia"));
    monsterTab->setText(tr("Monsters"));
    itemTab->setText(tr("Items"));
    description->setHtml(tr("Select an entry"));
}

void Encyclopedia::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;
    showingMonsters=true;
    monsterListProxy->setVisible(true);
    monsterList->setVisible(true);
    itemListProxy->setVisible(false);
    itemList->setVisible(false);
    populateMonsterList();
}

void Encyclopedia::onMonsterTab()
{
    if(showingMonsters)
        return;
    showingMonsters=true;
    monsterListProxy->setVisible(true);
    monsterList->setVisible(true);
    itemListProxy->setVisible(false);
    itemList->setVisible(false);
    monsterTab->setImage(":/CC/images/interface/greenbutton.png");
    itemTab->setImage(":/CC/images/interface/button.png");
    populateMonsterList();
    description->setHtml(tr("Select an entry"));
}

void Encyclopedia::onItemTab()
{
    if(!showingMonsters)
        return;
    showingMonsters=false;
    monsterListProxy->setVisible(false);
    monsterList->setVisible(false);
    itemListProxy->setVisible(true);
    itemList->setVisible(true);
    monsterTab->setImage(":/CC/images/interface/button.png");
    itemTab->setImage(":/CC/images/interface/greenbutton.png");
    populateItemList();
    description->setHtml(tr("Select an entry"));
}

void Encyclopedia::onKnownFilterChanged()
{
    if(showingMonsters)
        populateMonsterList();
    else
        populateItemList();
}

void Encyclopedia::populateMonsterList()
{
    monsterList->clear();
    if(connexionManager==nullptr)
        return;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    const bool knownOnly=knownOnlyFilter->isChecked();

    // get all monster IDs and sort
    std::vector<uint16_t> monsterIds;
    for(const auto &pair : QtDatapackClientLoader::datapackLoader->get_monsterExtra())
        monsterIds.push_back(pair.first);
    std::sort(monsterIds.begin(),monsterIds.end());

    QFont unknownFont;
    unknownFont.setItalic(true);

    for(const uint16_t &monsterId : monsterIds)
    {
        bool known=false;
        if(playerInformations.encyclopedia_monster!=nullptr)
            known=(playerInformations.encyclopedia_monster[monsterId/8] & (1<<(7-monsterId%8)))!=0;

        if(knownOnly && !known)
            continue;

        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,monsterId);

        if(known)
        {
            const QtDatapackClientLoader::MonsterExtra &extra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monsterId);
            item->setText(QString::fromStdString(extra.name));
            item->setIcon(QPixmap(QString::fromStdString(extra.frontPath)));
        }
        else
        {
            item->setText(tr("???"));
            item->setFont(unknownFont);
            item->setForeground(QBrush(QColor(100,100,100)));
            item->setData(99,0);
        }
        monsterList->addItem(item);
    }
}

void Encyclopedia::populateItemList()
{
    itemList->clear();
    if(connexionManager==nullptr)
        return;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    const bool knownOnly=knownOnlyFilter->isChecked();

    // get all item IDs and sort
    std::vector<uint16_t> itemIds;
    for(const auto &pair : QtDatapackClientLoader::datapackLoader->get_itemsExtra())
        itemIds.push_back(pair.first);
    std::sort(itemIds.begin(),itemIds.end());

    QFont unknownFont;
    unknownFont.setItalic(true);

    for(const uint16_t &itemId : itemIds)
    {
        bool known=false;
        if(playerInformations.encyclopedia_item!=nullptr)
            known=(playerInformations.encyclopedia_item[itemId/8] & (1<<(7-itemId%8)))!=0;

        if(knownOnly && !known)
            continue;

        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,itemId);

        if(known)
        {
            const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(itemId);
            item->setText(QString::fromStdString(extra.name));
            item->setIcon(QPixmap(QString::fromStdString(extra.imagePath)));
        }
        else
        {
            item->setText(tr("???"));
            item->setFont(unknownFont);
            item->setForeground(QBrush(QColor(100,100,100)));
            item->setData(99,0);
        }
        itemList->addItem(item);
    }
}

void Encyclopedia::on_monsterList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=monsterList->selectedItems();
    if(items.size()!=1)
    {
        description->setHtml(tr("Select a monster"));
        return;
    }
    uint16_t monsterId=static_cast<uint16_t>(items.first()->data(99).toUInt());
    if(monsterId==0)
    {
        description->setHtml(tr("This monster is unknown"));
        return;
    }

    if(QtDatapackClientLoader::datapackLoader->get_monsterExtra().find(monsterId)==
            QtDatapackClientLoader::datapackLoader->get_monsterExtra().cend())
    {
        description->setHtml(tr("Monster data not found"));
        return;
    }

    const QtDatapackClientLoader::MonsterExtra &extra=QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monsterId);
    QString html;
    html+=QStringLiteral("<b>%1</b><br/>").arg(QString::fromStdString(extra.name));
    html+=QString::fromStdString(extra.description);

    if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters().find(monsterId)!=
            CatchChallenger::CommonDatapack::commonDatapack.get_monsters().cend())
    {
        const CatchChallenger::Monster &monsterDef=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monsterId);
        // show types
        if(!monsterDef.type.empty())
        {
            html+=QStringLiteral("<br/><i>%1: ").arg(tr("Type"));
            unsigned int tIndex=0;
            while(tIndex<monsterDef.type.size())
            {
                if(tIndex>0)
                    html+=QStringLiteral(", ");
                if(QtDatapackClientLoader::datapackLoader->get_typeExtra().find(monsterDef.type.at(tIndex))!=
                        QtDatapackClientLoader::datapackLoader->get_typeExtra().cend())
                    html+=QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_typeExtra().at(monsterDef.type.at(tIndex)).name);
                else
                    html+=tr("Type %1").arg(monsterDef.type.at(tIndex));
                tIndex++;
            }
            html+=QStringLiteral("</i>");
        }
    }
    description->setHtml(html);
}

void Encyclopedia::on_itemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=itemList->selectedItems();
    if(items.size()!=1)
    {
        description->setHtml(tr("Select an item"));
        return;
    }
    uint16_t itemId=static_cast<uint16_t>(items.first()->data(99).toUInt());
    if(itemId==0)
    {
        description->setHtml(tr("This item is unknown"));
        return;
    }

    if(QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(itemId)==
            QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend())
    {
        description->setHtml(tr("Item data not found"));
        return;
    }

    const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemsExtra().at(itemId);
    QString html;
    html+=QStringLiteral("<b>%1</b><br/>").arg(QString::fromStdString(extra.name));
    html+=QString::fromStdString(extra.description);

    if(CatchChallenger::CommonDatapack::commonDatapack.get_items().item.find(itemId)!=
            CatchChallenger::CommonDatapack::commonDatapack.get_items().item.cend())
    {
        const CatchChallenger::Item &itemDef=CatchChallenger::CommonDatapack::commonDatapack.get_items().item.at(itemId);
        if(itemDef.price>0)
            html+=QStringLiteral("<br/>%1: %2$").arg(tr("Price")).arg(itemDef.price);
        else
            html+=QStringLiteral("<br/><i>%1</i>").arg(tr("Can't be sold"));
        if(!itemDef.consumeAtUse)
            html+=QStringLiteral("<br/><i>%1</i>").arg(tr("Infinite use"));
    }
    description->setHtml(html);
}
