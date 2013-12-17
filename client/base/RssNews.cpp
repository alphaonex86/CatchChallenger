#include "RssNews.h"
#include "PlatformMacro.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/GeneralVariable.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QDomDocument>
#include <QDomElement>
#include <QRegularExpression>

RssNews *RssNews::rssNews=NULL;

RssNews::RssNews()
{
    start();
    moveToThread(this);
    qnam=NULL;
    connect(&newUpdateTimer,&QTimer::timeout,this,&RssNews::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&RssNews::downloadFile);
    newUpdateTimer.start(60*60*1000);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(5);
}

RssNews::~RssNews()
{
    if(qnam!=NULL)
        delete qnam;
}

void RssNews::downloadFile()
{
    if(qnam==NULL)
        qnam=new QNetworkAccessManager(this);
    QNetworkRequest networkRequest(QStringLiteral(CATCHCHALLENGER_RSS_URL));
    reply = qnam->get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &RssNews::httpFinished);
}

void RssNews::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error())
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    loadRss(reply->readAll());
}

void RssNews::loadRss(const QByteArray &data)
{
    QList<RssEntry> entryList;
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(data, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the rss, Parse error at line %1, column %2: %3").arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("rss"))
        return;

    //load the content
    QDomElement channelItem = root.firstChildElement(QStringLiteral("channel"));
    if(!channelItem.isNull())
    {
        if(channelItem.isElement())
        {
            QDomElement item = channelItem.firstChildElement(QStringLiteral("item"));
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    QString description,title,link,pubDate;
                    {
                        QDomElement descriptionItem = item.firstChildElement(QStringLiteral("description"));
                        if(!descriptionItem.isNull())
                        {
                            if(descriptionItem.isElement())
                                description=descriptionItem.text();
                        }
                    }
                    {
                        QDomElement titleItem = item.firstChildElement(QStringLiteral("title"));
                        if(!titleItem.isNull())
                        {
                            if(titleItem.isElement())
                                title=titleItem.text();
                        }
                    }
                    {
                        QDomElement linkItem = item.firstChildElement(QStringLiteral("link"));
                        if(!linkItem.isNull())
                        {
                            if(linkItem.isElement())
                                link=linkItem.text();
                        }
                    }
                    {
                        QDomElement pubDateItem = item.firstChildElement(QStringLiteral("pubDate"));
                        if(!pubDateItem.isNull())
                        {
                            if(pubDateItem.isElement())
                                pubDate=pubDateItem.text();
                        }
                    }
                    pubDate = pubDate.remove(QStringLiteral(" GMT"), Qt::CaseInsensitive);
                    pubDate = pubDate.remove(QRegularExpression(QStringLiteral("\\+0[0-9]{4}$")));
                    QDateTime date=QDateTime::fromString(pubDate,"ddd, dd MMM yyyy hh:mm:ss");
                    if(!date.isValid())
                        pubDate.clear();
                    RssEntry rssEntry;
                    rssEntry.description=description;
                    rssEntry.title=title;
                    rssEntry.pubDate=date;
                    rssEntry.link=link;
                    entryList << rssEntry;
                }
                item = item.nextSiblingElement(QStringLiteral("item"));
            }
        }
    }
    if(!entryList.isEmpty())
        emit rssEntryList(entryList);
}
