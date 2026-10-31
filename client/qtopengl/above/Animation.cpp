#include "Animation.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libcatchchallenger/MonsterEvolution.hpp"
#include <QPainter>
#include <QWidget>
#include <iostream>

Animation::Animation() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this),
    imageIsTo(false),
    imageSize(0),
    monsterSize(120),
    state(State_Evolved)
{
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

    button=new CustomButton(":/CC/images/interface/greenbutton.png",wdialog);
    button->setText(tr("OK"));
    button->setVisible(false);

    monsterImage=new QGraphicsPixmapItem(this);

    evolutionText=new QGraphicsTextItem(this);
    evolutionText->setDefaultTextColor(Qt::white);

    animTimer.setInterval(50);
    if(!connect(&animTimer,&QTimer::timeout,this,&Animation::animationTick))
        abort();
    if(!connect(button,&CustomButton::clicked,this,&Animation::onButtonClicked))
        abort();

    title->setText(tr("Evolution"));
}

Animation::~Animation()
{
    delete wdialog;
}

QRectF Animation::boundingRect() const
{
    return QRectF();
}

void Animation::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,180));
    int idealW=500;
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
        button->setSize(148,61);
        button->setPixelSize(23);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
    }
    else
    {
        label.setScale(1);
        button->setSize(223,92);
        button->setPixelSize(35);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
    }

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    if(widget->width()<800 || widget->height()<600)
        monsterSize=60;
    else
        monsterSize=120;
    updateMonsterImage();
    const int monsterY=y+idealH/2-monsterSize/2-space;
    monsterImage->setPos(x+idealW/2-monsterSize/2,monsterY);

    // evolution text
    QFont font=evolutionText->font();
    if(widget->width()<800 || widget->height()<600)
        font.setPixelSize(10);
    else
        font.setPixelSize(16);
    evolutionText->setFont(font);
    evolutionText->setTextWidth(idealW-space*2);
    evolutionText->setPos(x+space,monsterY+monsterSize+space/2);

    // finish button
    button->setPos(idealW/2-button->width()/2,idealH-button->height()-space/2);
}

void Animation::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    button->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Animation::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    button->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Animation::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void Animation::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

QString Animation::monsterName(const uint16_t monsterId)
{
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId))
        return QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId).name);
    return QString::number(monsterId);
}

QPixmap Animation::monsterFront(const uint16_t monsterId)
{
    const QPixmap front=QtDatapackClientLoader::datapackLoader->getMonsterExtra(monsterId).front;
    if(front.isNull())
        return QPixmap(":/CC/images/monsters/default/front.png");
    return front;
}

void Animation::startEvolution(const uint16_t monsterIdFrom, const uint16_t monsterIdTo, const bool canCancel)
{
    pixFrom=monsterFront(monsterIdFrom);
    pixTo=monsterFront(monsterIdTo);
    nameFrom=monsterName(monsterIdFrom);
    nameTo=monsterName(monsterIdTo);
    state=State_Evolving;
    imageIsTo=false;
    imageSize=0;
    updateMonsterImage();
    evolutionText->setHtml(tr("%1 is evolving!").arg(nameFrom));
    button->setText(tr("Cancel"));
    button->setVisible(canCancel);
    elapsed.start();
    animTimer.start();
}

void Animation::updateMonsterImage()
{
    if(imageSize==monsterSize)
        return;
    imageSize=monsterSize;
    //pixel art: no smoothing
    if(imageIsTo)
        monsterImage->setPixmap(pixTo.scaled(monsterSize,monsterSize,Qt::KeepAspectRatio));
    else
        monsterImage->setPixmap(pixFrom.scaled(monsterSize,monsterSize,Qt::KeepAspectRatio));
}

void Animation::animationTick()
{
    const uint32_t elapsedMs=static_cast<uint32_t>(elapsed.elapsed());
    if(elapsedMs>=CatchChallenger::MonsterEvolution::animationMs)
        showEnd(State_Evolved);
    else
    {
        const bool showTo=CatchChallenger::MonsterEvolution::showEvolved(elapsedMs);
        if(showTo!=imageIsTo)
        {
            imageIsTo=showTo;
            imageSize=0;
            updateMonsterImage();
        }
    }
}

void Animation::showEnd(const State &newState)
{
    animTimer.stop();
    state=newState;
    imageIsTo=(newState==State_Evolved);
    imageSize=0;
    updateMonsterImage();
    if(newState==State_Evolved)
        evolutionText->setHtml(tr("%1 evolved into %2!").arg(nameFrom).arg(nameTo));
    else
        evolutionText->setHtml(tr("%1 stopped evolving.").arg(nameFrom));
    button->setText(tr("OK"));
    button->setVisible(true);
    if(newState==State_Evolved)
        emit evolved();
}

void Animation::onButtonClicked()
{
    if(state==State_Evolving)
        showEnd(State_Canceled);
    else
    {
        emit setAbove(nullptr);
        emit closed();
    }
}
