#include "AddServer.hpp"
#include "ui_AddServer.h"

#if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
#error no web and tcp socket selected
#endif

AddOrEditServer::AddOrEditServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddServer)
{
    ui->setupUi(this);
    ok=false;
    #if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
        ui->type->hide();
        #if defined(NOTCPSOCKET)
            ui->port->hide();
            ui->server->setPlaceholderText("ws://www.server.com:9999/");
        #endif
    #endif
    ui->warning->setVisible(false);
}

AddOrEditServer::~AddOrEditServer()
{
    delete ui;
}

int AddOrEditServer::type() const
{
#if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
return ui->type->currentIndex();
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
ui->type->setCurrentIndex(type);
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
        ui->ok->setText(tr("Save"));
}

void AddOrEditServer::on_ok_clicked()
{
    if(ui->name->text()==QStringLiteral("Internal") || ui->name->text()==QStringLiteral("internal"))
    {
        ui->warning->setText(tr("The name can't be \"internal\""));
        ui->warning->setVisible(true);
        return;
    }
    if(type()==0)
    {
        if(!server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$")))
        {
            ui->warning->setText(tr("The host seam don't be a valid hostname or ip"));
            ui->warning->setVisible(true);
            return;
        }
    }
    else if(type()==1)
    {
        if(!server().startsWith("ws://") && !server().startsWith("wss://"))
        {
            ui->warning->setText(tr("The web socket url seam wrong, not start with ws:// or wss://"));
            ui->warning->setVisible(true);
            return;
        }
    }
    ok=true;
    accept();
    //close();
}

QString AddOrEditServer::server() const
{
    return ui->server->text();
}

uint16_t AddOrEditServer::port() const
{
    return static_cast<uint16_t>(ui->port->value());
}

QString AddOrEditServer::proxyServer() const
{
    return ui->proxyServer->text();
}

uint16_t AddOrEditServer::proxyPort() const
{
    return static_cast<uint16_t>(ui->proxyPort->value());
}

QString AddOrEditServer::name() const
{
    return ui->name->text();
}

void AddOrEditServer::setServer(const QString &server)
{
    ui->server->setText(server);
}

void AddOrEditServer::setPort(const uint16_t &port)
{
    ui->port->setValue(port);
}

void AddOrEditServer::setName(const QString &name)
{
    ui->name->setText(name);
}

void AddOrEditServer::setProxyServer(const QString &proxyServer)
{
    ui->proxyServer->setText(proxyServer);
}

void AddOrEditServer::setProxyPort(const uint16_t &proxyPort)
{
    ui->proxyPort->setValue(proxyPort);
}

bool AddOrEditServer::isOk() const
{
    return ok;
}

void AddOrEditServer::on_type_currentIndexChanged(int index)
{
    switch(index) {
    case 0:
        ui->port->show();
        ui->server->setPlaceholderText("www.server.com");
        break;
    case 1:
        ui->port->hide();
        ui->server->setPlaceholderText("ws://www.server.com:9999/");
        break;
    default:
        return;
    }
}

void AddOrEditServer::on_cancel_clicked()
{
    ok=false;
    reject();
}
