#include "Factory.hpp"
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
#include <QDateTime>

Factory::Factory() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    connexionManager=nullptr;
    m_factoryId=0;
    factoryInProduction=false;

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

    resourcesTitle=new QGraphicsTextItem(this);
    resourcesTitle->setDefaultTextColor(Qt::white);
    productsTitle=new QGraphicsTextItem(this);
    productsTitle->setDefaultTextColor(Qt::white);

    factoryResources=new QListWidget();
    resourcesProxy=new QGraphicsProxyWidget(this);
    resourcesProxy->setWidget(factoryResources);
    factoryProducts=new QListWidget();
    productsProxy=new QGraphicsProxyWidget(this);
    productsProxy->setWidget(factoryProducts);

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    factoryDescription=new QGraphicsTextItem(this);
    factoryStatus=new QGraphicsTextItem(this);
    factoryStatus->setDefaultTextColor(Qt::white);

    buyButton=new CustomButton(":/CC/images/interface/validate.png",this);
    sellButton=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(quit,&CustomButton::clicked,this,&Factory::removeAbove))
        abort();
    if(!connect(buyButton,&CustomButton::clicked,this,&Factory::onBuyClicked))
        abort();
    if(!connect(sellButton,&CustomButton::clicked,this,&Factory::onSellClicked))
        abort();
    if(!connect(factoryResources,&QListWidget::itemSelectionChanged,this,&Factory::on_factoryResources_itemSelectionChanged))
        abort();
    if(!connect(factoryProducts,&QListWidget::itemSelectionChanged,this,&Factory::on_factoryProducts_itemSelectionChanged))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&Factory::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Factory::~Factory()
{
    delete wdialog;
}

void Factory::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Factory::boundingRect() const
{
    return QRectF();
}

void Factory::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    QFont font=factoryDescription->font();
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        buyButton->setSize(83/2,94/2);
        sellButton->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        buyButton->setSize(83,94);
        sellButton->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    int halfW=idealW/2-space-wdialog->currentBorderSize()-space/4;
    int btnH=buyButton->height();
    int statusH=font.pixelSize()*2;
    int listH=idealH-(contentY-y)-btnH-statusH-space*2;
    if(listH<50) listH=50;

    // resources title + list (left)
    QFont titleFont=resourcesTitle->font();
    titleFont.setPixelSize(font.pixelSize());
    resourcesTitle->setFont(titleFont);
    resourcesTitle->setPos(x+wdialog->currentBorderSize()+space,contentY-font.pixelSize()-2);
    factoryResources->setFixedSize(halfW,listH);
    resourcesProxy->setPos(x+wdialog->currentBorderSize()+space,contentY);

    // products title + list (right)
    productsTitle->setFont(titleFont);
    productsTitle->setPos(widget->width()/2+space/2,contentY-font.pixelSize()-2);
    factoryProducts->setFixedSize(halfW,listH);
    productsProxy->setPos(widget->width()/2+space/2,contentY);

    // sell button under resources
    sellButton->setPos(x+wdialog->currentBorderSize()+space,contentY+listH+space/2);
    // buy button under products
    buyButton->setPos(x+idealW-wdialog->currentBorderSize()-space-buyButton->width(),contentY+listH+space/2);

    // status text
    QFont statusFont=factoryStatus->font();
    statusFont.setPixelSize(font.pixelSize());
    factoryStatus->setFont(statusFont);
    factoryStatus->setPos(x+idealW/2-factoryStatus->boundingRect().width()/2,contentY+listH+space/2);

    // description
    int descY=contentY+listH+space/2+btnH+space/4;
    descriptionBack->setPos(x+wdialog->currentBorderSize()+space,descY);
    descriptionBack->setSize(idealW-space*2-wdialog->currentBorderSize()*2,statusH);
    factoryDescription->setPos(x+wdialog->currentBorderSize()+space+descriptionBack->currentBorderSize(),
                               descY+descriptionBack->currentBorderSize());
    factoryDescription->setTextWidth(idealW-space*2-wdialog->currentBorderSize()*2-descriptionBack->currentBorderSize()*2);
    factoryDescription->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Factory::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    buyButton->mousePressEventXY(p,pressValidated);
    sellButton->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Factory::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    buyButton->mouseReleaseEventXY(p,pressValidated);
    sellButton->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Factory::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Factory::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Factory::newLanguage()
{
    title->setText(tr("Factory"));
    resourcesTitle->setHtml(tr("Resources (sell)"));
    productsTitle->setHtml(tr("Products (buy)"));
    buyButton->setText(tr("Buy"));
    sellButton->setText(tr("Sell"));
    factoryDescription->setHtml(tr("Select an item"));
}

void Factory::setVar(ConnexionManager *connexionManager, const uint16_t factoryId)
{
    this->connexionManager=connexionManager;
    this->m_factoryId=factoryId;
    factoryResources->clear();
    factoryProducts->clear();
    itemsToBuy.clear();
    itemsToSell.clear();
    factoryInProduction=false;
    buyButton->setEnabled(false);
    sellButton->setEnabled(false);
    factoryDescription->setHtml(tr("Loading..."));
    factoryStatus->setHtml(QString());

    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveFactoryList,
                this,&Factory::haveFactoryList,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveBuyFactoryObject,
                this,&Factory::haveBuyFactoryObject,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QthaveSellFactoryObject,
                this,&Factory::haveSellFactoryObject,Qt::UniqueConnection))
        abort();
}

void Factory::haveFactoryList(const uint32_t &remainingProductionTime, const std::vector<CatchChallenger::ItemToSellOrBuy> &resources, const std::vector<CatchChallenger::ItemToSellOrBuy> &products)
{
    Q_UNUSED(remainingProductionTime);

    factoryResources->clear();
    unsigned int index=0;
    while(index<resources.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,resources.at(index).object);
        item->setData(98,resources.at(index).price);
        item->setData(97,resources.at(index).quantity);
        factoryToResourceItem(item);
        factoryResources->addItem(item);
        index++;
    }

    factoryProducts->clear();
    index=0;
    while(index<products.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,products.at(index).object);
        item->setData(98,products.at(index).price);
        item->setData(97,products.at(index).quantity);
        factoryToProductItem(item);
        factoryProducts->addItem(item);
        index++;
    }

    if(remainingProductionTime>0)
    {
        factoryInProduction=true;
        factoryStatus->setHtml(tr("In production"));
    }
    else
    {
        factoryInProduction=false;
        factoryStatus->setHtml(tr("Production stopped"));
    }
    factoryDescription->setHtml(tr("Select an item"));
}

void Factory::factoryToResourceItem(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    QFont missingFont;
    missingFont.setItalic(true);

    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));

    const uint16_t itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
    {
        item->setIcon(QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).imagePath)));
        item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        item->setToolTip(tr("Item %1\nPrice: %2$").arg(itemId).arg(item->data(98).toUInt()));
    }

    if(playerInformations.items.find(itemId)==playerInformations.items.cend())
    {
        item->setFont(missingFont);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void Factory::factoryToProductItem(QListWidgetItem *item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    QFont missingFont;
    missingFont.setItalic(true);

    if(item->data(97).toUInt()>1)
        item->setText(QStringLiteral("%1 at %2$").arg(item->data(97).toUInt()).arg(item->data(98).toUInt()));
    else
        item->setText(QStringLiteral("%1$").arg(item->data(98).toUInt()));

    const uint16_t itemId=static_cast<uint16_t>(item->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
    {
        item->setIcon(QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).imagePath)));
        item->setToolTip(tr("%1\nPrice: %2$").arg(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(item->data(98).toUInt()));
    }
    else
    {
        item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        item->setToolTip(tr("Item %1\nPrice: %2$").arg(itemId).arg(item->data(98).toUInt()));
    }

    if(item->data(98).toUInt()>playerInformations.cash)
    {
        item->setFont(missingFont);
        item->setForeground(QBrush(QColor(200,20,20)));
    }
}

void Factory::on_factoryResources_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=factoryResources->selectedItems();
    if(items.size()!=1)
    {
        factoryDescription->setHtml(tr("Select a resource to sell"));
        sellButton->setEnabled(false);
        return;
    }
    uint16_t itemId=static_cast<uint16_t>(items.first()->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
        factoryDescription->setHtml(tr("%1 - Price: %2$").arg(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(items.first()->data(98).toUInt()));
    else
        factoryDescription->setHtml(tr("Price: %1$").arg(items.first()->data(98).toUInt()));
    sellButton->setEnabled(true);
}

void Factory::on_factoryProducts_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=factoryProducts->selectedItems();
    if(items.size()!=1)
    {
        factoryDescription->setHtml(tr("Select a product to buy"));
        buyButton->setEnabled(false);
        return;
    }
    uint16_t itemId=static_cast<uint16_t>(items.first()->data(99).toUInt());
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId))
        factoryDescription->setHtml(tr("%1 - Price: %2$").arg(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId).name)).arg(items.first()->data(98).toUInt()));
    else
        factoryDescription->setHtml(tr("Price: %1$").arg(items.first()->data(98).toUInt()));
    buyButton->setEnabled(true);
}

void Factory::onBuyClicked()
{
    QList<QListWidgetItem *> items=factoryProducts->selectedItems();
    if(items.size()!=1)
        return;
    QListWidgetItem *item=items.first();
    uint16_t id=static_cast<uint16_t>(item->data(99).toUInt());
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    if(playerInformations.cash<price)
    {
        emit showTip(tr("Not enough cash").toStdString());
        return;
    }

    uint32_t quantityToBuy=1;
    quantity-=quantityToBuy;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
        factoryToProductItem(item);

    CatchChallenger::ItemToSellOrBuy buyEntry;
    buyEntry.object=id;
    buyEntry.quantity=quantityToBuy;
    buyEntry.price=quantityToBuy*price;
    itemsToBuy.push_back(buyEntry);
    emit removeCash(buyEntry.price);
    connexionManager->client->buyFactoryProduct(id,quantityToBuy,price);
}

void Factory::onSellClicked()
{
    QList<QListWidgetItem *> items=factoryResources->selectedItems();
    if(items.size()!=1)
        return;
    QListWidgetItem *item=items.first();
    uint16_t itemId=static_cast<uint16_t>(item->data(99).toUInt());
    uint32_t price=item->data(98).toUInt();
    uint32_t quantity=item->data(97).toUInt();

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    if(playerInformations.items.find(itemId)==playerInformations.items.cend())
    {
        emit showTip(tr("You don't have this item").toStdString());
        return;
    }

    uint32_t quantityToSell=1;
    quantity-=quantityToSell;
    item->setData(97,quantity);
    if(quantity<1)
        delete item;
    else
        factoryToResourceItem(item);

    CatchChallenger::ItemToSellOrBuy sellEntry;
    sellEntry.object=itemId;
    sellEntry.quantity=quantityToSell;
    sellEntry.price=price;
    itemsToSell.push_back(sellEntry);
    emit remove_to_inventory(itemId,quantityToSell);
    connexionManager->client->sellFactoryResource(itemId,quantityToSell,price);
}

void Factory::haveBuyFactoryObject(const CatchChallenger::BuyStat &stat, const uint32_t &newPrice)
{
    if(itemsToBuy.empty())
        return;
    const CatchChallenger::ItemToSellOrBuy &item=itemsToBuy.front();
    switch(stat)
    {
        case CatchChallenger::BuyStat_Done:
            emit add_to_inventory(item.object,item.quantity,true);
        break;
        case CatchChallenger::BuyStat_BetterPrice:
            if(newPrice==0) return;
            emit addCash(item.price);
            emit removeCash(newPrice*item.quantity);
            emit add_to_inventory(item.object,item.quantity,true);
        break;
        case CatchChallenger::BuyStat_HaveNotQuantity:
            emit addCash(item.price);
            emit showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::BuyStat_PriceHaveChanged:
            emit addCash(item.price);
            emit showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
        break;
    }
    itemsToBuy.erase(itemsToBuy.cbegin());
}

void Factory::haveSellFactoryObject(const CatchChallenger::SoldStat &stat, const uint32_t &newPrice)
{
    if(itemsToSell.empty())
        return;
    const CatchChallenger::ItemToSellOrBuy &item=itemsToSell.front();
    switch(stat)
    {
        case CatchChallenger::SoldStat_Done:
            emit addCash(item.price*item.quantity);
            emit showTip(tr("Item sold").toStdString());
        break;
        case CatchChallenger::SoldStat_BetterPrice:
            if(newPrice==0) return;
            emit addCash(newPrice*item.quantity);
            emit showTip(tr("Item sold at better price").toStdString());
        break;
        case CatchChallenger::SoldStat_WrongQuantity:
            emit add_to_inventory(item.object,item.quantity,false);
            emit showTip(tr("Sorry but have not the quantity of this item").toStdString());
        break;
        case CatchChallenger::SoldStat_PriceHaveChanged:
            emit add_to_inventory(item.object,item.quantity,false);
            emit showTip(tr("Sorry but now the price is worse").toStdString());
        break;
        default:
        break;
    }
    itemsToSell.erase(itemsToSell.cbegin());
}
