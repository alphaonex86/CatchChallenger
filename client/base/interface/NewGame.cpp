#include "NewGame.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "../../../general/base/CommonDatapack.h"
#include "ui_NewGame.h"

#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QDebug>

NewGame::NewGame(const QString &skinPath, const std::vector<uint8_t> &forcedSkin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    ui->setupUi(this);
    this->forcedSkin=forcedSkin;
    step=Step1;
    currentMonsterGroupId=0;
    currentMonsterGroup=0;
    this->skinPath=skinPath.toStdString();
    unsigned int index=0;
    while(index<CatchChallenger::CommonDatapack::commonDatapack.skins.size())
    {
        if(forcedSkin.empty() || vectorcontainsAtLeastOne(forcedSkin,index))
        {
            const std::string &currentPath=skinPath.toStdString()+CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
            if(QFile::exists(QString::fromStdString(currentPath+"/back.png")) && QFile::exists(QString::fromStdString(currentPath+"/front.png")) && QFile::exists(QString::fromStdString(currentPath+"/trainer.png")))
            {
                skinList.push_back(CatchChallenger::CommonDatapack::commonDatapack.skins.at(index));
                skinListId.push_back(index);
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

    if(step==Step1)
    {
        if(currentSkin>=skinList.size())
            return;
        ui->previousSkin->setEnabled(currentSkin>0);
        ui->nextSkin->setEnabled(currentSkin<(skinList.size()-1));
        std::string path=skinPath+skinList.at(currentSkin)+"/front.png";
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup>=skinList.size())
            return;
        ui->previousSkin->setEnabled(currentMonsterGroup>0);
        ui->nextSkin->setEnabled(currentMonsterGroup<(skinList.size()-1));
        std::string path=skinPath+skinList.at(currentSkin)+"/front.png";
    }
    else
        return;

    QImage skin=QImage(QString::fromStdString(path));
    if(skin.isNull())
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("But the skin can't be loaded: %1").arg(QString::fromStdString(path)));
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
    return QString::fromStdString(skinList.at(currentSkin));
}

uint8_t NewGame::skinId()
{
    return skinListId.at(currentSkin);
}

uint8_t NewGame::monsterGroupId()
{
    return currentMonsterGroupId;
}

bool NewGame::haveSkin()
{
    return skinList.size()>0;
}

void NewGame::on_ok_clicked()
{
    if(ui->pseudo->text().isEmpty())
    {
        QMessageBox::error(this,tr("Error"),tr("Your pseudo can't be empty"));
        return;
    }
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
    if(step==Step1)
    {
        if(currentSkin<(skinList.size()-1))
            currentSkin++;
        else
            return;
        updateSkin();
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup<(skinList.size()-1))
            currentMonsterGroup++;
        else
            return;
        updateSkin();
    }
    else
        return;
}

void NewGame::on_previousSkin_clicked()
{
    if(step==Step1)
    {
        if(currentSkin<=0)
            return;
        currentSkin--;
        updateSkin();
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup<=0)
            return;
        currentMonsterGroup--;
        updateSkin();
    }
    else
        return;
}
