#include "TargetFilter.h"
#include "ui_TargetFilter.h"

TargetFilter::TargetFilter(QWidget *parent, bool dirt, bool itemOnMap, bool fight, bool shop, bool heal, bool wildMonster) :
    QDialog(parent),
    ui(new Ui::TargetFilter)
{
    ui->setupUi(this);
    ui->dirt->setChecked(dirt);
    ui->itemOnMap->setChecked(itemOnMap);
    ui->fight->setChecked(fight);
    ui->shop->setChecked(shop);
    ui->heal->setChecked(heal);
    ui->wildMonster->setChecked(wildMonster);
}

TargetFilter::~TargetFilter()
{
    delete ui;
}

void TargetFilter::on_buttonBox_clicked(QAbstractButton *button)
{
    Q_UNUSED(button);
    close();
}

bool TargetFilter::get_dirt()
{
    return ui->dirt->isChecked();
}

bool TargetFilter::get_itemOnMap()
{
    return ui->itemOnMap->isChecked();
}

bool TargetFilter::get_fight()
{
    return ui->fight->isChecked();
}

bool TargetFilter::get_shop()
{
    return ui->shop->isChecked();
}

bool TargetFilter::get_heal()
{
    return ui->heal->isChecked();
}

bool TargetFilter::get_wildMonster()
{
    return ui->wildMonster->isChecked();
}

