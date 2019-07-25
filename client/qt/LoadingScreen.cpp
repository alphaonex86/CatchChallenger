#include "LoadingScreen.h"
#include "ui_LoadingScreen.h"

LoadingScreen::LoadingScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingScreen)
{
    ui->setupUi(this);
}

LoadingScreen::~LoadingScreen()
{
    delete ui;
}

void LoadingScreen::resizeEvent(QResizeEvent *)
{
    if(width()<600)
        ui->teacher->setVisible(false);
    else
        ui->teacher->setVisible(true);
}
