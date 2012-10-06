#include "NewGame.h"
#include "../../general/base/GeneralVariable.h"
#include "ui_NewGame.h"

#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QDebug>

NewGame::NewGame(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    ui->setupUi(this);
    ok=false;
    skinPath=QCoreApplication::applicationDirPath()+"/datapack/"+DATAPACK_BASE_PATH_SKIN;
    int index=1;
    while(QFileInfo(skinPath+QString::number(index)).isDir())
    {
        QString currentPath=skinPath+QString::number(index);
        if(QFile::exists(currentPath+"/back.png") && QFile::exists(currentPath+"/front.png") && QFile::exists(currentPath+"/trainer.png"))
            skinNumber << index;
        index++;
    }

    ui->previousSkin->setVisible(skinNumber.size()>=2);
    ui->nextSkin->setVisible(skinNumber.size()>=2);

    currentSkin=0;
    updateSkin();
    ui->pseudo->setFocus();
}

NewGame::~NewGame()
{
    delete ui;
}

void NewGame::updateSkin()
{
    skinLoaded=false;
    if(currentSkin>=skinNumber.size() || currentSkin<-1)
        return;
    ui->previousSkin->setEnabled(currentSkin>0);
    ui->nextSkin->setEnabled(currentSkin<(skinNumber.size()-1));
    QString path=skinPath+QString::number(skinNumber.at(currentSkin))+"/front.png";
    QImage skin=QImage(path);
    if(skin.isNull())
    {
        QMessageBox::critical(this,tr("Error"),QString("But the skin can't be loaded: %1").arg(path));
        return;
    }
    QImage scaledSkin=skin.scaled(160,160,Qt::IgnoreAspectRatio);
    QPixmap pixmap;
    pixmap.convertFromImage(scaledSkin);
    ui->skin->setPixmap(pixmap);
    skinLoaded=true;
}

QString NewGame::gameName()
{
    return ui->gameName->text();
}

bool NewGame::haveTheInformation()
{
    return okCanBeEnabled() && ok;
}

bool NewGame::okCanBeEnabled()
{
    return !ui->pseudo->text().isEmpty() && skinLoaded && !ui->gameName->text().isEmpty();
}

QString NewGame::pseudo()
{
    return ui->pseudo->text();
}

QString NewGame::skin()
{
    return skinPath+QString::number(currentSkin)+"/";
}

bool NewGame::haveSkin()
{
    return skinNumber.size()>0;
}

void NewGame::on_ok_clicked()
{
    if(ui->pseudo->text().isEmpty())
        return;
    ok=true;
    accept();
}

void NewGame::on_pseudo_textChanged(const QString &)
{
    ui->ok->setEnabled(okCanBeEnabled());
}

void NewGame::on_pseudo_returnPressed()
{
    on_ok_clicked();
}

void NewGame::on_nextSkin_clicked()
{
    if(currentSkin<(skinNumber.size()-1))
        currentSkin++;
    else
        return;
    updateSkin();
}

void NewGame::on_previousSkin_clicked()
{
    if(currentSkin<=0)
        return;
    currentSkin--;
    updateSkin();
}

void NewGame::on_gameName_textChanged(const QString &)
{
    ui->ok->setEnabled(okCanBeEnabled());
}
