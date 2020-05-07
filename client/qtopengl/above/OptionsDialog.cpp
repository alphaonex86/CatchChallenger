#include "OptionsDialog.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#include "../AudioGL.hpp"
#include "../../qt/Ultimate.hpp"
#include "../Language.hpp"
#include <QDesktopServices>
#include <QLineEdit>

OptionsDialog::OptionsDialog() :
    wdialog(new CCWidget(this)),
    label(this)
{
    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/quit.png",this);
//    quit->updateTextPercent(75);
    connect(quit,&CustomButton::clicked,this,&OptionsDialog::removeAbove);
    title=new CCDialogTitle(this);
    title->setText(tr("Options"));

    volumeText=new CCGraphicsTextItem(this);
    volumeSlider=new CCSliderH(this);
    volumeSlider->setValue(Settings::settings->value("audioVolume").toUInt());
    productKeyText=new QGraphicsTextItem(this);
    QPixmap p=*GameLoader::gameLoader->getImage(":/CC/images/interface/inputText.png");
    p=p.scaled(p.width(),50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    productKeyInput=new LineEdit(this);
    buy=new CustomButton(":/CC/images/interface/buy.png",this);
    buy->updateTextPercent(75);

    languagesText=new CCGraphicsTextItem(this);
    languagesList=new ComboBox(this);
    languagesList->addItem("English");
    languagesList->addItem("French");
    languagesList->addItem("Spanish");
    languagesList->setPos(100,200);

    {
        volumeSlider->setValue(Settings::settings->value("keyaudioVolume").toUInt());
        QString key=Settings::settings->value("key").toString();
        productKeyInput->setText(key);
        if(Ultimate::ultimate.isUltimate())
            productKeyInput->setStyleSheet("");
        else
            productKeyInput->setStyleSheet("color:red");
        previousKey=key;
        QString language=Settings::settings->value("language").toString();
        if(language=="fr")
            languagesList->setCurrentIndex(1);
        else if(language=="es")
            languagesList->setCurrentIndex(2);
        else
            languagesList->setCurrentIndex(0);
    }

    if(!connect(volumeSlider,&CCSliderH::sliderReleased,this,&OptionsDialog::volumeSliderChange))
        abort();
    if(!connect(productKeyInput,&LineEdit::textChanged,this,&OptionsDialog::productKeyChange,Qt::QueuedConnection))
        abort();
    if(!connect(languagesList,static_cast<void(ComboBox::*)(int)>(&ComboBox::currentIndexChanged),this,&OptionsDialog::languagesChange))
        abort();
    if(!connect(&Language::language,&Language::newLanguage,this,&OptionsDialog::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(buy,&CustomButton::clicked,this,&OptionsDialog::openBuy,Qt::QueuedConnection))
        abort();
    newLanguage();
}

OptionsDialog::~OptionsDialog()
{
    delete wdialog;
    delete quit;
    delete buy;
    delete title;
}

QRectF OptionsDialog::boundingRect() const
{
    return QRectF();
}

void OptionsDialog::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    p->fillRect(0,0,widget->width(),widget->height(),QColor(0,0,0,120));
    bool forcedCenter=true;
    int idealW=900;
    int idealH=400;
    if(widget->width()<600 || widget->height()<480)
    {
        idealW/=2;
        idealH/=2;
    }
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

    if(x<0 || y<0 || forcedCenter)
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

    auto font=volumeText->font();
    if(widget->width()<600 || widget->height()<480)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        buy->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(30/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        buy->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(30);
    }
    volumeText->setFont(font);
    productKeyText->setFont(font);
    languagesText->setFont(font);

    unsigned int productKeyBackgroundNewHeight=50;
    if(widget->width()<600 || widget->height()<480)
    {
        font.setPixelSize(30*0.75/2);
        productKeyInput->setFont(font);
        productKeyBackgroundNewHeight=50/2;
    }
    else
    {
        font.setPixelSize(30*0.75);
        productKeyInput->setFont(font);
    }

    const QRectF &volumeTextRect=volumeText->boundingRect();
    const QRectF &productKeyTextRect=productKeyText->boundingRect();

    const unsigned int &productKeyBackgroundNewWidth=idealW-productKeyTextRect.width()-wdialog->currentBorderSize()*4-10-buy->width();
    productKeyInput->setMaximumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);
    productKeyInput->setMinimumSize(productKeyBackgroundNewWidth,productKeyBackgroundNewHeight);

    quit->setPos(x+(idealW-quit->width())/2,y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    {
        volumeText->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        const unsigned int volumeSliderW=volumeText->x()+volumeTextRect.width();
        volumeSlider->setPos(volumeSliderW,volumeText->y()+(volumeTextRect.height()-volumeSlider->boundingRect().height())/2);
        volumeSlider->setWidth(idealW-volumeTextRect.width()-wdialog->currentBorderSize()*4);
    }
    {
        productKeyText->setPos(x+wdialog->currentBorderSize()*2,volumeText->y()+volumeTextRect.height()+10);
        const unsigned int productKeyBackgroundW=productKeyText->x()+productKeyTextRect.width();
        productKeyInput->setPos(productKeyBackgroundW,productKeyText->y()+(productKeyTextRect.height()-productKeyInput->boundingRect().height())/2);
        buy->setPos(productKeyBackgroundW+productKeyBackgroundNewWidth+10,productKeyText->y()+(productKeyTextRect.height()-buy->boundingRect().height())/2);
    }
    {
        languagesText->setPos(x+wdialog->currentBorderSize()*2,productKeyText->y()+volumeTextRect.height()+10);
        const unsigned int productKeyBackgroundW=languagesText->x()+productKeyTextRect.width();
        languagesList->setPos(productKeyBackgroundW,languagesText->y()+(productKeyTextRect.height()-languagesList->boundingRect().height())/2);
        languagesList->setMinimumWidth(idealW-productKeyTextRect.width()-wdialog->currentBorderSize()*4);
        languagesList->setMaximumWidth(idealW-productKeyTextRect.width()-wdialog->currentBorderSize()*4);
    }
}

void OptionsDialog::mousePressEventXY(const QPointF &p, bool &pressValidated)
{
    quit->mousePressEventXY(p,pressValidated);
    buy->mousePressEventXY(p,pressValidated);
    volumeSlider->mousePressEventXY(p,pressValidated);
}

void OptionsDialog::mouseReleaseEventXY(const QPointF &p,bool &pressValidated)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    buy->mouseReleaseEventXY(p,pressValidated);
    volumeSlider->mouseReleaseEventXY(p,pressValidated);
}

void OptionsDialog::mouseMoveEventXY(const QPointF &p,bool &pressValidated)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
    //quit->mouseMoveEventXY(p,pressValidated);
    volumeSlider->mouseMoveEventXY(p,pressValidated);
}

void OptionsDialog::volumeSliderChange()
{
    AudioGL::audio->setVolume(volumeSlider->value());
    Settings::settings->setValue("audioVolume",volumeSlider->value());
}

void OptionsDialog::productKeyChange()
{
    QString key=productKeyInput->text();
    if(key==previousKey)
        return;
    previousKey=key;
    Settings::settings->setValue("key",key);
    if(Ultimate::ultimate.setKey(key.toStdString()))
        productKeyInput->setStyleSheet("");
    else
        productKeyInput->setStyleSheet("color:red");
}

void OptionsDialog::languagesChange(int)
{
    switch(languagesList->currentIndex())
    {
    default:
    case 0:
    Settings::settings->setValue("language","en");
    break;
    case 1:
    Settings::settings->setValue("language","fr");
    break;
    case 2:
    Settings::settings->setValue("language","es");
    break;
    }
    Language::language.setLanguage(Settings::settings->value("language").toString());
}

void OptionsDialog::newLanguage()
{
    languagesText->setHtml(tr("Language: "));
    productKeyText->setHtml(tr("Product key: "));
    volumeText->setHtml(tr("Volume: "));
}

void OptionsDialog::openBuy()
{
    QDesktopServices::openUrl(QUrl(tr("https://shop.first-world.info/en/")));
}
