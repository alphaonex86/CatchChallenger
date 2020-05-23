#include "NewGame.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "ui_NewGame.h"

#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QDebug>
#include <QLabel>

NewGame::NewGame(const std::string &skinPath, const std::string &monsterPath, std::vector<std::vector<CatchChallenger::Profile::Monster> > monstergroup, const std::vector<uint8_t> &forcedSkin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGame)
{
    srand(static_cast<uint32_t>(time(NULL)));
    ui->setupUi(this);
    this->forcedSkin=forcedSkin;
    this->monsterPath=monsterPath;
    this->monstergroup=monstergroup;
    okAccepted=true;
    step=Step1;
    currentMonsterGroup=0;
    if(!monstergroup.empty())
        currentMonsterGroup=static_cast<uint8_t>(rand()%monstergroup.size());
    this->skinPath=skinPath;
    uint8_t index=0;
    while(index<CatchChallenger::CommonDatapack::commonDatapack.skins.size())
    {
        if(forcedSkin.empty() || vectorcontainsAtLeastOne(forcedSkin,(uint8_t)index))
        {
            const std::string &currentPath=skinPath+CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
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
    if(!skinList.empty())
        currentSkin=static_cast<uint8_t>(rand()%skinList.size());
    updateSkin();
    ui->pseudo->setFocus();
    if(skinList.empty())
    {
        QMessageBox::critical(this,tr("Error"),tr("No skin to select!"));
        close();
        return;
    }

    if(!connect(&timer,&QTimer::timeout,      this,&NewGame::timerSlot,    Qt::QueuedConnection))
        abort();
}

NewGame::~NewGame()
{
    delete ui;
}

void NewGame::updateSkin()
{
    skinLoaded=false;

    std::vector<std::string> paths;
    if(step==Step1)
    {
        if(currentSkin>=skinList.size())
            return;
        ui->previousSkin->setEnabled(currentSkin>0);
        ui->nextSkin->setEnabled(currentSkin<(skinList.size()-1));
        paths.push_back(skinPath+skinList.at(currentSkin)+"/front.png");
    }
    else if(step==Step2)
    {
        if(currentMonsterGroup>=monstergroup.size())
            return;
        ui->previousSkin->setEnabled(currentMonsterGroup>0);
        ui->nextSkin->setEnabled(currentMonsterGroup<(monstergroup.size()-1));
        const std::vector<CatchChallenger::Profile::Monster> &monsters=monstergroup.at(currentMonsterGroup);
        unsigned int index=0;
        while(index<monsters.size())
        {
            const CatchChallenger::Profile::Monster &monster=monsters.at(index);
            paths.push_back(monsterPath+std::to_string(monster.id)+"/front.png");
            index++;
        }
    }
    else
        return;

    {
        QLayoutItem *child;
        while ((child = ui->horizontalLayout->takeAt(0)) != 0)
        {
            delete child->widget();
            delete child;
        }
    }
    if(!paths.empty())
    {
        unsigned int index=0;
        while(index<paths.size())
        {
            const std::string &path=paths.at(index);

            QImage skin=QImage(QString::fromStdString(path));
            if(skin.isNull())
            {
                QMessageBox::critical(this,tr("Error"),QStringLiteral("But the skin can't be loaded: %1").arg(QString::fromStdString(path)));
                return;
            }
            QImage scaledSkin=skin.scaled(160,160,Qt::IgnoreAspectRatio);
            QPixmap pixmap;
            pixmap.convertFromImage(scaledSkin);
            QLabel *label=new QLabel();
            label->setMinimumSize(160,160);
            label->setMaximumSize(160,160);
            ui->horizontalLayout->addWidget(label);
            label->setPixmap(pixmap);
            skinLoaded=true;

            index++;
        }
    }
    else
    {
        skinLoaded=false;
    }
}

bool NewGame::haveTheInformation()
{
    return okCanBeEnabled() && step==StepOk;
}

bool NewGame::okCanBeEnabled()
{
    return !ui->pseudo->text().isEmpty() && skinLoaded;
}

std::string NewGame::pseudo()
{
    return ui->pseudo->text().toStdString();
}

std::string NewGame::skin()
{
    return skinList.at(currentSkin);
}

uint8_t NewGame::skinId()
{
    return skinListId.at(currentSkin);
}

uint8_t NewGame::monsterGroupId()
{
    return currentMonsterGroup;
}

bool NewGame::haveSkin()
{
    return skinList.size()>0;
}

void NewGame::on_ok_clicked()
{
    if(!okAccepted)
        return;
    okAccepted=false;
    timer.start(20);
    if(step==Step1)
    {
        if(ui->pseudo->text().isEmpty())
        {
            QMessageBox::warning(this,tr("Error"),tr("Your pseudo can't be empty"));
            return;
        }
        step=Step2;
        ui->pseudo->hide();
        updateSkin();
        if(monstergroup.size()<2)
            on_ok_clicked();
    }
    else if(step==Step2)
    {
        step=StepOk;
        accept();
    }
    else
        return;
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
        if(currentMonsterGroup<(monstergroup.size()-1))
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

void NewGame::timerSlot()
{
    okAccepted=true;
}
