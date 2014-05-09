#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../Options.h"

using namespace CatchChallenger;

void CatchChallenger::BaseWindow::on_forceZoom_toggled(bool checked)
{
    if(checked)
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(true);
        ui->zoom->setValue(2);
        MapController::mapController->setScale(2);
        Options::options.setForcedZoom(2);
        ui->zoom->setEnabled(true);
    }
    else
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(false);
        ui->zoom->setValue(CommonDatapack::commonDatapack.layersOptions.zoom);
        MapController::mapController->setScale(CommonDatapack::commonDatapack.layersOptions.zoom);
        Options::options.setForcedZoom(0);
    }
}

void CatchChallenger::BaseWindow::on_zoom_valueChanged(int value)
{
    if(!ui->zoom->isEnabled())
        return;
    Options::options.setForcedZoom(value);
    MapController::mapController->setScale(value);
}

void BaseWindow::on_checkBoxLimitFPS_toggled(bool checked)
{
    Options::options.setLimitedFPS(checked);
    MapController::mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::on_spinBoxMaxFPS_editingFinished()
{
    Options::options.setFPS(ui->spinBoxMaxFPS->value());
    MapController::mapController->setTargetFPS(Options::options.getFinalFPS());
}

void CatchChallenger::BaseWindow::on_audioVolume_valueChanged(int value)
{
    Options::options.setAudioVolume(value);
    int index=0;
    while(index<ambianceList.size())
    {
        //ambianceList.at(index)->setVolume((qreal)value/(qreal)100);
        index++;
    }
}

void BaseWindow::loadSettings()
{
    ui->audioVolume->setValue(Options::options.getAudioVolume());
    ui->checkBoxLimitFPS->setChecked(Options::options.getLimitedFPS());
    ui->spinBoxMaxFPS->setValue(Options::options.getFPS());
    MapController::mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::loadSettingsWithDatapack()
{
    if(Options::options.getForcedZoom()==0)
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(false);
        ui->zoom->setValue(CommonDatapack::commonDatapack.layersOptions.zoom);
        MapController::mapController->setScale(CommonDatapack::commonDatapack.layersOptions.zoom);
    }
    else
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(true);
        ui->zoom->setValue(Options::options.getForcedZoom());
        MapController::mapController->setScale(Options::options.getForcedZoom());
        ui->zoom->setEnabled(true);
    }
}
