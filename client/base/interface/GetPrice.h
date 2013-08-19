#ifndef GETPRICE_H
#define GETPRICE_H

#include <QDialog>

namespace Ui {
class GetPrice;
}

class GetPrice : public QDialog
{
    Q_OBJECT
public:
    explicit GetPrice(QWidget *parent, bool bitcoin);
    ~GetPrice();
    quint32 price();
    double bitcoin();
    bool isOK();
private slots:
    void on_cancel_clicked();
    void on_ok_clicked();
private:
    Ui::GetPrice *ui;
    bool ok;
};

#endif // GETPRICE_H
