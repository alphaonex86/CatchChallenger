#include "LoadingScreen.h"
#include "ui_LoadingScreen.h"

LoadingScreen::LoadingScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoadingScreen)
{
    ui->setupUi(this);

    widget = new CCWidget(this);
    widget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    widget->setMinimumSize(QSize(180,100));
    widget->setMaximumSize(QSize(800,300));
    horizontalLayout = new QHBoxLayout(widget);
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
    info->setWordWrap(true);

    ui->horizontalLayout_2->insertWidget(1,widget);
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
    widget->updateGeometry();
    if(width()<400 || height()<320)
    {
        horizontalLayout->setContentsMargins(8, 8, 8, 8);
        teacher->setVisible(false);
        widget->setMinimumHeight(100);
    }
    else
    {
        horizontalLayout->setContentsMargins(24, 24, 24, 24);
        teacher->setVisible(true);
        widget->setMinimumHeight(260);
    }


    if(height()<500)
        progressbar->setMinimumHeight(0);
    else if(height()<800)
        progressbar->setMinimumHeight(45);
    else
        progressbar->setMinimumHeight(55);
}
