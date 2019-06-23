#include "AddServer.h"
#include "ui_AddServer.h"

#include <QMessageBox>

AddOrEditServer::AddOrEditServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddServer)
{
    ui->setupUi(this);
    ok=false;
    #if defined(NOTCPSOCKET) || defined(NOWEBSOCKET)
    ui->type->hide();
    #endif
}

AddOrEditServer::~AddOrEditServer()
{
    delete ui;
}

int AddOrEditServer::type() const
{
    return ui->type->currentIndex();
}

void AddOrEditServer::setType(const int &type)
{
    ui->type->setCurrentIndex(type);
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
        QMessageBox::warning(this,tr("Error"),tr("The name can't be \"internal\""));
        return;
    }
    ok=true;
    close();
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
