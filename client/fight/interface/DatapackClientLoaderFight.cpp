#include "../../base/interface/DatapackClientLoader.h"
#include "../../base/LanguagesSelect.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/FacilityLib.h"
#include "../../../general/fight/FightLoader.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../fight/interface/ClientFightEngine.h"

#include <tinyxml2::XMLElement>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QDebug>

void DatapackClientLoader::parseMonstersExtra()
{
    QString STATIC_DATAPACK_BASE_PATH_MONSTERS=QStringLiteral(DATAPACK_BASE_PATH_MONSTERS);
    const QString &frontgif=QStringLiteral("/front.gif");
    const QString &frontpng=QStringLiteral("/front.png");
    const QString &backgif=QStringLiteral("/back.gif");
    const QString &backpng=QStringLiteral("/back.png");
    const QString &smallgif=QStringLiteral("/small.gif");
    const QString &smallpng=QStringLiteral("/small.png");
    QDir dir(datapackPath+STATIC_DATAPACK_BASE_PATH_MONSTERS);
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
        QDomDocument domDocument;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
        else
        {
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                qDebug() << (QStringLiteral("Unable to open the xml monster extra file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
        }
        tinyxml2::XMLElement root = domDocument.RootElement();
        if(root.tagName()!=DatapackClientLoader::text_monsters)
        {
            //qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"monsters\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        //load the content
        bool ok;
        tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_monster);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(!ok || tempid>65535)
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        const uint16_t id=static_cast<uint16_t>(tempid);
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsters.find(id)
                                ==CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster list into monster extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        else
                        {
                            DatapackClientLoader::MonsterExtra monsterExtraEntry;
                            #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                            qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                            #endif
                            {
                                bool found=false;
                                tinyxml2::XMLElement name = item.FirstChildElement(DatapackClientLoader::text_name);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!name.isNull())
                                    {
                                        if(name.isElement())
                                        {
                                            if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                monsterExtraEntry.name=name.text();
                                                found=true;
                                                break;
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                    }
                                if(!found)
                                {
                                    name = item.FirstChildElement(DatapackClientLoader::text_name);
                                    while(!name.isNull())
                                    {
                                        if(name.isElement())
                                        {
                                            if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                monsterExtraEntry.name=name.text();
                                                break;
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                    }
                                }
                            }
                            {
                                bool found=false;
                                tinyxml2::XMLElement description = item.FirstChildElement(DatapackClientLoader::text_description);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!description.isNull())
                                    {
                                        if(description.isElement())
                                        {
                                            if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                    monsterExtraEntry.description=description.text();
                                                    found=true;
                                                    break;
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        description = description.NextSiblingElement(DatapackClientLoader::text_description);
                                    }
                                if(!found)
                                {
                                    description = item.FirstChildElement(DatapackClientLoader::text_description);
                                    while(!description.isNull())
                                    {
                                        if(description.isElement())
                                        {
                                            if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                    monsterExtraEntry.description=description.text();
                                                    break;
                                            }
                                        }
                                        else
                                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                        description = description.NextSiblingElement(DatapackClientLoader::text_description);
                                    }
                                }
                            }
                            //load the kind
                            {
                                bool kind_found=false;
                                tinyxml2::XMLElement kind = item.FirstChildElement(DatapackClientLoader::text_kind);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!kind.isNull())
                                    {
                                        if(kind.isElement())
                                        {
                                            if(kind.hasAttribute(DatapackClientLoader::text_lang) && kind.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                monsterExtraEntry.kind=kind.text();
                                                kind_found=true;
                                                break;
                                            }
                                        }
                                        kind = kind.NextSiblingElement(DatapackClientLoader::text_kind);
                                    }
                                if(!kind_found)
                                {
                                    kind = item.FirstChildElement(DatapackClientLoader::text_kind);
                                    while(!kind.isNull())
                                    {
                                        if(kind.isElement())
                                        {
                                            if(!kind.hasAttribute(DatapackClientLoader::text_lang) || kind.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                monsterExtraEntry.kind=kind.text();
                                                kind_found=true;
                                                break;
                                            }
                                        }
                                        kind = kind.NextSiblingElement(DatapackClientLoader::text_kind);
                                    }
                                }
                            }

                            //load the habitat
                            {
                                bool habitat_found=false;
                                tinyxml2::XMLElement habitat = item.FirstChildElement(DatapackClientLoader::text_habitat);
                                if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                    while(!habitat.isNull())
                                    {
                                        if(habitat.isElement())
                                        {
                                            if(habitat.hasAttribute(DatapackClientLoader::text_lang) && habitat.attribute(DatapackClientLoader::text_lang)==language)
                                            {
                                                monsterExtraEntry.habitat=habitat.text();
                                                habitat_found=true;
                                                break;
                                            }
                                        }
                                        habitat = habitat.NextSiblingElement(DatapackClientLoader::text_habitat);
                                    }
                                if(!habitat_found)
                                {
                                    habitat = item.FirstChildElement(DatapackClientLoader::text_habitat);
                                    while(!habitat.isNull())
                                    {
                                        if(habitat.isElement())
                                        {
                                            if(!habitat.hasAttribute(DatapackClientLoader::text_lang) || habitat.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                            {
                                                monsterExtraEntry.habitat=habitat.text();
                                                habitat_found=true;
                                                break;
                                            }
                                        }
                                        habitat = habitat.NextSiblingElement(DatapackClientLoader::text_habitat);
                                    }
                                }
                            }
                            const QString &basepath=datapackPath+DatapackClientLoader::text_slash+STATIC_DATAPACK_BASE_PATH_MONSTERS+QString::number(id);
                            if(monsterExtraEntry.name.isEmpty())
                                monsterExtraEntry.name=tr("Unknown");
                            if(monsterExtraEntry.description.isEmpty())
                                monsterExtraEntry.description=tr("Unknown");
                            monsterExtraEntry.frontPath=basepath+frontpng;
                            monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                            if(monsterExtraEntry.front.isNull())
                            {
                                monsterExtraEntry.frontPath=basepath+frontgif;
                                monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                                if(monsterExtraEntry.front.isNull())
                                {
                                    monsterExtraEntry.frontPath=QStringLiteral(":/images/monsters/default/front.png");
                                    monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                                }
                            }
                            monsterExtraEntry.backPath=basepath+backpng;
                            monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                            if(monsterExtraEntry.back.isNull())
                            {
                                monsterExtraEntry.backPath=basepath+backgif;
                                monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                                if(monsterExtraEntry.back.isNull())
                                {
                                    monsterExtraEntry.backPath=QStringLiteral(":/images/monsters/default/back.png");
                                    monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                                }
                            }
                            monsterExtraEntry.thumb=basepath+smallpng;
                            if(monsterExtraEntry.thumb.isNull())
                            {
                                monsterExtraEntry.thumb=basepath+smallgif;
                                if(monsterExtraEntry.thumb.isNull())
                                    monsterExtraEntry.thumb=QPixmap(QStringLiteral(":/images/monsters/default/small.png"));
                            }
                            monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
                            DatapackClientLoader::datapackLoader.monsterExtra[id]=monsterExtraEntry;
                        }
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the monster id at monster extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.NextSiblingElement(DatapackClientLoader::text_monster);
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.monsters.begin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.monsters.cend())
    {
        if(!DatapackClientLoader::datapackLoader.monsterExtra.contains(i->first))
        {
            qDebug() << (QStringLiteral("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i->first));
            DatapackClientLoader::MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(QStringLiteral(":/images/monsters/default/front.png"));
            monsterExtraEntry.back=QPixmap(QStringLiteral(":/images/monsters/default/back.png"));
            monsterExtraEntry.thumb=QPixmap(QStringLiteral(":/images/monsters/default/small.png"));
            monsterExtraEntry.thumb=monsterExtraEntry.thumb.scaled(64,64);
            DatapackClientLoader::datapackLoader.monsterExtra[i->first]=monsterExtraEntry;
        }
        ++i;
    }

    qDebug() << QStringLiteral("%1 monster(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterExtra.size());
}

void DatapackClientLoader::parseTypesExtra()
{
    const QString &file=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MONSTERS)+QStringLiteral("type.xml");
    QDomDocument domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
        domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
    else
    {
        QFile itemsFile(file);
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
            return;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
    }
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!=DatapackClientLoader::text_types)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"types\" root balise not found for the xml file").arg(file);
        return;
    }

    //load the content
    {
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        QSet<QString> duplicate;
        tinyxml2::XMLElement typeItem = root.FirstChildElement(DatapackClientLoader::text_type);
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(DatapackClientLoader::text_name))
                {
                    QString name=typeItem.attribute(DatapackClientLoader::text_name);
                    if(!duplicate.contains(name))
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
                                qDebug() << (QStringLiteral("Unable to open the file: %1, color is not valid: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber()));
                        }

                        bool found=false;
                        tinyxml2::XMLElement nameItem = typeItem.FirstChildElement(DatapackClientLoader::text_name);
                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                            while(!nameItem.isNull())
                            {
                                if(nameItem.isElement())
                                {
                                    if(nameItem.hasAttribute(DatapackClientLoader::text_lang) && nameItem.attribute(DatapackClientLoader::text_lang)==language)
                                    {
                                        type.name=nameItem.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(nameItem.tagName()).arg(nameItem.lineNumber()));
                                nameItem = nameItem.NextSiblingElement(DatapackClientLoader::text_name);
                            }
                        if(!found)
                        {
                            nameItem = typeItem.FirstChildElement(DatapackClientLoader::text_name);
                            while(!nameItem.isNull())
                            {
                                if(nameItem.isElement())
                                {
                                    if(!nameItem.hasAttribute(DatapackClientLoader::text_lang) || nameItem.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                    {
                                        type.name=nameItem.text();
                                        break;
                                    }
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(nameItem.tagName()).arg(nameItem.lineNumber()));
                                nameItem = nameItem.NextSiblingElement(DatapackClientLoader::text_name);
                            }
                        }
                        if(DatapackClientLoader::datapackLoader.typeExtra.size()>255)
                        {
                            qDebug() << QStringLiteral("Unable to open the file: %1, DatapackClientLoader::datapackLoader.typeExtra.size()>255: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                            abort();
                        }
                        DatapackClientLoader::datapackLoader.typeExtra[
                                static_cast<uint8_t>(DatapackClientLoader::datapackLoader.typeExtra.size())
                                ]=type;
                    }
                    else
                        qDebug() << QStringLiteral("Unable to open the file: %1, name is already set for type: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(typeItem.tagName()).arg(typeItem.lineNumber());
            typeItem = typeItem.NextSiblingElement(DatapackClientLoader::text_type);
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
        QDomDocument domDocument;
        //open and quick check the file
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
        else
        {
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
        }
        tinyxml2::XMLElement root = domDocument.RootElement();
        if(root.tagName()!=DatapackClientLoader::text_buffs)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"buffs\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        //load the content
        bool ok;
        tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_buff);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(!ok || tempid>255)
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        const uint8_t &id=static_cast<uint8_t>(tempid);
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(id)==CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster buff list into buff extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        else
                        {
                            DatapackClientLoader::MonsterExtra::Buff monsterBuffExtraEntry;
                            #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                            qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                            #endif
                            bool found=false;
                            tinyxml2::XMLElement name = item.FirstChildElement(DatapackClientLoader::text_name);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterBuffExtraEntry.name=name.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            if(!found)
                            {
                                name = item.FirstChildElement(DatapackClientLoader::text_name);
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterBuffExtraEntry.name=name.text();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            }
                            found=false;
                            tinyxml2::XMLElement description = item.FirstChildElement(DatapackClientLoader::text_description);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterBuffExtraEntry.description=description.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            if(!found)
                            {
                                description = item.FirstChildElement(DatapackClientLoader::text_description);
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterBuffExtraEntry.description=description.text();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.NextSiblingElement(DatapackClientLoader::text_description);
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
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the buff id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.NextSiblingElement(DatapackClientLoader::text_buff);
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
        QDomDocument domDocument;
        if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
            domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
        else
        {
            QFile xmlFile(file);
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                file_index++;
                continue;
            }
        }
        tinyxml2::XMLElement root = domDocument.RootElement();
        if(root.tagName()!=DatapackClientLoader::text_skills)
        {
            qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"skills\" root balise not found for the xml file").arg(file));
            file_index++;
            continue;
        }

        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        bool found;
        //load the content
        bool ok;
        tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_skill);
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(DatapackClientLoader::text_id))
                {
                    const uint32_t tempid=item.attribute(DatapackClientLoader::text_id).toUInt(&ok);
                    if(!ok || tempid>65535)
                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        const uint16_t &id=static_cast<uint16_t>(tempid);
                        if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(id)==CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                        {}//qDebug() << (QStringLiteral("Unable to open the xml file: %1, id not into monster skill list into skill extra: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                        else
                        {
                            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
                            #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                            qDebug() << (QStringLiteral("monster extra loading: %1").arg(id));
                            #endif
                            found=false;
                            tinyxml2::XMLElement name = item.FirstChildElement(DatapackClientLoader::text_name);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute(DatapackClientLoader::text_lang) && name.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterSkillExtraEntry.name=name.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            if(!found)
                            {
                                name = item.FirstChildElement(DatapackClientLoader::text_name);
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute(DatapackClientLoader::text_lang) || name.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterSkillExtraEntry.name=name.text();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.NextSiblingElement(DatapackClientLoader::text_name);
                                }
                            }
                            found=false;
                            tinyxml2::XMLElement description = item.FirstChildElement(DatapackClientLoader::text_description);
                            if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute(DatapackClientLoader::text_lang) && description.attribute(DatapackClientLoader::text_lang)==language)
                                        {
                                            monsterSkillExtraEntry.description=description.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.NextSiblingElement(DatapackClientLoader::text_description);
                                }
                            if(!found)
                            {
                                description = item.FirstChildElement(DatapackClientLoader::text_description);
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute(DatapackClientLoader::text_lang) || description.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                        {
                                            monsterSkillExtraEntry.description=description.text();
                                            break;
                                        }
                                    }
                                    else
                                        qDebug() << (QStringLiteral("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.NextSiblingElement(DatapackClientLoader::text_description);
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
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            }
            else
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
            item = item.NextSiblingElement(DatapackClientLoader::text_skill);
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
            QDomDocument domDocument;
            //open and quick check the file
            if(CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.find(file.toStdString())!=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.cend())
                domDocument=CatchChallenger::CommonDatapack::commonDatapack.xmlLoadedFileQt.at(file.toStdString());
            else
            {
                QFile xmlFile(file);
                QByteArray xmlContent;
                if(!xmlFile.open(QIODevice::ReadOnly))
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, error: %2").arg(file).arg(xmlFile.errorString()));
                    index_file++;
                    continue;
                }
                xmlContent=xmlFile.readAll();
                xmlFile.close();
                QString errorStr;
                int errorLine,errorColumn;
                if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
                {
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr));
                    index_file++;
                    continue;
                }
            }
            tinyxml2::XMLElement root = domDocument.RootElement();
            if(root.tagName()!=DatapackClientLoader::text_fights)
            {
                qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"fights\" root balise not found for the xml file: %2!=%3").arg(file).arg(root.tagName()).arg(DatapackClientLoader::text_fights));
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            tinyxml2::XMLElement item = root.FirstChildElement(DatapackClientLoader::text_fight);
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
                                        tinyxml2::XMLElement start = item.FirstChildElement(DatapackClientLoader::text_start);
                                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(start.hasAttribute(DatapackClientLoader::text_lang) && start.attribute(DatapackClientLoader::text_lang)==language)
                                                    {
                                                        botFightExtra.start=start.text();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                start = start.NextSiblingElement(DatapackClientLoader::text_start);
                                            }
                                        if(!found)
                                        {
                                            start = item.FirstChildElement(DatapackClientLoader::text_start);
                                            while(!start.isNull())
                                            {
                                                if(start.isElement())
                                                {
                                                    if(!start.hasAttribute(DatapackClientLoader::text_lang) || start.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                                    {
                                                        botFightExtra.start=start.text();
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, start balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                start = start.NextSiblingElement(DatapackClientLoader::text_start);
                                            }
                                        }
                                        found=false;
                                        tinyxml2::XMLElement win = item.FirstChildElement(DatapackClientLoader::text_win);
                                        if(!language.isEmpty() && language!=DatapackClientLoader::text_en)
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(win.hasAttribute(DatapackClientLoader::text_lang) && win.attribute(DatapackClientLoader::text_lang)==language)
                                                    {
                                                        botFightExtra.win=win.text();
                                                        found=true;
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                win = win.NextSiblingElement(DatapackClientLoader::text_win);
                                            }
                                        if(!found)
                                        {
                                            win = item.FirstChildElement(DatapackClientLoader::text_win);
                                            while(!win.isNull())
                                            {
                                                if(win.isElement())
                                                {
                                                    if(!win.hasAttribute(DatapackClientLoader::text_lang) || win.attribute(DatapackClientLoader::text_lang)==DatapackClientLoader::text_en)
                                                    {
                                                        botFightExtra.win=win.text();
                                                        break;
                                                    }
                                                }
                                                else
                                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, win balise is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                                                win = win.NextSiblingElement(DatapackClientLoader::text_win);
                                            }
                                        }
                                    }
                                    botFightsExtra[id]=botFightExtra;
                                }
                                else
                                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, id is already into the botFight extra, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                            }
                            else
                                qDebug() << (QStringLiteral("Unable to open the xml file: %1, bot fights have not the id %4, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id));
                        }
                        else
                            qDebug() << (QStringLiteral("Unable to open the xml file: %1, id is not a number to parse bot fight extra, child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                    }
                }
                else
                    qDebug() << (QStringLiteral("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()));
                item = item.NextSiblingElement(DatapackClientLoader::text_fight);
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
