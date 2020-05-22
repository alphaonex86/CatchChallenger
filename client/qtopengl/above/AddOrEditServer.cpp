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
    wdialog(new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this)),
    label(this)
{
    ok=false;
    edit=false;

    x=-1;
    y=-1;
    label.setPixmap(*GameLoader::gameLoader->getImage(":/CC/images/interface/label.png"));
    label.setTransformationMode(Qt::SmoothTransformation);
    label.setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    quit=new CustomButton(":/CC/images/interface/cancel.png",this);
    validate=new CustomButton(":/CC/images/interface/validate.png",this);
    connect(quit,&CustomButton::clicked,this,&AddOrEditServer::removeAbove);
    QLinearGradient gradient1( 0, 0, 0, 100 );
    gradient1.setColorAt( 0.25, QColor(230,153,0));
    gradient1.setColorAt( 0.75, QColor(255,255,255));
    QLinearGradient gradient2( 0, 0, 0, 100 );
    gradient2.setColorAt( 0, QColor(64,28,2));
    gradient2.setColorAt( 1, QColor(64,28,2));
    title=new CustomText(gradient1,gradient2,this);

    QPixmap p=*GameLoader::gameLoader->getImage(":/CC/images/interface/inputText.png");
    p=p.scaled(p.width(),50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    m_type=new ComboBox(this);
    m_type->addItem(QString("Tcp"));
    m_type->addItem(QString("WS"));
    serverText=new QGraphicsTextItem(this);
    serverText->setVisible(false);
    serverInput=new LineEdit(this);
    serverInput->setPlaceholderText("ws://www.server.com:9999/");
    portInput=new SpinBox(this);
    portInput->setMinimum(1);
    portInput->setMaximum(65535);
    portInput->setValue(42489);

    nameText=new CCGraphicsTextItem(this);
    nameInput=new LineEdit(this);

    proxyText=new QGraphicsTextItem(this);
    proxyInput=new LineEdit(this);
    proxyPortInput=new SpinBox(this);
    proxyPortInput->setMinimum(1);
    proxyPortInput->setMaximum(65535);

    warning=new QGraphicsTextItem(this);
    warning->setVisible(false);

    if(!connect(&Language::language,&Language::newLanguage,this,&AddOrEditServer::newLanguage,Qt::QueuedConnection))
        abort();
    if(!connect(quit,&CustomButton::clicked,this,&AddOrEditServer::on_cancel_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(validate,&CustomButton::clicked,this,&AddOrEditServer::on_ok_clicked,Qt::QueuedConnection))
        abort();
    if(!connect(m_type,static_cast<void(ComboBox::*)(int)>(&ComboBox::currentIndexChanged),this,&AddOrEditServer::on_type_currentIndexChanged,Qt::QueuedConnection))
        abort();
    on_type_currentIndexChanged(0);

    newLanguage();

    #if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
        m_type->setVisible(false);
        #if defined(NOTCPSOCKET)
            proxyPortInput->setVisible(false);
            serverInput->setPlaceholderText("ws://www.server.com:9999/");
        #endif
    #endif
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
    if((unsigned int)nameInput->width()!=nameBackgroundNewWidth ||
            (unsigned int)nameInput->height()!=nameBackgroundNewHeight)
    {
        serverInput->setFixedHeight(nameBackgroundNewHeight);
        portInput->setFixedHeight(nameBackgroundNewHeight);
        if(portInput->isVisible())
        {
            serverInput->setFixedWidth(serverBackgroundNewWidth*3/4);
            portInput->setFixedWidth(serverBackgroundNewWidth*1/4);
        }
        else
            serverInput->setFixedWidth(serverBackgroundNewWidth);
        nameInput->setFixedSize(nameBackgroundNewWidth,nameBackgroundNewHeight);
        proxyInput->setFixedSize(proxyBackgroundNewWidth*3/4,nameBackgroundNewHeight);
        proxyPortInput->setFixedSize(proxyBackgroundNewWidth*1/4,nameBackgroundNewHeight);
    }
    {
        m_type->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        serverText->setPos(x+wdialog->currentBorderSize()*2,y+top*1.5);
        const unsigned int serverBackgroundW=serverText->x()+serverTextRect.width();
        serverInput->setPos(serverBackgroundW,serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
        if(portInput->isVisible())
            portInput->setPos(serverBackgroundW+serverInput->width(),serverText->y()+(serverTextRect.height()-serverInput->boundingRect().height())/2);
    }
    {
        nameText->setPos(x+wdialog->currentBorderSize()*2,serverText->y()+serverTextRect.height()+space);
        const unsigned int nameBackgroundW=nameText->x()+nameTextRect.width();
        nameInput->setPos(nameBackgroundW,nameText->y()+(nameTextRect.height()-nameInput->boundingRect().height())/2);
    }
    {
        proxyText->setPos(x+wdialog->currentBorderSize()*2,nameText->y()+nameTextRect.height()+space);
        const unsigned int proxyBackgroundW=proxyText->x()+proxyTextRect.width();
        proxyInput->setPos(proxyBackgroundW,proxyText->y()+(proxyTextRect.height()-proxyInput->boundingRect().height())/2);
        proxyPortInput->setPos(proxyBackgroundW+proxyInput->width(),proxyText->y()+(proxyTextRect.height()-proxyInput->boundingRect().height())/2);
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
    if(edit)
        title->setText(tr("Edit"));
    else
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
    m_type->setCurrentIndex(1);
    #else
        #if defined(NOWEBSOCKET)
        m_type->setCurrentIndex(0);
        #endif
    #endif
#endif
        Q_UNUSED(type);
}

void AddOrEditServer::setEdit(const bool &edit)
{
    this->edit=edit;
    title->setText(tr("Edit"));
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
    emit removeAbove();
    //close();
}

QString AddOrEditServer::server() const
{
    return serverInput->text();
}

uint16_t AddOrEditServer::port() const
{
    return static_cast<uint16_t>(portInput->value());
}

QString AddOrEditServer::proxyServer() const
{
    return proxyInput->text();
}

uint16_t AddOrEditServer::proxyPort() const
{
    return static_cast<uint16_t>(proxyPortInput->value());
}

QString AddOrEditServer::name() const
{
    return nameInput->text();
}

void AddOrEditServer::setServer(const QString &server)
{
    serverInput->setText(server);
}

void AddOrEditServer::setPort(const uint16_t &port)
{
    portInput->setValue(port);
}

void AddOrEditServer::setName(const QString &name)
{
    nameInput->setText(name);
}

void AddOrEditServer::setProxyServer(const QString &proxyServer)
{
    proxyInput->setText(proxyServer);
}

void AddOrEditServer::setProxyPort(const uint16_t &proxyPort)
{
    proxyPortInput->setValue(proxyPort);
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
    emit removeAbove();
}
