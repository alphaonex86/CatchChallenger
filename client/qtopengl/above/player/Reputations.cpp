#include "Reputations.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include <QPainter>
#include <iostream>

Reputations::Reputations() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    label.setPixmap(QPixmap(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&Reputations::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    labelReputation=new QGraphicsTextItem(wdialog);

    if(!connect(&Language::language,&Language::newLanguage,this,&Reputations::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&Reputations::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&Reputations::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Reputations::sendBack,Qt::QueuedConnection))
        abort();

    newLanguage();
}

Reputations::~Reputations()
{
    delete wdialog;
    delete quit;
    delete title;
}

void Reputations::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF Reputations::boundingRect() const
{
    return QRectF();
}

void Reputations::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=labelReputation->font();
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

    labelReputation->setPos(space,space+label.pixmap().height()/2*label.scale());

    labelReputation->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void Reputations::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Reputations::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void Reputations::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Reputations::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void Reputations::newLanguage()
{
    title->setText(tr("Reputations"));
}

void Reputations::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;

    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    if(playerInformations.reputation.size()==0)
    {
        labelReputation->setHtml(tr("Empty"));
        return;
    }
    std::string html="<ul>";
    auto i=playerInformations.reputation.begin();
    while(i!=playerInformations.reputation.cend())
    {
        if(i->second.level>=0)
        {
            if((i->second.level+1)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                             CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                             ).reputation_positive.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_positive.at(i->second.level+1);
                if(next_level_xp==0)
                {
                    //error("Next level can't be need 0 xp");
                    return;
                }
                else
                {
                    std::string text=QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                            ).reputation_positive.at(i->second.level);
                    html+=QStringLiteral("<li>%1% %2</li>").arg((i->second.point*100)/next_level_xp).arg(QString::fromStdString(text)).toStdString();
                }
            }
        }
        else
        {
            if((-i->second.level)==(int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.size())
                html+=QStringLiteral("<li>100% %1</li>")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                             CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                             ).reputation_negative.back())).toStdString();
            else
            {
                int32_t next_level_xp=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).reputation_negative.at(-i->second.level);
                if(next_level_xp==0)
                {
                    //error("Next level can't be need 0 xp");
                    return;
                }
                else
                {
                    std::string text=QtDatapackClientLoader::datapackLoader->get_reputationExtra().at(
                            CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(i->first).name
                            ).reputation_negative.at(-i->second.level-1);
                    html+=QStringLiteral("<li>%1% %2</li>")
                        .arg((i->second.point*100)/next_level_xp)
                        .arg(QString::fromStdString(text)).toStdString();
                }
            }
        }
        ++i;
    }
    html+="</ul>";
    labelReputation->setHtml(QString::fromStdString(html));
}
