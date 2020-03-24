#include "OptionsDialog.h"
#include "../qt/GameLoader.h"

OptionsDialog::OptionsDialog() :
    wdialog(new CCWidget(this)),
    label(this),
    x(-1),
    y(-1)
{
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/quit.png",this);
    quit->setOutlineColor(QColor(0,79,154));
    quit->setPointSize(28);
    quit->updateTextPercent(75);
    connect(quit,&CustomButton::clicked,this,&OptionsDialog::quitOption);
}

OptionsDialog::~OptionsDialog()
{
    delete wdialog;
    delete quit;
}

QRectF OptionsDialog::boundingRect() const
{
    return QRectF();
}

void OptionsDialog::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    int idealW=450;
    int idealH=200;
    if(idealW>widget->width())
        idealW=widget->width();
    int top=36;
    int bottom=94/2;
    if(widget->width()<600 || widget->height()<480)
    {
        top/=2;
        bottom/=2;
    }
    if((idealH+top+bottom)>widget->height())
        idealH=widget->height()-(top+bottom);

    if(x<0 || y<0)
    {
        x=widget->width()/2-idealW/2;
        y=widget->height()/2-idealH/2;
    }
    else
    {
        if((x+idealW)>widget->width())
            x=widget->width()-idealW;
        if((y+idealH+bottom)>widget->height())
            y=widget->height()-(idealH+bottom);
        if(y<top)
            y=top;
    }

    wdialog->setPos(x,y);
    wdialog->setSize(idealW,idealH);

    if(widget->width()<600 || widget->height()<480)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
    }
    quit->setPos(x+(idealW-quit->width())/2,y+idealH-quit->height()/2);
}

void OptionsDialog::mousePressEventXY(const QPointF &p)
{
    quit->mousePressEventXY(p);
}

void OptionsDialog::mouseReleaseEventXY(const QPointF &p)
{
    bool pressValidated=quit->mouseReleaseEventXY(p);
    Q_UNUSED(pressValidated);
//    pressValidated|=multi->mouseReleaseEventXY(p,pressValidated);
}
