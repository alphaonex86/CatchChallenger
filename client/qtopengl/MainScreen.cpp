#include "MainScreen.h"
#include "../qt/InternetUpdater.h"
#include "../qt/FeedNews.h"
#include "../qt/GameLoader.h"
#include <iostream>
#include <QDesktopServices>
#include "../qt/Settings.h"
#include <QWidget>

#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.h"
#endif

MainScreen::MainScreen()
{
    title=new CCTitle(this);
    title->setText("Update!");

    solo=new CustomButton(":/CC/images/interface/button.png",this);
    solo->setText("Solo");
    multi=new CustomButton(":/CC/images/interface/button.png",this);
    multi->setText("Multi");
    options=new CustomButton(":/CC/images/interface/options.png",this);
    facebook=new CustomButton(":/CC/images/interface/bluetoolbox.png",this);
    facebook->setText("f");
    facebook->setOutlineColor(QColor(0,79,154));
    facebook->setPointSize(28);
    website=new CustomButton(":/CC/images/interface/bluetoolbox.png",this);
    website->setText("w");
    website->setOutlineColor(QColor(0,79,154));
    website->setPointSize(28);
    website->updateTextPercent(75);

    haveFreshFeed=false;
    news=new CCWidget(this);
    newsText=new QGraphicsTextItem(news);
    if(Settings::settings.contains("news"))
    {
        const QByteArray &data=Settings::settings.value("news").toByteArray();
        QDataStream in(data);
        in.setVersion(QDataStream::Qt_4_0);
        quint8 size=0;
        in >> size;
        quint8 index=0;
        while(index<size)
        {
            FeedNews::FeedEntry n;
            in >> n.description;
            in >> n.link;
            in >> n.pubDate;
            in >> n.title;
            if(n.title.isEmpty())
                break;
            entryList.push_back(n);
            index++;
        }
    }
    newsText->setOpenExternalLinks(true);
    newsText->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    updateNews();
    newsWait=new QGraphicsPixmapItem(news);
    newsWait->setPixmap(GameLoader::gameLoader->getImage(":/CC/images/multi/busy.png"));

    warning=new QGraphicsTextItem(this);
    warning->setVisible(true);
    warningString=QString("<div style=\"background-color: rgb(255, 180, 180);border: 1px solid rgb(255, 221, 50);border-radius:5px;color: rgb(0, 0, 0);\">&nbsp;%1&nbsp;</div>");
    newsUpdate=new CustomButton(":/CC/images/interface/greenbutton.png",news);
    newsUpdate->setText(tr("Update!"));
    newsUpdate->setOutlineColor(QColor(44,117,0));

    #ifndef __EMSCRIPTEN__
    InternetUpdater::internetUpdater=new InternetUpdater();
    if(!connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainScreen::newUpdate))
        abort();
    #endif
    FeedNews::feedNews=new FeedNews();
    if(!connect(FeedNews::feedNews,&FeedNews::feedEntryList,this,&MainScreen::feedEntryList))
        std::cerr << "connect(RssNews::rssNews,&RssNews::rssEntryList,this,&MainWindow::rssEntryList) failed" << std::endl;
    #ifndef NOSINGLEPLAYER
    /*solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath().toStdString()+
                              "/datapack/internal/",
                              QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString()+
                              "/savegames/",false);
    if(!connect(solowindow,&SoloWindow::back,this,&MainWindow::gameSolo_back))
        abort();
    if(!connect(solowindow,&SoloWindow::play,this,&MainWindow::gameSolo_play))
        abort();
    ui->stackedWidget->addWidget(solowindow);
    if(ui->stackedWidget->indexOf(solowindow)<0)
        solo->hide();*/
    #endif
    #ifdef NOSINGLEPLAYER
    solo->hide();
    #endif
    if(!connect(facebook,&CustomButton::clicked,this,&MainScreen::openFacebook))
        abort();
    if(!connect(website,&CustomButton::clicked,this,&MainScreen::openWebsite))
        abort();
    if(!connect(newsUpdate,&CustomButton::clicked,this,&MainScreen::openUpdate))
        abort();

/*    if(!connect(solo,&CustomButton::clicked,this,&MainScreen::goToSolo))
        abort();
    if(!connect(multi,&CustomButton::clicked,this,&MainScreen::goToMulti))
        abort();
    if(!connect(options,&CustomButton::clicked,this,&MainScreen::goToOptions))
        abort();*/

    haveUpdate=false;
    currentNewsType=0;

    #ifndef CATCHCHALLENGER_NOAUDIO
    const std::string &terr=AudioGL::audioGL.startAmbiance(":/CC/music/loading.opus");
    if(!terr.empty())
        setError(terr);
    /*else
        setError("Sound ok: "+QAudioDeviceInfo::defaultOutputDevice().deviceName().toStdString());
    #else
        setError("Sound disabled");*/
    #endif
}

MainScreen::~MainScreen()
{
}

void MainScreen::setError(const std::string &error)
{
    warning->setVisible(true);
    warning->setHtml(warningString.arg(QString::fromStdString(error)));
}

void MainScreen::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    int buttonMargin=10;
    if(widget->width()<600 || widget->height()<600)
    {
        solo->setSize(148,61);
        solo->setPointSize(23);
        multi->setSize(148,61);
        multi->setPointSize(23);
        options->setSize(41,46);
        options->setPointSize(23);
        facebook->setSize(41,46);
        facebook->setPointSize(18);
        website->setSize(41,46);
        website->setPointSize(18);
    }
    else {
        buttonMargin=30;
        solo->setSize(223,92);
        solo->setPointSize(35);
        multi->setSize(223,92);
        multi->setPointSize(35);
        options->setSize(62,70);
        options->setPointSize(35);
        facebook->setSize(62,70);
        facebook->setPointSize(28);
        website->setSize(62,70);
        website->setPointSize(28);
    }
    solo->setPos(widget->width()/2-solo->width()/2,widget->height()/2-multi->height()/2-solo->height()-buttonMargin);
    multi->setPos(widget->width()/2-multi->width()/2,widget->height()/2-multi->height()/2);
    const int horizontalMargin=(multi->width()-options->width()-facebook->width()-website->width())/2;
    options->setPos(widget->width()/2-facebook->width()/2-horizontalMargin-options->width(),widget->height()/2+multi->height()/2+buttonMargin);
    facebook->setPos(widget->width()/2-facebook->width()/2,widget->height()/2+multi->height()/2+buttonMargin);
    website->setPos(widget->width()/2+facebook->width()/2+horizontalMargin,widget->height()/2+multi->height()/2+buttonMargin);
    warning->setPos(widget->width()/2-warning->boundingRect().width()/2,5);

    if(widget->height()<280)
    {
        newsUpdate->setVisible(false);
        if(currentNewsType!=0)
        {
            currentNewsType=0;
            updateNews();
        }
    }
    else {
        news->setPos(widget->width()/2-news->width()/2,widget->height()-news->height()-5);
        newsText->setPos(news->currentBorderSize()+2,news->currentBorderSize()+2);
        newsWait->setPos(news->width()-newsWait->pixmap().width()-news->currentBorderSize()-2,news->height()/2-newsWait->pixmap().height()/2);
        news->setVisible(true);

        if(widget->width()<600 || widget->height()<480)
        {
            int w=widget->width()-9*2;
            if(w>300)
                w=300;
            news->setSize(w,40);
            newsWait->setVisible(widget->width()>450 && !haveFreshFeed);
            newsUpdate->setVisible(false);

            if(currentNewsType!=1)
            {
                currentNewsType=1;
                updateNews();
            }
        }
        else {
            news->setSize(600-9*2,120);
            newsWait->setVisible(!haveFreshFeed);
            newsUpdate->setSize(136,57);
            newsUpdate->setPointSize(18);
            newsUpdate->setPos(news->width()-136-news->currentBorderSize()-2,news->height()/2-57/2);
            newsUpdate->setVisible(haveUpdate);

            if(currentNewsType!=2)
            {
                currentNewsType=2;
                updateNews();
            }
        }
    }
}

#ifndef __EMSCRIPTEN__
void MainScreen::newUpdate(const std::string &version)
{
    Q_UNUSED(version);
    haveUpdate=true;
/*
    ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
    ui->update->setVisible(true);*/
}
#endif

void MainScreen::feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error)
{
    this->entryList=entryList;
    if(entryList.empty())
    {
        if(!error.empty())
            setError(error);
        return;
    }
    if(currentNewsType!=0)
        updateNews();

    if(entryList.size()<255)
    {
        QByteArray data;
        QDataStream out(&data,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << (quint8)entryList.size();
        for(const FeedNews::FeedEntry &n : entryList) {
            out << n.description;
            out << n.link;
            out << n.pubDate;
            out << n.title;
        }
        Settings::settings.setValue("news",data);
    }
    newsWait->setVisible(false);
    haveFreshFeed=true;
}

void MainScreen::updateNews()
{
    switch(currentNewsType)
    {
        case 0:
            if(news->isVisible())
                news->setVisible(false);
        break;
        case 1:
            if(entryList.empty())
                news->setVisible(false);
            else
            {
                if(entryList.size()>1)
                    newsText->setHtml(QStringLiteral("<a href=\"%1\">%2</a>")
                              .arg(entryList.at(0).link)
                              .arg(entryList.at(0).title));
                newsText->setVisible(true);
            }
        break;
        default:
        case 2:
        if(entryList.empty())
            news->setVisible(false);
        else
        {
            if(entryList.size()==1)
                newsText->setHtml(tr("Latest news:")+" "+QStringLiteral("<a href=\"%1\">%2</a>")
                                  .arg(entryList.at(0).link)
                                  .arg(entryList.at(0).title));
            else
            {
                QStringList entryHtmlList;
                unsigned int index=0;
                while(index<entryList.size() && index<3)
                {
                    entryHtmlList << QStringLiteral(" - <a href=\"%1\">%2</a>")
                                     .arg(entryList.at(index).link)
                                     .arg(entryList.at(index).title);
                    index++;
                }
                newsText->setHtml(tr("Latest news:")+QStringLiteral("<br />")+entryHtmlList.join("<br />"));
            }
            //newsText->setStyleSheet("#news{background-color:rgb(220,220,240);border:1px solid rgb(100,150,240);border-radius:5px;color:rgb(0,0,0);}");
            newsText->setVisible(true);
        }
        break;
    }
}

void MainScreen::openWebsite()
{
    if(!QDesktopServices::openUrl(QUrl("https://catchchallenger.first-world.info/")))
        std::cerr << "MainScreen::openWebsite() failed" << std::endl;
}

void MainScreen::openFacebook()
{
    if(!QDesktopServices::openUrl(QUrl("https://www.facebook.com/CatchChallenger/")))
        std::cerr << "MainScreen::openFacebook() failed" << std::endl;
}

void MainScreen::openUpdate()
{
    if(!QDesktopServices::openUrl(QUrl("https://catchchallenger.first-world.info/download.html")))
        std::cerr << "MainScreen::openFacebook() failed" << std::endl;
}

QRectF MainScreen::boundingRect() const
{
    return QRectF();
}

void MainScreen::mousePressEventXY(const QPointF &p)
{
    solo->mousePressEventXY(p);
    multi->mousePressEventXY(p);
    options->mousePressEventXY(p);
    facebook->mousePressEventXY(p);
    website->mousePressEventXY(p);
    newsUpdate->mousePressEventXY(p);
}

void MainScreen::mouseReleaseEventXY(const QPointF &p)
{
    bool pressValidated=solo->mouseReleaseEventXY(p);
    pressValidated|=multi->mouseReleaseEventXY(p,pressValidated);
    pressValidated|=options->mouseReleaseEventXY(p,pressValidated);
    pressValidated|=facebook->mouseReleaseEventXY(p,pressValidated);
    pressValidated|=website->mouseReleaseEventXY(p,pressValidated);
    pressValidated|=newsUpdate->mouseReleaseEventXY(p,pressValidated);
}
