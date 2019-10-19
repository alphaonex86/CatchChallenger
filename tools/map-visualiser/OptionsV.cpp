#include "OptionsV.h"
#include "ui_OptionsV.h"

#include <QFileDialog>

OptionsV::OptionsV(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsV)
{
    ui->setupUi(this);
}

OptionsV::~OptionsV()
{
    delete ui;
}

void OptionsV::on_browse_clicked()
{
    QString source = QFileDialog::getOpenFileName(this,"Select map",QString(),"CatchChallenger map (*.tmx)");
    if(source.isEmpty() || source.isNull() || source=="")
        return;
    ui->baseFile->setText(source);
    ui->load->setEnabled(true);
}

void OptionsV::on_load_clicked()
{
    mapController=new MapControllerV(ui->centerOnPlayer->isChecked(),ui->debugTags->isChecked(),
                                     ui->cache->isChecked(),ui->opengl->isChecked());

    //mapController->setShowFPS(ui->showFPS->isChecked());
    mapController->setScale(ui->doubleSize->value());
    if(ui->limitFPS->isChecked())
        mapController->setTargetFPS(ui->targetFPS->value());
    else
        mapController->setTargetFPS(0);
    mapController->setBotNumber(ui->botCount->value());

    mapController->current_map=ui->baseFile->text().toStdString();
    mapController->viewMap(ui->baseFile->text());

    //QApplication::setQuitOnLastWindowClosed(true);
    close();
}

void OptionsV::on_baseFile_textChanged(const QString &arg1)
{
    ui->load->setEnabled(!arg1.isEmpty());
}
