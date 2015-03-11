#include "AddServer.h"
#include "ui_AddServer.h"

#include <QMessageBox>

AddOrEditServer::AddOrEditServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddServer)
{
    ui->setupUi(this);
    ok=false;
}

AddOrEditServer::~AddOrEditServer()
{
    delete ui;
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

quint16 AddOrEditServer::port() const
{
    return ui->port->value();
}

QString AddOrEditServer::proxyServer() const
{
    return ui->proxyServer->text();
}

quint16 AddOrEditServer::proxyPort() const
{
    return ui->proxyPort->value();
}

QString AddOrEditServer::name() const
{
    return ui->name->text();
}

void AddOrEditServer::setServer(const QString &server)
{
    ui->server->setText(server);
}

void AddOrEditServer::setPort(const quint16 &port)
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

void AddOrEditServer::setProxyPort(const quint16 &proxyPort)
{
    ui->proxyPort->setValue(proxyPort);
}

bool AddOrEditServer::isOk() const
{
    return ok;
}
