#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <stdio.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    progressbar(new CCprogressbar)
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(progressbar);

    progressbar->setMinimumHeight(82);
    progressbar->setMaximumHeight(82);
    progressbar->setValue(ui->horizontalSlider->value());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    progressbar->setValue(value);
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    progressbar->setMinimumHeight(value);
    progressbar->setMaximumHeight(value);
}
