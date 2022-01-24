#include "Options.hpp"
#include "Settings.hpp"

#include <QCoreApplication>

Options Options::options;

Options::Options()
{
    fps=25;
    limitedFPS=true;
    zoom=0;
    audioVolume=100;
    language="en";
}

Options::~Options()
{
}

void Options::loadVar()
{
    bool ok;
    /* for portable version
    settings=new QSettings(QCoreApplication::applicationDirPath()+"/client-settings.conf",QSettings::IniFormat); */
    if(Settings::settings->contains("fps"))
    {
        fps=static_cast<uint8_t>(Settings::settings->value("fps").toUInt(&ok));
        if(!ok)
            fps=25;
    }
    else
        fps=25;
    if(Settings::settings->contains("limitedFPS"))
        limitedFPS=Settings::settings->value("limitedFPS").toBool();
    else
        limitedFPS=true;
    if(Settings::settings->contains("zoom"))
    {
        zoom=static_cast<uint8_t>(Settings::settings->value("zoom").toUInt());
        if(zoom>4)
            zoom=0;
    }
    else
        zoom=0;
    if(Settings::settings->contains("audioVolume"))
    {
        audioVolume=static_cast<uint8_t>(Settings::settings->value("audioVolume").toUInt(&ok));
        if(!ok || audioVolume>100)
            audioVolume=100;
    }
    else
        audioVolume=100;
    if(Settings::settings->contains("language"))
        language=Settings::settings->value("language").toString().toStdString();
}

void Options::setFPS(const uint16_t &fps)
{
    if(this->fps==fps)
        return;
    this->fps=fps;
    Settings::settings->setValue("fps",fps);
    emit newFPS(fps);
    if(limitedFPS)
        emit newFinalFPS(fps);
}

void Options::setLimitedFPS(const bool &limitedFPS)
{
    if(this->limitedFPS==limitedFPS)
        return;
    this->limitedFPS=limitedFPS;
    Settings::settings->setValue("limitedFPS",limitedFPS);
    emit newLimitedFPS(limitedFPS);
    if(limitedFPS)
        emit newFinalFPS(fps);
    else
        emit newFinalFPS(0);
}

void Options::setForcedZoom(const uint8_t &zoom/*0 is no forced*/)
{
    if(this->zoom==zoom)
        return;
    this->zoom=zoom;
    Settings::settings->setValue("zoom",zoom);
    emit newZoomEnabled(zoom);
}

void Options::setAudioVolume(const uint8_t &audioVolume)
{
    if(this->audioVolume==audioVolume)
        return;
    if(audioVolume>100)
        return;
    this->audioVolume=audioVolume;
    Settings::settings->setValue("audioVolume",audioVolume);
    emit newAudioVolume(audioVolume);
}

void Options::setLanguage(const std::string &language)//the main code
{
    if(this->language==language)
        return;
    this->language=language;
    Settings::settings->setValue("language",QString::fromStdString(language));
    emit newLanguage(language);
}

void Options::setDeviceIndex(const int &indexDevice)
{
    if(indexDevice<(int)devices.size() && indexDevice>=0)
    {
        if(this->indexDevice==indexDevice)
            return;
        this->indexDevice=indexDevice;
        Settings::settings->setValue("audioDevice",devices.at(indexDevice));
        emit newAudioDevice(indexDevice);
    }
}

void Options::setAudioDeviceList(const QStringList &devices)
{
    indexDevice=devices.indexOf(Settings::settings->value("audioDevice").toString());
    if(indexDevice==-1)
    {
        if(!devices.isEmpty())
            indexDevice=0;
        Settings::settings->remove("audioDevice");
    }
    this->devices=devices;
}

int Options::getIndexDevice() const
{
    return indexDevice;
}

uint16_t Options::getFPS() const
{
    return fps;
}

bool Options::getLimitedFPS() const
{
    return limitedFPS;
}

uint16_t Options::getFinalFPS() const
{
    if(!limitedFPS)
        return 0;
    else
        return fps;
}

uint8_t Options::getForcedZoom() const/*0 is no forced*/
{
    return zoom;
}

uint8_t Options::getAudioVolume() const
{
    return audioVolume;
}

std::string Options::getLanguage() const//the main code
{
    return language;
}
