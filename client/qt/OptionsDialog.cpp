#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    w=new CCWidget();
    w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    QHBoxLayout *verticalLayoutNews = new QHBoxLayout(w);
    verticalLayoutNews->addWidget(ui->scrollArea);
    //ui->scrollArea->setParent(w);
    ui->horizontalLayoutMiddle->insertWidget(1,w);
    //w->show();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::resizeEvent(QResizeEvent * e)
{
    unsigned int b=w->currentBorderSize();
    Q_UNUSED(b);
    //ui->verticalLayoutSCContebt->setContentsMargins(b/3,b,b/3,b);
    //w->adjustSize();

    QWidget::resizeEvent(e);
}


void OptionsDialog::on_close_clicked()
{
    emit quitOption();
}
