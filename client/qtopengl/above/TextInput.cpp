#include "TextInput.hpp"
#include <QPainter>
#include <QWidget>

TextInput::TextInput() :
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
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

    input=new LineEdit(this);

    okButton=new CustomButton(":/CC/images/interface/validate.png",this);

    if(!connect(quit,&CustomButton::clicked,this,&TextInput::onCancel))
        abort();
    if(!connect(okButton,&CustomButton::clicked,this,&TextInput::onValidate))
        abort();
    if(!connect(input,&LineEdit::returnPressed,this,&TextInput::onValidate))
        abort();
}

TextInput::~TextInput()
{
    delete wdialog;
}

void TextInput::onCancel()
{
    emit canceled();
    emit setAbove(nullptr);
}

void TextInput::onValidate()
{
    const QString text=input->text().trimmed();
    emit setAbove(nullptr);
    if(text.isEmpty())
        emit canceled();
    else
        emit accepted(text);
}

QRectF TextInput::boundingRect() const
{
    return QRectF();
}

void TextInput::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    int idealW=600;
    int idealH=220;
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
        okButton->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
    }
    else
    {
        label.setScale(1);
        quit->setSize(83,94);
        okButton->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
    }

    quit->setPos(idealW-quit->width()/1.5,-quit->height()/2.5);

    int contentY=y+label.pixmap().height()/2*label.scale()+space;
    int inputX=x+wdialog->currentBorderSize()+space;
    int inputW=idealW-wdialog->currentBorderSize()*2-space*2;
    input->setFixedSize(inputW,okButton->height()/2>24?okButton->height()/2:24);
    input->setPos(inputX,contentY);

    int yTemp=contentY+input->size().height()+space/2;
    int xTempUse=x+idealW-wdialog->currentBorderSize()-space-okButton->width();
    okButton->setPos(xTempUse,yTemp);

    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);
}

void TextInput::mousePressEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    quit->mousePressEventXY(p,pressValidated);
    okButton->mousePressEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void TextInput::mouseReleaseEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    okButton->mouseReleaseEventXY(p,pressValidated);
    pressValidated=true;
    callParentClass=true;
}

void TextInput::keyPressEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void TextInput::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    eventTriggerGeneral=true;
}

void TextInput::setVar(const QString &titleText,const QString &placeholder,const int maxLength)
{
    title->setText(titleText);
    input->clear();
    input->setPlaceholderText(placeholder);
    if(maxLength>0)
        input->setMaxLength(maxLength);
}
