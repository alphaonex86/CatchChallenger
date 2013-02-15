#include "DatapackGeneralLoader.h"
#include "DebugClass.h"

#include <QFile>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>

using namespace CatchChallenger;

QHash<QString, Reputation> DatapackGeneralLoader::loadReputation(const QString &file)
{
    QHash<QString, Reputation> reputation;
    //open and quick check the file
    QFile itemsFile(file);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString()));
        return reputation;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return reputation;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        DebugClass::debugConsole(QString("Unable to open the items file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName()));
        return reputation;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("reputation");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("type"))
            {
                QList<qint32> point_list_positive,point_list_negative;
                QStringList text_positive,text_negative;
                QDomElement level = item.firstChildElement("level");
                ok=true;
                while(!level.isNull() && ok)
                {
                    if(level.isElement())
                    {
                        if(level.hasAttribute("point"))
                        {
                            qint32 point=level.attribute("point").toInt(&ok);
                            QString text_val;
                            if(ok)
                            {
                                QDomElement text = level.firstChildElement("text");
                                ok=true;
                                while(!text.isNull() && ok)
                                {
                                    if(text.isElement())
                                    {
                                        if(text.hasAttribute("lang"))
                                            if(text.attribute("lang")=="en" && !text.text().isEmpty())
                                            {
                                                text_val=text.text();
                                                break;
                                            }
                                    }
                                    else
                                        DebugClass::debugConsole(QString("Unable to open the items file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    text = text.nextSiblingElement("level");
                                }
                                if(text_val.isEmpty())
                                    ok=false;
                                if(ok)
                                {
                                    bool found=false;
                                    int index=0;
                                    if(point>=0)
                                    {
                                        while(index<point_list_positive.size())
                                        {
                                            if(point_list_positive.at(index)==point)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the items file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                found=true;
                                                ok=false;
                                                break;
                                            }
                                            if(point_list_positive.at(index)>point)
                                            {
                                                point_list_positive.insert(index,point);
                                                text_positive.insert(index,text_val);
                                                found=true;
                                                break;
                                            }
                                            index++;
                                        }
                                        if(!found)
                                        {
                                            point_list_positive << point;
                                            text_positive << text_val;
                                        }
                                    }
                                    else
                                    {
                                        while(index<point_list_negative.size())
                                        {
                                            if(point_list_negative.at(index)==point)
                                            {
                                                DebugClass::debugConsole(QString("Unable to open the items file: %1, reputation level with same number of point found!: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                                found=true;
                                                ok=false;
                                                break;
                                            }
                                            if(point_list_negative.at(index)>point)
                                            {
                                                point_list_negative.insert(index,point);
                                                text_negative.insert(index,text_val);
                                                found=true;
                                                break;
                                            }
                                            index++;
                                        }
                                        if(!found)
                                        {
                                            point_list_negative << point;
                                            text_negative << text_val;
                                        }
                                    }
                                }
                            }
                            else
                                DebugClass::debugConsole(QString("Unable to open the items file: %1, point is not number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        }
                    }
                    else
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, point attribute not found: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    level = level.nextSiblingElement("level");
                }
                if(ok)
                    if(point_list_positive.size()<2)
                    {
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, reputation have to few level: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_positive.contains(0))
                    {
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, no starting level for the positive: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!point_list_negative.empty() && !point_list_negative.contains(-1))
                    {
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, no starting level for the negative: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                        ok=false;
                    }
                if(ok)
                    if(!item.attribute("type").contains(QRegExp("^[a-z]{1,32}$")))
                    {
                        DebugClass::debugConsole(QString("Unable to open the items file: %1, the type %4 don't match wiuth the regex: ^[a-z]{1,32}$: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()).arg(item.attribute("type")));
                        ok=false;
                    }
                if(ok)
                {
                    reputation[item.attribute("type")].reputation_positive=point_list_positive;
                    reputation[item.attribute("type")].reputation_negative=point_list_negative;
                    reputation[item.attribute("type")].text_positive=text_positive;
                    reputation[item.attribute("type")].text_negative=text_negative;
                }
            }
            else
                DebugClass::debugConsole(QString("Unable to open the items file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            DebugClass::debugConsole(QString("Unable to open the items file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("reputation");
    }

    return reputation;
}
