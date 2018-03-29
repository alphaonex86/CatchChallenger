#include "FeedNews.h"
#include "PlatformMacro.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/Version.h"
#include "ClientVariable.h"
#include "InternetUpdater.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QDomDocument>
#include <QDomElement>
#include <QRegularExpression>

FeedNews *FeedNews::feedNews=NULL;

FeedNews::FeedNews()
{
    start();
    moveToThread(this);
    qnam=NULL;
    connect(&newUpdateTimer,&QTimer::timeout,this,&FeedNews::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&FeedNews::downloadFile);
    newUpdateTimer.start(60*60*1000);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(5);
}

FeedNews::~FeedNews()
{
    if(qnam!=NULL)
        delete qnam;
    exit();
    quit();
    wait();
}

void FeedNews::downloadFile()
{
    QString catchChallengerVersion;
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    catchChallengerVersion=QStringLiteral("CatchChallenger Ultimate/%1").arg(CATCHCHALLENGER_VERSION);
    #else
        #ifdef CATCHCHALLENGER_VERSION_SINGLESERVER
        catchChallengerVersion=QStringLiteral("CatchChallenger SingleServer/%1").arg(CATCHCHALLENGER_VERSION);
        #else
            #ifdef CATCHCHALLENGER_VERSION_SOLO
            catchChallengerVersion=QStringLiteral("CatchChallenger Solo/%1").arg(CATCHCHALLENGER_VERSION);
            #else
            catchChallengerVersion=QStringLiteral("CatchChallenger/%1").arg(CATCHCHALLENGER_VERSION);
            #endif
        #endif
    #endif
    #if defined(_WIN32) || defined(Q_OS_MAC)
    catchChallengerVersion+=QStringLiteral(" (OS: %1)").arg(InternetUpdater::GetOSDisplayString());
    #endif
    if(qnam==NULL)
        qnam=new QNetworkAccessManager(this);
    QNetworkRequest networkRequest(QStringLiteral(CATCHCHALLENGER_RSS_URL));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,catchChallengerVersion);
    reply = qnam->get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &FeedNews::httpFinished);
}

void FeedNews::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        qDebug() << (QStringLiteral("get the new update failed: not finished"));
        reply->deleteLater();
        return;
    }
    else if (reply->error())
    {
        emit feedEntryList(QList<FeedEntry>(),reply->errorString());
        qDebug() << (QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    else if (!redirectionTarget.isNull())
    {
        emit feedEntryList(QList<FeedEntry>(),tr("Redirection denied to"));
        qDebug() << (QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    loadFeeds(reply->readAll());
}

void FeedNews::loadFeeds(const QByteArray &data)
{
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(data, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the rss, Parse error at line %1, column %2: %3").arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()==QStringLiteral("rss"))
        loadRss(root);
    else if(root.tagName()==QStringLiteral("feed"))
        loadAtom(root);
    else
        emit feedEntryList(QList<FeedEntry>(),tr("Not Rss or Atom feed"));
}

void FeedNews::loadRss(const QDomElement &root)
{
    QList<FeedEntry> entryList;
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
                    FeedEntry rssEntry;
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
    emit feedEntryList(entryList);
}

void FeedNews::loadAtom(const QDomElement &root)
{
    QList<FeedEntry> entryList;
    //load the content
    QDomElement item = root.firstChildElement(QStringLiteral("entry"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            QString description,title,link,pubDate;
            {
                QDomElement descriptionItem = item.firstChildElement(QStringLiteral("content"));
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
                        link=linkItem.attribute(QStringLiteral("href"));
                }
            }
            {
                QDomElement pubDateItem = item.firstChildElement(QStringLiteral("published"));
                if(!pubDateItem.isNull())
                {
                    if(pubDateItem.isElement())
                        pubDate=pubDateItem.text();
                }
            }
            pubDate = pubDate.remove(QStringLiteral(" GMT"), Qt::CaseInsensitive);
            pubDate = pubDate.remove(QRegularExpression(QStringLiteral("\\+0[0-9]{4}$")));
            QDateTime date=QDateTime::fromString(pubDate,"yyyy-MMM-ddThh:mm:ss");
            if(!date.isValid())
                pubDate.clear();
            FeedEntry rssEntry;
            rssEntry.description=description;
            rssEntry.title=title;
            rssEntry.pubDate=date;
            rssEntry.link=link;
            entryList << rssEntry;
        }
        item = item.nextSiblingElement(QStringLiteral("entry"));
    }
    emit feedEntryList(entryList);
}
