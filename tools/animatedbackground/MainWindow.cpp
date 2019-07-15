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

    p=new CustomButton(QPixmap(":/quit.png"),m_CCBackground);
    p->setText(tr("Normal"));
    if(!connect(p,&QPushButton::clicked,&QCoreApplication::quit))
        abort();
    /*p->setStyleSheet("QPushButton::hover {background-position:left center;} QPushButton::pressed {background-position:left bottom;} "
                     "QPushButton {background-image: url(:/quit.png);"
                     "border-image: url(:/empty.png);color: rgb(255, 255, 255);padding-left:0px;"
                     "padding-right:0px;padding-top:0px;padding-bottom:16px;}");
    p->setMinimumWidth(223);
    p->setMinimumHeight(93);*/

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
    p->setText(text);
}
