#ifndef ADDSERVER_H
#define ADDSERVER_H

#include <QDialog>

namespace Ui {
class AddServer;
}

class AddServer : public QDialog
{
    Q_OBJECT

public:
    explicit AddServer(QWidget *parent = 0);
    ~AddServer();
    QString server() const;
    quint16 port() const;
    QString name() const;
    bool isOk() const;
private slots:
    void on_ok_clicked();
private:
    Ui::AddServer *ui;
    bool ok;
};

#endif // ADDSERVER_H
