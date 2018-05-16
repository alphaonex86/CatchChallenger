#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <tinyxml2::XMLElement>
#include <QDomDocument>
#include <QDebug>

#define LANG "fr"

QString datapackPath;

struct Item
{
    QString nameEn;
    QString nameJp;
    QString nameFr;
    QString nameDe;
    QString nameIt;
    QString nameEs;
    QString nameKo;
};
QHash<quint32,Item> itemList;

bool loadItems(const QString &path,QString data)
{
    data.remove(QRegularExpression("^.*List of moves in other languages",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    data.remove(QRegularExpression("Shadow moves.*$",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    QStringList itemStringTempList=data.split("</td></tr>");

    const QString &preg=QStringLiteral("<tr style=\"text-align:center; background:#[0-9a-zA-Z]+\">[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*<a [^>]+>([^<]+)</a>[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td lang=\"ja\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                        "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*"
                                       );
    int index=0;
    while(index<itemStringTempList.size())
    {
        Item item;
        QString tempString=itemStringTempList.at(index);

        if(tempString.contains(QRegularExpression(preg,QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
        {
            QString id=tempString;
            id.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
            id.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            id.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEn=tempString;
            item.nameEn.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
            item.nameEn.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEn.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameJp=tempString;
            item.nameJp.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
            item.nameJp.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameJp.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameFr=tempString;
            item.nameFr.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\5");
            item.nameFr.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameFr.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameDe=tempString;
            item.nameDe.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\6");
            item.nameDe.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameDe.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameIt=tempString;
            item.nameIt.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\7");
            item.nameIt.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameIt.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEs=tempString;
            item.nameEs.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\8");
            item.nameEs.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEs.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameKo=tempString;
            item.nameKo.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\9");
            item.nameKo.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameKo.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            itemList[id.toUInt()]=item;
        }
        else
            qDebug() << QStringLiteral("item ignored");

        index++;
    }
    return true;
}

void parseItemsExtra()
{
    QSet<quint32> itemLoaded;
    //open and quick check the file
    QFile itemsFile(datapackPath+"monsters/skill.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadWrite))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!="list")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    tinyxml2::XMLElement skillitem = root.FirstChildElement("skill");
    while(!skillitem.isNull())
    {
        if(skillitem.isElement())
        {
            if(skillitem.hasAttribute("id"))
            {
                quint32 id=skillitem.attribute("id").toULongLong(&ok);
                if(ok)
                {
                    if(itemList.contains(id))
                    {
                        itemLoaded << id;
                        if(!itemList[id].nameEn.isEmpty())
                        {
                            bool found=false;
                            //set the english name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameEn);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameEn);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameJp.isEmpty())
                        {
                            bool found=false;
                            //set the jp name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="jp")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameJp);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","jp");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameJp);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameFr.isEmpty())
                        {
                            bool found=false;
                            //set the fr name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="fr")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameFr);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","fr");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameFr);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameDe.isEmpty())
                        {
                            bool found=false;
                            //set the de name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="de")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameDe);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","de");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameDe);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameIt.isEmpty())
                        {
                            bool found=false;
                            //set the it name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="it")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameIt);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","it");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameIt);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameEs.isEmpty())
                        {
                            bool found=false;
                            //set the es name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="es")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameEs);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","es");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameEs);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[id].nameKo.isEmpty())
                        {
                            bool found=false;
                            //set the ko name
                            tinyxml2::XMLElement name = skillitem.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="ko")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameKo);
                                        QDomNodeList nodeList=name.childNodes();
                                        int index=0;
                                        while(index<nodeList.size())
                                        {
                                            nodeList.at(index).parentNode().removeChild(nodeList.at(index));
                                            index++;
                                        }
                                        name.appendChild(newTextElement);
                                        found=true;
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!found)
                            {
                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                name.setAttribute("lang","ko");
                                skillitem.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[id].nameKo);
                                name.appendChild(newTextElement);
                            }
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
        skillitem = skillitem.NextSiblingElement("skill");
    }

    QHashIterator<quint32, Item> i(itemList);
    while (i.hasNext()) {
        i.next();
        if(!itemLoaded.contains(i.key()))
        {
            tinyxml2::XMLElement item=domDocument.createElement("skill");
            item.setAttribute("id",i.key());
            if(!itemList[i.key()].nameEn.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameEn);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameJp.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","jp");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameJp);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameFr.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","fr");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameFr);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameDe.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","de");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameDe);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameIt.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","it");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameIt);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameEs.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","es");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameEs);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameKo.isEmpty())
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang","ko");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameKo);
                name.appendChild(newTextElement);
            }
            root.appendChild(item);
        }
    }
    itemsFile.seek(0);
    itemsFile.resize(0);
    itemsFile.write(domDocument.toByteArray(4));
    itemsFile.close();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList arguments=QCoreApplication::arguments();
    if(arguments.size()!=3)
    {
        qDebug() << "pass only the file to parse and datapack path to update as arguments";
        return 1;
    }
    if(!QFile(arguments.last()+"/items/items.xml").exists())
    {
        qDebug() << "missing items/items.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/skill.xml").exists())
    {
        qDebug() << "missing monsters/skill.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/monster.xml").exists())
    {
        qDebug() << "missing monsters/monster.xml into the path given";
        return 2;
    }
    datapackPath=arguments.last()+"/";

    QByteArray endOfString;
    endOfString[0]=0x00;
    QFile file(arguments.at(1));
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray rawData=file.readAll();
        rawData.replace(endOfString,QByteArray());
        QString data=QString::fromUtf8(rawData);
        file.close();
        data.remove(QRegularExpression("<span[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
        data.remove("</span>");
        data.remove(QRegularExpression("<p[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
        data.remove("</p>");
        data.replace(QRegularExpression("<br />[^>]+[ \r\t\n]*</td>"),"</td>");
        loadItems(arguments.at(1),data);
    }
    parseItemsExtra();

    return 0;
}
