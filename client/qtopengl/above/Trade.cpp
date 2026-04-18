#include "Trade.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../ConnexionManager.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <iostream>

Trade::Trade() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    connexionManager=nullptr;
    tradeOtherStat=TradeOtherStat_InWait;

    label.setPixmap(QPixmap(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QLinearGradient gradient1(0,0,0,100);
    gradient1.setColorAt(0.25,QColor(230,153,0));
    gradient1.setColorAt(0.75,QColor(255,255,255));
    QLinearGradient gradient2(0,0,0,100);
    gradient2.setColorAt(0,QColor(64,28,2));
    gradient2.setColorAt(1,QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    // player side
    playerPseudo=new QGraphicsTextItem(this);
    playerPseudo->setDefaultTextColor(Qt::white);
    playerImage=new QGraphicsPixmapItem(this);
    playerItemsList=new QListWidget();
    playerItemsProxy=new QGraphicsProxyWidget(this);
    playerItemsProxy->setWidget(playerItemsList);
    playerCashLabel=new QGraphicsTextItem(this);
    playerCashLabel->setDefaultTextColor(Qt::white);
    playerCash=new SpinBox(this);
    playerCash->setMinimum(0);
    playerCash->setMaximum(999999);

    addItemBtn=new CustomButton(":/CC/images/interface/button.png",this);
    addMonsterBtn=new CustomButton(":/CC/images/interface/button.png",this);

    // other player side
    otherPseudo=new QGraphicsTextItem(this);
    otherPseudo->setDefaultTextColor(Qt::white);
    otherImage=new QGraphicsPixmapItem(this);
    otherItemsList=new QListWidget();
    otherItemsProxy=new QGraphicsProxyWidget(this);
    otherItemsProxy->setWidget(otherItemsList);
    otherCashLabel=new QGraphicsTextItem(this);
    otherCashLabel->setDefaultTextColor(Qt::white);
    otherStatusText=new QGraphicsTextItem(this);
    otherStatusText->setDefaultTextColor(QColor(200,200,100));

    validateBtn=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    cancelBtn=new CustomButton(":/CC/images/interface/cancel.png",this);

    if(!connect(addItemBtn,&CustomButton::clicked,this,&Trade::onAddItemClicked))
        abort();
    if(!connect(addMonsterBtn,&CustomButton::clicked,this,&Trade::onAddMonsterClicked))
        abort();
    if(!connect(validateBtn,&CustomButton::clicked,this,&Trade::onValidateClicked))
        abort();
    if(!connect(cancelBtn,&CustomButton::clicked,this,&Trade::onCancelClicked))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Trade::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Trade::~Trade()
{
    delete wdialog;
}

QRectF Trade::boundingRect() const
{
    return QRectF();
}

void Trade::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=900;
    int idealH=500;
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

    int btnH=61;
    int btnW=148;
    int skinSize=48;
    auto font=playerPseudo->font();
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(10);
        btnH=30;
        btnW=74;
        skinSize=24;
        addItemBtn->setSize(74,30);
        addItemBtn->setPixelSize(10);
        addMonsterBtn->setSize(74,30);
        addMonsterBtn->setPixelSize(10);
        validateBtn->setSize(74,30);
        validateBtn->setPixelSize(10);
        cancelBtn->setSize(42,47);
    }
    else
    {
        label.setScale(1);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(16);
        addItemBtn->setSize(148,61);
        addItemBtn->setPixelSize(23);
        addMonsterBtn->setSize(148,61);
        addMonsterBtn->setPixelSize(23);
        validateBtn->setSize(148,61);
        validateBtn->setPixelSize(23);
        cancelBtn->setSize(83,94);
    }

    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    int halfW=(idealW-space*3)/2;
    int listH=idealH-(contentY-y)-btnH*2-space*3;
    if(listH<50) listH=50;

    // left side (player)
    playerPseudo->setFont(font);
    playerPseudo->setPos(x+space+skinSize+4,contentY);
    playerImage->setPos(x+space,contentY);

    int listY=contentY+skinSize+space/2;
    playerItemsList->setFixedSize(halfW,listH);
    playerItemsProxy->setPos(x+space,listY);

    playerCashLabel->setFont(font);
    playerCashLabel->setPos(x+space,listY+listH+space/4);
    playerCash->setFixedSize(halfW/2,btnH);
    playerCash->setPos(x+space+halfW/2,listY+listH+space/4);

    addItemBtn->setPos(x+space,listY+listH+space/4+btnH+space/4);
    addMonsterBtn->setPos(x+space+btnW+space/2,listY+listH+space/4+btnH+space/4);

    // right side (other player)
    otherPseudo->setFont(font);
    otherPseudo->setPos(x+idealW-space-halfW+skinSize+4,contentY);
    otherImage->setPos(x+idealW-space-halfW,contentY);

    otherItemsList->setFixedSize(halfW,listH);
    otherItemsProxy->setPos(x+idealW-space-halfW,listY);

    otherCashLabel->setFont(font);
    otherCashLabel->setPos(x+idealW-space-halfW,listY+listH+space/4);

    otherStatusText->setFont(font);
    otherStatusText->setTextWidth(halfW);
    otherStatusText->setPos(x+idealW-space-halfW,listY+listH+space/4+font.pixelSize()+4);

    // validate/cancel buttons
    validateBtn->setPos(x+idealW-space-btnW,y+idealH-btnH-space/2);
    cancelBtn->setPos(x+space,y+idealH-cancelBtn->height()-space/2);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Trade::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    addItemBtn->mousePressEventXY(p,pressValidated);
    addMonsterBtn->mousePressEventXY(p,pressValidated);
    validateBtn->mousePressEventXY(p,pressValidated);
    cancelBtn->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Trade::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    addItemBtn->mouseReleaseEventXY(p,pressValidated);
    addMonsterBtn->mouseReleaseEventXY(p,pressValidated);
    validateBtn->mouseReleaseEventXY(p,pressValidated);
    cancelBtn->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Trade::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Trade::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Trade::newLanguage()
{
    title->setText(tr("Trade"));
    addItemBtn->setText(tr("Add Item"));
    addMonsterBtn->setText(tr("Add Monster"));
    validateBtn->setText(tr("Validate"));
    playerCashLabel->setHtml(tr("Cash:"));
}

void Trade::setVar(ConnexionManager *connexionManager, const std::string &otherPseudoStr, const uint8_t &otherSkinId)
{
    this->connexionManager=connexionManager;
    tradeOtherStat=TradeOtherStat_InWait;

    // reset current player
    playerCash->setValue(0);
    playerItemsList->clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeCurrentMonstersPosition.clear();
    addItemBtn->setEnabled(true);
    addMonsterBtn->setEnabled(true);
    validateBtn->setEnabled(true);

    // set player info
    playerPseudo->setHtml(QString::fromStdString(connexionManager->client->getPseudo()));

    // set other player info
    otherPseudo->setHtml(QString::fromStdString(otherPseudoStr));
    const std::string skinPath=QtDatapackClientLoader::datapackLoader->getFrontSkinPath(otherSkinId);
    QPixmap skin(QString::fromStdString(skinPath));
    if(!skin.isNull())
    {
        otherImage->setPixmap(skin.scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        otherImage->setVisible(true);
    }
    else
        otherImage->setVisible(false);

    // reset other player
    otherItemsList->clear();
    tradeOtherObjects.clear();
    tradeOtherMonsters.clear();
    otherCashLabel->setHtml(tr("Cash: 0$"));
    otherStatusText->setHtml(tr("Waiting for validation..."));
}

void Trade::onAddItemClicked()
{
    emit selectObjectForTrade();
}

void Trade::onAddMonsterClicked()
{
    emit selectMonsterForTrade();
}

void Trade::onValidateClicked()
{
    // disable controls after validation
    addItemBtn->setEnabled(false);
    addMonsterBtn->setEnabled(false);
    validateBtn->setEnabled(false);
    playerCash->setMinimum(playerCash->value());
    playerCash->setMaximum(playerCash->value());

    // send cash if any
    if(playerCash->value()>0)
    {
        connexionManager->client->tradeAddTradeCash(playerCash->value());
        emit removeCash(playerCash->value());
    }

    connexionManager->client->tradeFinish();

    if(tradeOtherStat==TradeOtherStat_Accepted)
        otherStatusText->setHtml(tr("Both validated, waiting server..."));
    else
        otherStatusText->setHtml(tr("Waiting for other player..."));
}

void Trade::onCancelClicked()
{
    // return items/cash/monsters to player
    emit addCash(playerCash->value());
    for(const auto &obj : tradeCurrentObjects)
        emit add_to_inventory(obj.first,obj.second,false);

    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherObjects.clear();
    tradeOtherMonsters.clear();

    connexionManager->client->tradeRefused();
    emit tradeClosed();
    emit setAbove(nullptr);
}

void Trade::tradeCanceledByOther()
{
    emit addCash(playerCash->value());
    for(const auto &obj : tradeCurrentObjects)
        emit add_to_inventory(obj.first,obj.second,false);

    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherObjects.clear();
    tradeOtherMonsters.clear();

    emit showTip(tr("Trade canceled by other player").toStdString());
    emit tradeClosed();
    emit setAbove(nullptr);
}

void Trade::tradeFinishedByOther()
{
    tradeOtherStat=TradeOtherStat_Accepted;
    if(!validateBtn->isEnabled())
        otherStatusText->setHtml(tr("Both validated, waiting server..."));
    else
        otherStatusText->setHtml(tr("Other player has validated"));
}

void Trade::tradeValidatedByTheServer()
{
    // receive other player's items
    for(const auto &obj : tradeOtherObjects)
        emit add_to_inventory(obj.first,obj.second,true);

    tradeCurrentObjects.clear();
    tradeOtherObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();

    emit showTip(tr("Trade successful!").toStdString());
    emit tradeClosed();
    emit setAbove(nullptr);
}

void Trade::tradeAddTradeCash(const uint64_t &cash)
{
    otherCashLabel->setHtml(tr("Cash: %1$").arg(cash));
}

void Trade::tradeAddTradeObject(const uint16_t &item, const uint32_t &quantity)
{
    if(tradeOtherObjects.find(item)!=tradeOtherObjects.cend())
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    updateOtherItemsDisplay();
}

void Trade::tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster)
{
    tradeOtherMonsters.push_back(monster);
    // add to other player list display
    QListWidgetItem *listItem=new QListWidgetItem();
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monster.monster))
    {
        listItem->setText(tr("%1 Lv.%2").arg(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).name)).arg(monster.level));
        listItem->setIcon(QPixmap(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).frontPath)));
    }
    else
        listItem->setText(tr("Monster Lv.%1").arg(monster.level));
    otherItemsList->addItem(listItem);
}

void Trade::updateCurrentItemsDisplay()
{
    playerItemsList->clear();
    for(const auto &obj : tradeCurrentObjects)
    {
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(obj.first))
        {
            const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemExtra(obj.first);
            item->setText(tr("%1 x%2").arg(QString::fromStdString(extra.name)).arg(obj.second));
            item->setIcon(QPixmap(QString::fromStdString(extra.imagePath)));
        }
        else
            item->setText(tr("Item %1 x%2").arg(obj.first).arg(obj.second));
        playerItemsList->addItem(item);
    }
    // add monsters
    for(const auto &m : tradeCurrentMonsters)
    {
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(m.monster))
        {
            item->setText(tr("%1 Lv.%2").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.monster).name)).arg(m.level));
            item->setIcon(QPixmap(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.monster).frontPath)));
        }
        else
            item->setText(tr("Monster Lv.%1").arg(m.level));
        playerItemsList->addItem(item);
    }
}

void Trade::updateOtherItemsDisplay()
{
    otherItemsList->clear();
    for(const auto &obj : tradeOtherObjects)
    {
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(obj.first))
        {
            const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemExtra(obj.first);
            item->setText(tr("%1 x%2").arg(QString::fromStdString(extra.name)).arg(obj.second));
            item->setIcon(QPixmap(QString::fromStdString(extra.imagePath)));
        }
        else
            item->setText(tr("Item %1 x%2").arg(obj.first).arg(obj.second));
        otherItemsList->addItem(item);
    }
    for(const auto &m : tradeOtherMonsters)
    {
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(m.monster))
        {
            item->setText(tr("%1 Lv.%2").arg(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.monster).name)).arg(m.level));
            item->setIcon(QPixmap(QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.monster).frontPath)));
        }
        else
            item->setText(tr("Monster Lv.%1").arg(m.level));
        otherItemsList->addItem(item);
    }
}
