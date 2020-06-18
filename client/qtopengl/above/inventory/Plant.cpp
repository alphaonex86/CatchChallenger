#include "Plant.hpp"
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

Plant::Plant() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    lastItemSelected=-1;
    lastItemSize=0;
    inSelection=false;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Plant::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    plantListProxy=new QGraphicsProxyWidget(this);
    plantList=new QTreeWidget();
    plantList->setColumnCount(7);
    plantList->setHeaderHidden(true);
    plantList->setIconSize(QSize(64,64));
    plantList->setColumnWidth(0,230);
    plantList->setColumnWidth(1,0);
    plantList->setColumnWidth(2,0);
    plantList->setColumnWidth(3,0);
    plantList->setColumnWidth(4,0);
    plantList->setColumnWidth(5,270);
    plantList->setColumnWidth(6,150);
    //plantList->setText(0, QCoreApplication::translate("interfaceCopy", "Source", nullptr));
    if(!connect(plantList,&QTreeWidget::itemSelectionChanged,this,&Plant::on_inventory_itemSelectionChanged))
        abort();
    plantListProxy->setWidget(plantList);

    descriptionBack=new ImagesStrechMiddle(26,":/CC/images/interface/b1.png",this);
    inventory_description=new QGraphicsTextItem(this);
    inventoryUse=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(&Language::language,&Language::newLanguage,this,&Plant::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Plant::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Plant::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Plant::sendBack,Qt::QueuedConnection))
        abort();
    if(!connect(inventoryUse,&CustomButton::clicked,this,&Plant::inventoryUse_slot,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Plant::~Plant()
{
    delete wdialog;
    delete quit;
    delete title;
}

void Plant::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Plant::boundingRect() const
{
    return QRectF();
}

void Plant::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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
        inventoryUse->setSize(83/2,94/2);
        back->setSize(83/2,94/2);
        next->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        inventoryUse->setSize(83,94);
        back->setSize(83,94);
        next->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);
    plantListProxy->setPos(x+wdialog->currentBorderSize()+space,y+label.pixmap().height()/2+space);
    plantList->setFixedSize(idealW-space*2-wdialog->currentBorderSize()*2,idealH-label.pixmap().height()/2-space-space-inventoryUse->height()-space);
    back->setPos(widget->width()/2-label.pixmap().width()/2*label.scale()-back->width(),label.y());
    next->setPos(widget->width()/2+label.pixmap().width()/2*label.scale(),label.y());

    int yTemp=y+label.pixmap().height()/2+space+plantList->height()+space/2;
    int xTemp=x+wdialog->currentBorderSize()+space;
    xTemp+=inventoryUse->width()+space/2;
    descriptionBack->setPos(xTemp,yTemp);
    inventory_description->setPos(xTemp+descriptionBack->currentBorderSize()+space/3,yTemp+descriptionBack->currentBorderSize()+space/3);
    inventory_description->setTextWidth(descriptionBack->width()-descriptionBack->currentBorderSize()*2-space/2);
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-inventoryUse->width();
    inventoryUse->setPos(xTempUse,yTemp);
    descriptionBack->setSize(xTempUse-xTemp-space/2,inventoryUse->height());

    inventory_description->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Plant::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    inventoryUse->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Plant::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    inventoryUse->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Plant::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Plant::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Plant::newLanguage()
{
    title->setText(tr("Plant"));
}

void Plant::setVar(ConnexionManager *connexionManager, const bool inSelection, const int lastItemSelected)
{
    this->connexionManager=connexionManager;
    this->inSelection=inSelection;
    this->lastItemSelected=lastItemSelected;
    updatePlant();
}

QPixmap Plant::setCanvas(const QPixmap &image, unsigned int size)
{
    int wratio=floor((float)size/(float)image.width());
    int hratio=floor((float)size/(float)image.height());
    if(wratio<1)
        wratio=1;
    if(hratio<1)
        hratio=1;
    int minratio=wratio;
    if(minratio>hratio)
        minratio=hratio;
    QPixmap i=image.scaledToWidth(image.width()*minratio);
    QImage temp(size,size,QImage::Format_ARGB32_Premultiplied);
    temp.fill(Qt::transparent);
    QPainter paint;
    if(i.isNull())
        abort();
    paint.begin(&temp);
    paint.drawPixmap((size-i.width())/2,(size-i.height())/2,i.width(),i.height(),i);
    return QPixmap::fromImage(temp);
}

QPixmap Plant::setCanvas(const QImage &image, unsigned int size)
{
    int wratio=floor((float)size/(float)image.width());
    int hratio=floor((float)size/(float)image.height());
    if(wratio<1)
        wratio=1;
    if(hratio<1)
        hratio=1;
    int minratio=wratio;
    if(minratio>hratio)
        minratio=hratio;
    QImage i=image.scaledToWidth(image.width()*minratio);
    QImage temp(size,size,QImage::Format_ARGB32_Premultiplied);
    temp.fill(Qt::transparent);
    QPainter paint;
    if(i.isNull())
        abort();
    paint.begin(&temp);
    paint.drawImage((size-i.width())/2,(size-i.height())/2,i,0,0,i.width(),i.height());
    return QPixmap::fromImage(temp);
}

void Plant::updatePlant()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_inventory()";
    #endif
    inventory_description->setEnabled(false);
    inventoryUse->setEnabled(false);
    back->setVisible(!inSelection);
    next->setVisible(!inSelection);
    plantList->clear();
    //plantList->setIconSize(QSize(targetSize,targetSize));
    auto i=playerInformations.items.begin();
    while (i!=playerInformations.items.cend())
    {
        const uint16_t id=i->first;
        if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(id)!=QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
        {
            const DatapackClientLoader::ItemExtra &itemExtra=QtDatapackClientLoader::datapackLoader->itemsExtra.at(id);
            const QtDatapackClientLoader::QtPlantExtra &contentExtra=QtDatapackClientLoader::datapackLoader->QtplantExtra[id];
            const CatchChallenger::Plant &plant=CatchChallenger::CommonDatapack::commonDatapack.plants[id];
            QTreeWidgetItem *item=new QTreeWidgetItem();
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(id)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                QPixmap p=QtDatapackClientLoader::datapackLoader->getImageExtra(id).image;
                //Plant::setCanvas(p,64)
                p=p.scaledToWidth(p.width()*2);
                item->setIcon(0,p);

                item->setText(0,QString::fromStdString(itemExtra.name)+"\n"+tr("Quantity: ")+QString::number(i->second)+" ");

                item->setIcon(1,Plant::setCanvas(contentExtra.tileset->tileAt(0)->image(),32));
                item->setIcon(2,Plant::setCanvas(contentExtra.tileset->tileAt(1)->image(),32));
                item->setIcon(3,Plant::setCanvas(contentExtra.tileset->tileAt(2)->image(),32));
                item->setIcon(4,Plant::setCanvas(contentExtra.tileset->tileAt(3)->image(),32));
                item->setIcon(5,Plant::setCanvas(contentExtra.tileset->tileAt(4)->image(),32));

                item->setText(5,QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(plant.fruits_seconds)));
                item->setText(6,tr("Quantity: %1").arg((double)plant.fix_quantity+((double)plant.random_quantity)/RANDOM_FLOAT_PART_DIVIDER));
            }
            else
            {
                item->setIcon(0,QtDatapackClientLoader::datapackLoader->defaultInventoryImage());

                if(i->second>1)
                    item->setText(0,QStringLiteral("id: %1 (x%2)").arg(id).arg(i->second));
                else
                    item->setText(0,QStringLiteral("id: %1").arg(id));

                item->setIcon(1,contentExtra.tileset->tileAt(0)->image());
                item->setIcon(2,contentExtra.tileset->tileAt(1)->image());
                item->setIcon(3,contentExtra.tileset->tileAt(2)->image());
                item->setIcon(4,contentExtra.tileset->tileAt(3)->image());
                item->setIcon(5,contentExtra.tileset->tileAt(4)->image());

                item->setText(5,QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(plant.fruits_seconds)));
                item->setText(6,tr("Quantity: %1").arg((double)plant.fix_quantity+((double)plant.random_quantity)/RANDOM_FLOAT_PART_DIVIDER));
            }
            item->setData(0,99,id);
            plantList->addTopLevelItem(item);
            if(id==lastItemSelected)
                item->setSelected(true);
        }
        ++i;
    }
}

void Plant::inventoryUse_slot()
{
    QList<QTreeWidgetItem *> items=plantList->selectedItems();
    if(items.size()!=1)
        return;
    emit useItem(items.first()->data(0,99).toInt());
}

void Plant::on_inventory_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> items=plantList->selectedItems();
    if(items.size()!=1)
    {
        inventory_description->setHtml(tr("Select an object"));
        inventory_description->setVisible(false);
        inventoryUse->setEnabled(false);
        return;
    }
    QTreeWidgetItem *item=items.first();
    lastItemSelected=item->data(0,99).toInt();
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(item->data(0,99).toInt())==
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        inventory_description->setVisible(false);
        inventoryUse->setEnabled(false);
        inventory_description->setHtml(tr("Unknown description"));
        return;
    }
    const QtDatapackClientLoader::ItemExtra &content=QtDatapackClientLoader::datapackLoader->itemsExtra.at(item->data(0,99).toInt());
    inventory_description->setHtml(QString::fromStdString(content.description));
    //std::cout << "description: " << content.description << std::endl;

    inventory_description->setVisible(!inSelection &&
                                         /* is a plant */
                                         QtDatapackClientLoader::datapackLoader->itemToPlants.find(item->data(0,99).toInt())!=
                                         QtDatapackClientLoader::datapackLoader->itemToPlants.cend()
                                         );
    inventoryUse->setEnabled(true);
}
