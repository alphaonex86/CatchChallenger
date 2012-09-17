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
    mapController=new MapController(ui->centerOnPlayer->isChecked(),ui->debugTags->isChecked(),ui->cache->isChecked(),ui->openGL->isChecked());

    mapController->setShowFPS(ui->showFPS->isChecked());
    if(ui->doubleSize->isChecked())
        mapController->setScale(2);
    else
        mapController->setScale(1);
    if(ui->limitFPS->isChecked())
        mapController->setTargetFPS(ui->targetFPS->value());
    else
        mapController->setTargetFPS(0);

    mapController->viewMap(ui->baseFile->text());

    close();
}
