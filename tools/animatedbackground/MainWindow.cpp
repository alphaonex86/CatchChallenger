#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_CCBackground=new CCBackground(ui->centralWidget);
    m_CCBackground->move(0,0);
    m_CCBackground->setMinimumSize(size());
    m_CCBackground->setMaximumSize(size());
    setMinimumHeight(120);
    setMinimumWidth(120);
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
}
