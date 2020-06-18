#include "FinishedQuests.hpp"
#include "../../Language.hpp"
#include "../../ConnexionManager.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../cc/QtDatapackClientLoader.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Ultimate.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include <QPainter>
#include <iostream>

FinishedQuests::FinishedQuests() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/labelL.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",wdialog);
    connect(quit,&CustomButton::clicked,this,&FinishedQuests::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    next=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    finishedQuests=new QGraphicsTextItem(wdialog);

    if(!connect(&Language::language,&Language::newLanguage,this,&FinishedQuests::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&FinishedQuests::removeAbove,Qt::QueuedConnection))
        abort();
    if(!connect(next,&CustomButton::clicked,this,&FinishedQuests::sendNext,Qt::QueuedConnection))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&FinishedQuests::sendBack,Qt::QueuedConnection))
        abort();

    {
        QFile file(":/CC/images/interface/repeatable.png");
        if(file.open(QIODevice::ReadOnly))
        {
            imagesInterfaceRepeatableString=QStringLiteral("<img src=\"data:image/png;base64,%1\" alt=\"Repeatable\" title=\"Repeatable\" />").arg(QString(file.readAll().toBase64())).toStdString();
            file.close();
        }
    }
    {
        QFile file(":/CC/images/interface/in-progress.png");
        if(file.open(QIODevice::ReadOnly))
        {
            imagesInterfaceInProgressString=QStringLiteral("<img src=\"data:image/png;base64,%1\" alt=\"In progress\" title=\"In progress\" />").arg(QString(file.readAll().toBase64())).toStdString();
            file.close();
        }
    }

    newLanguage();
}

FinishedQuests::~FinishedQuests()
{
    delete wdialog;
    delete quit;
    delete title;
}

void FinishedQuests::removeAbove()
{
    emit setAbove(nullptr);
}

QRectF FinishedQuests::boundingRect() const
{
    return QRectF();
}

void FinishedQuests::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=finishedQuests->font();
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

    finishedQuests->setPos(space,space);
    finishedQuests->setTextWidth(idealW-space*2);

    finishedQuests->setFont(font);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void FinishedQuests::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    next->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void FinishedQuests::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    next->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void FinishedQuests::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void FinishedQuests::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
    return;
}

void FinishedQuests::newLanguage()
{
    title->setText(tr("Finisheds quests"));
}

void FinishedQuests::setVar(ConnexionManager *connexionManager)
{
    this->connexionManager=connexionManager;

    this->connexionManager=connexionManager;
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    std::string html("<ul>");
    auto i=playerInformations.quests.begin();
    while(i!=playerInformations.quests.cend())
    {
        const uint16_t questId=i->first;
        const CatchChallenger::PlayerQuest &value=i->second;
        if(QtDatapackClientLoader::datapackLoader->questsExtra.find(questId)!=QtDatapackClientLoader::datapackLoader->questsExtra.cend())
        {
            if(value.step==0 || value.finish_one_time)
            {
                if(Ultimate::ultimate.isUltimate())
                {
                    html+="<li>";
                    if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(questId).repeatable)
                        html+=imagesInterfaceRepeatableString;
                    if(value.step>0)
                        html+=imagesInterfaceInProgressString;
                    html+=QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name+"</li>";
                }
                else
                    html+="<li>"+QtDatapackClientLoader::datapackLoader->questsExtra.at(questId).name+"</li>";
            }
        }
        ++i;
    }
    html+="</ul>";
    if(html=="<ul></ul>")
        html="<i>None</i>";
    finishedQuests->setHtml(QString::fromStdString(html));
}
