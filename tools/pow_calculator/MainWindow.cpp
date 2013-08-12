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
    ui->xp_for_level->setText(QString::number(xp_for_this_level*xp_for_max_level/max_xp));
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
