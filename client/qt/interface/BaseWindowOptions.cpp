#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../Options.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../Audio.h"
#endif

using namespace CatchChallenger;

void CatchChallenger::BaseWindow::on_forceZoom_toggled(bool checked)
{
    if(checked)
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(true);
        ui->zoom->setValue(2);
        mapController->setScale(2);
        Options::options.setForcedZoom(2);
        ui->zoom->setEnabled(true);
    }
    else
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(false);
        ui->zoom->setValue(CommonDatapack::commonDatapack.layersOptions.zoom);
        mapController->setScale(CommonDatapack::commonDatapack.layersOptions.zoom);
        Options::options.setForcedZoom(0);
    }
}

void CatchChallenger::BaseWindow::on_zoom_valueChanged(int value)
{
    if(!ui->zoom->isEnabled())
        return;
    Options::options.setForcedZoom(static_cast<uint8_t>(value));
    mapController->setScale(value);
}

void BaseWindow::on_checkBoxLimitFPS_toggled(bool checked)
{
    Options::options.setLimitedFPS(checked);
    mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::on_spinBoxMaxFPS_editingFinished()
{
    Options::options.setFPS(static_cast<uint16_t>(ui->spinBoxMaxFPS->value()));
    mapController->setTargetFPS(Options::options.getFinalFPS());
}

void CatchChallenger::BaseWindow::on_audioVolume_valueChanged(int value)
{
    Options::options.setAudioVolume(static_cast<uint8_t>(value));
    #ifndef CATCHCHALLENGER_NOAUDIO
    Audio::audio.setVolume(value);
    #endif
}

void BaseWindow::loadSettings()
{
    ui->audioVolume->setValue(Options::options.getAudioVolume());
    ui->checkBoxLimitFPS->setChecked(Options::options.getLimitedFPS());
    ui->spinBoxMaxFPS->setValue(Options::options.getFPS());
    mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::loadSettingsWithDatapack()
{
    const uint8_t &forcedZoom=Options::options.getForcedZoom();
    if(forcedZoom==0)
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(false);
        ui->zoom->setValue(CommonDatapack::commonDatapack.layersOptions.zoom);
        mapController->setScale(CommonDatapack::commonDatapack.layersOptions.zoom);
    }
    else
    {
        ui->zoom->setEnabled(false);
        ui->forceZoom->setChecked(true);
        ui->zoom->setValue(forcedZoom);
        mapController->setScale(Options::options.getForcedZoom());
        ui->zoom->setEnabled(true);
    }
}
