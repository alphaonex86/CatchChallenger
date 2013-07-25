#include "StepType.h"
#include "ui_StepType.h"

StepType::StepType(const QHash<QString,QString> &allowedType,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StepType)
{
    ui->setupUi(this);
    QHash<QString,QString>::const_iterator i = allowedType.constBegin();
    while (i != allowedType.constEnd()) {
        ui->comboBox->addItem(i.value(),i.key());
        ++i;
    }
    mValidated=false;
}

StepType::~StepType()
{
    delete ui;
}

bool StepType::validated()
{
    return mValidated;
}


void StepType::on_cancel_clicked()
{
    mValidated=false;
    close();
}

void StepType::on_ok_clicked()
{
    mValidated=true;
}

QString StepType::type()
{
    if(!mValidated)
        return QString();
    return ui->comboBox->itemData(ui->comboBox->currentIndex()).toString();
}
