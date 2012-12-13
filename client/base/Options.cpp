#include "Options.h"

#include <QCoreApplication>

Options Options::options;

Options::Options()
{
    bool ok;
    settings=new QSettings();
    /* for portable version
    settings=new QSettings(QCoreApplication::applicationDirPath()+"/client-settings.conf",QSettings::IniFormat); */
    if(settings->contains("fps"))
    {
        fps=settings->value("fps").toUInt(&ok);
        if(!ok)
            fps=25;
    }
    else
        fps=25;
    if(settings->contains("limitedFPS"))
        limitedFPS=settings->value("limitedFPS").toBool();
    else
        limitedFPS=true;
    if(settings->contains("zoom"))
        zoom=settings->value("zoom").toBool();
    else
        zoom=true;
}

Options::~Options()
{
    delete settings;
}

void Options::setFPS(const quint16 &fps)
{
    if(this->fps==fps)
        return;
    this->fps=fps;
    settings->setValue("fps",fps);
    emit newFPS(fps);
    if(limitedFPS)
        emit newFinalFPS(fps);
}

void Options::setLimitedFPS(const bool &limitedFPS)
{
    if(this->limitedFPS==limitedFPS)
        return;
    this->limitedFPS=limitedFPS;
    settings->setValue("limitedFPS",limitedFPS);
    emit newLimitedFPS(limitedFPS);
    if(limitedFPS)
        emit newFinalFPS(fps);
    else
        emit newFinalFPS(0);
}

void Options::setZoomEnabled(const bool &zoom)
{
    if(this->zoom==zoom)
        return;
    this->zoom=zoom;
    settings->setValue("zoom",zoom);
    emit newZoomEnabled(zoom);
}

quint16 Options::getFPS()
{
    return fps;
}

bool Options::getLimitedFPS()
{
    return limitedFPS;
}

quint16 Options::getFinalFPS()
{
    if(!limitedFPS)
        return 0;
    else
        return fps;
}

bool Options::getZoomEnabled()
{
    return zoom;
}
