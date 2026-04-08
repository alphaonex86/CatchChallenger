#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#include <QCoreApplication>
#include <QMediaDevices>
#include <QAudioDevice>

AudioGL::AudioGL()
{
}

AudioGL::~AudioGL()
{
}

//if already playing ambiance then call stopCurrentAmbiance
std::string AudioGL::startAmbiance(const std::string &soundPath)
{
    const QString QtsoundPath=QString::fromStdString(soundPath);
    if(!QtsoundPath.startsWith(":/"))
        return Audio::startAmbiance(soundPath);
    stopCurrentAmbiance();

    QAudioDevice info=QMediaDevices::defaultAudioOutput();
    if (!info.isFormatSupported(m_format))
        return std::to_string((int)m_format.channelCount())+","+
                std::to_string((int)m_format.isValid())+","+
                std::to_string((int)m_format.sampleRate())+","+
                " "+info.description().toStdString();

    ambiance_player = new QAudioOutput();
    if(ambiance_player!=nullptr)
        return "QAudioOutput nullptr";
    else
        return "!GameLoader::gameLoader->musics.contains(QtsoundPath)";
}

void AudioGL::stopCurrentAmbiance()
{
    Audio::stopCurrentAmbiance();
}
#endif
