#include "NewProfile.h"
#include "ui_NewProfile.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QMessageBox>

#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../base/LanguagesSelect.h"

NewProfile::NewProfile(const QString &datapackPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProfile)
{
    ui->setupUi(this);
    this->datapackPath=datapackPath;
    this->mOk=false;
    ui->description->setText(tr("Profile loading..."));

    if(CatchChallenger::CommonDatapack::commonDatapack.profileList.isEmpty())
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
    const QString &xmlFile=datapackPath+DATAPACK_BASE_PATH_PLAYER+"start.xml";
    QList<QDomElement> xmlList=CatchChallenger::DatapackGeneralLoader::loadProfileList(datapackPath,xmlFile).first;
    int index=0;
    while(index<xmlList.size())
    {
        ProfileText profile;
        QDomElement startItem=xmlList.at(index);
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found=false;
        QDomElement name = startItem.firstChildElement("name");
        if(!language.isEmpty() && language!="en")
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                    {
                        profile.name=name.text();
                        found=true;
                        break;
                    }
                }
                name = name.nextSiblingElement("name");
            }
        if(!found)
        {
            name = startItem.firstChildElement("name");
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                    {
                        profile.name=name.text();
                        break;
                    }
                }
                name = name.nextSiblingElement("name");
            }
        }
        if(profile.name.isEmpty())
        {
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile).arg(startItem.tagName()).arg(startItem.lineNumber()));
            startItem = startItem.nextSiblingElement("start");
            continue;
        }
        found=false;
        QDomElement description = startItem.firstChildElement("description");
        if(!language.isEmpty() && language!="en")
            while(!description.isNull())
            {
                if(description.isElement())
                {
                    if(description.hasAttribute("lang") && description.attribute("lang")==language)
                    {
                        profile.description=description.text();
                        found=true;
                        break;
                    }
                }
                description = description.nextSiblingElement("description");
            }
        if(!found)
        {
            description = startItem.firstChildElement("description");
            while(!description.isNull())
            {
                if(description.isElement())
                {
                    if(!description.hasAttribute("lang") || description.attribute("lang")=="en")
                    {
                        profile.description=description.text();
                        break;
                    }
                }
                description = description.nextSiblingElement("description");
            }
        }
        if(profile.description.isEmpty())
        {
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile).arg(startItem.tagName()).arg(startItem.lineNumber()));
            startItem = startItem.nextSiblingElement("start");
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
        CatchChallenger::DebugClass::debugConsole("Default profile selected");
        mOk=true;
        close();//only one, then select it
        return;
    }
    ui->ok->setEnabled(true);
    show();
}
