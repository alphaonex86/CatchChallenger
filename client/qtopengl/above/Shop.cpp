#include "Shop.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../ConnexionManager.hpp"
#include "../FacilityLibClient.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include <QPainter>
#include <QWidget>
#include <QGraphicsProxyWidget>
#include <iostream>

Shop::Shop() :
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

    sellerImage=new QGraphicsPixmapItem(this);
    sellerNameText=new QGraphicsTextItem(this);
    sellerNameText->setDefaultTextColor(Qt::white);

    shopItemList=new QListWidget();
    shopListProxy=new QGraphicsProxyWidget(this);
    shopListProxy->setWidget(shopItemList);

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    shopDescription=new QGraphicsTextItem(this);
    shopBuy=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(quit,&CustomButton::clicked,this,&Shop::removeAbove))
        abort();
    if(!connect(shopBuy,&CustomButton::clicked,this,&Shop::onBuyClicked))
        abort();
    if(!connect(shopItemList,&QListWidget::itemSelectionChanged,this,&Shop::on_shopItemList_itemSelectionChanged))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Shop::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Shop::~Shop()
{
    delete wdialog;
}

void Shop::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Shop::boundingRect() const
{
    return QRectF();
}

void Shop::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    QFont font=shopDescription->font();
    int sellerSize=80;
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        shopBuy->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
        sellerSize=40;
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        shopBuy->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    // seller image and name on the left
    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    sellerImage->setPos(x+wdialog->currentBorderSize()+space,contentY);
    sellerNameText->setPos(x+wdialog->currentBorderSize()+space,contentY+sellerSize+2);
    QFont nameFont=sellerNameText->font();
    nameFont.setPixelSize(font.pixelSize());
    sellerNameText->setFont(nameFont);

    // shop list on the right of seller
    int listX=x+wdialog->currentBorderSize()+space+sellerSize+space;
    int listW=idealW-wdialog->currentBorderSize()*2-space*3-sellerSize;
    if(listW<100) { listX=x+wdialog->currentBorderSize()+space; listW=idealW-space*2-wdialog->currentBorderSize()*2; }
    int descH=shopBuy->height();
    int listH=idealH-(contentY-y)-descH-space-space/2;
    if(listH<50) listH=50;

    shopItemList->setFixedSize(listW,listH);
    shopListProxy->setPos(listX,contentY);

    // description + buy button
    int yTemp=contentY+listH+space/2;
    int xTemp=x+wdialog->currentBorderSize()+space;
    descriptionBack->setPos(xTemp,yTemp);
    shopDescription->setPos(xTemp+descriptionBack->currentBorderSize()+space/3,yTemp+descriptionBack->currentBorderSize()+space/3);
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-shopBuy->width();
    shopBuy->setPos(xTempUse,yTemp);
    descriptionBack->setSize(xTempUse-xTemp-space/2,descH);
    shopDescription->setTextWidth(descriptionBack->width()-descriptionBack->currentBorderSize()*2-space/2);
    shopDescription->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Shop::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    shopBuy->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Shop::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    shopBuy->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Shop::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Shop::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Shop::newLanguage()
{
    title->setText(tr("Shop"));
    shopBuy->setText(tr("Buy"));
    shopDescription->setHtml(tr("Waiting the shop content"));
}

void Shop::setVar(ConnexionManager *connexionManager, const std::string &sellerName, const uint8_t &skinId)
{
    this->connexionManager=connexionManager;
    shopItemList->clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    itemsIntoTheShop.clear();
    itemsToBuy.clear();
    shopBuy->setEnabled(false);

    sellerNameText->setHtml(QString::fromStdString(sellerName));

    // load seller skin
    const std::string skinPath=QtDatapackClientLoader::datapackLoader->getFrontSkinPath(skinId);
    QPixmap skin(QString::fromStdString(skinPath));
    if(!skin.isNull())
    {
        sellerImage->setPixmap(skin.scaled(80,80,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        sellerImage->setVisible(true);
    }
    else
        sellerImage->setVisible(false);

    shopDescription->setHtml(tr("Waiting the shop content"));

    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveShopList,
                this,&Shop::haveShopList,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveBuyObject,
                this,&Shop::haveBuyObject,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveSellObject,
                this,&Shop::haveSellObject,Qt::UniqueConnection))
        abort();
}

void Shop::haveShopList(const std::vector<CatchChallenger::ItemToSellOrBuy> &items)
{
    shopItemList->clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    itemsIntoTheShop.clear();

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();

    QFont disableFont;
    disableFont.setItalic(true);

    unsigned int index=0;
    while(index<items.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        const uint16_t objectId=items.at(index).object;
        shop_items_to_graphical[objectId]=item;
        shop_items_graphical[item]=objectId;
        itemsIntoTheShop[objectId]=items.at(index);

        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(objectId))
        {
            const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemExtra(objectId);
            item->setIcon(QPixmap(QString::fromStdString(extra.imagePath)));
            item->setText(tr("%1\nPrice: %2$").arg(QString::fromStdString(extra.name)).arg(items.at(index).price));
        }
        else
        {
            item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
            item->setText(tr("Item %1\nPrice: %2$").arg(objectId).arg(items.at(index).price));
        }

        if(items.at(index).price>playerInformations.cash)
        {
            item->setFont(disableFont);
            item->setForeground(QBrush(QColor(200,20,20)));
        }

        shopItemList->addItem(item);
        index++;
    }
    on_shopItemList_itemSelectionChanged();
}

void Shop::on_shopItemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=shopItemList->selectedItems();
    if(items.size()!=1)
    {
        shopDescription->setHtml(tr("Select an item"));
        shopBuy->setEnabled(false);
        return;
    }
    QListWidgetItem *item=items.first();
    if(shop_items_graphical.find(item)==shop_items_graphical.cend())
    {
        shopBuy->setEnabled(false);
        return;
    }
    uint16_t objectId=shop_items_graphical.at(item);
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(objectId))
    {
        const QtDatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemExtra(objectId);
        shopDescription->setHtml(QString::fromStdString(extra.description));
    }
    else
        shopDescription->setHtml(tr("Unknown item"));

    shopBuy->setEnabled(true);
}

void Shop::onBuyClicked()
{
    QList<QListWidgetItem *> items=shopItemList->selectedItems();
    if(items.size()!=1)
        return;
    QListWidgetItem *item=items.first();
    if(shop_items_graphical.find(item)==shop_items_graphical.cend())
        return;
    uint16_t objectId=shop_items_graphical.at(item);
    if(itemsIntoTheShop.find(objectId)==itemsIntoTheShop.cend())
        return;

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    const CatchChallenger::ItemToSellOrBuy &shopItem=itemsIntoTheShop.at(objectId);
    uint32_t price=shopItem.price;

    if(playerInformations.cash<price)
    {
        emit showTip(tr("Not enough cash").toStdString());
        return;
    }

    uint32_t quantity=1;
    // buy the item
    CatchChallenger::ItemToSellOrBuy buyEntry;
    buyEntry.object=objectId;
    buyEntry.quantity=quantity;
    buyEntry.price=price*quantity;
    itemsToBuy.push_back(buyEntry);
    emit removeCash(buyEntry.price);
    connexionManager->client->buyObject(objectId,quantity,price);
}

void Shop::haveBuyObject(const CatchChallenger::BuyStat &stat, const uint32_t &newPrice)
{
    if(itemsToBuy.empty())
        return;
    const CatchChallenger::ItemToSellOrBuy &itemToSellOrBuy=itemsToBuy.front();
    switch(stat)
    {
        case CatchChallenger::BuyStat_Done:
            emit add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity,true);
        break;
        case CatchChallenger::BuyStat_BetterPrice:
            if(newPrice==0)
                return;
            emit addCash(itemToSellOrBuy.price);
            emit removeCash(newPrice*itemToSellOrBuy.quantity);
            emit add_to_inventory(itemToSellOrBuy.object,itemToSellOrBuy.quantity,true);
        break;
        case CatchChallenger::BuyStat_HaveNotQuantity:
            emit addCash(itemToSellOrBuy.price);
            emit showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::BuyStat_PriceHaveChanged:
            emit addCash(itemToSellOrBuy.price);
            emit showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
        break;
    }
    itemsToBuy.erase(itemsToBuy.cbegin());
}

void Shop::haveSellObject(const CatchChallenger::SoldStat &stat, const uint32_t &newPrice)
{
    Q_UNUSED(stat);
    Q_UNUSED(newPrice);
    // selling handled via inventory selection, not directly in shop screen
}
