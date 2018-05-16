#include "NewProfile.h"
#include "ui_NewProfile.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <tinyxml2::XMLElement>
#include <QMessageBox>
#include <QDebug>

#include "../../../general/base/tinyXML2/tinyxml2.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/DatapackGeneralLoader.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/LanguagesSelect.h"

NewProfile::NewProfile(const QString &datapackPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProfile)
{
    const uint32_t timeVar=static_cast<uint32_t>(time(NULL));
    srand(timeVar);
    ui->setupUi(this);
    this->datapackPath=datapackPath;
    this->mOk=false;
    ui->description->setText(tr("Profile loading..."));

    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.empty())
    {
        QMessageBox::critical(this,tr("Error"),tr("No profile selected to start a new game"));
        close();
        return;
    }
    loadProfileText();
    ui->comboBox->clear();
    int index=0;
    while(index<profileTextList.size())
    {
        ui->comboBox->addItem(profileTextList.at(index).name);
        index++;
    }
    if(ui->comboBox->count()>0)
    {
        srand(static_cast<uint32_t>(time(NULL)));
        ui->comboBox->setCurrentIndex(rand()%ui->comboBox->count());
        ui->description->setText(profileTextList.at(ui->comboBox->currentIndex()).description);
        ui->ok->setEnabled(true);
    }
}

NewProfile::~NewProfile()
{
    delete ui;
}

void NewProfile::loadProfileText()
{
    const QString &xmlFile=datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"start.xml";
    std::vector<const TiXmlElement *> xmlList=CatchChallenger::DatapackGeneralLoader::loadProfileList(
                datapackPath.toStdString(),xmlFile.toStdString(),
                CatchChallenger::CommonDatapack::commonDatapack.items.item,
                CatchChallenger::CommonDatapack::commonDatapack.monsters,
                CatchChallenger::CommonDatapack::commonDatapack.reputation
                ).first;
    unsigned int index=0;
    while(index<xmlList.size())
    {
        ProfileText profile;
        const TiXmlElement * startItem=xmlList.at(index);
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found=false;
        const TiXmlElement * name = startItem->FirstChildElement("name");
        if(!language.isEmpty() && language!="en")
            while(name!=NULL)
            {
                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language)
                {
                    profile.name=name->GetText();
                    found=true;
                    break;
                }
                name = name->NextSiblingElement("name");
            }
        if(!found)
        {
            name = startItem->FirstChildElement("name");
            while(name!=NULL)
            {
                if(name->Attribute("lang")==NULL || *name->Attribute(std::string("lang"))=="en")
                {
                    profile.name=name->GetText();
                    break;
                }
                name = name->NextSiblingElement("name");
            }
        }
        if(profile.name.isEmpty())
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, name empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile).arg(startItem->GetText()).arg("?"));
            startItem = startItem->NextSiblingElement("start");
            continue;
        }
        found=false;
        const TiXmlElement * description = startItem->FirstChildElement("description");
        if(!language.isEmpty() && language!="en")
            while(description!=NULL)
            {
                if(description->Attribute("lang")!=NULL && *description->Attribute(std::string("lang"))==language.toStdString())
                {
                    profile.description=description->GetText();
                    found=true;
                    break;
                }
                description = description->NextSiblingElement("description");
            }
        if(!found)
        {
            description = startItem->FirstChildElement("description");
            while(description!=NULL)
            {
                if(description->Attribute("lang")==NULL || *description->Attribute(std::string("lang"))=="en")
                {
                    profile.description=description->GetText();
                    break;
                }
                description = description->NextSiblingElement("description");
            }
        }
        if(profile.description.isEmpty())
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, description empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile).arg(startItem->GetText()).arg("?"));
            startItem = startItem->NextSiblingElement("start");
            continue;
        }
        profileTextList << profile;
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
        ui->description->setText(profileTextList.at(index).description);
}

void NewProfile::datapackParsed()
{
    if(isParsingDatapack)
    {
        isParsingDatapack=false;
        emit parseDatapack(datapackPath);
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.size()==1)//do into the next screen:  && CatchChallenger::CommonDatapack::commonDatapack.profileList.first().forcedskin.size()==1
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
