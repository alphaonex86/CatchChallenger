#include "Crafting.hpp"
#include "../../Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../cc/QtDatapackClientLoader.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/tiled/tiled_tile.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <QImage>
#include <iostream>
#include <math.h>

Crafting::Crafting() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    lastItemSelected=-1;
    lastItemSize=0;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Crafting::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    products_title=new QGraphicsTextItem(this);
    materials_title=new QGraphicsTextItem();

    listCraftingList=new QListWidget();
    productsListProxy=new QGraphicsProxyWidget(this);
    productsListProxy->setWidget(listCraftingList);
    listCraftingMaterials=new QListWidget();
    materialsListProxy=new QGraphicsProxyWidget(this);
    materialsListProxy->setWidget(listCraftingMaterials);
    if(!connect(listCraftingList,&QListWidget::itemSelectionChanged,this,&Crafting::on_listCraftingList_itemSelectionChanged))
        abort();

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    inventory_description=new QGraphicsTextItem(this);
    craftingUse=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(&Language::language,&Language::newLanguage,this,&Crafting::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Crafting::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Crafting::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Crafting::sendBack,Qt::QueuedConnection))
        abort();
    if(!connect(craftingUse,&CustomButton::clicked,this,&Crafting::inventoryUse_slot,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Crafting::~Crafting()
{
    delete wdialog;
    //delete quit;
    //delete title;
}

void Crafting::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Crafting::boundingRect() const
{
    return QRectF();
}

void Crafting::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=inventory_description->font();
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        craftingUse->setSize(83/2,94/2);
        back->setSize(83/2,94/2);
        next->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        craftingUse->setSize(83,94);
        back->setSize(83,94);
        next->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    products_title->setPos(x+idealW/4-products_title->boundingRect().width()-2,y+label.pixmap().height()/2+space-products_title->boundingRect().height());
    materials_title->setPos(x+idealW*3/4-products_title->boundingRect().width()-2,y+label.pixmap().height()/2+space-products_title->boundingRect().height());

    listCraftingList->setFixedSize(idealW/2-space-wdialog->currentBorderSize()-space/2,idealH-label.pixmap().height()/2-space-space-craftingUse->height()-space);
    productsListProxy->setPos(x+wdialog->currentBorderSize()+space,y+label.pixmap().height()/2+space);
    listCraftingMaterials->setFixedSize(idealW/2-space-wdialog->currentBorderSize()-space/2,idealH-label.pixmap().height()/2-space-space-craftingUse->height()-space);
    materialsListProxy->setPos(widget->width()/2+space/2,y+label.pixmap().height()/2+space);

    back->setPos(widget->width()/2-label.pixmap().width()/2*label.scale()-back->width(),label.y());
    next->setPos(widget->width()/2+label.pixmap().width()/2*label.scale(),label.y());

    int yTemp=y+label.pixmap().height()/2+space+listCraftingList->height()+space/2;
    int xTemp=x+wdialog->currentBorderSize()+space;
    xTemp+=craftingUse->width()+space/2;
    descriptionBack->setPos(xTemp,yTemp);
    inventory_description->setPos(xTemp+descriptionBack->currentBorderSize()+space/3,yTemp+descriptionBack->currentBorderSize()+space/3);
    inventory_description->setTextWidth(descriptionBack->width()-descriptionBack->currentBorderSize()*2-space/2);
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-craftingUse->width();
    craftingUse->setPos(xTempUse,yTemp);
    descriptionBack->setSize(xTempUse-xTemp-space/2,craftingUse->height());

    inventory_description->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Crafting::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    craftingUse->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Crafting::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    craftingUse->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Crafting::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Crafting::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Crafting::newLanguage()
{
    title->setText(tr("Crafting"));
    products_title->setHtml(tr("Products"));
    materials_title->setHtml(tr("Materials"));
}

void Crafting::setVar(ConnexionManager *connexionManager,int lastItemSelected)
{
    this->connexionManager=connexionManager;
    this->lastItemSelected=lastItemSelected;
    connect(connexionManager->client,&CatchChallenger::Api_protocol_Qt::QtrecipeUsed,this,&Crafting::recipeUsed,Qt::UniqueConnection);
    /*materialOfRecipeInUsing.clear();
    productOfRecipeInUsing.clear();*/
    updateCrafting();
}

void Crafting::updateCrafting()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_crafting_inventory()";
    #endif
    listCraftingList->clear();
    if(playerInformations.recipes==NULL)
    {
        qDebug() << "BaseWindow::load_crafting_inventory(), crafting null";
        return;
    }
    uint16_t index=0;
    while(index<=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipesMaxId)
    {
        uint16_t recipe=index;
        if(playerInformations.recipes[recipe/8] & (1<<(7-recipe%8)))
        {
            //load the material item
            if(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.find(recipe)
                    !=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.cend())
            {
                const uint16_t &itemId=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[recipe].doItemId;
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(itemId)!=
                        QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->getImageExtra(itemId).image);
                    item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[itemId].name));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                    item->setText(tr("Unknow item: %1").arg(itemId));
                }
                item->setData(99,recipe);
                item->setData(98,itemId);
                listCraftingList->addItem(item);
            }
            else
                qDebug() << QStringLiteral("BaseWindow::load_crafting_inventory(), crafting id not found into crafting recipe").arg(recipe);
        }
        ++index;
    }
    on_listCraftingList_itemSelectionChanged();
}

void Crafting::inventoryUse_slot()
{
    QList<QListWidgetItem *> items=listCraftingList->selectedItems();
    if(items.size()!=1)
        return;

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    //recipeInUsing
    QList<QListWidgetItem *> displayedItems=listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
        return;
    QListWidgetItem *selectedItem=displayedItems.first();
    const CatchChallenger::CraftingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[items.first()->data(99).toInt()];

    QStringList mIngredients;
    /*QString mRecipe;
    QString mProduct;*/
    //load the materials
    unsigned int index=0;
    while(index<content.materials.size())
    {
        if(playerInformations.items.find(content.materials.at(index).item)==playerInformations.items.cend())
            return;
        if(playerInformations.items.at(content.materials.at(index).item)<content.materials.at(index).quantity)
            return;
        uint32_t sub_index=0;
        while(sub_index<content.materials.at(index).quantity)
        {
            mIngredients.push_back(QUrl::fromLocalFile(
                                       QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].imagePath)
                                   ).toEncoded());
            sub_index++;
        }
        index++;
    }
    index=0;
    std::vector<std::pair<uint16_t,uint32_t> > recipeUsage;
    while(index<content.materials.size())
    {
        std::pair<uint16_t,uint32_t> pair;
        pair.first=content.materials.at(index).item;
        pair.second=content.materials.at(index).quantity;
        emit remove_to_inventory(pair.first,pair.second);
        recipeUsage.push_back(pair);
        index++;
    }
    materialOfRecipeInUsing.push_back(recipeUsage);
    //the product do
    std::pair<uint16_t,uint32_t> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing.push_back(pair);
    /*mProduct=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].imagePath)).toEncoded();
    mRecipe=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.itemToLearn].imagePath)).toEncoded();*/
    //update the UI
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    connexionManager->client->useRecipe(items.first()->data(99).toInt());
    appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(items.first()->data(99).toInt()).rewards.reputation);
}

void Crafting::on_listCraftingList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=listCraftingList->selectedItems();
    if(items.size()!=1)
    {
        inventory_description->setHtml(tr("Select an object"));
        inventory_description->setVisible(false);
        craftingUse->setEnabled(false);
        return;
    }
    QListWidgetItem *item=items.first();
    lastItemSelected=item->data(99).toInt();
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(item->data(99).toInt())==
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        inventory_description->setVisible(false);
        craftingUse->setEnabled(false);
        inventory_description->setHtml(tr("Unknown description"));
        return;
    }
    const QtDatapackClientLoader::ItemExtra &contentItem=QtDatapackClientLoader::datapackLoader->itemsExtra.at(item->data(99).toInt());
    inventory_description->setHtml(QString::fromStdString(contentItem.description));
    //std::cout << "description: " << content.description << std::endl;

    /*inventory_description->setVisible(!inSelection &&
                                         QtDatapackClientLoader::datapackLoader->itemToCraftings.find(item->data(99).toInt())!=
                                         QtDatapackClientLoader::datapackLoader->itemToCraftings.cend()
                                         );*/
    craftingUse->setEnabled(true);

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    listCraftingMaterials->clear();
    QList<QListWidgetItem *> displayedItems=listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
    {
        inventory_description->setHtml(tr("Select a recipe"));
        return;
    }
    QListWidgetItem *itemMaterials=displayedItems.first();
    const CatchChallenger::CraftingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(items.first()->data(99).toInt());

    qDebug() << "on_listCraftingList_itemSelectionChanged() load the name";
    //load the name
    QString description;
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.doItemId)!=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        description=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(content.doItemId).description);
    else
        description=tr("Unknow item (%1)").arg(content.doItemId);
    inventory_description->setHtml(description);

    //load the materials
    bool haveMaterials=true;
    unsigned int index=0;
    QString nameMaterials;
    uint32_t quantity;
    QFont disableIntoListFont;
    QBrush disableIntoListBrush;
    disableIntoListFont.setItalic(true);
    disableIntoListBrush=QBrush(QColor(200,20,20));
    while(index<content.materials.size())
    {
        //load the material item
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.materials.at(index).item)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            nameMaterials=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].name);
            item->setIcon(QtDatapackClientLoader::datapackLoader->getImageExtra(content.materials.at(index).item).image);
        }
        else
        {
            nameMaterials=tr("Unknow item (%1)").arg(content.materials.at(index).item);
            item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(playerInformations.items.find(content.materials.at(index).item)!=playerInformations.items.cend())
            quantity=playerInformations.items.at(content.materials.at(index).item);

        //load the display
        item->setText(tr("Needed: %1 %2\nIn the inventory: %3 %4").arg(content.materials.at(index).quantity).arg(nameMaterials).arg(quantity).arg(nameMaterials));
        if(quantity<content.materials.at(index).quantity)
        {
            item->setFont(disableIntoListFont);
            item->setForeground(disableIntoListBrush);
        }

        if(quantity<content.materials.at(index).quantity)
            haveMaterials=false;

        listCraftingMaterials->addItem(item);
        craftingUse->setEnabled(haveMaterials);
        index++;
    }
}


void Crafting::recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage)
{
    switch(recipeUsage)
    {
        case CatchChallenger::RecipeUsage_ok:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            add_to_inventory(productOfRecipeInUsing.front().first,productOfRecipeInUsing.front().second);
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            on_listCraftingList_itemSelectionChanged();
        break;
        case CatchChallenger::RecipeUsage_impossible:
        {
            unsigned int index=0;
            while(index<materialOfRecipeInUsing.front().size())
            {
                add_to_inventory(materialOfRecipeInUsing.front().at(index).first,
                                 materialOfRecipeInUsing.front().at(index).first,false);
                index++;
            }
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            on_listCraftingList_itemSelectionChanged();
        }
        break;
        case CatchChallenger::RecipeUsage_failed:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
        break;
        default:
        qDebug() << "recipeUsed() unknow code";
        return;
    }
}

