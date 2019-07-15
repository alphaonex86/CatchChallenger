#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CustomButton.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_CCBackground=new CCBackground(ui->centralWidget);
    m_CCBackground->move(0,0);
    setMinimumHeight(120);
    setMinimumWidth(120);

    p=new CustomButton(":/quit.png",m_CCBackground);
    p->setText(tr("Normal"));
    if(!connect(p,&QPushButton::clicked,&QCoreApplication::quit))
        abort();
    p->setMinimumWidth(223);
    p->setMinimumHeight(93);
    setStyleSheet("");

    //showFullScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    (void)event;
    m_CCBackground->setMinimumSize(size());
    m_CCBackground->setMaximumSize(size());

    QString text;
    switch (rand()%4) {
    case 0:
        text="f";
        break;
    case 1:
        text="quit";
        break;
    default:
        text="фхцчшщ";
        break;
    }
    const int w=rand()%173+50;
    const int h=rand()%73+20;
    p->setMinimumWidth(w);
    p->setMinimumHeight(h);
    p->setMaximumWidth(w);
    p->setMaximumHeight(h);
    p->setText(text);
}
