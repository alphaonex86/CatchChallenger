#include "SslCert.h"
#include "ui_SslCert.h"

SslCert::SslCert(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SslCert),
    mValidated(false)
{
    ui->setupUi(this);
}

SslCert::~SslCert()
{
    delete ui;
}

void SslCert::on_pushButton_abort_clicked()
{
    close();
}

void SslCert::on_pushButton_continue_clicked()
{
    mValidated=true;
    close();
}

bool SslCert::validated()
{
    return mValidated;
}
