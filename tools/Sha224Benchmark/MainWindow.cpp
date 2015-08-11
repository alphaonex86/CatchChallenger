#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QCryptographicHash>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    quint64 count=0;
    QByteArray data;
    QTime time;
    time.restart();
    QCryptographicHash hash(QCryptographicHash::Sha224);
    do
    {
        int index=0;
        while(index<10000)
        {
            {
                hash.reset();
                hash.addData(data);
                data=hash.result();
            }
            count++;
            index++;
        }
    } while(time.elapsed()<8000);
    ui->label->setText(tr("Result: %1 in %2ms\nMean:%3H/s")
                       .arg(count)
                       .arg(time.elapsed())
                       .arg((double)count*1000/(double)time.elapsed())
                       );
}
