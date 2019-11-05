#include "Solo.h"
#include "ui_Solo.h"

Solo::Solo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Solo)
{
    ui->setupUi(this);
    connect(ui->back,&QPushButton::clicked,this,&Solo::backMain);
}

Solo::~Solo()
{
    delete ui;
}
