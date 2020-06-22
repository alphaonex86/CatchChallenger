#include "Inventory.hpp"
#include "../../Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../cc/QtDatapackClientLoader.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <iostream>

Inventory::Inventory() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    lastItemSelected=-1;
    lastItemSize=0;
    itemCount=0;
    waitedObjectType=Inventory::ObjectType::ObjectType_All;
    inSelection=false;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Inventory::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    proxy=new QGraphicsProxyWidget(this);
    inventory=new QListWidget();
    inventory->setViewMode(QListWidget::IconMode);
    inventory->setMovement(QListWidget::Static);
    if(!connect(inventory,&QListWidget::itemSelectionChanged,this,&Inventory::on_inventory_itemSelectionChanged))
        abort();
    proxy->setWidget(inventory);

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    inventory_description=new QGraphicsTextItem(this);
    inventoryDestroy=new CustomButton(":/CC/images/interface/delete.png",this);
    inventoryUse=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(&Language::language,&Language::newLanguage,this,&Inventory::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Inventory::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Inventory::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Inventory::sendBack,Qt::QueuedConnection))
        abort();
    if(!connect(inventoryUse,&CustomButton::clicked,this,&Inventory::inventoryUse_slot,Qt::QueuedConnection))
        abort();
    if(!connect(inventoryDestroy,&CustomButton::clicked,this,&Inventory::inventoryDestroy_slot,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Inventory::~Inventory()
{
    delete wdialog;
    /*delete quit;
    delete title;*/
}

void Inventory::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Inventory::boundingRect() const
{
    return QRectF();
}

void Inventory::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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
        inventoryDestroy->setSize(83/2,94/2);
        inventoryUse->setSize(83/2,94/2);
        back->setSize(83/2,94/2);
        next->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
        updateInventory(24);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        inventoryDestroy->setSize(83,94);
        inventoryUse->setSize(83,94);
        back->setSize(83,94);
        next->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
        if(widget->width()>1400 && widget->height()>800)
        {
            if(itemCount<7)
                updateInventory(96);
            else
                updateInventory(64);
        }
        else
        {
            if(itemCount<7)
                updateInventory(48);
            else
                updateInventory(24);
        }
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);
    proxy->setPos(x+wdialog->currentBorderSize()+space,y+label.pixmap().height()/2+space);
    inventory->setFixedSize(idealW-space*2-wdialog->currentBorderSize()*2,idealH-label.pixmap().height()/2-space-space-inventoryUse->height()-space);
    back->setPos(widget->width()/2-label.pixmap().width()/2*label.scale()-back->width(),label.y());
    next->setPos(widget->width()/2+label.pixmap().width()/2*label.scale(),label.y());

    int yTemp=y+label.pixmap().height()/2+space+inventory->height()+space/2;
    int xTemp=x+wdialog->currentBorderSize()+space;
    inventoryDestroy->setPos(xTemp,yTemp);
    xTemp+=inventoryDestroy->width()+space/2;
    descriptionBack->setPos(xTemp,yTemp);
    inventory_description->setPos(xTemp+descriptionBack->currentBorderSize()+space/3,yTemp+descriptionBack->currentBorderSize()+space/3);
    inventory_description->setTextWidth(descriptionBack->width()-descriptionBack->currentBorderSize()*2-space/2);
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-inventoryUse->width();
    inventoryUse->setPos(xTempUse,yTemp);
    descriptionBack->setSize(xTempUse-xTemp-space/2,inventoryDestroy->height());

    inventory_description->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Inventory::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    inventoryDestroy->mousePressEventXY(p,pressValidated);
    inventoryUse->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Inventory::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    inventoryDestroy->mouseReleaseEventXY(p,pressValidated);
    inventoryUse->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Inventory::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Inventory::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Inventory::newLanguage()
{
    title->setText(tr("Bag","You can translate by inventory if the word is shorter"));
}

void Inventory::setVar(ConnexionManager *connexionManager, const Inventory::ObjectType &waitedObjectType, const bool inSelection,const int lastItemSelected)
{
    this->connexionManager=connexionManager;
    this->inSelection=inSelection;
    this->waitedObjectType=waitedObjectType;
    this->lastItemSelected=lastItemSelected;

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    itemCount=playerInformations.items.size();
}

void Inventory::updateInventory(uint8_t targetSize)
{
    if(lastItemSize==targetSize)
        return;
    lastItemSize=targetSize;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_inventory()";
    #endif
    inventory_description->setEnabled(false);
    inventoryUse->setEnabled(false);
    inventoryDestroy->setEnabled(false);
    back->setVisible(!inSelection);
    next->setVisible(!inSelection);
    inventory->clear();
    inventory->setIconSize(QSize(targetSize,targetSize));
    itemCount=playerInformations.items.size();
    auto i=playerInformations.items.begin();
    while (i!=playerInformations.items.cend())
    {
        const uint16_t id=i->first;
        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    //reputation requierement control is into load_plant_inventory() NOT: on_listPlantList_itemSelectionChanged()
                    if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(id)!=QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(connexionManager->client->isInFightWithWild() && CatchChallenger::CommonDatapack::commonDatapack.items.trap.find(id)!=CatchChallenger::CommonDatapack::commonDatapack.items.trap.cend())
                        show=true;
                    else if(CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.find(id)!=CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
                        show=true;
                    else
                        show=false;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            const DatapackClientLoader::ItemExtra &itemExtra=QtDatapackClientLoader::datapackLoader->itemsExtra.at(id);
            QListWidgetItem *item=new QListWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(id)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                QPixmap p=QtDatapackClientLoader::datapackLoader->getImageExtra(id).image;
                p=p.scaledToHeight(targetSize);
                item->setIcon(p);
                if(i->second>1)
                    item->setText(QString::number(i->second)+" ");
                item->setText(item->text()+QString::fromStdString(itemExtra.name));
                item->setToolTip(QString::fromStdString(itemExtra.name));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                if(i->second>1)
                    item->setText(QStringLiteral("id: %1 (x%2)").arg(id).arg(i->second));
                else
                    item->setText(QStringLiteral("id: %1").arg(id));
            }
            item->setData(99,id);
            inventory->addItem(item);
            if(id==lastItemSelected)
                item->setSelected(true);
        }
        ++i;
    }
}

void Inventory::inventoryUse_slot()
{
    QList<QListWidgetItem *> items=inventory->selectedItems();
    if(items.size()!=1)
        return;
    emit useItem(items.first()->data(99).toInt());
}

void Inventory::inventoryDestroy_slot()
{
    QList<QListWidgetItem *> items=inventory->selectedItems();
    if(items.size()!=1)
        return;
    emit deleteItem(items.first()->data(99).toInt());
}

void Inventory::on_inventory_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=inventory->selectedItems();
    if(items.size()!=1)
    {
        inventory_description->setHtml(tr("Select an object"));
        inventory_description->setVisible(false);
        inventoryUse->setEnabled(false);
        inventoryDestroy->setEnabled(false);
        return;
    }
    QListWidgetItem *item=items.first();
    lastItemSelected=item->data(99).toInt();
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(item->data(99).toInt())==
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        inventory_description->setVisible(false);
        inventoryUse->setEnabled(false);
        inventoryDestroy->setEnabled(!inSelection);
        inventory_description->setHtml(tr("Unknown description"));
        return;
    }
    const QtDatapackClientLoader::ItemExtra &content=QtDatapackClientLoader::datapackLoader->itemsExtra.at(item->data(99).toInt());
    inventoryDestroy->setEnabled(!inSelection);
    inventory_description->setHtml(QString::fromStdString(content.description));
    //std::cout << "description: " << content.description << std::endl;

    inventory_description->setVisible(!inSelection &&
                                         /* is a plant */
                                         QtDatapackClientLoader::datapackLoader->itemToPlants.find(item->data(99).toInt())!=
                                         QtDatapackClientLoader::datapackLoader->itemToPlants.cend()
                                         );
    bool isRecipe=false;
    {
        /* is a recipe */
        isRecipe=CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes
                .find(item->data(99).toInt())!=
                CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes.cend();
        if(isRecipe)
        {
            const uint16_t &recipeId=CatchChallenger::CommonDatapack::commonDatapack.itemToCraftingRecipes.at(item->data(99).toInt());
            const CatchChallenger::CraftingRecipe &recipe=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(recipeId);
            if(!connexionManager->client->haveReputationRequirements(recipe.requirements.reputation))
            {
                std::string string;
                unsigned int index=0;
                while(index<recipe.requirements.reputation.size())
                {
                    string+=CatchChallenger::FacilityLibClient::reputationRequirementsToText(recipe.requirements.reputation.at(index));
                    index++;
                }
                inventory_description->setHtml(inventory_description->toPlainText()+"<br />"+
                                                   tr("<span style=\"color:#D50000\">Don't meet the requirements: %1</span>")
                                                   .arg(QString::fromStdString(string))
                                                   );
                isRecipe=false;
            }
        }
    }
    inventoryUse->setEnabled(inSelection ||
                                 isRecipe
                                 ||
                                 /* is a repel */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.repel.find(item->data(99).toInt())!=
            CatchChallenger::CommonDatapack::commonDatapack.items.repel.cend()
                                 ||
                                 /* is a item with monster effect */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.find(item->data(99).toInt())!=
            CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.cend()
                                 ||
                                 /* is a item with monster effect out of fight */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(item->data(99).toInt())!=
            CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend() && !connexionManager->client->isInFight())
                                 ||
                                 /* is a evolution item */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(item->data(99).toInt())!=
            CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend()
                                 ||
                                 /* is a evolution item */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.find(item->data(99).toInt())!=
            CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.cend() && !connexionManager->client->isInFight())
                                         );
}
