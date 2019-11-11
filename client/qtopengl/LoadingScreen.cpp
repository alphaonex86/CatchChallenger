#include "LoadingScreen.h"
#include "../qt/GameLoader.h"
#include "../../general/base/Version.h"

LoadingScreen::LoadingScreen()
{
    widget = new CCWidget(this);
    teacher = new QGraphicsPixmapItem(widget);
    teacher->setPixmap(QPixmap(":/CC/images/interface/teacher.png"));
    info = new QGraphicsTextItem(widget);
    info->setPlainText(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
    info->setDefaultTextColor(QColor(64,28,02));
    version = new QGraphicsTextItem(widget);
    version->setPlainText(QStringLiteral("<span style=\"color:#9090f0;\">%1</span>").arg(QString::fromStdString(CatchChallenger::Version::str)));
    //version->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont font = version->font();
    font.setPointSize(7);
    version->setFont(font);
    progressbar=new CCprogressbar(this);
    progressbar->setMaximum(100);
    progressbar->setMinimum(0);
    progressbar->setValue(0);

    if(GameLoader::gameLoader==nullptr)
        GameLoader::gameLoader=new GameLoader();
    if(!QObject::connect(GameLoader::gameLoader,&GameLoader::progression,this,&LoadingScreen::progression))
        abort();
    if(!QObject::connect(GameLoader::gameLoader,&GameLoader::dataIsParsed,this,&LoadingScreen::dataIsParsed))
        abort();

    timer.setSingleShot(true);
    if(!QObject::connect(&timer,&QTimer::timeout,this,&LoadingScreen::canBeChanged))
        abort();
    timer.start(1000);
    doTheNext=false;
}

LoadingScreen::~LoadingScreen()
{
}

/*void LoadingScreen::resizeEvent(QResizeEvent *)
{
    widget->updateGeometry();
    if(width()<400 || height()<320)
    {
        teacher->setVisible(false);
        widget->setMinimumHeight(100);
    }
    else
    {
        teacher->setVisible(true);
        widget->setMinimumHeight(260);
    }
    horizontalLayout->setContentsMargins(
                widget->currentBorderSize(),widget->currentBorderSize(),
                widget->currentBorderSize(),widget->currentBorderSize()
                );

    QFont font = version->font();
    if(height()<500)
    {
        progressbar->setMinimumHeight(0);
        font.setPointSize(9);
    }
    else if(height()<800)
    {
        progressbar->setMinimumHeight(45);
        font.setPointSize(11);
    }
    else
    {
        progressbar->setMinimumHeight(55);
        font.setPointSize(14);
    }
    version->setFont(font);
}*/

void LoadingScreen::canBeChanged()
{
    if(doTheNext)
        emit finished();
    else
        doTheNext=true;
}

void LoadingScreen::dataIsParsed()
{
    if(doTheNext)
        emit finished();
    else
    {
        doTheNext=true;
        info->setPlainText(tr("%1 is loaded").arg("<b>CatchChallenger</b>"));
    }
}

void LoadingScreen::progression(uint32_t size,uint32_t total)
{
    if(size<=total)
        progressbar->setValue(size*100/total);
    else
        progressbar->setValue(progressbar->maximum());//abort();
}

void LoadingScreen::setText(QString text)
{
    if(!text.isEmpty())
        info->setPlainText(text);
    else
        info->setPlainText(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
}

void LoadingScreen::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
}

QRectF LoadingScreen::boundingRect() const
{
    return QRectF();
}
