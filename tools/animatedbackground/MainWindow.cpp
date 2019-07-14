#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

/*    QTransform transform;
    transform.reset();

    QGraphicsScene *scene=new QGraphicsScene();
    QGraphicsRectItem *rect = scene->addRect(QRectF(0, 0, 100, 100));

    QGraphicsPixmapItem *t=new QGraphicsPixmapItem(QPixmap(":/cloud.png"));
    scene->addPixmap(t);

    view=new View(ui->centralWidget);
    view->setScene(scene);*/

    m_CCBackground=new CCBackground(ui->centralWidget);
    m_CCBackground->move(0,0);
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

    /*view->setMinimumSize(size());
    view->setMaximumSize(size());*/
}
