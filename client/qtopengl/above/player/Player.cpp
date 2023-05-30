#include "Player.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <iostream>

Player::Player() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    label.setPixmap(QPixmap(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Player::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    name_label=new QGraphicsTextItem(wdialog);
    name_value=new QGraphicsTextItem(wdialog);
    cash_label=new QGraphicsTextItem(wdialog);
    cash_value=new QGraphicsTextItem(wdialog);
    type_label=new QGraphicsTextItem(wdialog);
    type_value=new QGraphicsTextItem(wdialog);

    if(!connect(&Language::language,&Language::newLanguage,this,&Player::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Player::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Player::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Player::sendBack,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Player::~Player()
{
    delete wdialog;
    delete quit;
    delete title;
}

void Player::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Player::boundingRect() const
{
    return QRectF();
}

void Player::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=name_value->font();
    if(widget->width()<800 || widget->height()<600)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        back->setSize(83/2,94/2);
        next->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(18/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        back->setSize(83,94);
        next->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(18);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);
    back->setPos(widget->width()/2-label.pixmap().width()/2*label.scale()-back->width(),label.y());
    next->setPos(widget->width()/2+label.pixmap().width()/2*label.scale(),label.y());

    name_label->setFont(font);
    name_value->setFont(font);
    cash_label->setFont(font);
    cash_value->setFont(font);
    type_label->setFont(font);
    type_value->setFont(font);

    int tempY=space+label.pixmap().height()/2*label.scale();
    name_label->setPos(space,tempY);
    name_value->setPos(idealW/2,tempY);
    tempY+=name_label->boundingRect().height()+space;
    cash_label->setPos(space,tempY);
    cash_value->setPos(idealW/2,tempY);
    tempY+=cash_label->boundingRect().height()+space;
    type_label->setPos(space,tempY);
    type_value->setPos(idealW/2,tempY);
    tempY+=type_label->boundingRect().height()+space;

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Player::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Player::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Player::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Player::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Player::newLanguage()
{
    title->setText(tr("Player"));

    name_label->setHtml("Name: ");
    cash_label->setHtml("Cash: ");
    type_label->setHtml("Type: ");
}

void Player::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();

    name_value->setHtml(QString::fromStdString(playerInformations.public_informations.pseudo));
    cash_value->setHtml(QString::number(playerInformations.cash));
    switch(playerInformations.public_informations.type)
    {
        default:
        case CatchChallenger::Player_type_normal:
            type_value->setHtml(tr("Normal player"));
        break;
        case CatchChallenger::Player_type_premium:
            type_value->setHtml(tr("Premium player"));
        break;
        case CatchChallenger::Player_type_dev:
            type_value->setHtml(tr("Developer player"));
        break;
        case CatchChallenger::Player_type_gm:
            type_value->setHtml(tr("Admin"));
        break;
    }
}
