#include "NewGame.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonDatapack.h"
#include "ui_NewGame.h"

#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QDebug>

NewGame::NewGame(const QString &skinPath,const QList<quint8> &forcedSkin,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    ui->setupUi(this);
    this->forcedSkin=forcedSkin;
    ok=false;
    this->skinPath=skinPath;
    int index=0;
    while(index<CatchChallenger::CommonDatapack::commonDatapack.skins.size())
    {
        if(forcedSkin.empty() || forcedSkin.contains(index))
        {
            const QString &currentPath=skinPath+CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
            if(QFile::exists(currentPath+"/back.png") && QFile::exists(currentPath+"/front.png") && QFile::exists(currentPath+"/trainer.png"))
            {
                skinList << CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
                skinListId << index;
            }
        }
        index++;
    }

    ui->pseudo->setMaxLength(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size);
    ui->previousSkin->setVisible(skinList.size()>=2);
    ui->nextSkin->setVisible(skinList.size()>=2);

    currentSkin=0;
    updateSkin();
    ui->pseudo->setFocus();
    if(skinList.empty())
    {
        QMessageBox::critical(this,tr("Error"),tr("No skin to select!"));
        close();
        return;
    }
}

NewGame::~NewGame()
{
    delete ui;
}

void NewGame::updateSkin()
{
    skinLoaded=false;
    if(currentSkin>=skinList.size() || currentSkin<-1)
        return;
    ui->previousSkin->setEnabled(currentSkin>0);
    ui->nextSkin->setEnabled(currentSkin<(skinList.size()-1));
    QString path=skinPath+skinList.at(currentSkin)+"/front.png";
    QImage skin=QImage(path);
    if(skin.isNull())
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("But the skin can't be loaded: %1").arg(path));
        return;
    }
    QImage scaledSkin=skin.scaled(160,160,Qt::IgnoreAspectRatio);
    QPixmap pixmap;
    pixmap.convertFromImage(scaledSkin);
    ui->skin->setPixmap(pixmap);
    skinLoaded=true;
}

bool NewGame::haveTheInformation()
{
    return okCanBeEnabled() && ok;
}

bool NewGame::okCanBeEnabled()
{
    return !ui->pseudo->text().isEmpty() && skinLoaded;
}

QString NewGame::pseudo()
{
    return ui->pseudo->text();
}

QString NewGame::skin()
{
    return skinList.at(currentSkin);
}

quint32 NewGame::skinId()
{
    return skinListId.at(currentSkin);
}

bool NewGame::haveSkin()
{
    return skinList.size()>0;
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
    if(currentSkin<(skinList.size()-1))
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
