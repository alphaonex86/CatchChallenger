#include "MainScreen.h"
#include "ui_MainScreen.h"
#include "InternetUpdater.h"
#include "FeedNews.h"
#include <iostream>
#include <QDesktopServices>
#include "Settings.h"

MainScreen::MainScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainScreen)
{
    ui->setupUi(this);
    setMinimumSize(QSize(320,240));
    ui->warning->setVisible(false);
    updateButton=new CustomButton(":/CC/images/interface/greenbutton.png",ui->widgetUpdate);
    updateButton->setMaximumSize(QSize(136,57));
    updateButton->setMinimumSize(QSize(136,57));
    updateButton->setText(tr("Update!"));
    updateButton->setOutlineColor(QColor(44,117,0));
    updateButton->setPointSize(20);
    ui->horizontalLayoutUpdateFull->addWidget(updateButton);
    title=new CCTitle(ui->widgetUpdate);
    title->setText("Update!");
    ui->verticalLayoutUpdateTitle->insertWidget(0,title);

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
    news=new CCWidget(this);

    ui->horizontalLayoutBottom->insertWidget(1,news);
    ui->verticalLayoutMiddle->insertWidget(0,solo);
    ui->verticalLayoutMiddle->insertWidget(1,multi);
    ui->horizontalLayoutMiddle->insertWidget(0,options);
    ui->horizontalLayoutMiddle->insertWidget(1,facebook);
    ui->horizontalLayoutMiddle->insertWidget(2,website);

    verticalLayoutNews = new QHBoxLayout(news);
    verticalLayoutNews->setSpacing(6);
    newsText=new QLabel(news);
    newsText->setText(Settings::settings.value("news").toString());
    newsText->setOpenExternalLinks(true);
    newsWait=new QLabel(news);
    newsWait->setPixmap(QPixmap(":/CC/images/multi/busy.png"));
    newsWait->setMinimumWidth(64);
    newsWait->setMaximumWidth(64);
    newsUpdate=new CustomButton(":/CC/images/interface/greenbutton.png",news);
    newsUpdate->setText(tr("Update!"));
    newsUpdate->setOutlineColor(QColor(44,117,0));
    verticalLayoutNews->addWidget(newsText);
    verticalLayoutNews->addWidget(newsWait);
    verticalLayoutNews->addWidget(newsUpdate);

    #ifndef __EMSCRIPTEN__
    InternetUpdater::internetUpdater=new InternetUpdater();
    if(!connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainScreen::newUpdate))
        abort();
    #endif
    FeedNews::feedNews=new FeedNews();
    if(!connect(FeedNews::feedNews,&FeedNews::feedEntryList,this,&MainScreen::feedEntryList))
        qDebug() << "connect(RssNews::rssNews,&RssNews::rssEntryList,this,&MainWindow::rssEntryList) failed";
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
    if(!connect(facebook,&QPushButton::clicked,this,&MainScreen::openFacebook))
        abort();
    if(!connect(website,&QPushButton::clicked,this,&MainScreen::openWebsite))
        abort();
    if(!connect(updateButton,&QPushButton::clicked,this,&MainScreen::openUpdate))
        abort();
    if(!connect(newsUpdate,&QPushButton::clicked,this,&MainScreen::openUpdate))
        abort();

    if(!connect(solo,&QPushButton::clicked,this,&MainScreen::goToSolo))
        abort();
    if(!connect(multi,&QPushButton::clicked,this,&MainScreen::goToMulti))
        abort();
    if(!connect(options,&QPushButton::clicked,this,&MainScreen::goToOptions))
        abort();

    haveUpdate=false;
}

MainScreen::~MainScreen()
{
    delete ui;
}

void MainScreen::setError(const std::string &error)
{
    ui->warning->setVisible(true);
    ui->warning->setText(QString::fromStdString(error));
}

void MainScreen::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);
}

void MainScreen::paintEvent(QPaintEvent * e)
{
    if(width()<600 || height()<600)
    {
        solo->setMaximumSize(QSize(148,61));
        solo->setMinimumSize(QSize(148,61));
        solo->setPointSize(23);
        multi->setMaximumSize(QSize(148,61));
        multi->setMinimumSize(QSize(148,61));
        multi->setPointSize(23);
        options->setMaximumSize(QSize(41,46));
        options->setMinimumSize(QSize(41,46));
        options->setPointSize(23);
        facebook->setMaximumSize(QSize(41,46));
        facebook->setMinimumSize(QSize(41,46));
        facebook->setPointSize(18);
        website->setMaximumSize(QSize(41,46));
        website->setMinimumSize(QSize(41,46));
        website->setPointSize(18);
    }
    else {
        solo->setMaximumSize(QSize(223,92));
        solo->setMinimumSize(QSize(223,92));
        solo->setPointSize(35);
        multi->setMaximumSize(QSize(223,92));
        multi->setMinimumSize(QSize(223,92));
        multi->setPointSize(35);
        options->setMaximumSize(QSize(62,70));
        options->setMinimumSize(QSize(62,70));
        options->setPointSize(35);
        facebook->setMaximumSize(QSize(62,70));
        facebook->setMinimumSize(QSize(62,70));
        facebook->setPointSize(28);
        website->setMaximumSize(QSize(62,70));
        website->setMinimumSize(QSize(62,70));
        website->setPointSize(28);
    }
    if(width()<600)
    {
        news->setMaximumSize(QSize(width()-9*2,height()*15/100));
        news->setMinimumSize(QSize(width()-9*2,height()*15/100));
        verticalLayoutNews->setContentsMargins(news->currentBorderSize(), news->currentBorderSize(),
                                               news->currentBorderSize(), news->currentBorderSize());

        newsWait->setVisible(width()>450);
    }
    else {
        news->setMaximumSize(QSize(600-9*2,120));
        news->setMinimumSize(QSize(600-9*2,120));
        verticalLayoutNews->setContentsMargins(12, 9, 12, 9);

        newsWait->setVisible(true);
    }
    if(news->width()<450 || news->height()<80)
    {
        newsUpdate->setMaximumSize(QSize(90,38));
        newsUpdate->setMinimumSize(QSize(90,38));
        newsUpdate->setPointSize(12);
        newsUpdate->setVisible(news->height()>50);
    }
    else
    {
        newsUpdate->setMaximumSize(QSize(136,57));
        newsUpdate->setMinimumSize(QSize(136,57));
        newsUpdate->setPointSize(20);
    }


    unsigned int centerWidget=solo->height()+ui->horizontalLayoutMiddle->spacing()+
            multi->height()+ui->horizontalLayoutMiddle->spacing()+options->height();

    ui->widget->setMinimumSize(9+solo->width()+9,9+centerWidget+9);
    //ui->widget->adjustSize();
    if((height()-(9+6+6+ui->widget->height()+6+6+9))<(unsigned int)news->minimumHeight())
        news->setVisible(false);
    else
        news->setVisible(true);
    if(news->isVisible())
    {
        if(height()>600 && width()>510)
        {
            //news->setVisible(false);
            ui->widgetUpdate->setVisible(haveUpdate);
            newsUpdate->setVisible(false);
        }
        else
        {
            ui->widgetUpdate->setVisible(false);
            newsUpdate->setVisible(haveUpdate);
            //news->setVisible(true);
        }
    }
    else
        ui->widgetUpdate->setVisible(false);
    QWidget::paintEvent(e);
}

#ifndef __EMSCRIPTEN__
void MainScreen::newUpdate(const std::string &version)
{
    haveUpdate=true;
    QWidget::update();
    /*
    ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
    ui->update->setVisible(true);*/
}
#endif

void MainScreen::feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error)
{
    if(entryList.empty())
    {
        if(error.empty())
            newsWait->setVisible(false);
        else
        {
            newsText->setToolTip(QString::fromStdString(error));
            newsText->setStyleSheet("#news{background-color: rgb(220, 220, 240);\nborder: 1px solid rgb(100, 150, 240);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\nbackground-image: url(:/images/multi/warning.png);\nbackground-repeat: no-repeat;\nbackground-position: right;}");
        }
        return;
    }
    if(entryList.size()==1)
        newsText->setText(tr("Latest news:")+" "+QStringLiteral("<a href=\"%1\">%2</a>")
                          .arg(QString::fromStdString(entryList.at(0).link))
                          .arg(QString::fromStdString(entryList.at(0).title)));
    else
    {
        QStringList entryHtmlList;
        unsigned int index=0;
        while(index<entryList.size() && index<3)
        {
            entryHtmlList << QStringLiteral(" - <a href=\"%1\">%2</a>")
                             .arg(QString::fromStdString(entryList.at(index).link))
                             .arg(QString::fromStdString(entryList.at(index).title));
            index++;
        }
        newsText->setText(tr("Latest news:")+QStringLiteral("<br />")+entryHtmlList.join("<br />"));
    }
    Settings::settings.setValue("news",newsText->text());
    newsText->setStyleSheet("#news{background-color:rgb(220,220,240);border:1px solid rgb(100,150,240);border-radius:5px;color:rgb(0,0,0);}");
    newsText->setVisible(true);
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
