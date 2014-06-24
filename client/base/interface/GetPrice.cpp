#include "GetPrice.h"
#include "ui_GetPrice.h"

GetPrice::GetPrice(QWidget *parent, quint32 suggestedPrice) :
    QDialog(parent),
    ui(new Ui::GetPrice)
{
    ok=false;
    ui->setupUi(this);
    ui->price->setValue(suggestedPrice);
}

GetPrice::~GetPrice()
{
    delete ui;
}

void GetPrice::on_cancel_clicked()
{
    close();
}

void GetPrice::on_ok_clicked()
{
    ok=true;
    close();
}

quint32 GetPrice::price()
{
    return ui->price->value();
}

bool GetPrice::isOK()
{
    return ok;
}
