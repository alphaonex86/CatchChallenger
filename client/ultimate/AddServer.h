#ifndef ADDSERVER_H
#define ADDSERVER_H

#include <QDialog>

namespace Ui {
class AddServer;
}

class AddOrEditServer : public QDialog
{
    Q_OBJECT

public:
    explicit AddOrEditServer(QWidget *parent = 0);
    ~AddOrEditServer();

    bool isOk() const;
    void setEdit(const bool &edit);

    QString server() const;
    uint16_t port() const;
    QString name() const;
    QString proxyServer() const;
    uint16_t proxyPort() const;

    void setServer(const QString &server);
    void setPort(const uint16_t &port);
    void setName(const QString &name);
    void setProxyServer(const QString &proxyServer);
    void setProxyPort(const uint16_t &proxyPort);
private slots:
    void on_ok_clicked();
private:
    Ui::AddServer *ui;
    bool ok;
};

#endif // ADDSERVER_H
