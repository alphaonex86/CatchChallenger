#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../Options.h"

using namespace Pokecraft;

void BaseWindow::on_checkBoxZoom_toggled(bool checked)
{
    Options::options.setZoomEnabled(checked);
    if(Options::options.getZoomEnabled())
        mapController->setScale(2);
    else
        mapController->setScale(1);
}

void BaseWindow::on_checkBoxLimitFPS_toggled(bool checked)
{
    Options::options.setLimitedFPS(checked);
    mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::on_spinBoxMaxFPS_editingFinished()
{
    Options::options.setFPS(ui->spinBoxMaxFPS->value());
    mapController->setTargetFPS(Options::options.getFinalFPS());
}

void BaseWindow::loadSettings()
{
    ui->checkBoxZoom->setChecked(Options::options.getZoomEnabled());
    ui->checkBoxLimitFPS->setChecked(Options::options.getLimitedFPS());
    ui->spinBoxMaxFPS->setValue(Options::options.getFPS());
    if(Options::options.getZoomEnabled())
        mapController->setScale(2);
    mapController->setTargetFPS(Options::options.getFinalFPS());
}
