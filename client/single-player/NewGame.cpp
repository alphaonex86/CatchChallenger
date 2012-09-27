#include "NewGame.h"
#include "ui_NewGame.h"

NewGame::NewGame(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    ui->setupUi(this);
    ok=false;
}

NewGame::~NewGame()
{
    delete ui;
}

bool NewGame::haveTheInformation()
{
    return !ui->pseudo->text().isEmpty() && ok;
}

QString NewGame::pseudo()
{
    return ui->pseudo->text();
}

QString NewGame::skin()
{
    return "1";
}

void NewGame::on_ok_clicked()
{
    ok=true;
    accept();
}
