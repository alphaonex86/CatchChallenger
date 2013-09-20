#include "SimpleSoloServer.h"
#include "ui_SimpleSoloServer.h"

SimpleSoloServer::SimpleSoloServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimpleSoloServer)
{
    ui->setupUi(this);
    socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath()+"/datapack/",QCoreApplication::applicationDirPath()+"/savegames/",false);
    connect(solowindow,&SoloWindow::back,this,&SimpleSoloServer::back);
    connect(solowindow,&SoloWindow::play,this,&SimpleSoloServer::play);
    ui->verticalLayout->addWidget(solowindow);
    //solowindow->show();
}

SimpleSoloServer::~SimpleSoloServer()
{
    delete ui;
}

void SimpleSoloServer::play(const QString &savegamesPath)
{
}

void SimpleSoloServer::back()
{
}
