#include "AddOrEditServer.hpp"
#include "../../qt/GameLoader.hpp"
#include "../../qt/Settings.hpp"
#include "../Language.hpp"
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTextDocument>
#include "../../qt/Ultimate.hpp"
#include <QDesktopServices>

AddOrEditServer::AddOrEditServer() :
    wdialog(new CCWidget(this)),
    label(this)
{
    ok=false;
    #if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
        ui->type->hide();
        #if defined(NOTCPSOCKET)
            ui->port->hide();
            ui->server->setPlaceholderText("ws://www.server.com:9999/");
        #endif
    #endif

    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",this);
    validate=new CustomButton(":/CC/images/interface/validate.png",this);
    connect(quit,&CustomButton::clicked,this,&AddOrEditServer::quitOption);
    title=new CCDialogTitle(this);

    QPixmap p=*GameLoader::gameLoader->getImage(":/CC/images/interface/inputText.png");
    p=p.scaled(p.width(),50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    typeListProxy=new QGraphicsProxyWidget(this);
    m_type=new QComboBox();
    m_type->addItem(QString("Tcp"));
    m_type->addItem(QString("WS"));
    typeListProxy->setWidget(m_type);
    serverText=new QGraphicsTextItem(this);
    serverBackground=new QGraphicsPixmapItem(this);
    serverInput=new CCGraphicsTextItem(this);
    portBackground=new QGraphicsPixmapItem(p,this);
    portInput=new CCGraphicsTextItem(this);

    nameText=new QGraphicsTextItem(this);
    nameBackground=new QGraphicsPixmapItem(p,this);
    nameInput=new CCGraphicsTextItem(this);

    proxyText=new QGraphicsTextItem(this);
    proxyBackground=new QGraphicsPixmapItem(p,this);
    proxyInput=new CCGraphicsTextItem(this);
    proxyPortBackground=new QGraphicsPixmapItem(p,this);
    proxyPortInput=new CCGraphicsTextItem(this);

    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);

    if(!connect(&Language::language,&Language::newLanguage,this,&AddOrEditServer::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&AddOrEditServer::on_cancel_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(validate,&CustomButton::clicked,this,&AddOrEditServer::on_ok_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(m_type,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&AddOrEditServer::on_type_currentIndexChanged,Qt::QueuedConnection))
        abort();

    newLanguage();
}

AddOrEditServer::~AddOrEditServer()
{
    delete wdialog;
    delete quit;
    delete validate;
    delete title;
}

QRectF AddOrEditServer::boundingRect() const
{
    return QRectF();
}

void AddOrEditServer::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget)
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

    auto font=serverText->font();
    if(widget->width()<600 || widget->height()<480)
    {
        label.setScale(0.5);
        quit->setSize(83/2,94/2);
        validate->setSize(83/2,94/2);
        label.setPos(x+(idealW-label.pixmap().width()/2)/2,y-36/2);
        title->setPixelSize(30/2);
        font.setPixelSize(30/2);
    }
    else {
        label.setScale(1);
        quit->setSize(83,94);
        validate->setSize(83,94);
        label.setPos(x+(idealW-label.pixmap().width())/2,y-36);
        title->setPixelSize(30);
        font.setPixelSize(30);
    }
    serverText->setFont(font);
    nameText->setFont(font);
    proxyText->setFont(font);

    unsigned int nameBackgroundNewHeight=50;
    unsigned int space=30;
    if(widget->width()<600 || widget->height()<480)
    {
        font.setPixelSize(30*0.75/2);
        serverInput->setFont(font);
        portInput->setFont(font);
        nameInput->setFont(font);
        proxyInput->setFont(font);
        proxyPortInput->setFont(font);
        space=10;
        nameBackgroundNewHeight=50/2;
    }
    else
    {
        font.setPixelSize(30*0.75);
        serverInput->setFont(font);
        portInput->setFont(font);
        nameInput->setFont(font);
        proxyInput->setFont(font);
        proxyPortInput->setFont(font);
    }

    quit->setPos(x+(idealW/2-quit->width()-space/2),y+idealH-quit->height()/2);
    validate->setPos(x+(idealW/2+space/2),y+idealH-quit->height()/2);
    const QRectF trect=title->boundingRect();
    title->setPos(x+(idealW-title->boundingRect().width())/2,y-trect.height()/2);

    const QRectF &serverTextRect=serverText->boundingRect();
    const QRectF &nameTextRect=nameText->boundingRect();
    const QRectF &proxyTextRect=proxyText->boundingRect();

    const unsigned int &serverBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    const unsigned int &nameBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    const unsigned int &proxyBackgroundNewWidth=idealW-nameTextRect.width()-wdialog->currentBorderSize()*4;
    if((unsigned int)nameBackground->pixmap().width()!=nameBackgroundNewWidth ||
            (unsigned int)nameBackground->pixmap().height()!=nameBackgroundNewHeight)
    {
        QPixmap p=*GameLoader::gameLoader->getImage(":/CC/images/interface/inputText.png");
        QPixmap serverPix=p.scaled(serverBackgroundNewWidth*3/4,nameBackgroundNewHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        serverBackground->setPixmap(serverPix);
        QPixmap portPix=p.scaled(serverBackgroundNewWidth*1/4,nameBackgroundNewHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        portBackground->setPixmap(portPix);
        QPixmap namePix=p.scaled(nameBackgroundNewWidth,nameBackgroundNewHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        nameBackground->setPixmap(namePix);
        QPixmap proxyPix=p.scaled(proxyBackgroundNewWidth*3/4,nameBackgroundNewHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        proxyBackground->setPixmap(proxyPix);
        QPixmap proxyPortPix=p.scaled(proxyBackgroundNewWidth*1/4,nameBackgroundNewHeight,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        proxyPortBackground->setPixmap(proxyPortPix);
    }
    {
        serverText->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        const unsigned int serverBackgroundW=serverText->x()+serverTextRect.width();
        serverBackground->setPos(serverBackgroundW,serverText->y()+(serverTextRect.height()-serverBackground->boundingRect().height())/2);
        serverInput->setPos(serverBackgroundW,serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
        serverInput->setTextWidth(serverBackground->boundingRect().width());
        portBackground->setPos(serverBackgroundW+serverBackground->boundingRect().width(),serverText->y()+(serverTextRect.height()-serverBackground->boundingRect().height())/2);
        portInput->setPos(serverBackgroundW+serverBackground->boundingRect().width(),serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
        portInput->setTextWidth(portBackground->boundingRect().width());
    }
}

void AddOrEditServer::mousePressEventXY(const QPointF &p, bool &pressValidated)
{
    quit->mousePressEventXY(p,pressValidated);
    validate->mousePressEventXY(p,pressValidated);
}

void AddOrEditServer::mouseReleaseEventXY(const QPointF &p,bool &pressValidated)
{
    quit->mouseReleaseEventXY(p,pressValidated);
    validate->mouseReleaseEventXY(p,pressValidated);
}

void AddOrEditServer::mouseMoveEventXY(const QPointF &p,bool &pressValidated)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
}

void AddOrEditServer::newLanguage()
{
    proxyText->setHtml(tr("Proxy: "));
    nameText->setHtml(tr("Name: "));
    serverText->setHtml(tr("Server: "));
    title->setText(tr("Add"));
}

int AddOrEditServer::type() const
{
#if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
return m_type->currentIndex();
#else
    #if defined(NOTCPSOCKET)
    return 1;
    #else
        #if defined(NOWEBSOCKET)
        return 0;
        #else
        #error add server but no tcp or web socket defined
        return -1;
        #endif
    #endif
#endif
}

void AddOrEditServer::setType(const int &type)
{
#if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
m_type->setCurrentIndex(type);
#else
    #if defined(NOTCPSOCKET)
    ui->type->setCurrentIndex(1);
    #else
        #if defined(NOWEBSOCKET)
        ui->type->setCurrentIndex(0);
        #endif
    #endif
#endif
        Q_UNUSED(type);
}

void AddOrEditServer::setEdit(const bool &edit)
{
    if(edit)
        validate->setText(tr("Save"));
}

void AddOrEditServer::on_ok_clicked()
{
    if(nameText->toPlainText()==QStringLiteral("Internal") || nameText->toPlainText()==QStringLiteral("internal"))
    {
        warning->setHtml(tr("The name can't be \"internal\""));
        warning->setVisible(true);
        return;
    }
    if(type()==0)
    {
        if(!server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$")))
        {
            warning->setHtml(tr("The host seam don't be a valid hostname or ip"));
            warning->setVisible(true);
            return;
        }
    }
    else if(type()==1)
    {
        if(!server().startsWith("ws://") && !server().startsWith("wss://"))
        {
            warning->setHtml(tr("The web socket url seam wrong, not start with ws:// or wss://"));
            warning->setVisible(true);
            return;
        }
    }
    ok=true;
    emit quitOption();
    //close();
}

QString AddOrEditServer::server() const
{
    return serverInput->toPlainText();
}

uint16_t AddOrEditServer::port() const
{
    return static_cast<uint16_t>(portInput->toPlainText().toUInt());
}

QString AddOrEditServer::proxyServer() const
{
    return proxyInput->toPlainText();
}

uint16_t AddOrEditServer::proxyPort() const
{
    return static_cast<uint16_t>(proxyPortInput->toPlainText().toUInt());
}

QString AddOrEditServer::name() const
{
    return nameText->toPlainText();
}

void AddOrEditServer::setServer(const QString &server)
{
    serverInput->setHtml(server);
}

void AddOrEditServer::setPort(const uint16_t &port)
{
    portInput->setHtml(QString::number(port));
}

void AddOrEditServer::setName(const QString &name)
{
    nameInput->setHtml(name);
}

void AddOrEditServer::setProxyServer(const QString &proxyServer)
{
    proxyInput->setHtml(proxyServer);
}

void AddOrEditServer::setProxyPort(const uint16_t &proxyPort)
{
    proxyPortInput->setHtml(QString::number(proxyPort));
}

bool AddOrEditServer::isOk() const
{
    return ok;
}

void AddOrEditServer::on_type_currentIndexChanged(int index)
{
    switch(index) {
    case 0:
        portInput->show();
        serverInput->setPlaceholderText("www.server.com");
        break;
    case 1:
        portInput->hide();
        serverInput->setPlaceholderText("ws://www.server.com:9999/");
        break;
    default:
        return;
    }
}

void AddOrEditServer::on_cancel_clicked()
{
    ok=false;
    emit quitOption();
}

