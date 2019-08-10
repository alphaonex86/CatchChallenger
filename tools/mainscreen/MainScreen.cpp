#include "MainScreen.h"
#include "ui_MainScreen.h"
#include <iostream>

MainScreen::MainScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainScreen)
{
    ui->setupUi(this);
    setMinimumSize(QSize(320,240));

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
    newsText->setText("News");
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
}

MainScreen::~MainScreen()
{
    delete ui;
}

void MainScreen::resizeEvent(QResizeEvent * e)
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
            news->setVisible(false);
            ui->widgetUpdate->setVisible(true);
        }
        else
        {
            ui->widgetUpdate->setVisible(false);
            news->setVisible(true);
        }
    }
    else
        ui->widgetUpdate->setVisible(false);
    QWidget::resizeEvent(e);
}
