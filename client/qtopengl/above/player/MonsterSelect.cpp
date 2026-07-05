#include "MonsterSelect.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <QWidget>

MonsterSelect::MonsterSelect() :
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

    monsterList=new QListWidget();
    listProxy=new QGraphicsProxyWidget(this);
    listProxy->setWidget(monsterList);

    selectButton=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(quit,&CustomButton::clicked,this,&MonsterSelect::onCancel))
        abort();
    if(!connect(selectButton,&CustomButton::clicked,this,&MonsterSelect::onSelectClicked))
        abort();
    if(!connect(monsterList,&QListWidget::itemSelectionChanged,this,&MonsterSelect::on_list_itemSelectionChanged))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&MonsterSelect::newLanguage,Qt::QueuedConnection))
        abort();

    newLanguage();
}

MonsterSelect::~MonsterSelect()
{
    delete wdialog;
}

void MonsterSelect::onCancel()
{
    emit canceled();
    emit setAbove(nullptr);
}

QRectF MonsterSelect::boundingRect() const
{
    return QRectF();
}

void MonsterSelect::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=700;
    int idealH=400;
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

    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        selectButton->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        selectButton->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    int descH=selectButton->height();
    int listH=idealH-(contentY-y)-descH-space-space/2;
    if(listH<50)
        listH=50;
    int listX=x+wdialog->currentBorderSize()+space;
    int listW=idealW-wdialog->currentBorderSize()*2-space*2;
    monsterList->setFixedSize(listW,listH);
    listProxy->setPos(listX,contentY);

    int yTemp=contentY+listH+space/2;
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-selectButton->width();
    selectButton->setPos(xTempUse,yTemp);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void MonsterSelect::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    selectButton->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void MonsterSelect::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    selectButton->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void MonsterSelect::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void MonsterSelect::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void MonsterSelect::newLanguage()
{
    title->setText(tr("Select a monster"));
    selectButton->setText(tr("Select"));
}

void MonsterSelect::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;
    monsterList->clear();
    selectButton->setEnabled(false);

    const CatchChallenger::Player_private_and_public_informations &info=connexionManager->client->get_player_informations_ro();
    unsigned int index=0;
    while(index<info.monsters.size())
    {
        const CatchChallenger::PlayerMonster &m=info.monsters.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(m.monster))
        {
            item->setIcon(QtDatapackClientLoader::datapackLoader->getMonsterExtra(m.monster).front);
            item->setText(tr("%1\nLevel %2 - HP %3")
                          .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.monster).name))
                          .arg(m.level).arg(static_cast<qulonglong>(m.hp)));
        }
        else
            item->setText(tr("Monster %1\nLevel %2 - HP %3").arg(m.monster).arg(m.level).arg(static_cast<qulonglong>(m.hp)));
        item->setData(99,index);
        monsterList->addItem(item);
        index++;
    }
}

void MonsterSelect::on_list_itemSelectionChanged()
{
    selectButton->setEnabled(monsterList->selectedItems().size()==1);
}

void MonsterSelect::onSelectClicked()
{
    const QList<QListWidgetItem *> items=monsterList->selectedItems();
    if(items.size()!=1)
        return;
    const uint8_t position=static_cast<uint8_t>(items.first()->data(99).toInt());
    emit setAbove(nullptr);
    emit useMonster(position);
}
