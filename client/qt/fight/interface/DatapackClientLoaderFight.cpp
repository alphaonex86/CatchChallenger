#include "../../QtDatapackClientLoader.h"
#include "../../LanguagesSelect.h"
#include "../../../../general/base/GeneralVariable.h"
#include "../../../../general/base/FacilityLib.h"
#include "../../../../general/fight/FightLoader.h"
#include "../../../../general/base/CommonDatapack.h"
#include "../../../../general/base/CommonDatapackServerSpec.h"
#include "../../fight/interface/ClientFightEngine.h"

#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <iostream>

void QtDatapackClientLoader::parseMonstersExtra()
{
    QDir dir(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MONSTERS);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        tinyxml2::XMLDocument *domDocument=NULL;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=
                CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"monsters")!=0)
        {
            //qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"monsters\" root balise not found for the xml file").arg(QString::fromStdString(file)));
            file_index++;
            continue;
        }

        const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("monster");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t tempid=stringtouint32(item->Attribute("id"),&ok);
                if(!ok)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                else
                {
                    const uint16_t id=static_cast<uint16_t>(tempid);
                    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(id)
                            ==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster list into monster extra: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        QtDatapackClientLoader::MonsterExtra monsterExtraEntry;
                        QtDatapackClientLoader::QtMonsterExtra QtmonsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        {
                            bool found=false;
                            const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.name=name->GetText();
                                        found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("name");
                                }
                            if(!found)
                            {
                                name = item->FirstChildElement("name");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                        if(name->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.name=name->GetText();
                                            break;
                                        }
                                    name = name->NextSiblingElement("name");
                                }
                            }
                        }
                        {
                            bool found=false;
                            const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                            if(!language.empty() && language!="en")
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language && description->GetText()!=NULL)
                                    {
                                            monsterExtraEntry.description=description->GetText();
                                            found=true;
                                            break;
                                    }
                                    description = description->NextSiblingElement("description");
                                }
                            if(!found)
                            {
                                description = item->FirstChildElement("description");
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                        if(description->GetText()!=NULL)
                                        {
                                                monsterExtraEntry.description=description->GetText();
                                                break;
                                        }
                                    description = description->NextSiblingElement("description");
                                }
                            }
                        }
                        //load the kind
                        {
                            bool kind_found=false;
                            const tinyxml2::XMLElement *kind = item->FirstChildElement("kind");
                            if(!language.empty() && language!="en")
                                while(kind!=NULL)
                                {
                                    if(kind->Attribute("lang")!=NULL && kind->Attribute("lang")==language && kind->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.kind=kind->GetText();
                                        kind_found=true;
                                        break;
                                    }
                                    kind = kind->NextSiblingElement("kind");
                                }
                            if(!kind_found)
                            {
                                kind = item->FirstChildElement("kind");
                                while(kind!=NULL)
                                {
                                    if(kind->Attribute("lang")==NULL || strcmp(kind->Attribute("lang"),"en")==0)
                                        if(kind->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.kind=kind->GetText();
                                            kind_found=true;
                                            break;
                                        }
                                    kind = kind->NextSiblingElement("kind");
                                }
                            }
                        }

                        //load the habitat
                        {
                            bool habitat_found=false;
                            const tinyxml2::XMLElement *habitat = item->FirstChildElement("habitat");
                            if(!language.empty() && language!="en")
                                while(habitat!=NULL)
                                {
                                    if(habitat->Attribute("lang") && habitat->Attribute("lang")==language && habitat->GetText()!=NULL)
                                    {
                                        monsterExtraEntry.habitat=habitat->GetText();
                                        habitat_found=true;
                                        break;
                                    }
                                    habitat = habitat->NextSiblingElement("habitat");
                                }
                            if(!habitat_found)
                            {
                                habitat = item->FirstChildElement("habitat");
                                while(habitat!=NULL)
                                {
                                    if(habitat->Attribute("lang")==NULL || strcmp(habitat->Attribute("lang"),"en")==0)
                                        if(habitat->GetText()!=NULL)
                                        {
                                            monsterExtraEntry.habitat=habitat->GetText();
                                            habitat_found=true;
                                            break;
                                        }
                                    habitat = habitat->NextSiblingElement("habitat");
                                }
                            }
                        }
                        const std::string basepath=datapackPath+"/"+DATAPACK_BASE_PATH_MONSTERS+std::to_string(id);
                        if(monsterExtraEntry.name.empty())
                            monsterExtraEntry.name=tr("Unknown").toStdString();
                        if(monsterExtraEntry.description.empty())
                            monsterExtraEntry.description=tr("Unknown").toStdString();
                        monsterExtraEntry.frontPath=basepath+"/front.png";
                        QtmonsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                        if(QtmonsterExtraEntry.front.isNull())
                        {
                            monsterExtraEntry.frontPath=basepath+"/front.gif";
                            QtmonsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                            if(QtmonsterExtraEntry.front.isNull())
                            {
                                monsterExtraEntry.frontPath=":/CC/images/monsters/default/front.png";
                                QtmonsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                            }
                        }
                        monsterExtraEntry.backPath=basepath+"/back.png";
                        QtmonsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                        if(QtmonsterExtraEntry.back.isNull())
                        {
                            monsterExtraEntry.backPath=basepath+"/back.gif";
                            QtmonsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                            if(QtmonsterExtraEntry.back.isNull())
                            {
                                monsterExtraEntry.backPath=":/CC/images/monsters/default/back.png";
                                QtmonsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                            }
                        }
                        QtmonsterExtraEntry.thumb=QString::fromStdString(basepath+"/small.png");
                        if(QtmonsterExtraEntry.thumb.isNull())
                        {
                            QtmonsterExtraEntry.thumb=QString::fromStdString(basepath+"/small.gif");
                            if(QtmonsterExtraEntry.thumb.isNull())
                                QtmonsterExtraEntry.thumb=QPixmap(":/CC/images/monsters/default/small.png");
                        }
                        QtmonsterExtraEntry.thumb=QtmonsterExtraEntry.thumb.scaled(64,64);
                        QtDatapackClientLoader::datapackLoader->monsterExtra[id]=monsterExtraEntry;
                        QtDatapackClientLoader::datapackLoader->QtmonsterExtra[id]=QtmonsterExtraEntry;
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster id at monster extra: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement("monster");
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsters.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->monsterExtra.find(i->first)==
                QtDatapackClientLoader::datapackLoader->monsterExtra.cend())
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i->first));
            QtDatapackClientLoader::MonsterExtra monsterExtraEntry;
            QtDatapackClientLoader::QtMonsterExtra QtmonsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown").toStdString();
            monsterExtraEntry.description=tr("Unknown").toStdString();
            QtmonsterExtraEntry.front=QPixmap(":/CC/images/monsters/default/front.png");
            QtmonsterExtraEntry.back=QPixmap(":/CC/images/monsters/default/back.png");
            QtmonsterExtraEntry.thumb=QPixmap(":/CC/images/monsters/default/small.png");
            QtmonsterExtraEntry.thumb=QtmonsterExtraEntry.thumb.scaled(64,64);
            QtDatapackClientLoader::datapackLoader->monsterExtra[i->first]=monsterExtraEntry;
            QtDatapackClientLoader::datapackLoader->QtmonsterExtra[i->first]=QtmonsterExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->monsterExtra.size());
}


void QtDatapackClientLoader::parseBuffExtra()
{
    QDir dir(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_BUFF);
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        const std::string dotpng(".png");
        const std::string dotgif(".gif");
        tinyxml2::XMLDocument *domDocument;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=
                CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFile[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"buffs")!=0)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"buffs\" root balise not found for the xml file")
                         .arg(QString::fromStdString(file)));
            file_index++;
            continue;
        }

        const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("buff");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t tempid=stringtouint8(item->Attribute("id"),&ok);
                if(!ok || tempid>255)
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                else
                {
                    const uint8_t &id=static_cast<uint8_t>(tempid);
                    if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(id)==
                            CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster buff list into buff extra: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        QtDatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                        QtDatapackClientLoader::QtMonsterExtra::QtBuff QtmonsterBuffExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        bool found=false;
                        const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                        if(!language.empty() && language!="en")
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                {
                                    monsterBuffExtraEntry.name=name->GetText();
                                    found=true;
                                    break;
                                }
                                name = name->NextSiblingElement("name");
                            }
                        if(!found)
                        {
                            name = item->FirstChildElement("name");
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    if(name->GetText()!=NULL)
                                    {
                                        monsterBuffExtraEntry.name=name->GetText();
                                        break;
                                    }
                                name = name->NextSiblingElement("name");
                            }
                        }
                        found=false;
                        const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                        if(!language.empty() && language!="en")
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang") && description->Attribute("lang")==language && description->GetText()!=NULL)
                                {
                                    monsterBuffExtraEntry.description=description->GetText();
                                    found=true;
                                    break;
                                }
                                description = description->NextSiblingElement("description");
                            }
                        if(!found)
                        {
                            description = item->FirstChildElement("description");
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                    if(description->GetText()!=NULL)
                                    {
                                        monsterBuffExtraEntry.description=description->GetText();
                                        break;
                                    }
                                description = description->NextSiblingElement("description");
                            }
                        }
                        if(monsterBuffExtraEntry.name.empty())
                            monsterBuffExtraEntry.name=tr("Unknown").toStdString();
                        if(monsterBuffExtraEntry.description.empty())
                            monsterBuffExtraEntry.description=tr("Unknown").toStdString();
                        const std::string basePath=datapackPath+DATAPACK_BASE_PATH_BUFFICON+std::to_string(id);
                        const std::string pngFile=basePath+dotpng;
                        const std::string gifFile=basePath+dotgif;
                        if(QFile(QString::fromStdString(pngFile)).exists())
                            QtmonsterBuffExtraEntry.icon=QIcon(QString::fromStdString(pngFile));
                        else if(QFile(QString::fromStdString(gifFile)).exists())
                            QtmonsterBuffExtraEntry.icon=QIcon(QString::fromStdString(gifFile));
                        const QList<QSize> &availableSizes=QtmonsterBuffExtraEntry.icon.availableSizes();
                        if(QtmonsterBuffExtraEntry.icon.isNull() || availableSizes.isEmpty())
                            QtmonsterBuffExtraEntry.icon=QIcon(QStringLiteral(":/CC/images/interface/buff.png"));
                        QtDatapackClientLoader::datapackLoader->monsterBuffsExtra[id]=monsterBuffExtraEntry;
                        QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra[id]=QtmonsterBuffExtraEntry;
                    }
                }
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the buff id: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement("buff");
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(i->first)==
                QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster buffer extra for id: %1").arg(i->first));
            QtDatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
            QtDatapackClientLoader::QtMonsterExtra::QtBuff QtmonsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown").toStdString();
            monsterBuffExtraEntry.description=tr("Unknown").toStdString();
            QtmonsterBuffExtraEntry.icon=QIcon(":/CC/images/interface/buff.png");
            QtDatapackClientLoader::datapackLoader->monsterBuffsExtra[i->first]=monsterBuffExtraEntry;
            QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra[i->first]=QtmonsterBuffExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 buff(s) extra loaded").arg(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.size());
}
