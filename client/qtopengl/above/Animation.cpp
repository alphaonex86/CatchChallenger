#include "Animation.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include <QPainter>
#include <QWidget>
#include <iostream>

Animation::Animation() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    animStep=0;
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

    finishButton=new CustomButton(":/CC/images/interface/greenbutton.png",wdialog);
    finishButton->setText(tr("OK"));
    finishButton->setVisible(false);

    monsterImageFrom=new QGraphicsPixmapItem(this);
    monsterImageTo=new QGraphicsPixmapItem(this);
    monsterImageTo->setVisible(false);

    evolutionText=new QGraphicsTextItem(this);
    evolutionText->setDefaultTextColor(Qt::white);

    animTimer.setInterval(50);
    if(!connect(&animTimer,&QTimer::timeout,this,&Animation::animationTick))
        abort();
    if(!connect(finishButton,&CustomButton::clicked,this,&Animation::onFinishClicked))
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

    int monsterSize=120;
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        finishButton->setSize(148,61);
        finishButton->setPixelSize(23);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        monsterSize=60;
    }
    else
    {
        label.setScale(1);
        finishButton->setSize(223,92);
        finishButton->setPixelSize(35);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
    }

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    // position monster images centered
    int monsterY=y+idealH/2-monsterSize/2-space;
    if(!pixFrom.isNull())
    {
        QPixmap scaledFrom=pixFrom.scaled(monsterSize,monsterSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        monsterImageFrom->setPixmap(scaledFrom);
    }
    monsterImageFrom->setPos(x+idealW/2-monsterSize-space/2,monsterY);

    if(!pixTo.isNull())
    {
        QPixmap scaledTo=pixTo.scaled(monsterSize,monsterSize,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        monsterImageTo->setPixmap(scaledTo);
    }
    monsterImageTo->setPos(x+idealW/2+space/2,monsterY);

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
    finishButton->setPos(idealW/2-finishButton->width()/2,idealH-finishButton->height()-space/2);
}

void Animation::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    finishButton->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Animation::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    finishButton->mouseReleaseEventXY(p,pressValidated);
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

void Animation::startEvolution(const uint16_t monsterIdFrom, const uint16_t monsterIdTo)
{
    animStep=0;
    finishButton->setVisible(false);
    monsterImageTo->setVisible(false);
    monsterImageFrom->setVisible(true);
    monsterImageFrom->setOpacity(1.0);

    // load monster images
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterIdFrom))
    {
        const std::string &path=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterIdFrom).frontPath;
        pixFrom=QPixmap(QString::fromStdString(path));
        evolutionText->setHtml(tr("Your %1 is evolving!")
            .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterIdFrom).name)));
    }
    else
    {
        pixFrom=QPixmap(":/CC/images/monsters/default/front.png");
        evolutionText->setHtml(tr("Your monster is evolving!"));
    }

    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterIdTo))
    {
        const std::string &path=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterIdTo).frontPath;
        pixTo=QPixmap(QString::fromStdString(path));
    }
    else
        pixTo=QPixmap(":/CC/images/monsters/default/front.png");

    animTimer.start();
}

void Animation::animationTick()
{
    animStep++;
    if(animStep<ANIM_TOTAL_STEPS/2)
    {
        // fade out old monster with blinking
        double opacity=1.0-((double)animStep/(ANIM_TOTAL_STEPS/2));
        if(animStep%4<2)
            monsterImageFrom->setOpacity(opacity);
        else
            monsterImageFrom->setOpacity(opacity*0.3);
    }
    else if(animStep==ANIM_TOTAL_STEPS/2)
    {
        // switch monsters
        monsterImageFrom->setVisible(false);
        monsterImageTo->setVisible(true);
        monsterImageTo->setOpacity(0.0);
    }
    else if(animStep<ANIM_TOTAL_STEPS)
    {
        // fade in new monster
        double opacity=(double)(animStep-ANIM_TOTAL_STEPS/2)/(ANIM_TOTAL_STEPS/2);
        monsterImageTo->setOpacity(opacity);
    }
    else
    {
        // animation finished
        animTimer.stop();
        monsterImageTo->setOpacity(1.0);
        finishButton->setVisible(true);

        uint16_t monsterIdTo=0;
        // update text
        if(!pixTo.isNull())
        {
            evolutionText->setHtml(tr("Your monster has evolved!"));
        }
    }
}

void Animation::onFinishClicked()
{
    animTimer.stop();
    emit animationFinished();
    emit setAbove(nullptr);
}
