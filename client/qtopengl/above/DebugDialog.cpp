#include "DebugDialog.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../AudioGL.hpp"
#endif
#include "../../qt/Ultimate.hpp"
#include "../Language.hpp"
#include <QDesktopServices>
#include <QLineEdit>

DebugDialog::DebugDialog() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/quit.png",this);
//    quit->updateTextPercent(75);
    connect(quit,&CustomButton::clicked,this,&DebugDialog::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);
    title->setText(tr("Debug"));

    debugText=new CCGraphicsTextItem(this);
    debugIsShow=false;
    lastUpdate.restart();
}

DebugDialog::~DebugDialog()
{
    delete wdialog;
    delete quit;
    delete title;
}

QRectF DebugDialog::boundingRect() const
{
    return QRectF();
}

void DebugDialog::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=widget->width();
    int idealH=widget->height();

    int x=0;
    int y=0;
    wdialog->setPos(x,y);
    wdialog->setSize(idealW,idealH);

    auto font=debugText->font();
    if(widget->width()<600 || widget->height()<480)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(12);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(14);
    }
    debugText->setFont(font);

    quit->setPos(x+(idealW-quit->width())/2,y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    debugText->setPos(x+wdialog->currentBorderSize()*2,y+wdialog->currentBorderSize()*2);

    if(lastUpdate.elapsed()>1000 || !debugIsShow)
    {
        debugIsShow=true;
        debugText->setHtml(QString("logicalDpiX: ")+QString::number(widget->logicalDpiX())+", "
                           "logicalDpiY: "+QString::number(widget->logicalDpiY())+"<br />"+
                           "physicalDpiX: "+QString::number(widget->physicalDpiX())+", "+
                           "physicalDpiY: "+QString::number(widget->physicalDpiY())+"<br />"+
                           "width: "+QString::number(widget->width())+", "+
                           "height: "+QString::number(widget->height())+"<br />"
                           "thread: "+QString::number(QThread::idealThreadCount())+"<br />"
                           );
        lastUpdate.restart();
    }
}

void DebugDialog::mousePressEventXY(const QPointF &p, bool &pressValidated)
{
    quit->mousePressEventXY(p,pressValidated);
}

void DebugDialog::mouseReleaseEventXY(const QPointF &p,bool &pressValidated)
{
    quit->mouseReleaseEventXY(p,pressValidated);
}
