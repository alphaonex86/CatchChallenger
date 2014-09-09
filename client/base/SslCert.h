#ifndef SSLCERT_H
#define SSLCERT_H

#include <QDialog>

namespace Ui {
class SslCert;
}

class SslCert : public QDialog
{
    Q_OBJECT

public:
    explicit SslCert(QWidget *parent = 0);
    ~SslCert();
    bool validated();
private slots:
    void on_pushButton_abort_clicked();
    void on_pushButton_continue_clicked();
private:
    Ui::SslCert *ui;
    bool mValidated;
};

#endif // SSLCERT_H
