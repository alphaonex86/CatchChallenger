#include "NewProfile.h"
#include "ui_NewProfile.h"

#include <QFile>
#include <QByteArray>
#include <QMessageBox>
#include <QDebug>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"

NewProfile::NewProfile(const std::string &datapackPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProfile)
{
    const uint32_t timeVar=static_cast<uint32_t>(time(NULL));
    srand(timeVar);
    ui->setupUi(this);
    this->datapackPath=datapackPath;
    this->mOk=false;
    ui->description->setText(tr("Profile loading..."));

    if(CatchChallenger::CommonDatapack::commonDatapack.get_profileList().empty())
    {
        QMessageBox::critical(this,tr("Error"),tr("No profile selected to start a new game"));
        close();
        return;
    }
    loadProfileText();
    ui->comboBox->clear();
    unsigned int index=0;
    while(index<profileTextList.size())
    {
        ui->comboBox->addItem(QString::fromStdString(profileTextList.at(index).name));
        index++;
    }
    if(ui->comboBox->count()>0)
    {
        srand(static_cast<uint32_t>(time(NULL)));
        ui->comboBox->setCurrentIndex(rand()%ui->comboBox->count());
        ui->description->setText(QString::fromStdString(profileTextList.at(ui->comboBox->currentIndex()).description));
        ui->ok->setEnabled(true);
    }
}

NewProfile::~NewProfile()
{
    delete ui;
}

void NewProfile::loadProfileText()
{
    const std::unordered_map<uint32_t,DatapackClientLoader::ProfileText> &textList=
            QtDatapackClientLoader::datapackLoader->get_profileTextList();
    profileTextList.clear();
    unsigned int index=0;
    while(index<static_cast<unsigned int>(CatchChallenger::CommonDatapack::commonDatapack.get_profileList().size()))
    {
        const auto it=textList.find(index);
        if(it!=textList.cend())
        {
            ProfileText profile;
            profile.name=it->second.name;
            profile.description=it->second.description;
            profileTextList.push_back(profile);
        }
        index++;
    }
}

bool NewProfile::ok()
{
    return mOk;
}

int NewProfile::getProfileIndex()
{
    if(ui->comboBox->count()>0)
        return ui->comboBox->currentIndex();
    return 0;
}

int NewProfile::getProfileCount()
{
    return ui->comboBox->count();
}

void NewProfile::on_ok_clicked()
{
    mOk=true;
    close();
}

void NewProfile::on_pushButton_2_clicked()
{
    close();
}

void NewProfile::on_comboBox_currentIndexChanged(int index)
{
    if(ui->comboBox->count()>0)
        ui->description->setText(QString::fromStdString(profileTextList.at(index).description));
}

void NewProfile::datapackParsed()
{
    if(isParsingDatapack)
    {
        isParsingDatapack=false;
        emit parseDatapack(datapackPath);
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.get_profileList().size()==1)//do into the next screen:  && CatchChallenger::CommonDatapack::commonDatapack.profileList.first().forcedskin.size()==1
    {
        emit finished(0);
        qDebug() << ("Default profile selected");
        mOk=true;
        close();//only one, then select it
        return;
    }
    ui->ok->setEnabled(true);
    show();
}
