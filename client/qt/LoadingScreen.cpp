#include "LoadingScreen.h"
#include "ui_LoadingScreen.h"
#include "../../general/base/Version.h"

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
    teacher->setPixmap(QPixmap(":/CC/images/interface/teacher.png"));
    horizontalLayout->addWidget(teacher);
    info = new QLabel(widget);
    horizontalLayout->addWidget(info);
    info->setText(tr("%1 is loading...").arg("<b>CatchChallenger</b>"));
    info->setStyleSheet("color:#401c02;");
    info->setWordWrap(true);
    version = new QLabel(widget);
    version->setText(QStringLiteral("<span style=\"color:#9090f0;\">%1</span>").arg(QString::fromStdString(CatchChallenger::Version::str)));
    version->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont font = version->font();
    font.setPointSize(7);
    version->setFont(font);
    ui->verticalLayout->insertWidget(0,version);

    ui->horizontalLayout_2->insertWidget(1,widget);
    progressbar=new CCprogressbar(this);
    progressbar->setMaximum(100);
    progressbar->setMinimum(0);
    progressbar->setValue(0);
    ui->verticalLayout->insertWidget(ui->verticalLayout->count(),progressbar);

    if(GameLoader::gameLoader==nullptr)
        GameLoader::gameLoader=new GameLoader();
    connect(GameLoader::gameLoader,&GameLoader::progression,this,&LoadingScreen::progression);
    connect(GameLoader::gameLoader,&GameLoader::dataIsParsed,this,&LoadingScreen::dataIsParsed);

    timer.setSingleShot(true);
    connect(&timer,&QTimer::timeout,this,&LoadingScreen::canBeChanged);
    timer.start(1000);
    doTheNext=false;
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
        teacher->setVisible(false);
        widget->setMinimumHeight(100);
    }
    else
    {
        teacher->setVisible(true);
        widget->setMinimumHeight(260);
    }
    horizontalLayout->setContentsMargins(
                widget->currentBorderSize(),widget->currentBorderSize(),
                widget->currentBorderSize(),widget->currentBorderSize()
                );

    QFont font = version->font();
    if(height()<500)
    {
        progressbar->setMinimumHeight(0);
        font.setPointSize(9);
    }
    else if(height()<800)
    {
        progressbar->setMinimumHeight(45);
        font.setPointSize(11);
    }
    else
    {
        progressbar->setMinimumHeight(55);
        font.setPointSize(14);
    }
    version->setFont(font);
}

void LoadingScreen::canBeChanged()
{
    if(doTheNext)
        emit finished();
    else
        doTheNext=true;
}

void LoadingScreen::dataIsParsed()
{
    if(doTheNext)
        emit finished();
    else
    {
        doTheNext=true;
        info->setText(tr("%1 is loaded").arg("<b>CatchChallenger</b>"));
    }
}

void LoadingScreen::progression(uint32_t size,uint32_t total)
{
    if(size<=total)
        progressbar->setValue(size*100/total);
    else
        abort();
}
