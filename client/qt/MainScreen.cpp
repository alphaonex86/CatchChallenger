#include "MainScreen.h"
#include "ui_MainScreen.h"

MainScreen::MainScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainScreen)
{
    ui->setupUi(this);

}

MainScreen::~MainScreen()
{
    delete ui;
}
