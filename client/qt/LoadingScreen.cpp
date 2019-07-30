#include "LoadingScreen.h"
#include "ui_LoadingScreen.h"

LoadingScreen::LoadingScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingScreen)
{
    ui->setupUi(this);

    widget = new CCWidget(this);
    widget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);
    widget->setMinimumSize(QSize(120, 120));
    widget->setMaximumWidth(600);
    horizontalLayout = new QHBoxLayout(widget);
    horizontalLayout->setSpacing(20);
    horizontalLayout->setContentsMargins(24, 24, 24, 24);
    teacher = new QLabel(widget);
    teacher->setMaximumSize(QSize(189, 206));
    teacher->setMinimumSize(QSize(189, 206));
    teacher->setPixmap(QPixmap(":/images/interface/teacher.png"));
    horizontalLayout->addWidget(teacher);
    info = new QLabel(widget);
    horizontalLayout->addWidget(info);
    info->setText(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
    info->setStyleSheet("color:#401c02;");

    ui->horizontalLayout_2->addWidget(widget);
    progressbar=new CCprogressbar(this);
    progressbar->setMaximum(100);
    progressbar->setMinimum(0);
    progressbar->setValue(0);
    ui->verticalLayout->insertWidget(ui->verticalLayout->count(),progressbar);
}

LoadingScreen::~LoadingScreen()
{
    delete ui;
}

void LoadingScreen::resizeEvent(QResizeEvent *)
{
    if(width()<600 || height()<600)
    {
        teacher->setVisible(false);
        widget->setMinimumHeight(120);
    }
    else
    {
        teacher->setVisible(true);
        widget->setMinimumHeight(206+24*2);
    }

    if(height()<500)
    {
        progressbar->setMinimumHeight(0);
    }
    else if(height()<800)
    {
        progressbar->setMinimumHeight(45);
    }
    else
    {
        progressbar->setMinimumHeight(55);
    }
}
