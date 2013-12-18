#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QtCore/qmath.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateTheValue();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTheValue()
{
    quint64 xp_for_this_level=qPow(ui->level->value(),ui->pow->value());
    quint64 xp_for_max_level=ui->xp_max->value();
    quint64 max_xp=qPow(ui->level->maximum(),ui->pow->value());
    quint64 xp_to_level_up=xp_for_this_level*xp_for_max_level/max_xp;
    ui->xp_for_level->setText(tr("Xp for the level: %1").arg(xp_to_level_up));
    if(ui->giveXp->value()==0 || ui->level->value()==0)
    {
        ui->monster_for_level->setText(tr("Same monster to kill to level up: inifity"));
        ui->XpGiven->setText(tr("Xp given by monter at this level: 0"));
    }
    else
    {
        double giveXpTemp=((double)ui->giveXp->value()*(double)ui->level->value())/(double)ui->level->maximum();
        ui->XpGiven->setText(tr("Xp given by monter at this level: %1").arg(giveXpTemp));
        ui->monster_for_level->setText(tr("Same monster to kill to level up: %1").arg((double)xp_to_level_up/giveXpTemp));
    }
}

void MainWindow::on_xp_max_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    updateTheValue();
}

void MainWindow::on_pow_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    updateTheValue();
}

void MainWindow::on_level_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    updateTheValue();
}

void MainWindow::on_giveXp_valueChanged(int arg1)
{
    Q_UNUSED(arg1);
    updateTheValue();
}
