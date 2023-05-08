#include "LoadingScreen.hpp"
#include "../../../general/base/Version.hpp"
#include "../../libqtcatchchallenger/Language.hpp"
#include <QWidget>
#include "../GameLoader.hpp"

LoadingScreen::LoadingScreen()
{
    widget = new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    teacher = new QGraphicsPixmapItem(widget);
    /*if(GameLoader::gameLoader==nullptr)
        teacher->setPixmap(QPixmap(":/CC/images/interface/teacher.png"));
    else*/
        teacher->setPixmap(QPixmap(":/CC/images/interface/teacher.png"));
    info = new QGraphicsTextItem(widget);
    info->setDefaultTextColor(QColor(64,28,02));
    version = new QGraphicsTextItem(this);
    version->setHtml(QStringLiteral("<span style=\"color:#9090f0;\">%1</span>").arg(QString::fromStdString(CatchChallenger::Version::str)));
    //version->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont font = version->font();
    font.setPixelSize(7);
    version->setFont(font);
    progressbar=new CCprogressbar(this);

    if(GameLoader::gameLoader==nullptr)
    {
        GameLoader::gameLoader=new GameLoader();
        if(!QObject::connect(GameLoader::gameLoader,&GameLoader::progression,this,&LoadingScreen::progression))
            abort();
        if(!QObject::connect(GameLoader::gameLoader,&GameLoader::dataIsParsed,this,&LoadingScreen::dataIsParsed))
            abort();
        if(!connect(&Language::language,&Language::newLanguage,this,&LoadingScreen::newLanguage,Qt::QueuedConnection))
            abort();
    }

    timer.setSingleShot(true);
    if(!QObject::connect(&timer,&QTimer::timeout,this,&LoadingScreen::canBeChanged))
        abort();
    //timer.start(1000);
    doTheNext=false;

    //slow down progression
    slowDownProgressionTimer.setSingleShot(false);
    slowDownProgressionTimer.start(10);//20x
    if(!QObject::connect(&slowDownProgressionTimer,&QTimer::timeout,this,&LoadingScreen::updateProgression))
        abort();
    reset();
}

LoadingScreen::~LoadingScreen()
{
}

void LoadingScreen::reset()
{
    info->setHtml(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
    progressbar->setMaximum(100);
    progressbar->setMinimum(0);
    progressbar->setValue(0);
    //slowDownProgressionTimer.start(10);//20x
    lastProgression=0;
    timerProgression=0;
}

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
        info->setHtml(tr("%1 is loaded").arg("<b>CatchChallenger</b>"));
    }
}

void LoadingScreen::updateProgression()
{
    if(timerProgression<100)
    {
        timerProgression+=5;
        if(timerProgression<lastProgression)
            progressbar->setValue(timerProgression);
        else
            progressbar->setValue(lastProgression);
    }
    if(timerProgression>=100)
    {
        canBeChanged();
        slowDownProgressionTimer.stop();
    }
}

void LoadingScreen::progression(uint32_t size,uint32_t total)
{
    if(total==0)
    {
        progressbar->setMinimum(0);
        progressbar->setMaximum(0);
        progressbar->setValue(0);
    }
    else if(size<=total)
    {
        lastProgression=size*100/total;
        if(timerProgression<lastProgression)
            progressbar->setValue(timerProgression);
        else
            progressbar->setValue(lastProgression);
    }
    else
        progressbar->setValue(progressbar->maximum());//abort();
}

void LoadingScreen::setText(QString text)
{
    if(!text.isEmpty())
        info->setHtml(text);
    else
        info->setHtml(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
}

void LoadingScreen::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    int progressBarHeight=82;
    if(widget->height()<800)
    {
        progressBarHeight=widget->height()/10;
        if(progressBarHeight<24)
            progressBarHeight=24;
    }
    progressbar->setPos(10,widget->height()-progressBarHeight-10);
    progressbar->setSize(widget->width()-10-10,progressBarHeight);

    if(widget->width()<800 || widget->height()<600)
    {
        int w=widget->width()-20;
        if(w>500)
            w=500;
        int h=widget->height()-40-progressBarHeight;
        if(h>100)
            h=100;
        this->widget->setWidth(w);
        this->widget->setHeight(h);
        this->widget->setPos(widget->width()/2-this->widget->width()/2,(widget->height()-progressBarHeight)/2-this->widget->height()/2);

        teacher->setVisible(false);
        info->setPos(this->widget->currentBorderSize(),h/2-info->boundingRect().height()/2);
    }
    else
    {
        if(widget->width()<500+20)
            this->widget->setWidth(widget->width()-20);
        else
            this->widget->setWidth(500);
        this->widget->setHeight(270-20);
        this->widget->setPos(widget->width()/2-this->widget->width()/2,widget->height()/2-this->widget->height()/2);

        teacher->setVisible(true);
        teacher->setPos(24,19);
        info->setPos(24+teacher->pixmap().width()+10,120);
    }

    version->setPos(widget->width()-version->shape().boundingRect().width()-10,5);
    QFont font = version->font();
    if(widget->height()<500)
        font.setPixelSize(9);
    else if(widget->height()<800)
        font.setPixelSize(11);
    else
        font.setPixelSize(14);
    version->setFont(font);
}

QRectF LoadingScreen::boundingRect() const
{
    return QRectF();
}

void LoadingScreen::newLanguage()
{
}
