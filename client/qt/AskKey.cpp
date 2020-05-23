#include "AskKey.hpp"
#include "ui_AskKey.h"

AskKey::AskKey(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AskKey)
{
    ui->setupUi(this);
}

AskKey::~AskKey()
{
    delete ui;
}

void AskKey::on_pushButton_clicked()
{
    close();
}

QString AskKey::key() const
{
    return ui->lineEdit->text();
}
