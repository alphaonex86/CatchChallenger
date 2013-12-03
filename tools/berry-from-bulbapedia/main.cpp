#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QDomElement>
#include <QDomDocument>
#include <QDebug>
#include <QImage>
#include <QPainter>

#define LANG "fr"

QString datapackPath;

struct Plant
{
    quint32 itemUsed;
    quint32 fruits_seconds;
    //float quantity;//splited into 2 integer
    quint16 fix_quantity;
    quint16 random_quantity;
    //minimal memory impact at exchange of drop dual xml parse
    quint16 sprouted_seconds;
    quint16 taller_seconds;
    quint16 flowering_seconds;
};
QHash<quint8, Plant> plants;

struct Item
{
    QString name;
    QString imageA,imageB,imageC,imageD,imageE;
};
QHash<QString,Item> plantExtraList;

QHash<QString,QString> itemsExtra;

bool loadBerry(const QString &path,QString data)
{
    data.remove(QRegularExpression("^.*but can also be used to make",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    data.remove(QRegularExpression("In Generation V.*$",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    data.remove(QRegularExpression("^.*List of Berries",QRegularExpression::CaseInsensitiveOption|QRegularExpression::DotMatchesEverythingOption));
    QStringList itemStringTempList=data.split(QRegularExpression("</td>[\n\r\t ]*</tr>"));

    const QString &preg=QStringLiteral("<tr style=\"background:#FFFFFF;\">[\n\r\t ]*"
                                       "<th>[\n\r\t ]*[0-9]+[\n\r\t ]*</th>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"[^\"]+\" width=\"24\" height=\"24\" />[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*[\n\r\t ]*([^<]+)[\n\r\t ]*[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*[^<]*[\n\r\t ]*[\n\r\t ]*[^<]+[\n\r\t ]*[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"([^<]+)\" width=\"22\" height=\"32\" />[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"([^<]+)\" width=\"22\" height=\"32\" />[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"([^<]+)\" width=\"22\" height=\"32\" />[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"([^<]+)\" width=\"22\" height=\"32\" />[\n\r\t ]*</td>[\n\r\t ]*"
                                       "<td>[\n\r\t ]*<img alt=\"[^\"]+\" src=\"([^<]+)\" width=\"22\" height=\"32\" />"
                                       );
    int index=0;
    while(index<itemStringTempList.size())
    {
        Item item;
        QString tempString=itemStringTempList.at(index);

        if(tempString.contains(QRegularExpression(preg,QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption)))
        {
            item.name=tempString;
            item.name.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
            item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            item.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
            QString code=item.name;
            code.remove(QRegularExpression("[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
            code=code.toLower();
            item.imageA=tempString;
            item.imageA.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
            item.imageA=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.imageA).absoluteFilePath();
            item.imageB=tempString;
            item.imageB.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
            item.imageB=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.imageB).absoluteFilePath();
            item.imageC=tempString;
            item.imageC.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\4");
            item.imageC=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.imageC).absoluteFilePath();
            item.imageD=tempString;
            item.imageD.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\5");
            item.imageD=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.imageD).absoluteFilePath();
            item.imageE=tempString;
            item.imageE.replace(QRegularExpression(".*"+preg+".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\6");
            item.imageE=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.imageE).absoluteFilePath();
            plantExtraList[code]=item;
        }
        else
            qDebug() << QString("plant ignored");

        index++;
    }
    return true;
}

void parseItemsExtra()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+"items/items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
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
                item.attribute("id").toULongLong(&ok);
                if(ok)
                {
                    if(!itemsExtra.contains(item.attribute("id")))
                    {
                        //load the name
                        {
                            QDomElement name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        QString tempName=name.text();
                                        tempName.remove(QRegularExpression("[ \r\n\t]+"));
                                        tempName=tempName.toLower();
                                        itemsExtra[item.attribute("id")]=tempName;
                                        break;
                                    }
                                }
                                name = name.nextSiblingElement("name");
                            }
                        }
                    }
                    else
                        qDebug() << QString("Unable to open the file: %1, id number already set: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
                }
                else
                    qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    qDebug() << QString("%1 item(s) extra loaded").arg(itemsExtra.size());
}

void createPlantImage()
{
    //open and quick check the file
    QFile itemsFile(datapackPath+"plants/plants.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadWrite))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="plants")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok,ok2;
    QDomElement skillitem = root.firstChildElement("plant");
    while(!skillitem.isNull())
    {
        if(skillitem.isElement())
        {
            if(skillitem.hasAttribute("id") && skillitem.hasAttribute("itemUsed"))
            {
                skillitem.attribute("id").toULongLong(&ok);
                skillitem.attribute("itemUsed").toULongLong(&ok2);
                if(ok && ok2)
                {
                    QString itemUsed=skillitem.attribute("itemUsed");
                    if(itemsExtra.contains(itemUsed))
                    {
                        QString imagePath=datapackPath+QString("plants/%1.png").arg(skillitem.attribute("id"));
                        if(!QFile(imagePath).exists() && itemsExtra.contains(itemUsed))
                        {
                            if(plantExtraList.contains(itemsExtra[itemUsed]))
                            {
                                QImage imageA(plantExtraList[itemsExtra[itemUsed]].imageA);
                                QImage imageB(plantExtraList[itemsExtra[itemUsed]].imageB);
                                QImage imageC(plantExtraList[itemsExtra[itemUsed]].imageC);
                                QImage imageD(plantExtraList[itemsExtra[itemUsed]].imageD);
                                QImage imageE(plantExtraList[itemsExtra[itemUsed]].imageE);
                                if(imageA.isNull() || imageB.isNull() || imageC.isNull() || imageD.isNull() || imageE.isNull())
                                    qDebug() << QString("Unable to open the file: %1, one image is null: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
                                else if(imageA.size()!=QSize(22,32) || imageB.size()!=QSize(22,32) || imageC.size()!=QSize(22,32) || imageD.size()!=QSize(22,32) || imageE.size()!=QSize(22,32))
                                    qDebug() << QString("Unable to open the file: %1, one image have wrong size: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
                                else
                                {
                                    QImage imageAR=imageA.copy(3,0,16,32);
                                    QImage imageBR=imageB.copy(3,0,16,32);
                                    QImage imageCR=imageC.copy(3,0,16,32);
                                    QImage imageDR=imageD.copy(3,0,16,32);
                                    QImage imageER=imageE.copy(3,0,16,32);

                                    QImage destImage = QImage(80, 32, QImage::Format_ARGB32);
                                    destImage.fill(Qt::transparent);
                                    QPoint destPos;

                                    QPainter painter(&destImage);
                                    destPos = QPoint(0, 0);
                                    painter.drawImage(destPos, imageAR);
                                    destPos = QPoint(15, 0);
                                    painter.drawImage(destPos, imageBR);
                                    destPos = QPoint(31, 0);
                                    painter.drawImage(destPos, imageCR);
                                    destPos = QPoint(47, 0);
                                    painter.drawImage(destPos, imageDR);
                                    destPos = QPoint(63, 0);
                                    painter.drawImage(destPos, imageER);
                                    painter.end();

                                    destImage.save(imagePath);
                                }
                            }
                        }
                    }
                }
                else
                    qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(skillitem.tagName()).arg(skillitem.lineNumber());
        skillitem = skillitem.nextSiblingElement("plant");
    }
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
        data.remove(QRegularExpression("<a[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
        data.remove("</a>");
        data.remove(QRegularExpression("<p[^>]*>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption));
        data.remove("</p>");
        data.replace(QRegularExpression("<br />[^>]+[ \r\t\n]*</td>"),"</td>");
        loadBerry(arguments.at(1),data);
    }
    parseItemsExtra();
    createPlantImage();

    return 0;
}
