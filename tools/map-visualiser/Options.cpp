#include "Options.h"
#include "ui_Options.h"

#include <QFileDialog>

Options::Options(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);
}

Options::~Options()
{
    delete ui;
}

void Options::on_browse_clicked()
{
    QString source = QFileDialog::getOpenFileName(this,"Select map",QString(),"Pokecraft map (*.tmx)");
    if(source.isEmpty() || source.isNull() || source=="")
        return;
    ui->baseFile->setText(source);
    ui->load->setEnabled(true);
}

void Options::on_load_clicked()
{
    mapVisualiser=new MapVisualiser(NULL,ui->centerOnPlayer->isChecked(),ui->debugTags->isChecked(),ui->targetFPS->value(),ui->cache->isChecked());
    mapVisualiser->viewMap(ui->baseFile->text());
    mapVisualiser->setWindowIcon(QIcon(":/icon.png"));
    mapVisualiser->show();
    close();
}
