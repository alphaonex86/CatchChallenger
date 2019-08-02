#include "MainScreen.h"
#include "ui_MainScreen.h"
#include <iostream>

MainScreen::MainScreen(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainScreen)
{
    ui->setupUi(this);
    setMinimumSize(QSize(320,240));

    /*update=new QLabel(this);
    update->setObjectName("update");
    update->setStyleSheet("#update{border-image: url(:/CC/images/interface/updatewidget.png) 0 0 0 0 stretch stretch;}");
    updateStar=new QLabel(update);
    updateText=new QLabel(update);
    updateButton=new CustomButton(":/CC/images/interface/greenbutton.png",update);
    update->setMinimumSize(QSize(493,156));
    update->setMaximumSize(QSize(493,156));*/

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
    news->setVisible(false);

    ui->horizontalLayoutBottom->insertWidget(1,news);
    ui->verticalLayoutMiddle->insertWidget(0,solo);
    ui->verticalLayoutMiddle->insertWidget(1,multi);
    ui->horizontalLayoutMiddle->insertWidget(0,options);
    ui->horizontalLayoutMiddle->insertWidget(1,facebook);
    ui->horizontalLayoutMiddle->insertWidget(2,website);
}

MainScreen::~MainScreen()
{
    delete ui;
}

void MainScreen::resizeEvent(QResizeEvent *)
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
        news->setMaximumSize(QSize(width(),height()*15/100));
        news->setMinimumSize(QSize(width(),height()*15/100));
        if(news->maximumWidth()>600)
            news->setMaximumWidth(600);
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
        news->setMaximumSize(QSize(600,120));
        news->setMinimumSize(QSize(600,120));
    }

    /*unsigned int centerWidget=solo->height()+ui->horizontalLayoutMiddle->spacing()+
            multi->height()+ui->horizontalLayoutMiddle->spacing()+options->height();*/

    ui->widget->adjustSize();
    //ui->widget->setMinimumSize(solo->width(),centerWidget);
    /*if((height()-centerWidget-options->height())<(unsigned int)news->minimumHeight())
        news->setVisible(false);
    else
        news->setVisible(true);*/
}
