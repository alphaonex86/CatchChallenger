#include "NewProfile.h"
#include "ui_NewProfile.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QMessageBox>

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/DebugClass.h"
#include "../base/interface/DatapackClientLoader.h"

NewProfile::NewProfile(const QString &datapackPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProfile)
{
    ui->setupUi(this);
    this->datapackPath=datapackPath;
    this->ok=false;
    ui->description->setText(tr("Profile loading..."));

    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_PLAYER+"start.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file to have new profile: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    //load the content
    bool ok;
    QDomElement startItem = root.firstChildElement("start");
    while(!startItem.isNull())
    {
        if(startItem.isElement())
        {
            Profile profile;
            QDomElement name = startItem.firstChildElement("name");
            while(!name.isNull())
            {
                if(name.isElement())
                {
                    if(name.hasAttribute("lang") && name.attribute("lang")=="en")
                    {
                        profile.name=name.text();
                        break;
                    }
                }
                name = name.nextSiblingElement("name");
            }
            if(profile.name.isEmpty())
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }
            QDomElement description = startItem.firstChildElement("description");
            while(!description.isNull())
            {
                if(description.isElement())
                {
                    if(description.hasAttribute("lang") && description.attribute("lang")=="en")
                    {
                        profile.description=description.text();
                        break;
                    }
                }
                description = description.nextSiblingElement("description");
            }
            if(profile.description.isEmpty())
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description empty or not found: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }
            QDomElement map = startItem.firstChildElement("map");
            if(!map.isNull() && map.isElement() && map.hasAttribute("file") && map.hasAttribute("x") && map.hasAttribute("y"))
            {
                profile.map=map.attribute("file");
                if(!QFile::exists(datapackPath+DATAPACK_BASE_PATH_MAP+profile.map))
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map don't exists %2: child.tagName(): %3 (at line: %4)").arg(xmlFile.fileName()).arg(profile.map).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
                profile.x=map.attribute("x").toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map x is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
                profile.y=map.attribute("y").toUShort(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, map y is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    startItem = startItem.nextSiblingElement("start");
                    continue;
                }
            }
            else
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, no correct map configuration: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }
            QDomElement forcedskin = startItem.firstChildElement("forcedskin");
            if(!forcedskin.isNull() && forcedskin.isElement() && forcedskin.hasAttribute("value"))
            {
                profile.forcedskin=forcedskin.attribute("value").split(";");
                int index=0;
                while(index<profile.forcedskin.size())
                {
                    if(!QFile::exists(datapackPath+DATAPACK_BASE_PATH_SKIN+profile.forcedskin.at(index)))
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, skin %4 don't exists: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()).arg(profile.forcedskin.at(index)));
                        profile.forcedskin.removeAt(index);
                    }
                    else
                        index++;
                }
            }
            profile.cash=0;
            QDomElement cash = startItem.firstChildElement("cash");
            if(!cash.isNull() && cash.isElement() && cash.hasAttribute("value"))
            {
                profile.cash=cash.attribute("value").toULongLong(&ok);
                if(!ok)
                {
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, cash is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                    profile.cash=0;
                }
            }
            QDomElement monstersElement = startItem.firstChildElement("monster");
            while(!monstersElement.isNull())
            {
                Profile::Monster monster;
                if(monstersElement.isElement() && monstersElement.hasAttribute("id") && monstersElement.hasAttribute("level") && monstersElement.hasAttribute("captured_with"))
                {
                    monster.id=monstersElement.attribute("id").toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement("monster");
                        continue;
                    }
                    monster.level=monstersElement.attribute("level").toUShort(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement("monster");
                        continue;
                    }
                    if(monster.level==0 || monster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, monster level is not into the range: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement("monster");
                        continue;
                    }
                    monster.captured_with=monstersElement.attribute("captured_with").toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, captured_with is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        monstersElement = monstersElement.nextSiblingElement("monster");
                        continue;
                    }
                    profile.monsters << monster;
                }
                monstersElement = monstersElement.nextSiblingElement("monster");
            }
            if(profile.monsters.empty())
            {
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, not monster to load: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                startItem = startItem.nextSiblingElement("start");
                continue;
            }
            QDomElement reputationElement = startItem.firstChildElement("reputation");
            while(!reputationElement.isNull())
            {
                Profile::Reputation reputationTemp;
                if(reputationElement.isElement() && reputationElement.hasAttribute("type") && reputationElement.hasAttribute("level"))
                {
                    reputationTemp.type=reputationElement.attribute("type");
                    reputationTemp.level=reputationElement.attribute("level").toShort(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation level is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        reputationElement = reputationElement.nextSiblingElement("reputation");
                        continue;
                    }
                    reputationTemp.point=0;
                    if(reputationElement.hasAttribute("point"))
                    {
                        reputationTemp.point=reputationElement.attribute("point").toInt(&ok);
                        if(!ok)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation point is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            reputationElement = reputationElement.nextSiblingElement("reputation");
                            continue;
                        }
                        if((reputationTemp.point>0 && reputationTemp.level<0) || (reputationTemp.point<0 && reputationTemp.level>=0))
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, reputation point is not negative/positive like the level: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            reputationElement = reputationElement.nextSiblingElement("reputation");
                            continue;
                        }
                    }
                    profile.reputation << reputationTemp;
                }
                reputationElement = reputationElement.nextSiblingElement("reputation");
            }
            QDomElement itemElement = startItem.firstChildElement("item");
            while(!itemElement.isNull())
            {
                Profile::Item itemTemp;
                if(itemElement.isElement() && itemElement.hasAttribute("id"))
                {
                    itemTemp.id=itemElement.attribute("id").toUInt(&ok);
                    if(!ok)
                    {
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item id is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                        itemElement = itemElement.nextSiblingElement("item");
                        continue;
                    }
                    itemTemp.quantity=0;
                    if(itemElement.hasAttribute("quantity"))
                    {
                        itemTemp.quantity=itemElement.attribute("quantity").toUInt(&ok);
                        if(!ok)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item quantity is not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            itemElement = itemElement.nextSiblingElement("item");
                            continue;
                        }
                        if(itemTemp.quantity==0)
                        {
                            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, item quantity is null: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
                            itemElement = itemElement.nextSiblingElement("item");
                            continue;
                        }
                    }
                    profile.items << itemTemp;
                }
                itemElement = itemElement.nextSiblingElement("item");
            }
            profileList << profile;
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(startItem.tagName()).arg(startItem.lineNumber()));
        startItem = startItem.nextSiblingElement("start");
    }
    if(profileList.size()==0)
    {
        QMessageBox::critical(this,tr("Error"),tr("No profile selected to start a new game"));
        close();
        return;
    }
    ui->comboBox->clear();
    int index=0;
    while(index<profileList.size())
    {
        ui->comboBox->addItem(profileList.at(index).name);
        index++;
    }
    if(ui->comboBox->count()>0)
        ui->description->setText(profileList.at(ui->comboBox->currentIndex()).description);

    connect(this,&NewProfile::parseDatapack,&DatapackClientLoader::datapackLoader,&DatapackClientLoader::parseDatapack,Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,&DatapackClientLoader::datapackParsed,this,&NewProfile::datapackParsed,Qt::QueuedConnection);
    isParsingDatapack=DatapackClientLoader::datapackLoader.isParsingDatapack();
    if(!isParsingDatapack)
        emit parseDatapack(datapackPath);
}

NewProfile::~NewProfile()
{
    delete ui;
}

NewProfile::Profile NewProfile::getProfile()
{
    if(ui->comboBox->count()>0)
        return profileList.at(ui->comboBox->currentIndex());
    else
    {
        NewProfile::Profile p;
        return p;
    }
}

void NewProfile::on_ok_clicked()
{
    ok=true;
    close();
}

void NewProfile::on_pushButton_2_clicked()
{
    close();
}

void NewProfile::on_comboBox_currentIndexChanged(int index)
{
    if(ui->comboBox->count()>0)
        ui->description->setText(profileList.at(index).description);
}

void NewProfile::datapackParsed()
{
    if(isParsingDatapack)
    {
        isParsingDatapack=false;
        emit parseDatapack(datapackPath);
    }
    if(profileList.size()==1)
    {
        CatchChallenger::DebugClass::debugConsole("Default profile selected");
        ok=true;
        close();//only one, then select it
        return;
    }
    ui->ok->setEnabled(true);
    show();
}
