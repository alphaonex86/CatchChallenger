#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/LanguagesSelect.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/fight/FightLoader.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../fight/interface/ClientFightEngine.h"

#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <iostream>

void DatapackClientLoader::parseMonstersExtra()
{
    const std::string frontgif("/front.gif");
    const std::string frontpng("/front.png");
    const std::string backgif("/back.gif");
    const std::string backpng("/back.png");
    const std::string smallgif("/small.gif");
    const std::string smallpng("/small.png");
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
        if(strcmp(root->Name(),"monsters")!=0)
        {
            //qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"monsters\" root balise not found for the xml file").arg(file));
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
                        DatapackClientLoader::MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                        #endif
                        {
                            bool found=false;
                            const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language)
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
                                    if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language)
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
                                    if(kind->Attribute("lang")!=NULL && kind->Attribute("lang")==language)
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
                                    if(habitat->Attribute("lang") && habitat->Attribute("lang")==language)
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
                                    {
                                        monsterExtraEntry.habitat=habitat->GetText();
                                        habitat_found=true;
                                        break;
                                    }
                                    habitat = habitat->NextSiblingElement("habitat");
                                }
                            }
                        }
                        const std::string &basepath=datapackPath+DatapackClientLoader::text_slash+DATAPACK_BASE_PATH_MONSTERS+std::to_string(id);
                        if(monsterExtraEntry.name.empty())
                            monsterExtraEntry.name=tr("Unknown").toStdString();
                        if(monsterExtraEntry.description.empty())
                            monsterExtraEntry.description=tr("Unknown").toStdString();
                        monsterExtraEntry.frontPath=basepath+frontpng;
                        monsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                        if(monsterExtraEntry.front.isNull())
                        {
                            monsterExtraEntry.frontPath=basepath+frontgif;
                            monsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                            if(monsterExtraEntry.front.isNull())
                            {
                                monsterExtraEntry.frontPath=":/images/monsters/default/front.png";
                                monsterExtraEntry.front=QPixmap(QString::fromStdString(monsterExtraEntry.frontPath));
                            }
                        }
                        monsterExtraEntry.backPath=basepath+backpng;
                        monsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                        if(monsterExtraEntry.back.isNull())
                        {
                            monsterExtraEntry.backPath=basepath+backgif;
                            monsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                            if(monsterExtraEntry.back.isNull())
                            {
                                monsterExtraEntry.backPath=":/images/monsters/default/back.png";
                                monsterExtraEntry.back=QPixmap(QString::fromStdString(monsterExtraEntry.backPath));
                            }
                        }
                        monsterExtraEntry.thumb=QString::fromStdString(basepath+smallpng);
                        if(monsterExtraEntry.thumb.isNull())
                        {
                            monsterExtraEntry.thumb=QString::fromStdString(basepath+smallgif);
                            if(monsterExtraEntry.thumb.isNull())
                                monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
                        }
                        monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
                        DatapackClientLoader::datapackLoader.monsterExtra[id]=monsterExtraEntry;
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
        if(DatapackClientLoader::datapackLoader.monsterExtra.find(i->first)==
                DatapackClientLoader::datapackLoader.monsterExtra.cend())
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i->first));
            DatapackClientLoader::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown").toStdString();
            monsterExtraEntry.description=tr("Unknown").toStdString();
            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
            monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
            monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
            DatapackClientLoader::datapackLoader.monsterExtra[i->first]=monsterExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterExtra.size());
}

void DatapackClientLoader::parseTypesExtra()
{
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_MONSTERS+"type.xml";
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
            return;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(strcmp(root->Name(),"types")!=0)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"types\" root balise not found for the xml file").arg(QString::fromStdString(file));
        return;
    }

    //load the content
    {
        const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        std::unordered_set<std::string> duplicate;
        const tinyxml2::XMLElement *typeItem = root->FirstChildElement("type");
        while(typeItem!=NULL)
        {
            if(typeItem->Attribute("name")!=NULL)
            {
                std::string name=typeItem->Attribute("name");
                if(duplicate.find(name)==duplicate.cend())
                {
                    TypeExtra type;

                    duplicate << name;
                    if(typeItem.hasAttribute(DatapackClientLoader::text_color))
                    {
                        QColor color;
                        color.setNamedColor(typeItem.attribute(DatapackClientLoader::text_color));
                        if(color.isValid())
                            type.color=color;
                        else
                            qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child->Name(): %2")
                                         .arg(QString::fromStdString(file)).arg(typeItem->Name()));
                    }

                    bool found=false;
                    tinyxml2::XMLElement nameItem = typeItem->FirstChildElement(DatapackClientLoader::text_name);
                    if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                        while(!nameItem.isNull())
                        {
                            if(nameItem.isElement())
                            {
                                if(nameItem.hasAttribute(DatapackClientLoader::text_lang) && nameItem.attribute(DatapackClientLoader::text_lang)==language)
                                {
                                    type.name=nameItem->GetText();
                                    found=true;
                                    break;
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                             .arg(QString::fromStdString(file)).arg(nameItem->Name()));
                            nameItem = nameItem->NextSiblingElement(DatapackClientLoader::text_name);
                        }
                    if(!found)
                    {
                        nameItem = typeItem->FirstChildElement(DatapackClientLoader::text_name);
                        while(!nameItem.isNull())
                        {
                            if(nameItem.isElement())
                            {
                                if(!nameItem.hasAttribute(DatapackClientLoader::text_lang) || nameItem.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                {
                                    type.name=nameItem->GetText();
                                    break;
                                }
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                             .arg(QString::fromStdString(file)).arg(nameItem->Name()));
                            nameItem = nameItem->NextSiblingElement(DatapackClientLoader::text_name);
                        }
                    }
                    if(DatapackClientLoader::datapackLoader.typeExtra.size()>255)
                    {
                        qDebug() << QStringLiteral("Unable to open the file: %1, DatapackClientLoader::datapackLoader.typeExtra.size()>255: child->Name(): %2")
                                    .arg(QString::fromStdString(file)).arg(typeItem->Name());
                        abort();
                    }
                    DatapackClientLoader::datapackLoader.typeExtra[
                            static_cast<uint8_t>(DatapackClientLoader::datapackLoader.typeExtra.size())
                            ]=type;
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child->Name(): %2")
                                .arg(QString::fromStdString(file)).arg(typeItem->Name());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child->Name(): %2")
                            .arg(QString::fromStdString(file)).arg(typeItem->Name());
            typeItem = typeItem->NextSiblingElement(DatapackClientLoader::text_type);
        }
    }

    qDebug() << QStringLiteral("%1 type(s) extra loaded").arg(DatapackClientLoader::datapackLoader.typeExtra.size());
}

void DatapackClientLoader::parseBuffExtra()
{
    QDir dir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_BUFF));
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const QString &file=fileList.at(file_index).absoluteFilePath();
        if(!file.endsWith(DatapackClientLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        const QString &dotpng=QStringLiteral(".png");
        const QString &dotgif=QStringLiteral(".gif");
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
        tinyxml2::XMLElement root = domDocument.RootElement();
        if(root->Name()!=DatapackClientLoader::text_buffs)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"buffs\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        //load the content
        bool ok;
        tinyxml2::XMLElement item = root->FirstChildElement(DatapackClientLoader::text_buff);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(!ok || tempid>255)
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        const uint8_t &id=static_cast<uint8_t>(tempid);
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(id)==CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster buff list into buff extra: child->Name(): %2")
                                         .arg(QString::fromStdString(file)).arg(item->Name()));
                        else
                        {
                            DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                            #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                            qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                            #endif
                            bool found=false;
                            tinyxml2::XMLElement name = item->FirstChildElement(DatapackClientLoader::text_name);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterBuffExtraEntry.name=name->GetText();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    name = name->NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            if(!found)
                            {
                                name = item->FirstChildElement(DatapackClientLoader::text_name);
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterBuffExtraEntry.name=name->GetText();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    name = name->NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            }
                            found=false;
                            tinyxml2::XMLElement description = item->FirstChildElement(DatapackClientLoader::text_description);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterBuffExtraEntry.description=description->GetText();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    description = description->NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            if(!found)
                            {
                                description = item->FirstChildElement(DatapackClientLoader::text_description);
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterBuffExtraEntry.description=description->GetText();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    description = description->NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            }
                            if(monsterBuffExtraEntry.name.isEmpty())
                                monsterBuffExtraEntry.name=tr("Unknown");
                            if(monsterBuffExtraEntry.description.isEmpty())
                                monsterBuffExtraEntry.description=tr("Unknown");
                            const QString &basePath=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_BUFFICON)+QString::number(id);
                            const QString &pngFile=basePath+dotpng;
                            const QString &gifFile=basePath+dotgif;
                            if(QFile(pngFile).exists())
                                monsterBuffExtraEntry.icon=QIcon(pngFile);
                            else if(QFile(gifFile).exists())
                                monsterBuffExtraEntry.icon=QIcon(gifFile);
                            QList<QSize> availableSizes=monsterBuffExtraEntry.icon.availableSizes();
                            if(monsterBuffExtraEntry.icon.isNull() || availableSizes.isEmpty())
                                monsterBuffExtraEntry.icon=QIcon(QStringLiteral(":/images/interface/buff.png"));
                            DatapackClientLoader::datapackLoader.monsterBuffsExtra[id]=monsterBuffExtraEntry;
                        }
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the buff id: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement(DatapackClientLoader::text_buff);
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
    {
        if(!DatapackClientLoader::datapackLoader.monsterBuffsExtra.contains(i->first))
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster buffer extra for id: %1").arg(i->first));
            DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
            monsterBuffExtraEntry.name=tr("Unknown");
            monsterBuffExtraEntry.description=tr("Unknown");
            monsterBuffExtraEntry.icon=QIcon(QStringLiteral(":/images/interface/buff.png"));
            DatapackClientLoader::datapackLoader.monsterBuffsExtra[i->first]=monsterBuffExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 buff(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterBuffsExtra.size());
}

void DatapackClientLoader::parseSkillsExtra()
{
    QDir dir(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_SKILL));
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const QString &file=fileList.at(file_index).absoluteFilePath();
        if(!file.endsWith(DatapackClientLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
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
        tinyxml2::XMLElement root = domDocument.RootElement();
        if(root->Name()!=DatapackClientLoader::text_skills)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"skills\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found;
        //load the content
        bool ok;
        tinyxml2::XMLElement item = root->FirstChildElement(DatapackClientLoader::text_skill);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    const uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(!ok || tempid>65535)
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child->Name(): %2")
                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                    else
                    {
                        const uint16_t &id=static_cast<uint16_t>(tempid);
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(id)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                        {}
                        else
                        {
                            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
                            #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                            qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                            #endif
                            found=false;
                            tinyxml2::XMLElement name = item->FirstChildElement(DatapackClientLoader::text_name);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterSkillExtraEntry.name=name->GetText();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    name = name->NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            if(!found)
                            {
                                name = item->FirstChildElement(DatapackClientLoader::text_name);
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterSkillExtraEntry.name=name->GetText();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    name = name->NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            }
                            found=false;
                            tinyxml2::XMLElement description = item->FirstChildElement(DatapackClientLoader::text_description);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterSkillExtraEntry.description=description->GetText();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    description = description->NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            if(!found)
                            {
                                description = item->FirstChildElement(DatapackClientLoader::text_description);
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterSkillExtraEntry.description=description->GetText();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child->Name(): %2")
                                                     .arg(QString::fromStdString(file)).arg(item->Name()));
                                    description = description->NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            }
                            if(monsterSkillExtraEntry.name.isEmpty())
                                monsterSkillExtraEntry.name=tr("Unknown");
                            if(monsterSkillExtraEntry.description.isEmpty())
                                monsterSkillExtraEntry.description=tr("Unknown");
                            monsterSkillsExtra[id]=monsterSkillExtraEntry;
                        }
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the skill id: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child->Name(): %2")
                             .arg(QString::fromStdString(file)).arg(item->Name()));
            item = item->NextSiblingElement(DatapackClientLoader::text_skill);
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
    {
        if(!monsterSkillsExtra.contains(i->first))
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster skill extra for id: %1").arg(i->first));
            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name=tr("Unknown");
            monsterSkillExtraEntry.description=tr("Unknown");
            monsterSkillsExtra[i->first]=monsterSkillExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 skill(s) extra loaded").arg(monsterSkillsExtra.size());
}

void DatapackClientLoader::parseBotFightsExtra()
{
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    bool found;
    QDir dir(datapackPath+DATAPACK_BASE_PATH_FIGHT1+mainDatapackCode+DATAPACK_BASE_PATH_FIGHT2);
    QFileInfoList list=dir.entryInfoList(QStringList(),QDir::NoDotAndDotDot|QDir::Files);
    int index_file=0;
    while(index_file<list.size())
    {
        if(list.at(index_file).isFile())
        {
            const QString &file=list.at(index_file).absoluteFilePath();
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
            tinyxml2::XMLElement root = domDocument.RootElement();
            if(root->Name()!=DatapackClientLoader::text_fights)
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file: %2!=%3").arg(file).arg(root->Name()).arg(DatapackClientLoader::text_fights));
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            tinyxml2::XMLElement item = root->FirstChildElement(DatapackClientLoader::text_fight);
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute(DatapackClientLoader::text_id))
                    {
                        const uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                        if(ok && tempid<65535)
                        {
                            const uint16_t id=static_cast<uint16_t>(tempid);
                            if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(id)!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                            {
                                if(!botFightsExtra.contains(id))
                                {
                                    BotFightExtra botFightExtra;
                                    botFightExtra.start=tr("Ready for the fight?");
                                    botFightExtra.win=tr("You are so strong for me!");
                                    {
                                        found=false;
                                        tinyxml2::XMLElement start = item->FirstChildElement(DatapackClientLoader::text_start);
                                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(start.hasAttribute(DatapackClientLoader::text_lang) && start.attribute(DatapackClientLoader::text_lang)==language)
                                                    {
                                                        botFightExtra.start=start->GetText();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child->Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                                                start = start->NextSiblingElement(DatapackClientLoader::text_start);
                                            }
                                        if(!found)
                                        {
                                            start = item->FirstChildElement(DatapackClientLoader::text_start);
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(!start.hasAttribute(DatapackClientLoader::text_lang) || start.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                                    {
                                                        botFightExtra.start=start->GetText();
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child->Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                                                start = start->NextSiblingElement(DatapackClientLoader::text_start);
                                            }
                                        }
                                        found=false;
                                        tinyxml2::XMLElement win = item->FirstChildElement(DatapackClientLoader::text_win);
                                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(win.hasAttribute(DatapackClientLoader::text_lang) && win.attribute(DatapackClientLoader::text_lang)==language)
                                                    {
                                                        botFightExtra.win=win->GetText();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child->Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                                                win = win->NextSiblingElement(DatapackClientLoader::text_win);
                                            }
                                        if(!found)
                                        {
                                            win = item->FirstChildElement(DatapackClientLoader::text_win);
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(!win.hasAttribute(DatapackClientLoader::text_lang) || win.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                                    {
                                                        botFightExtra.win=win->GetText();
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child->Name(): %2")
                                                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                                                win = win->NextSiblingElement(DatapackClientLoader::text_win);
                                            }
                                        }
                                    }
                                    botFightsExtra[id]=botFightExtra;
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id is already into the botFight extra, child->Name(): %2")
                                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, bot fights have not the id %3, child->Name(): %2")
                                             .arg(QString::fromStdString(file)).arg(item->Name()).arg(id));
                        }
                        else
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, id is not a number to parse bot fight extra, child->Name(): %2")
                                         .arg(QString::fromStdString(file)).arg(item->Name()));
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child->Name(): %2")
                                 .arg(QString::fromStdString(file)).arg(item->Name()));
                item = item->NextSiblingElement(DatapackClientLoader::text_fight);
            }
            index_file++;
        }
    }

    auto i=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.begin();
    while(i!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
    {
        if(!botFightsExtra.contains(i->first))
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into bot fight extra for id: %1").arg(i->first));
            BotFightExtra botFightExtra;
            botFightExtra.start=tr("Ready for the fight?");
            botFightExtra.win=tr("You are so strong for me!");
            botFightsExtra[i->first]=botFightExtra;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 fight extra(s) loaded").arg(botFightsExtra.size());
}
