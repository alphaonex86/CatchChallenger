#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QDomElement>
#include <QDomDocument>
#include <QDebug>

#define LANG "fr"

QString datapackPath;

struct Item
{
    QString image;
    QString nameEn;
    QString nameJp;
    QString nameFr;
    QString nameDe;
    QString nameIt;
    QString nameEs;
    QString nameKo;
};
QHash<QString,Item> itemList;

bool loadItems(const QString &path,QString data)
{
    data.remove(QRegularExpression("^.*Field and miscellaneous items",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    data.remove(QRegularExpression("<div id='catlinks' class='catlinks'>.*$",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    QStringList itemStringTempList=data.split("</td></tr>");

    const QString &preg=QStringLiteral("<tr style=\"text-align:center; background:#FFFFFF\">[\n\r\t ]*<td>[\n\r\t ]*<a href=\"http://bulbapedia.bulbagarden.net/wiki/File[^\"]+.png\" class=\"image\" title=\"[^\"]+\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"2[0-9]\" height=\"2[0-9]\" /></a>[\n\r\t ]*</td>[\n\r\t ]*<td>[\n\r\t ]*<a [^>]+>([^<]+)</a>[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td lang=\"ja\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)");
    const QString &preg2=QStringLiteral("<tr style=\"text-align:center; background:#FFFFFF\">[\n\r\t ]*<td>[\n\r\t ]*<a href=\"http://bulbapedia.bulbagarden.net/wiki/File[^\"]+.png\" class=\"image\" title=\"[^\"]+\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"2[0-9]\" height=\"2[0-9]\" /></a>[\n\r\t ]*</td>[\n\r\t ]*<td>[\n\r\t ]*<a [^>]+>([^<]+)</a>[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td lang=\"ja\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*([^<]+)");
    int index=0;
    while(index<itemStringTempList.size())
    {
        Item item;
        QString tempString=itemStringTempList.at(index);

        if(tempString.contains(QRegularExpression(preg,QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
        {
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
            item.image=tempString;
            item.image.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
            item.image=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.image).absoluteFilePath();
            QString englishName1=item.nameEn;
            englishName1.remove(QRegularExpression("[ \r\t\n\\.\\+]+"));
            englishName1=englishName1.toLower();
            itemList[englishName1]=item;
        }
        else if(tempString.contains(QRegularExpression(preg2,QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
        {
            item.nameEn=tempString;
            item.nameEn.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
            item.nameEn.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEn.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameJp=tempString;
            item.nameJp.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
            item.nameJp.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameJp.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameFr=tempString;
            item.nameFr.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\5");
            item.nameFr.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameFr.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameDe=tempString;
            item.nameDe.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\6");
            item.nameDe.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameDe.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameIt=tempString;
            item.nameIt.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\7");
            item.nameIt.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameIt.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEs=tempString;
            item.nameEs.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\8");
            item.nameEs.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.nameEs.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.image=tempString;
            item.image.replace(QRegularExpression(".*"+preg2+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
            item.image=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.image).absoluteFilePath();
            QString englishName=item.nameEn;
            englishName.remove(QRegularExpression("[ \r\t\n\\.\\+]+"));
            englishName=englishName.toLower();
            itemList[englishName]=item;
        }
        else
            qDebug() << QStringLiteral("item ignored");

        index++;
    }
    return true;
}

void parseItemsExtra()
{
    quint32 maxId=0;
    QSet<QString> itemLoaded;
    //open and quick check the file
    QFile itemsFile(datapackPath+"items/items.xml");
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
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("item");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toULongLong(&ok);
                if(id>maxId)
                    maxId=id;
                if(ok)
                {
                    const QString &language=LANG;

                    QString englishName;
                    //load the name
                    {
                        QDomElement name = item.firstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        englishName=name.text();
                                        englishName.remove(QRegularExpression("[ \r\t\n\\.\\+]+"));
                                        englishName=englishName.toLower();
                                        itemLoaded << englishName;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement("name");
                            }
                    }
                    if(!englishName.isEmpty() && itemList.contains(englishName))
                    {
                        if(QFile(itemList[englishName].image).exists())
                            QFile::remove(datapackPath+QStringLiteral("items/%1.png").arg(id));
                        if(QFile::copy(itemList[englishName].image,datapackPath+QStringLiteral("items/%1.png").arg(id)))
                            item.setAttribute("image",QStringLiteral("%1.png").arg(id));
                        if(!itemList[englishName].nameEn.isEmpty())
                        {
                            bool found=false;
                            //set the english name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameEn);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameEn);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameJp.isEmpty())
                        {
                            bool found=false;
                            //set the jp name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="jp")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameJp);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","jp");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameJp);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameFr.isEmpty())
                        {
                            bool found=false;
                            //set the fr name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="fr")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameFr);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","fr");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameFr);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameDe.isEmpty())
                        {
                            bool found=false;
                            //set the de name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="de")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameDe);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","de");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameDe);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameIt.isEmpty())
                        {
                            bool found=false;
                            //set the it name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="it")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameIt);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","it");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameIt);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameEs.isEmpty())
                        {
                            bool found=false;
                            //set the es name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="es")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameEs);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","es");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameEs);
                                name.appendChild(newTextElement);
                            }
                        }
                        if(!itemList[englishName].nameKo.isEmpty())
                        {
                            bool found=false;
                            //set the ko name
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")=="ko")
                                    {
                                        QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameKo);
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
                                name = name.nextSiblingElement("name");
                            }
                            if(!found)
                            {
                                QDomElement name=domDocument.createElement("name");
                                name.setAttribute("lang","ko");
                                item.appendChild(name);
                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[englishName].nameKo);
                                name.appendChild(newTextElement);
                            }
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    QHashIterator<QString, Item> i(itemList);
    while (i.hasNext()) {
        i.next();
        if(!itemLoaded.contains(i.key()))
        {
            maxId++;
            QDomElement item=domDocument.createElement("item");
            item.setAttribute("id",maxId);
            if(QFile(itemList[i.key()].image).exists())
                QFile::remove(datapackPath+QStringLiteral("items/%1.png").arg(maxId));
            if(QFile::copy(itemList[i.key()].image,datapackPath+QStringLiteral("items/%1.png").arg(maxId)))
                item.setAttribute("image",QStringLiteral("%1.png").arg(maxId));
            if(!itemList[i.key()].nameEn.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameEn);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameJp.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang","jp");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameJp);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameFr.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang","fr");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameFr);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameDe.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang","de");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameDe);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameIt.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang","it");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameIt);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameEs.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang","es");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[i.key()].nameEs);
                name.appendChild(newTextElement);
            }
            if(!itemList[i.key()].nameKo.isEmpty())
            {
                QDomElement name=domDocument.createElement("name");
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
